# Copyright 2022 The SQLNet Company GmbH
#
# This file is licensed under the Elastic License 2.0 (ELv2).
# Refer to the LICENSE.txt file in the root of the repository
# for details.
#


"""
Collection of helper functions that are not intended to be used by
the end-user.
"""

import json
import numbers
import os
import random
import string
from typing import Any, Dict, List, Optional, Tuple, Union

import numpy as np
import pandas as pd  # type: ignore
import pyarrow as pa  # type: ignore
from pyarrow import parquet  # type: ignore

import getml.communication as comm
from getml import constants
from getml.constants import COMPARISON_ONLY, TIME_STAMP
from getml.database import Connection, _retrieve_urls
from getml.database.helpers import _retrieve_temp_dir

from .columns import (
    BooleanColumnView,
    FloatColumn,
    FloatColumnView,
    StringColumn,
    StringColumnView,
    from_value,
)
from .roles import (
    _all_roles,
    _categorical_roles,
    _numerical_roles,
    time_stamp,
    unused_float,
    unused_string,
)
from .subroles import _all_subroles

# --------------------------------------------------------------------

OnType = Optional[Union[str, Tuple[str, str], List[Union[str, Tuple[str, str]]]]]

# --------------------------------------------------------------------


def list_data_frames() -> Dict[str, List[str]]:
    """Lists all available data frames of the project.

    Examples:
        ```python
        d, _ = getml.datasets.make_numerical()
        getml.data.list_data_frames()
        d.save()
        getml.data.list_data_frames()
        ```
    Returns:
        dict:
            Dict containing lists of strings representing the names of
            the data frames objects

        - 'in_memory'
            held in memory (RAM).
        - 'on_disk'
            stored on disk.
    """

    cmd: Dict[str, Any] = {}
    cmd["type_"] = "list_data_frames"
    cmd["name_"] = ""

    with comm.send_and_get_socket(cmd) as sock:
        msg = comm.recv_string(sock)
        if msg != "Success!":
            comm.engine_exception_handler(msg)
        json_str = comm.recv_string(sock)

    return json.loads(json_str)


# --------------------------------------------------------------------


def _check_if_exists(colnames: List[str], all_colnames: List[str]):

    for col in colnames:
        if col in all_colnames:
            raise ValueError("Duplicate column: '" + col + "'!")

        all_colnames.append(col)

    return all_colnames


# --------------------------------------------------------------------


def _check_join_key(candidate: Any, join_keys: List[str]) -> List[str]:
    if isinstance(candidate, str):
        candidate = [candidate]

    return [can for can in candidate if can not in join_keys]


# --------------------------------------------------------------------


def _empty_data_frame() -> str:
    return "Empty getml.DataFrame\nColumns: []\n"


# --------------------------------------------------------------------


def _exists_in_memory(name: str) -> bool:

    if not isinstance(name, str):
        raise TypeError("'name' must be of type str")

    all_df = list_data_frames()

    return name in all_df["in_memory"]


# --------------------------------------------------------------------


def _extract_shape(cmd, name):
    shape = cmd[name + "_shape_"]
    shape = np.asarray(shape).astype(np.int32)
    return shape.tolist()


# --------------------------------------------------------------------


def _finditems(key: str, dct: Dict[str, Any]) -> Any:
    if key in dct:
        yield dct[key]
    for k, v in dct.items():
        if isinstance(v, dict):
            yield from _finditems(key, v)


# --------------------------------------------------------------------


def _get_column(name, columns):
    for col in columns:
        if col.name == name:
            return col
    return None


# --------------------------------------------------------------------


def _handle_cols(
    cols: Union[
        str, FloatColumn, StringColumn, List[Union[str, FloatColumn, StringColumn]]
    ]
) -> List[str]:
    """
    Handles cols as supplied to DataFrame methods. Returns a list of column names.
    """

    if not isinstance(cols, list):
        cols = [cols]

    if not _is_typed_list(cols, (str, FloatColumn, StringColumn)):
        raise TypeError(
            "'cols' must be either a string, a FloatColumn, "
            "a StringColumn, or a list thereof."
        )

    names: List[str] = []

    for col in cols:

        if isinstance(col, list):
            names.extend(_handle_cols(col))

        if isinstance(col, str):
            names.append(col)

        if isinstance(col, (FloatColumn, StringColumn)):
            names.append(col.name)

    return names


