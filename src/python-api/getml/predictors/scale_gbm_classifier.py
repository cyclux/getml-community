# Copyright 2022 The SQLNet Company GmbH
#
# This file is licensed under the Elastic License 2.0 (ELv2).
# Refer to the LICENSE.txt file in the root of the repository
# for details.
#


"""
A gradient boosting model for predicting classification problems.
"""

import numbers
from dataclasses import dataclass
from typing import Any, Dict

import numpy as np

from getml.helpers import _check_parameter_bounds

from .predictor import _Predictor


def _validate_scalegbm_parameters(parameters: Dict[str, Any]):
    for kkey in parameters:
        if kkey == "colsample_bylevel":
            if not isinstance(parameters["colsample_bylevel"], numbers.Real):
                raise TypeError("'colsample_bylevel' must be a real number")
            _check_parameter_bounds(
                parameters["colsample_bylevel"],
                "colsample_bylevel",
                [np.finfo(np.float64).resolution, 1.0],  # pylint: disable=E1101
            )

        if kkey == "colsample_bytree":
            if not isinstance(parameters["colsample_bytree"], numbers.Real):
                raise TypeError("'colsample_bytree' must be a real number")
            _check_parameter_bounds(
                parameters["colsample_bytree"],
                "colsample_bytree",
                [np.finfo(np.float64).resolution, 1.0],  # pylint: disable=E1101
            )

        if kkey == "early_stopping_rounds":
            if not isinstance(parameters[kkey], numbers.Real):
                raise TypeError(f"'{kkey}' must be a real number")
            _check_parameter_bounds(parameters[kkey], kkey, [0, np.iinfo(np.int32).max])

        if kkey == "gamma":
            if not isinstance(parameters["gamma"], numbers.Real):
                raise TypeError("'gamma' must be a real number")
            _check_parameter_bounds(
                parameters["gamma"], "gamma", [0.0, np.finfo(np.float64).max]
            )

        if kkey == "goss_a":
            if not isinstance(parameters[kkey], numbers.Real):
                raise TypeError(f"'{kkey}' must be a real number")
            _check_parameter_bounds(
                parameters[kkey], kkey, [0, np.finfo(np.float64).max]
            )

        if kkey == "goss_b":
            if not isinstance(parameters[kkey], numbers.Real):
                raise TypeError(f"'{kkey}' must be a real number")
            _check_parameter_bounds(
                parameters[kkey], kkey, [0, np.finfo(np.float64).max]
            )

        if kkey == "learning_rate":
            if not isinstance(parameters["learning_rate"], numbers.Real):
                raise TypeError("'learning_rate' must be a real number")
            _check_parameter_bounds(
                parameters["learning_rate"], "learning_rate", [0.0, 1.0]
            )

        if kkey == "max_depth":
            if not isinstance(parameters["max_depth"], numbers.Real):
                raise TypeError("'max_depth' must be a real number")
            _check_parameter_bounds(
                parameters["max_depth"], "max_depth", [0.0, np.iinfo(np.int32).max]
            )

        if kkey == "min_child_weights":
            if not isinstance(parameters["min_child_weights"], numbers.Real):
                raise TypeError("'min_child_weights' must be a real number")
            _check_parameter_bounds(
                parameters["min_child_weights"],
                "min_child_weights",
                [0.0, np.finfo(np.float64).max],
            )

        if kkey == "n_estimators":
            if not isinstance(parameters["n_estimators"], numbers.Real):
                raise TypeError("'n_estimators' must be a real number")
            _check_parameter_bounds(
                parameters["n_estimators"], "n_estimators", [10, np.iinfo(np.int32).max]
            )

        if kkey == "n_jobs":
            if not isinstance(parameters["n_jobs"], numbers.Real):
                raise TypeError("'n_jobs' must be a real number")
            _check_parameter_bounds(
                parameters["n_jobs"], "n_jobs", [0, np.iinfo(np.int32).max]
            )

        if kkey == "reg_lambda":
            if not isinstance(parameters["reg_lambda"], numbers.Real):
                raise TypeError("'reg_lambda' must be a real number")
            _check_parameter_bounds(
                parameters["reg_lambda"], "reg_lambda", [0.0, np.finfo(np.float64).max]
            )

        if kkey == "seed":
            if not isinstance(parameters[kkey], numbers.Real):
                raise TypeError(f"'{kkey}' must be a real number")
            _check_parameter_bounds(parameters[kkey], kkey, [0, np.iinfo(np.int32).max])


