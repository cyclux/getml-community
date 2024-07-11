# Copyright 2022 The SQLNet Company GmbH
#
# This file is licensed under the Elastic License 2.0 (ELv2).
# Refer to the LICENSE.txt file in the root of the repository
# for details.
#

from typing import FrozenSet

from getml.data import subroles
from getml.data.subroles.types import ExcludeLike, IncludeLike, OnlyLike, Subrole

exclude: FrozenSet[ExcludeLike] = frozenset(
    {
        subroles.exclude.category_trimmer,
        subroles.exclude.fastprop,
        subroles.exclude.feature_learners,
        subroles.exclude.imputation,
        subroles.exclude.mapping,
        subroles.exclude.multirel,
        subroles.exclude.predictors,
        subroles.exclude.preprocessors,
        subroles.exclude.relboost,
        subroles.exclude.relmt,
        subroles.exclude.seasonal,
        subroles.exclude.text_field_splitter,
    }
)

include: FrozenSet[IncludeLike] = frozenset(
    {
        subroles.include.email,
        subroles.include.substring,
    }
)

only: FrozenSet[OnlyLike] = frozenset(
    {
        subroles.only.email,
        subroles.only.substring,
    }
)

all_: FrozenSet[Subrole] = exclude | include | only