# --------------------------------------------------------------------


def _handle_multiple_join_keys(
    join_keys: List[Union[str, Tuple[str, str]]]
) -> Tuple[str, str]:
    on = [(jk, jk) if isinstance(jk, str) else jk for jk in join_keys]
    left_join_keys = [o[0] for o in on]
    right_join_keys = [o[1] for o in on]
    ljk, rjk = _merge_join_keys(left_join_keys, right_join_keys)
    return (ljk, rjk)


# --------------------------------------------------------------------


def _handle_on(on: OnType) -> Tuple[str, str]:
    if on is None:
        return (constants.NO_JOIN_KEY, constants.NO_JOIN_KEY)

    if isinstance(on, str):
        return (on, on)

    if isinstance(on, list):
        return _handle_multiple_join_keys(on)

    return on


# --------------------------------------------------------------------


def _handle_ts(ts):
    if ts is None:
        return ("", "")
    return ts


# --------------------------------------------------------------------


def _is_numerical_type(coltype) -> bool:
    return coltype in [
        int,
        float,
        np.int_,
        np.int8,
        np.int16,
        np.int32,
        np.int64,
        np.uint8,
        np.uint16,
        np.uint32,
        np.uint64,
        np.float_,
        np.float16,
        np.float32,
        np.float64,
    ]


# --------------------------------------------------------------------


def _is_subclass_list(some_list, parent) -> bool:

    is_subclass_list = isinstance(some_list, list)

    is_subclass_list = is_subclass_list and all(
        [issubclass(type(ll), parent) for ll in some_list]
    )

    return is_subclass_list


# --------------------------------------------------------------------


def _is_typed_dict(some_dict, key_types, value_types) -> bool:

    if not isinstance(key_types, list):
        key_types = [key_types]

    if not isinstance(value_types, list):
        value_types = [value_types]

    is_typed_dict = isinstance(some_dict, dict)

    is_typed_dict = is_typed_dict and all(
        [any([isinstance(key, t) for t in key_types]) for key in some_dict.keys()]
    )

    is_typed_dict = is_typed_dict and all(
        [any([isinstance(val, t) for t in value_types]) for val in some_dict.values()]
    )

    return is_typed_dict


# --------------------------------------------------------------------


def _is_typed_list(some_list, types) -> bool:

    if isinstance(types, list):
        types = tuple(types)

    is_typed_list = isinstance(some_list, list)

    is_typed_list = is_typed_list and all([isinstance(ll, types) for ll in some_list])

    return is_typed_list


# --------------------------------------------------------------------


def _is_non_empty_typed_list(some_list, types) -> bool:
    return _is_typed_list(some_list, types) and len(some_list) > 0


# -----------------------------------------------------------------


def _is_unsupported_type(coltype) -> bool:
    return any(
        [
            pa.types.is_decimal(coltype),
            pa.types.is_dictionary(coltype),
            pa.types.is_large_list(coltype),
            pa.types.is_list(coltype),
            pa.types.is_map(coltype),
            pa.types.is_struct(coltype),
            pa.types.is_union(coltype),
        ]
    )


# -----------------------------------------------------------------


def _make_id() -> str:
    letters = string.ascii_letters + string.digits
    return "".join([random.choice(letters) for _ in range(6)])


# --------------------------------------------------------------------


def _make_default_slice(slc, end) -> Tuple[int, int, int]:
    start = slc.start or 0
    start = start if start >= 0 else end + start
    stop = slc.stop or end
    stop = stop if stop >= 0 else end + stop
    step = slc.step or 1
    return start, stop, step


# --------------------------------------------------------------------


def _merge_join_keys(join_key: List[str], other_join_key: List[str]) -> Tuple[str, str]:

    begin = constants.MULTIPLE_JOIN_KEYS_BEGIN
    end = constants.MULTIPLE_JOIN_KEYS_END
    sep = constants.JOIN_KEY_SEP
    len_jk = len_other_jk = 1

    if not other_join_key:
        other_join_key = join_key

    len_jk = len(join_key)
    if len_jk > 1:
        join_key_merged = begin + sep.join(join_key) + end
    else:
        join_key_merged = join_key[0]

    len_other_jk = len(other_join_key)
    if len_other_jk > 1:
        other_join_key_merged = begin + sep.join(other_join_key) + end
    else:
        other_join_key_merged = other_join_key[0]

    if len_jk != len_other_jk:
        raise ValueError(
            "The number of join keys passed to "
            + "'join_key' and 'other_join_key' "
            + "must match!"
        )

    return join_key_merged, other_join_key_merged


