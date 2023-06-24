# Copyright 2022 The SQLNet Company GmbH
#
# This file is licensed under the Elastic License 2.0 (ELv2).
# Refer to the LICENSE.txt file in the root of the repository
# for details.
#


"""
NUM_MIN aggregation.
"""

from typing import Optional

import numpy as np

from .helpers import _not_null


class _NumMin:
    def __init__(self):
        self.values = []

    def step(self, value: Optional[float]):
        """
        Executed every time the function is called.
        """
        if _not_null(value) and value is not None:
            self.values.append(value)

    def finalize(self) -> Optional[float]:
        """
        Executed after all values are inserted.
        """
        if not self.values:
            return None
        minimum = np.min(self.values)
        filtered = [0.0 for v in self.values if v == minimum]
        return float(len(filtered))
