# Copyright 2022 The SQLNet Company GmbH
#
# This file is licensed under the Elastic License 2.0 (ELv2).
# Refer to the LICENSE.txt file in the root of the repository
# for details.
#


"""
Feature learning based on Gradient Boosting.
"""

from dataclasses import dataclass, field
from typing import Any, Dict, Optional, Union

from .fastprop import FastProp
from .feature_learner import _FeatureLearner
from .validation import _validate_relboost_parameters
from .loss_functions import CrossEntropyLossType, SquareLossType

# --------------------------------------------------------------------


@dataclass(repr=False)
class RelMT(_FeatureLearner):
    """Feature learning based on relational linear model trees.

    [`RelMT`][getml.feature_learning.RelMT] automates feature learning
    for relational data and time series. It is based on a
    generalization of linear model trees to relational data, hence
    the name. A linear model tree is a decision tree
    with linear models on its leaves.


    For more information on the underlying feature learning
    algorithm, check out the User Guide: [RelMT][feature-engineering-algorithms-relmt].

    enterprise-adm: Enterprise edition
        This feature is exclusive to the Enterprise edition and is not available in the Community edition. Discover the [benefits of the Enterprise edition][enterprise-benefits] and [compare their features][enterprise-feature-list].

        For licensing information and technical support, please [contact us][contact-page].

    Args:
        allow_avg:
            Whether to allow an AVG aggregation. Particularly for time
            series problems, AVG aggregations are not necessary and you
            can save some time by taking them out.

        delta_t:
            Frequency with which lag variables will be explored in a
            time series setting. When set to 0.0, there will be no lag
            variables.

            For more information, please refer to [Time Series][data-model-time-series] in the User Guide. Range: [0, ∞]

        gamma:
            During the training of RelMT, which is based on
            gradient tree boosting, this value serves as the minimum
            improvement in terms of the `loss_function` required for a
            split of the tree to be applied. Larger `gamma` will lead
            to fewer partitions of the tree and a more conservative
            algorithm. Range: [0, ∞]

        loss_function:
            Objective function used by the feature learning algorithm
            to optimize your features. For regression problems use
            [`SquareLoss`][getml.feature_learning.loss_functions.SQUARELOSS] and for
            classification problems use
            [`CrossEntropyLoss`][getml.feature_learning.loss_functions.CROSSENTROPYLOSS].

        max_depth:
            Maximum depth of the trees generated during the gradient
            tree boosting. Deeper trees will result in more complex
            models and increase the risk of overfitting. Range: [0,
            ∞]

        min_df:
            Only relevant for columns with role [`text`][getml.data.roles.text].
            The minimum
            number of fields (i.e. rows) in [`text`][getml.data.roles.text] column a
            given word is required to appear in to be included in the bag of words.
            Range: [1, ∞]

        min_num_samples:
            Determines the minimum number of samples a subcondition
            should apply to in order for it to be considered. Higher
            values lead to less complex statements and less danger of
            overfitting. Range: [1, ∞]

        num_features:
            Number of features generated by the feature learning
            algorithm. Range: [1, ∞]

        num_subfeatures:
            The number of subfeatures you would like to extract in a
            subensemble (for snowflake data model only). See
            :ref:`data_model_snowflake_schema` for more
            information. Range: [1, ∞]

        num_threads:
            Number of threads used by the feature learning algorithm. If set to
            zero or a negative value, the number of threads will be
            determined automatically by the getML Engine. Range:
            [-∞, ∞]

        propositionalization:
            The feature learner used for joins which are flagged to be
            propositionalized (by setting a join's `relationship` parameter to
            [`propositionalization`][getml.data.relationship.propositionalization])

        reg_lambda:
            L2 regularization on the weights in the gradient boosting
            routine. This is one of the most important hyperparameters
            in the [`RelMT`][getml.feature_learning.RelMT] as it allows
            for the most direct regularization. Larger values will
            make the resulting model more conservative. Range: [0,
            ∞]

        sampling_factor:
            RelMT uses a bootstrapping procedure (sampling with
            replacement) to train each of the features. The sampling
            factor is proportional to the share of the samples
            randomly drawn from the population table every time
            RelMT generates a new feature. A lower sampling factor
            (but still greater than 0.0), will lead to less danger of
            overfitting, less complex statements and faster
            training. When set to 1.0, roughly 20,000 samples are drawn
            from the population table. If the population table
            contains less than 20,000 samples, it will use standard
            bagging. When set to 0.0, there will be no sampling at
            all. Range: [0, ∞]

        seed:
            Seed used for the random number generator that underlies
            the sampling procedure to make the calculation
            reproducible. Internally, a `seed` of None will be mapped to
            5543. Range: [0, ∞]

        shrinkage:
            Since RelMT works using a gradient-boosting-like
            algorithm, `shrinkage` (or learning rate) scales down the
            weights and thus the impact of each new tree. This gives
            more room for future ones to improve the overall
            performance of the model in this greedy algorithm. It must
            be between 0.0 and 1.0 with higher values leading to more
            danger of overfitting. Range: [0, 1]

        silent:
            Controls the logging during training.

        vocab_size:
            Determines the maximum number
            of words that are extracted in total from [`text`][getml.data.roles.text]
            columns. This can be interpreted as the maximum size of the bag of words.
            Range: [0, ∞]

    """

    # ----------------------------------------------------------------

    allow_avg: bool = True
    delta_t: float = 0.0
    gamma: float = 0.0
    loss_function: Optional[Union[CrossEntropyLossType, SquareLossType]] = None
    max_depth: int = 2
    min_df: int = 30
    min_num_samples: int = 1
    num_features: int = 30
    num_subfeatures: int = 30
    num_threads: int = 0
    propositionalization: FastProp = field(default_factory=FastProp)
    reg_lambda: float = 0.0
    sampling_factor: float = 1.0
    seed: int = 5543
    shrinkage: float = 0.1
    silent: bool = True
    vocab_size: int = 500

    # ----------------------------------------------------------------

    def validate(self, params: Optional[Dict[str, Any]] = None) -> None:
        """
        Checks both the types and the values of all instance
        variables and raises an exception if something is off.

        Args:
            params: A dictionary containing
                the parameters to validate. If not is passed,
                the own parameters will be validated.
        """

        # ------------------------------------------------------------

        if params is None:
            params = self.__dict__
        else:
            params = {**self.__dict__, **params}

        # ------------------------------------------------------------

        if not isinstance(params, dict):
            raise ValueError("params must be None or a dictionary!")

        # ------------------------------------------------------------

        for kkey in params:
            if kkey not in type(self)._supported_params:
                raise KeyError(
                    f"Instance variable '{kkey}' is not supported in {self.type}."
                )

        # ------------------------------------------------------------

        if not isinstance(params["silent"], bool):
            raise TypeError("'silent' must be of type bool")

        # ------------------------------------------------------------

        _validate_relboost_parameters(**params)

        # ------------------------------------------------------------