@dataclass(repr=False)
class ScaleGBMClassifier(_Predictor):
    """Standard gradient boosting classifier that fully supports memory mapping
       and can be used for datasets that do not fit into memory.


    Gradient tree boosting trains an ensemble of decision trees by training
    each tree to predict the *prediction error of all previous trees* in the
    ensemble:

    .. math::

        \\min_{\\nabla f_{t,i}} \\sum_i L(f_{t-1,i} + \\nabla f_{t,i}; y_i),

    where :math:`\\nabla f_{t,i}` is the prediction generated by the
    newest decision tree for sample :math:`i` and :math:`f_{t-1,i}` is
    the prediction generated by all previous trees, :math:`L(...)` is
    the loss function used and :math:`y_i` is the :ref:`target
    <annotating_roles_target>` we are trying to predict.

    XGBoost implements this general approach by adding two specific components:

    1. The loss function :math:`L(...)` is approximated using a Taylor series.

    2. The leaves of the decision tree :math:`\\nabla f_{t,i}` contain weights
       that can be regularized.

    These weights are calculated as follows:

    .. math::

        w_l = -\\frac{\\sum_{i \\in l} g_i}{ \\sum_{i \\in l} h_i + \\lambda},

    where :math:`g_i` and :math:`h_i` are the first and second order derivative
    of :math:`L(...)` w.r.t. :math:`f_{t-1,i}`, :math:`w_l` denotes the weight
    on leaf :math:`l` and :math:`i \\in l` denotes all samples on that leaf.

    :math:`\\lambda` is the regularization parameter `reg_lambda`.
    This hyperparameter can be set by the users or the hyperparameter
    optimization algorithm to avoid overfitting.

    Args:
        colsample_bylevel (float, optional):
            Subsample ratio for the columns used, for each level
            inside a tree.

            Note that ScaleGBM grows its trees level-by-level, not
            node-by-node.
            At each level, a subselection of the features will be randomly
            picked and the best
            feature for each split will be chosen. This hyperparameter
            determines the share of features randomly picked at each level.
            When set to 1, then now such sampling takes place.

            *Decreasing* this hyperparameter reduces the
            likelihood of overfitting.

            Range: (0, 1]

        colsample_bytree (float, optional):
            Subsample ratio for the columns used, for each tree.
            This means that for each tree, a subselection
            of the features will be randomly chosen. This hyperparameter
            determines the share of features randomly picked for each tree.

            *Decreasing* this hyperparameter reduces the
            likelihood of overfitting.

            Range: (0, 1]

        early_stopping_rounds (int, optional):
            The number of early_stopping_rounds for which we see
            no improvement on the validation set until we stop
            the training process.

            Range: (0, :math:`\\infty`]

        gamma (float, optional):
            Minimum loss reduction required for any update
            to the tree. This means that every potential update
            will first be evaluated for its improvement to the loss
            function. If the improvement exceeds gamma,
            the update will be accepted.

            *Increasing* this hyperparameter reduces the
            likelihood of overfitting.

            Range: [0, :math:`\\infty`]

        goss_a (float, optional):
            Share of the samples with the largest residuals
            taken for each tree.

            If `goss_a` is set to 1, then gradients one-sided
            sampling is effectively turned off.

            Range: [0, 1]

        goss_b (float, optional):
            Share of the samples that are not in the `goss_a`
            percentile of largest residuals randomly sampled
            for each tree.

            The sum of `goss_a` and `goss_b` cannot exceed
            1.

            Range: [0, 1]

        learning_rate (float, optional):
            Learning rate for the gradient boosting algorithm.
            When a new tree :math:`\\nabla f_{t,i}` is trained,
            it will be added to the existing trees
            :math:`f_{t-1,i}`. Before doing so, it will be
            multiplied by the *learning_rate*.

            *Decreasing* this hyperparameter reduces the
            likelihood of overfitting.

            Range: [0, 1]

        max_depth (int, optional):
            Maximum allowed depth of the trees.

            *Decreasing* this hyperparameter reduces the
            likelihood of overfitting.

            Range: [0, :math:`\\infty`]

        min_child_weights (float, optional):
            Minimum sum of weights needed in each child node for a
            split. The idea here is that any leaf should have
            a minimum number of samples in order to avoid overfitting.
            This very common form of regularizing decision trees is
            slightly
            modified to refer to weights instead of number of samples,
            but the basic idea is the same.

            *Increasing* this hyperparameter reduces the
            likelihood of overfitting.

            Range: [0, :math:`\\infty`]

        n_estimators (int, optional):
            Number of estimators (trees).

            *Decreasing* this hyperparameter reduces the
            likelihood of overfitting.

            Range: [10, :math:`\\infty`]

        n_jobs (int, optional):
            Number of parallel threads. When set to zero, then
            the optimal number of threads will be inferred automatically.

            Range: [0, :math:`\\infty`]

        reg_lambda (float, optional):
            L2 regularization on the weights. Please refer to
            the introductory remarks to understand how this
            hyperparameter influences your weights.

            *Increasing* this hyperparameter reduces the
            likelihood of overfitting.

            Range: [0, :math:`\\infty`]

        seed (int, optional):
            Seed used for the random sampling and other random
            factors.

            Range: [0, :math:`\\infty`]
    """

    colsample_bylevel: float = 1.0
    colsample_bytree: float = 1.0
    early_stopping_rounds: int = 10
    gamma: float = 0.0
    goss_a: float = 1.0
    goss_b: float = 0.0
    learning_rate: float = 0.1
    max_depth: int = 3
    min_child_weights: float = 1.0
    n_estimators: int = 100
    n_jobs: int = 1
    objective: str = "binary:logistic"
    reg_lambda: float = 1.0
    seed: int = 5843

    # ----------------------------------------------------------------

    def validate(self, params=None):
        """Checks both the types and the values of all instance
        variables and raises an exception if something is off.

        Args:
            params (dict, optional): A dictionary containing
                the parameters to validate. If not is passed,
                the own parameters will be validated.

        Examples:

            .. code-block:: python

                x = getml.predictors.XGBoostClassifier()
                x.gamma = 200
                x.validate()

        Note:

            This method is called at end of the __init__ constructor
            and every time before the predictor - or a class holding
            it as an instance variable - is send to the getML engine.
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

        _validate_scalegbm_parameters(params)
