# Copyright 2022 The SQLNet Company GmbH
#
# This file is licensed under the Elastic License 2.0 (ELv2).
# Refer to the LICENSE.txt file in the root of the repository
# for details.
#


"""
Custom class for handling the SQL code of the features.
"""

from __future__ import annotations

import re
import os
from pathlib import Path
from typing import Iterator, Sequence, Union

import numpy as np

from getml.data.helpers import _is_typed_list

from .dialect import _drop_table, _table_pattern, sqlite3
from .helpers import _edit_table_name, _edit_windows_filename
from .sql_string import SQLString


class SQLCode:
    """
    Custom class for handling the SQL code of the
    features generated by the pipeline.

    Example:
        .. code-block:: python

            sql_code = my_pipeline.features.to_sql()

            # You can access individual features
            # by index.
            feature_1_1 = sql_code[0]

            # You can also access them by name.
            feature_1_10 = sql_code["FEATURE_1_10"]

            # You can also type the name of
            # a table or column to find all
            # features related to that table
            # or column.
            features = sql_code.find("SOME_TABLE")

            # HINT: The generated SQL code always
            # escapes table and column names using
            # quotation marks. So if you want exact
            # matching, you can do this:
            features = sql_code.find('"SOME_TABLE"')
    """

    def __init__(
        self,
        code: Sequence[Union[str, SQLString]],
        dialect: str = sqlite3,
    ) -> None:

        if not _is_typed_list(code, str):
            raise TypeError("'code' must be a list of str.")

        self.code = [SQLString(elem) for elem in code]

        self.dialect = dialect

        self.tables = [
            _edit_table_name(table_name)
            for table_name in re.findall(_table_pattern(self.dialect), "".join(code))
        ]

    def __getitem__(self, key: Union[int, slice, str]) -> Union[SQLCode, SQLString]:

        if isinstance(key, int):
            return self.code[key]

        if isinstance(key, slice):
            return SQLCode(self.code[key], self.dialect)

        if isinstance(key, str):
            if key.upper() in self.tables:
                return self.find(_drop_table(self.dialect, key))[0]
            return SQLString("")

        raise TypeError(
            "Features can only be indexed by: int, slices, "
            f"or str, not {type(key).__name__}"
        )

    def __iter__(self) -> Iterator[SQLString]:
        yield from self.code

    def __len__(self) -> int:
        return len(self.code)

    def __repr__(self) -> str:
        return "\n\n\n".join(self.code)

    def _repr_markdown_(self) -> str:
        return "```sql\n" + self.__repr__() + "\n```"

    def find(self, keyword: str) -> SQLCode:
        """
        Returns the SQLCode for all features
        containing the keyword.

        Args:
            keyword (str): The keyword to be found.
        """
        if not isinstance(keyword, str):
            raise TypeError("'keyword' must be a str.")

        return SQLCode([elem for elem in self.code if keyword in elem], self.dialect)

    def save(self, fname: str, split: bool = True, remove: bool = False) -> None:
        """
        Saves the SQL code to a file.

        Args:
            fname (str):
                The name of the file or folder (if `split` is True)
                in which you want to save the features.

            split (bool):
                If True, the code will be split into multiple files, one for
                each feature and saved into a folder `fname`.

            remove (bool):
                If True, the existing SQL files in `fname` folder generated
                previously with the save method will be removed.
        """
        if not split:
            with open(fname, "w", encoding="utf-8") as sqlfile:
                sqlfile.write(str(self))
            return

        directory = Path(fname)

        if directory.exists():
            iter_dir = os.listdir(fname)

            pattern = "^\d{4}.*\_.*\.sql$"

            exist_files_path = [fp for fp in iter_dir if re.search(pattern, fp)]

            if not remove and exist_files_path:
                print(f"The following files already exist in the directory ({fname}):")
                for fp in np.sort(exist_files_path):
                    print(fp)
                print("Please set 'remove=True' to remove them.")
                return

            if remove and exist_files_path:
                for fp in exist_files_path:
                    os.remove(fname + "/" + fp)

        directory.mkdir(exist_ok=True)

        for index, code in enumerate(self.code, 1):
            match = re.search(_table_pattern(self.dialect), str(code))
            name = _edit_table_name(match.group(1).lower()) if match else "feature"
            name = _edit_windows_filename(name).replace(".", "_").replace("`", "")
            file_path = directory / f"{index:04d}_{name}.sql"
            with open(file_path, "w", encoding="utf-8") as sqlfile:
                sqlfile.write(str(code))

    def to_str(self) -> str:
        """
        Returns a raw string representation of the SQL code.
        """
        return str(self)