# --------------------------------------------------------------------


def _modify_pandas_columns(pandas_df: pd.DataFrame) -> pd.DataFrame:
    pandas_df_copy = pandas_df
    pandas_df_copy.columns = np.asarray(pandas_df.columns).astype(str).tolist()
    return pandas_df_copy


# --------------------------------------------------------------------


def _remove_trailing_underscores(some_dict: Dict[str, Any]) -> Dict[str, Any]:

    new_dict: Dict[str, Any] = {}

    for kkey in some_dict:

        new_key = kkey

        if kkey[-1] == "_":
            new_key = kkey[:-1]

        if isinstance(some_dict[kkey], dict):
            new_dict[new_key] = _remove_trailing_underscores(some_dict[kkey])

        elif isinstance(some_dict[kkey], list):
            new_dict[new_key] = [
                _remove_trailing_underscores(elem) if isinstance(elem, dict) else elem
                for elem in some_dict[kkey]
            ]

        else:
            new_dict[new_key] = some_dict[kkey]

    return new_dict


# --------------------------------------------------------------------


def _replace_non_alphanumeric(string: str) -> str:
    replaced = "".join([" " if not c.isalnum() else c for c in list(string)])
    stripped = replaced.strip()
    result = stripped.replace(" ", "_")
    while "___" in result:
        result.replace("___", "__")
    return result


# --------------------------------------------------------------------


def _replace_non_alphanumeric_cols(df_or_view):
    """
    Many databases only accept alphanumeric characters
    and underscores. This is meant to handle these
    problems.
    """
    non_alnum_colnames = [
        cname
        for cname in df_or_view.colnames
        if not cname.isalnum() and _replace_non_alphanumeric(cname) != cname
    ]

    if not non_alnum_colnames:
        return df_or_view

    for old_name in non_alnum_colnames:
        col = df_or_view[old_name]
        new_name = _replace_non_alphanumeric(old_name)
        if new_name != old_name:
            df_or_view = df_or_view.with_column(
                col=col,
                name=new_name,
                role=df_or_view.roles.infer(old_name),
                subroles=col.subroles,
                unit=col.unit,
                time_formats=None,
            )

    return df_or_view.drop(non_alnum_colnames)


# --------------------------------------------------------------------


def _update_sniffed_roles(
    sniffed_roles: Dict[str, List[str]], roles: Dict[str, List[str]]
) -> Dict[str, List[str]]:

    # -------------------------------------------------------

    if not isinstance(roles, dict):
        raise TypeError("roles must be a dict!")

    if not isinstance(sniffed_roles, dict):
        raise TypeError("sniffed_roles must be a dict!")

    for role in roles:
        if not _is_typed_list(roles[role], str):
            raise TypeError("Entries in roles must be lists of str!")

    for role in sniffed_roles:
        if not _is_typed_list(sniffed_roles[role], str):
            raise TypeError("Entries in sniffed_roles must be lists of str!")

    # -------------------------------------------------------

    for new_role in roles:

        for colname in roles[new_role]:

            for old_role in sniffed_roles:
                if colname in sniffed_roles[old_role]:
                    sniffed_roles[old_role].remove(colname)
                    break

            if new_role in sniffed_roles:
                sniffed_roles[new_role] += [colname]
            else:
                sniffed_roles[new_role] = [colname]

    # -------------------------------------------------------

    return sniffed_roles


# ------------------------------------------------------------


def _make_table(col, numpy_array) -> pa.Table:
    data_frame = pd.DataFrame()
    data_frame[col.name] = numpy_array
    return pa.Table.from_pandas(data_frame)


# ------------------------------------------------------------


def _send_numpy_array(col, numpy_array: np.ndarray):

    table = _make_table(col, numpy_array)

    with comm.send_and_get_socket(col.cmd) as sock:
        with sock.makefile(mode="wb") as sink:
            batches = table.to_batches()
            with pa.ipc.new_stream(sink, table.schema) as writer:
                for batch in batches:
                    writer.write_batch(batch)

        msg = comm.recv_string(sock)

    if msg != "Success!":
        comm.engine_exception_handler(msg)


