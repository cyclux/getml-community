# Copyright 2022 The SQLNet Company GmbH
#
# This file is licensed under the Elastic License 2.0 (ELv2).
# Refer to the LICENSE.txt file in the root of the repository
# for details.
#


"""
This module helps you handle your current project.
"""

from sys import modules
from types import ModuleType
from typing import TYPE_CHECKING

from getml.engine import is_alive

from .attrs import __getattr__, _all


if TYPE_CHECKING:
    from .containers import DataFrames, Hyperopts, Pipelines
    from .attrs import load, delete, restart, save, suspend, switch  # noqa: F401

    data_frames: DataFrames
    hyperopts: Hyperopts
    pipelines: Pipelines
    name: str


def __dir__():
    return _all


__all__ = _all + ["DataFrames", "Hyperopts", "Pipelines"] + ["__getattr__"]  # noqa: PLE0605


class ProjectModule(ModuleType):
    def __repr__(self):
        output = ""
        # resolving name has the side effect of reaching out to the monitor and
        # the engine and therefore triggers a message if either is unreachable
        project_name = self.name  # pylint: disable=E1101
        if is_alive():
            output += f"Current project:\n\n{project_name}"
        return output


modules[__name__].__class__ = ProjectModule
