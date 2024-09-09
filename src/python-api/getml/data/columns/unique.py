# Copyright 2024 Code17 GmbH
#
# This file is licensed under the Elastic License 2.0 (ELv2).
# Refer to the LICENSE.txt file in the root of the repository
# for details.
#


"""
Transform column to numpy array containing unique values
"""

import numpy as np

from .to_arrow import _to_arrow


def _unique(self) -> np.ndarray:
    """
    Transform column to numpy array containing all distinct values.
    """
    return _to_arrow(self, unique=True).to_numpy()