# --------------------------------------------------------------------


def _sniff_arrow(table) -> Dict[str, List[str]]:
    return _sniff_schema(table.schema)


# --------------------------------------------------------------------


def _sniff_csv(
    fnames: List[str],
    num_lines_sniffed: int = 1000,
    quotechar: str = '"',
    sep: str = ",",
    skip: int = 0,
    colnames: Optional[List[str]] = None,
) -> Dict[str, List[str]]:
    """Sniffs a list of CSV files and returns the result as a dictionary of
    roles.

    Args:
        fnames (List[str]): The list of CSV file names to be read.

        num_lines_sniffed (int, optional):

            Number of lines analysed by the sniffer.

        quotechar (str, optional):

            The character used to wrap strings.

        sep (str, optional):

            The character used for separating fields.

        skip (int, optional):
            Number of lines to skip at the beginning of each file.

        colnames (List[str] or None, optional): The first line of a CSV file
            usually contains the column names. When this is not the case, you need to
            explicitly pass them.

    Returns:
        dict: Keyword arguments (kwargs) that can be used to construct
              a DataFrame.
    """

    fnames_ = _retrieve_urls(fnames, verbose=False)

    cmd: Dict[str, Any] = {}

    cmd["name_"] = ""
    cmd["type_"] = "Database.sniff_csv"

    cmd["dialect_"] = "python"
    cmd["fnames_"] = fnames_
    cmd["num_lines_sniffed_"] = num_lines_sniffed
    cmd["quotechar_"] = quotechar
    cmd["sep_"] = sep
    cmd["skip_"] = skip
    cmd["conn_id_"] = "default"

    if colnames is not None:
        cmd["colnames_"] = colnames

    with comm.send_and_get_socket(cmd) as sock:
        msg = comm.recv_string(sock)
        if msg != "Success!":
            sock.close()
            raise IOError(msg)
        roles = comm.recv_string(sock)

    return json.loads(roles)


# --------------------------------------------------------------------


def _sniff_db(
    table_name: str, conn: Optional[Connection] = None
) -> Dict[str, List[str]]:
    """
    Sniffs a table in the database and returns a dictionary of roles.

    Args:
        table_name (str): Name of the table to be sniffed.

        conn ([`Connection`][getml.database.Connection], optional): The database
            connection to be used. If you don't explicitly pass a connection,
            the engine will use the default connection.

    Returns:
        dict: Keyword arguments (kwargs) that can be used to construct
              a DataFrame.
    """

    conn = conn or Connection()

    cmd: Dict[str, Any] = {}

    cmd["name_"] = table_name
    cmd["type_"] = "Database.sniff_table"

    cmd["conn_id_"] = conn.conn_id

    with comm.send_and_get_socket(cmd) as sock:
        msg = comm.recv_string(sock)
        if msg != "Success!":
            sock.close()
            raise Exception(msg)
        roles = comm.recv_string(sock)

    return json.loads(roles)


# --------------------------------------------------------------------


def _sniff_json(json_str: str) -> Dict[str, List[str]]:
    """Sniffs a JSON str and returns the result as a dictionary of
    roles.

    Args:
        json_str (str): The JSON string to be sniffed.

    Returns:
        dict: Roles that can be used to construct a DataFrame.
    """
    json_dict = json.loads(json_str)

    roles: Dict[str, List[str]] = {}
    roles["unused_float"] = []
    roles["unused_string"] = []

    for cname, col in json_dict.items():
        if _is_numerical_type(np.array(col).dtype):
            roles["unused_float"].append(cname)
        else:
            roles["unused_string"].append(cname)

    return roles


# --------------------------------------------------------------------


def _sniff_parquet(fname: str) -> Dict[str, List[str]]:
    schema = parquet.read_schema(fname)
    return _sniff_schema(schema)


# --------------------------------------------------------------------


def _sniff_pandas(pandas_df: pd.DataFrame) -> Dict[str, List[str]]:
    """Sniffs a pandas.DataFrame and returns the result as a dictionary of
    roles.

    Args:
        pandas_df (pandas.DataFrame): The pandas.DataFrame to be sniffed.

    Returns:
        dict: Roles that can be used to construct a DataFrame.
    """
    roles: Dict[str, Any] = {}
    roles["unused_float"] = []
    roles["unused_string"] = []

    colnames = pandas_df.columns
    coltypes = pandas_df.dtypes

    for cname, ctype in zip(colnames, coltypes):
        if _is_numerical_type(ctype):
            roles["unused_float"].append(cname)
        else:
            roles["unused_string"].append(cname)

    return roles


