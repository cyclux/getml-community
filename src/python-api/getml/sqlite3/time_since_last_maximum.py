# Copyright 2022 The SQLNet Company GmbH
#
# This file is licensed under the Elastic License 2.0 (ELv2).
# Refer to the LICENSE.txt file in the root of the repository
# for details.
#


"""
TIME_SINCE_LAST_MAXIMUM aggregation.
"""

import functools

from .helpers import _not_null

# ----------------------------------------------------------------------------


def _is_better(tup1, tup2):
    if tup2[1] == tup1[1]:
        if tup2[0] < tup1[0]:
            return tup2
        return tup1
    if tup2[1] > tup1[1]:
        return tup2
    return tup1


# ----------------------------------------------------------------------------


class _TimeSinceLastMaximum:
    def __init__(self):
        self.values = []

    def step(self, value, time_stamp):
        """
        Executed every time the function is called.
        """
        if _not_null(value) and _not_null(time_stamp):
            self.values.append((time_stamp, value))

    def finalize(self):
        """
        Executed after all values are inserted.
        """

        if not self.values:
            return None

        return functools.reduce(_is_better, self.values, self.values[0])[0]
