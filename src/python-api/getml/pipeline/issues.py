# Copyright 2022 The SQLNet Company GmbH
#
# This file is licensed under the Elastic License 2.0 (ELv2).
# Refer to the LICENSE.txt file in the root of the repository
# for details.
#


"""
A helper class meant to represent the
warnings generated by the check method of
the pipeline.
"""

from __future__ import annotations

from typing import Sequence

from getml.communication import _Issue
from getml.utilities.formatting import _Formatter


class Issues:
    """
    A helper class meant to represent the
    warnings generated by the check method of
    the pipeline.

    Args:
        data:
            The warnings generated by the check method
            of the pipeline.
    """

    def __init__(self, data: Sequence[_Issue]) -> None:
        self.data = data

    def __iter__(self):
        yield from self.data

    def __len__(self) -> int:
        return len(self.data)

    def __repr__(self) -> str:
        return self._format()._render_string()

    def _repr_html_(self) -> str:
        return self._format()._render_html()

    def _format(self) -> _Formatter:
        headers = ["type", "label", "message"]
        rows = [[w.warning_type, w.label, w.message] for w in self.data]
        return _Formatter([headers], rows)

    @property
    def info(self) -> Issues:
        """
        Returns a new container with all warnings
        labeled INFO.

        Returns:
            A new container with all warnings
            labeled INFO.
        """
        return Issues([d for d in self.data if d.warning_type == "INFO"])

    @property
    def warnings(self) -> Issues:
        """
        Returns a new container with all warnings
        labeled WARNING.

        Returns:
            A new container with all warnings
            labeled WARNING.
        """
        return Issues([d for d in self.data if d.warning_type == "WARNING"])