# --------------------------------------------------------------------


def _sniff_s3(
    bucket: str,
    keys: List[str],
    region: str,
    num_lines_sniffed: int = 1000,
    sep: str = ",",
    skip: int = 0,
    colnames: Optional[List[str]] = None,
) -> Dict[str, List[str]]:
    """Sniffs a list of CSV files located in an S3 bucket
    and returns the result as a dictionary of roles.

    Args:
        bucket (str):
            The bucket from which to read the files.

        keys (List[str]): The list of keys (files in the bucket) to be read.

        region (str):
            The region in which the bucket is located.

        num_lines_sniffed (int, optional):
            Number of lines analysed by the sniffer.

        sep (str, optional):
            The character used for separating fields.

        skip (int, optional):
            Number of lines to skip at the beginning of each file.

        colnames (List[str] or None, optional): The first line of a CSV file
            usually contains the column names. When this is not the case, you need to
            explicitly pass them.

    Returns:
        dict: Keyword arguments (kwargs) that can be used to construct
              a DataFrame.
    """

    cmd: Dict[str, Any] = {}

    cmd["name_"] = ""
    cmd["type_"] = "Database.sniff_s3"

    cmd["bucket_"] = bucket
    cmd["dialect_"] = "python"
    cmd["keys_"] = keys
    cmd["num_lines_sniffed_"] = num_lines_sniffed
    cmd["region_"] = region
    cmd["sep_"] = sep
    cmd["skip_"] = skip
    cmd["conn_id_"] = "default"

    if colnames is not None:
        cmd["colnames_"] = colnames

    with comm.send_and_get_socket(cmd) as sock:
        msg = comm.recv_string(sock)
        if msg != "Success!":
            sock.close()
            raise IOError(msg)
        roles = comm.recv_string(sock)

    return json.loads(roles)


# --------------------------------------------------------------------


def _sniff_schema(schema: pa.Schema) -> Dict[str, List[str]]:
    roles: Dict[str, List[str]] = {}
    roles["unused_float"] = []
    roles["unused_string"] = []

    colnames = schema.names
    coltypes = schema.types

    for cname, ctype in zip(colnames, coltypes):
        if _is_unsupported_type(ctype):
            continue
        if _is_numerical_type(ctype.to_pandas_dtype()):
            roles["unused_float"].append(cname)
        else:
            roles["unused_string"].append(cname)

    return roles


# --------------------------------------------------------------------


def _check_role(role: str):

    correct_role = isinstance(role, str)
    correct_role = correct_role and role in _all_roles

    if not correct_role:
        raise ValueError(
            """'role' must be None or of type str and in """ + str(_all_roles) + "."
        )


# ------------------------------------------------------------


def _to_arrow(df_or_view: Any) -> pa.Table:

    df_or_view.refresh()

    cmd: Dict[str, Any] = {}

    typename = type(df_or_view).__name__

    cmd["type_"] = typename + ".to_arrow"
    cmd["name_"] = df_or_view.name if typename == "DataFrame" else ""

    if typename == "View":
        cmd["view_"] = df_or_view._getml_deserialize()

    with comm.send_and_get_socket(cmd) as sock:
        msg = comm.recv_string(sock)
        if msg != "Success!":
            comm.engine_exception_handler(msg)
        with sock.makefile(mode="rb") as stream:
            with pa.ipc.open_stream(stream) as reader:
                return reader.read_all()


# ------------------------------------------------------------


def _to_parquet(df_or_view: Any, fname: str, compression: str):

    df_or_view.refresh()

    if not isinstance(fname, str):
        raise TypeError("'fname' must be of type str")

    if not isinstance(compression, str):
        raise TypeError("'compression' must be of type str")

    fname_ = os.path.abspath(fname)

    cmd: Dict[str, Any] = {}

    typename = type(df_or_view).__name__

    cmd["type_"] = typename + ".to_parquet"
    cmd["name_"] = df_or_view.name if typename == "DataFrame" else ""

    cmd["fname_"] = fname_
    cmd["compression_"] = compression

    if typename == "View":
        cmd["view_"] = df_or_view._getml_deserialize()

    comm.send(cmd)


