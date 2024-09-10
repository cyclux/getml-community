# Copyright 2024 Code17 GmbH
#
# This file is licensed under the Elastic License 2.0 (ELv2).
# Refer to the LICENSE.txt file in the root of the repository
# for details.
#


"""
Feature learning based on propositionalization.
"""

from dataclasses import dataclass
from typing import Any, ClassVar, Dict, Optional, Union, Iterable

from .aggregations.sets import (
    FASTPROP,
    FastPropAggregationsSets,
)
from .aggregations.types import FastPropAggregations
from .feature_learner import _FeatureLearner
from .validation import _validate_fastprop_parameters
from .loss_functions import CrossEntropyLossType, SquareLossType


@dataclass(repr=False)
class FastProp(_FeatureLearner):
    """
    Generates simple features based on propositionalization.

    [`FastProp`][getml.feature_learning.FastProp] generates simple and easily
    interpretable features for relational data and time series. It is based on a
    propositionalization approach and has been optimized for speed and memory efficiency.
    [`FastProp`][getml.feature_learning.FastProp] generates a large number
    of features and selects the most relevant ones based on the pair-wise correlation
    with the target(s).

    It is recommended to combine [`FastProp`][getml.feature_learning.FastProp] with
    the [`Mapping`][getml.preprocessors.Mapping] and [`Seasonal`][getml.preprocessors.Seasonal]
    preprocessors, which can drastically improve predictive accuracy.

    For more information on the underlying feature learning algorithm, check
    out the User guide: [FastProp][feature-engineering-algorithms-fastprop].

    Attributes:
        agg_sets:
            It is a class variable holding the available aggregation sets for the
            FastProp feature learner.
            Value: [`FASTPROP`][getml.feature_learning.aggregations.FASTPROP].

    Parameters:
        aggregation:
            Mathematical operations used by the automated feature
            learning algorithm to create new features.

            Must be an aggregation supported by FastProp feature learner
            ([`FASTPROP_AGGREGATIONS`][getml.feature_learning.aggregations.FASTPROP_AGGREGATIONS]).

        delta_t:
            Frequency with which lag variables will be explored in a
            time series setting. When set to 0.0, there will be no lag
            variables. Please note that you must also pass a value to
            max_lag.

            For more information please refer to
            [Data Model Time Series][data-model-time-series]. Range: [0, ∞]

        loss_function:
            Objective function used by the feature learning algorithm
            to optimize your features. For regression problems use
            [`SquareLoss`][getml.feature_learning.loss_functions.SQUARELOSS] and for
            classification problems use
            [`CrossEntropyLoss`][getml.feature_learning.loss_functions.CROSSENTROPYLOSS].

        max_lag:
            Maximum number of steps taken into the past to form lag variables. The
            step size is determined by delta_t. Please note that you must also pass
            a value to delta_t.

            For more information please refer to
            [Time Series][data-model-time-series]. Range: [0, ∞]

        min_df:
            Only relevant for columns with role [`text`][getml.data.roles.text].
            The minimum
            number of fields (i.e. rows) in [`text`][getml.data.roles.text] column a
            given word is required to appear in to be included in the bag of words.
            Range: [1, ∞]

        num_features:
            Number of features generated by the feature learning
            algorithm. Range: [1, ∞]

        n_most_frequent:
            [`FastProp`][getml.feature_learning.FastProp] can find the N most frequent
            categories in a categorical column and derive features from them.
            The parameter determines how many categories should be used.
            Range: [0, ∞]

        num_threads:
            Number of threads used by the feature learning algorithm. If set to
            zero or a negative value, the number of threads will be
            determined automatically by the getML Engine. Range:
            [0, ∞]

        sampling_factor:
            FastProp uses a bootstrapping procedure (sampling with replacement) to train
            each of the features. The sampling factor is proportional to the share of
            the samples randomly drawn from the population table every time Multirel
            generates a new feature. A lower sampling factor (but still greater than
            0.0), will lead to less danger of overfitting, less complex statements and
            faster training. When set to 1.0, roughly 2,000 samples are drawn from the
            population table. If the population table contains less than 2,000 samples,
            it will use standard bagging. When set to 0.0, there will be no sampling at
            all. Range: [0, ∞]

        silent:
            Controls the logging during training.

        vocab_size:
            Determines the maximum number
            of words that are extracted in total from [`text`][getml.data.roles.text]
            columns. This can be interpreted as the maximum size of the bag of words.
            Range: [0, ∞]

    """

    agg_sets: ClassVar[FastPropAggregationsSets] = FASTPROP

    aggregation: Iterable[FastPropAggregations] = FASTPROP.default
    delta_t: float = 0.0
    loss_function: Optional[Union[CrossEntropyLossType, SquareLossType]] = None
    max_lag: int = 0
    min_df: int = 30
    n_most_frequent: int = 0
    num_features: int = 200
    num_threads: int = 0
    sampling_factor: float = 1.0
    silent: bool = True
    vocab_size: int = 500

    def validate(self, params: Optional[Dict[str, Any]] = None) -> None:
        """
        Checks both the types and the values of all instance
        variables and raises an exception if something is off.

        Args:
            params:
                A dictionary containing the parameters to validate.
                params can hold the full set or a subset of the
                parameters explained in
                [`FastProp`][getml.feature_learning.FastProp].
                If params is None, the
                current set of parameters in the
                instance dictionary will be validated.


        """

        if params is None:
            params = self.__dict__
        else:
            params = {**self.__dict__, **params}

        if not isinstance(params, dict):
            raise ValueError("params must be None or a dictionary!")

        for kkey in params:
            if kkey not in type(self)._supported_params:
                raise KeyError(
                    f"Instance variable '{kkey}' is not supported in {self.type}."
                )

        _validate_fastprop_parameters(**params)
