# Copyright 2022 The SQLNet Company GmbH
#
# This file is licensed under the Elastic License 2.0 (ELv2).
# Refer to the LICENSE.txt file in the root of the repository
# for details.
#


"""
A class with a nice repr for suppressing parameter values.
"""


class _Ellipsis(str):
    """
    A class with a nice repr for suppressing parameter values.
    """

    def __repr__(self):
        return "..."