# ------------------------------------------------------------


def _to_pyspark(df_or_view: Any, name: str, spark: Any):
    if df_or_view.ncols() == 0:
        raise ValueError(
            "Cannot transform '"
            + df_or_view.name
            + "' to pyspark.DataFrame, because it contains no columns."
        )
    temp_dir = _retrieve_temp_dir()
    os.makedirs(temp_dir, exist_ok=True)
    path = os.path.join(temp_dir, name + ".parquet")
    _replace_non_alphanumeric_cols(df_or_view).to_parquet(path)
    spark_df = spark.read.parquet(path)
    spark_df.createOrReplaceTempView(name)
    spark.sql("CACHE TABLE `" + name + "`;")
    os.remove(path)
    return spark_df


# ------------------------------------------------------------


def _transform_col(col, role, is_boolean, is_float, is_string, time_formats):

    if (is_boolean or is_float) and role in _categorical_roles:
        return col.as_str()  # pytype: disable=attribute-error

    if (is_boolean or is_string) and role in _numerical_roles:
        if role == time_stamp:
            return col.as_ts(  # pytype: disable=attribute-error
                time_formats=time_formats
            )

        return col.as_num()  # pytype: disable=attribute-error

    return col


# --------------------------------------------------------------------


def _transform_timestamps(time_stamps: pd.DataFrame) -> np.ndarray:
    """Transforming a time stamp using to_numeric
    will result in the number of nanoseconds since
    the beginning of UNIX time. We want the number of seconds since
    UNIX time."""

    if not isinstance(time_stamps, pd.DataFrame):
        raise TypeError("'time_stamps' must be a pandas.DataFrame!")

    transformed = pd.DataFrame()

    for colname in time_stamps.columns:

        if pd.api.types.is_numeric_dtype(time_stamps[colname]):
            transformed[colname] = time_stamps[colname]

        elif pd.api.types.is_datetime64_ns_dtype(time_stamps[colname]):
            transformed[colname] = (
                time_stamps[[colname]]
                .apply(pd.to_datetime, errors="coerce")
                .apply(pd.to_numeric, errors="coerce")
                .apply(lambda val: val / 1.0e9)[colname]
            )

        else:
            raise TypeError(
                """
                Column '"""
                + colname
                + """' has the wrong type!

                If you want to send a numpy array or a column in a
                pandas.DataFrame to the engine as a time_stamp, its
                type must either be numerical or numpy.datetime64.

                To fix this problem, you can do one of the following:
                1) Read it in as an unused_string and then use
                   set_role(...) to make it a time stamp. (You might
                   have to explicitly set time_formats.)

                2) Cast your column or array as a numerical value
                   or a numpy.datetime64."""
            )

    return transformed.values


# --------------------------------------------------------------------


def _with_column(
    col: Union[
        bool,
        str,
        float,
        int,
        np.datetime64,
        FloatColumn,
        FloatColumnView,
        StringColumn,
        StringColumnView,
        BooleanColumnView,
    ],
    name: str,
    role: Optional[str] = None,
    subroles: Optional[List[str]] = None,
    unit: str = "",
    time_formats: Optional[List[str]] = None,
):

    # ------------------------------------------------------------

    if isinstance(col, (bool, str, int, float, numbers.Number, np.datetime64)):
        col = from_value(col)

    # ------------------------------------------------------------

    if not isinstance(
        col,
        (
            FloatColumn,
            FloatColumnView,
            StringColumn,
            StringColumnView,
            BooleanColumnView,
        ),
    ):
        raise TypeError("'col' must be a getml.data.Column or a value!")

    # ------------------------------------------------------------

    is_boolean = isinstance(col, BooleanColumnView)

    if not is_boolean:
        subroles = subroles or col.subroles  # type: ignore
        unit = unit or col.unit  # type: ignore

    # ------------------------------------------------------------

    subroles = subroles or []

    if isinstance(subroles, str):
        subroles = [subroles]

    # ------------------------------------------------------------

    if not isinstance(name, str):
        raise TypeError("'name' must be of type str")

    if not _is_typed_list(subroles, str):
        raise TypeError("'subroles' must be of type str, List[str] or None.")

    if not isinstance(unit, str):
        raise TypeError("'unit' must be of type str")

    # ------------------------------------------------------------

    invalid_subroles = [r for r in subroles if r not in _all_subroles]

    if invalid_subroles:
        raise ValueError(
            "'subroles' must be from getml.data.subroles, "
            + "meaning it is one of the following: "
            + str(_all_subroles)
            + ", got "
            + str(invalid_subroles)
        )

    # ------------------------------------------------------------

    time_formats = time_formats or constants.TIME_FORMATS

    # ------------------------------------------------------------

    is_string = "StringColumn" in col.cmd["type_"]

    is_float = "FloatColumn" in col.cmd["type_"]

    # ------------------------------------------------------------

    correct_coltype = is_boolean or is_string or is_float

    if not correct_coltype:
        raise TypeError("""'col' must be a getml.data.Column.""")

    # ------------------------------------------------------------

    if role is None:
        role = unused_float if is_float else unused_string

    _check_role(role)

    # ------------------------------------------------------------

    return (
        _transform_col(col, role, is_boolean, is_float, is_string, time_formats),
        role,
        subroles,
    )


