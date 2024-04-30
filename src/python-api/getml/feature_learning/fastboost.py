# Copyright 2022 The SQLNet Company GmbH
#
# This file is licensed under the Elastic License 2.0 (ELv2).
# Refer to the LICENSE.txt file in the root of the repository
# for details.
#


"""
Feature learning based on fast gradient boosting.
"""

from dataclasses import dataclass
from typing import Any, Dict, Literal, Optional

from .feature_learner import _FeatureLearner
from .validation import _validate_fastboost_parameters


@dataclass(repr=False)
class Fastboost(_FeatureLearner):
    """
    Feature learning based on Gradient Boosting.

    [`Fastboost`][getml.feature_learning.Fastboost] automates feature learning
    for relational data and time series. The algorithm used is slighly
    simpler than [`Relboost`][getml.feature_learning.Relboost] and much faster.

    Attributes:
        gamma:
            During the training of Fastboost, which is based on
            gradient tree boosting, this value serves as the minimum
            improvement in terms of the `loss_function` required for a
            split of the tree to be applied. Larger `gamma` will lead
            to fewer partitions of the tree and a more conservative
            algorithm. Range: [0, $\infty$]

        loss_function:
            Objective function used by the feature learning algorithm
            to optimize your features. For regression problems use
            [`SquareLoss`][getml.feature_learning.loss_functions.SquareLoss] and for
            classification problems use
            [`CrossEntropyLoss`][getml.feature_learning.loss_functions.CrossEntropyLoss].

        max_depth:
            Maximum depth of the trees generated during the gradient
            tree boosting. Deeper trees will result in more complex
            models and increase the risk of overfitting. Range: [0, $\infty$]

        min_child_weights:
            Determines the minimum sum of the weights a subcondition
            should apply to in order for it to be considered. Higher
            values lead to less complex statements and less danger of
            overfitting. Range: [1, $\infty$]

        num_features:
            Number of features generated by the feature learning
            algorithm. Range: [1, $\infty$]

        num_threads:
            Number of threads used by the feature learning algorithm. If set to
            zero or a negative value, the number of threads will be
            determined automatically by the getML engine. Range:
            [$0$, $\infty$]

        reg_lambda:
            L2 regularization on the weights in the gradient boosting
            routine. This is one of the most important hyperparameters
            in the [`Relboost`][getml.feature_learning.Relboost] as it allows
            for the most direct regularization. Larger values will
            make the resulting model more conservative. Range: [0,
            $\infty$]

        seed:
            Seed used for the random number generator that underlies
            the sampling procedure to make the calculation
            reproducible. Internally, a `seed` of None will be mapped to
            5543. Range: [0, $\infty$]

        shrinkage:
            Since Fastboost works using a gradient-boosting-like
            algorithm, `shrinkage` (or learning rate) scales down the
            weights and thus the impact of each new tree. This gives
            more room for future ones to improve the overall
            performance of the model in this greedy algorithm. It must
            be between 0.0 and 1.0 with higher values leading to a higher
            danger of overfitting. Range: [0, 1]

        silent:
            Controls the logging during training.

        subsample:
            Fastboost uses a bootstrapping procedure (sampling with
            replacement) to train each of the features. The sampling
            factor is proportional to the share of the samples
            randomly drawn from the population table every time
            Fastboost generates a new feature. A lower sampling factor
            (but still greater than 0.0), will lead to less danger of
            overfitting, less complex statements and faster
            training. When set to 1.0, the number of samples drawn will
            be identical to the size of the population table.
            When set to 0.0, there will be no sampling at all.
            Range: [0, 1]

    Note:
        Not supported in the getML community edition.

    """

    gamma: float = 0.0
    loss_function: Optional[Literal["CrossEntropyLoss", "SquareLoss"]] = None
    max_depth: int = 5
    min_child_weights: float = 1.0
    num_features: int = 100
    num_threads: int = 1
    reg_lambda: float = 1.0
    seed: int = 5543
    shrinkage: float = 0.1
    silent: bool = True
    subsample: float = 1.0

    def validate(self, params: Optional[Dict[str, Any]] = None) -> None:
        """
        Checks both the types and the values of all instance
        variables and raises an exception if something is off.

        Args:
            params:
                A dictionary containing the parameters to validate.
                params can hold the full set or a subset of the
                parameters explained in
                [`Fastboost`][getml.feature_learning.Fastboost].
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

        unsupported_params = [
            k for k in params if k not in type(self)._supported_params
        ]

        if unsupported_params:
            raise KeyError(
                "The following instance variables are not supported "
                + f"in {self.type}: {unsupported_params}"
            )

        _validate_fastboost_parameters(**params)