# --------------------------------------------------------------------


def _with_role(
    base,
    cols: Union[
        str, FloatColumn, StringColumn, List[Union[str, FloatColumn, StringColumn]]
    ],
    role: str,
    time_formats: Optional[List[str]],
):

    # ------------------------------------------------------------

    time_formats = time_formats or constants.TIME_FORMATS

    # ------------------------------------------------------------

    names = _handle_cols(cols)

    if not isinstance(role, str):
        raise TypeError("'role' must be str.")

    if not _is_non_empty_typed_list(time_formats, str):
        raise TypeError("'time_formats' must be a non-empty list of str")

    # ------------------------------------------------------------

    for nname in names:
        if nname not in base.colnames:
            raise ValueError("No column called '" + nname + "' found.")

    if role not in _all_roles:
        raise ValueError(
            "'role' must be one of the following values: " + str(_all_roles)
        )

    # ------------------------------------------------------------

    view = base

    for name in names:
        col = view[name]
        unit = TIME_STAMP + COMPARISON_ONLY if role == time_stamp else col.unit
        view = view.with_column(
            col=col,
            name=name,
            role=role,
            subroles=col.subroles,
            unit=unit,
            time_formats=time_formats,
        )

    # ------------------------------------------------------------

    return view


# ------------------------------------------------------------


def _with_subroles(
    base,
    cols: Union[
        str, FloatColumn, StringColumn, List[Union[str, FloatColumn, StringColumn]]
    ],
    subroles: Union[str, List[str]],
    append: bool = False,
):

    names = _handle_cols(cols)

    if isinstance(subroles, str):
        subroles = [subroles]

    if not _is_non_empty_typed_list(names, str):
        raise TypeError("'names' must be either a string or a list of strings.")

    if not _is_typed_list(subroles, str):
        raise TypeError("'subroles' must be either a string or a list of strings.")

    if not isinstance(append, bool):
        raise TypeError("'append' must be a bool.")

    # ------------------------------------------------------------

    if [r for r in subroles if r not in _all_subroles]:
        raise ValueError(
            "'subroles' must be from getml.data.subroles, "
            + "meaning it is one of the following: "
            + str(_all_subroles)
        )

    # ------------------------------------------------------------

    view = base

    for name in names:
        col = view[name]
        view = view.with_column(
            col=col,
            name=name,
            role=view.roles.infer(name),
            subroles=subroles,
            unit=col.unit,
            time_formats=None,
        )

    # ------------------------------------------------------------

    return view


# ------------------------------------------------------------


def _with_unit(
    base,
    cols: Union[
        str, FloatColumn, StringColumn, List[Union[str, FloatColumn, StringColumn]]
    ],
    unit: str,
    comparison_only=False,
):

    names = _handle_cols(cols)

    if not isinstance(unit, str):
        raise TypeError("Parameter 'unit' must be a str.")

    # ------------------------------------------------------------

    if comparison_only:
        unit += constants.COMPARISON_ONLY

    # ------------------------------------------------------------

    view = base

    for name in names:
        col = view[name]
        view = view.with_column(
            col=col,
            name=name,
            role=view.roles.infer(name),
            subroles=col.subroles,
            unit=unit,
            time_formats=None,
        )

    # ------------------------------------------------------------

    return view


# --------------------------------------------------------------------
