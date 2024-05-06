# Copyright 2022 The SQLNet Company GmbH
#
# This file is licensed under the Elastic License 2.0 (ELv2).
# Refer to the LICENSE.txt file in the root of the repository
# for details.
#


"""
A gradient boosting model for predicting regression problems.
"""

from dataclasses import dataclass

from .predictor import _Predictor
from .scale_gbm_classifier import _validate_scalegbm_parameters


@dataclass(repr=False)
class ScaleGBMRegressor(_Predictor):
    """Standard gradient boosting regressor that fully supports memory mapping
   and can be used for datasets that do not fit into memory.

Gradient tree boosting trains an ensemble of decision trees by training
each tree to predict the *prediction error of all previous trees* in the
ensemble:

$$
\min_{\\nabla f_{t,i}} \sum_i L(f_{t-1,i} + \\nabla f_{t,i}; y_i),
$$

where $\\nabla f_{t,i}$ is the prediction generated by the
newest decision tree for sample $i$ and $f_{t-1,i}$ is
the prediction generated by all previous trees, $L(...)$ is
the loss function used and $y_i$ is the [target][annotating-data-target] we are trying to predict.

The regressor implements this general approach by adding two specific components:

1. The loss function $L(...)$ is approximated using a Taylor series.

2. The leaves of the decision tree $\\nabla f_{t,i}$ contain weights
   that can be regularized.

These weights are calculated as follows:

$$
w_l = -\\frac{\sum_{i \in l} g_i}{ \sum_{i \in l} h_i + \lambda},
$$

where $g_i$ and $h_i$ are the first and second order derivative
of $L(...)$ w.r.t. $f_{t-1,i}$, $w_l$ denotes the weight
on leaf $l$ and $i \in l$ denotes all samples on that leaf.

$\lambda$ is the regularization parameter `reg_lambda`.
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

        Range: (0, $\infty$]

    gamma (float, optional):
        Minimum loss reduction required for any update
        to the tree. This means that every potential update
        will first be evaluated for its improvement to the loss
        function. If the improvement exceeds gamma,
        the update will be accepted.

        *Increasing* this hyperparameter reduces the
        likelihood of overfitting.

        Range: [0, $\infty$]

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
        When a new tree $\\nabla f_{t,i}$ is trained,
        it will be added to the existing trees
        $f_{t-1,i}$. Before doing so, it will be
        multiplied by the *learning_rate*.

        *Decreasing* this hyperparameter reduces the
        likelihood of overfitting.

        Range: [0, 1]

    max_depth (int, optional):
        Maximum allowed depth of the trees.

        *Decreasing* this hyperparameter reduces the
        likelihood of overfitting.

        Range: [0, $\infty$]

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

        Range: [0, $\infty$]

    n_estimators (int, optional):
        Number of estimators (trees).

        *Decreasing* this hyperparameter reduces the
        likelihood of overfitting.

        Range: [10, $\infty$]

    n_jobs (int, optional):
        Number of parallel threads. When set to zero, then
        the optimal number of threads will be inferred automatically.

        Range: [0, $\infty$]

    reg_lambda (float, optional):
        L2 regularization on the weights. Please refer to
        the introductory remarks to understand how this
        hyperparameter influences your weights.

        *Increasing* this hyperparameter reduces the
        likelihood of overfitting.

        Range: [0, $\infty$]

    seed (int, optional):
        Seed used for random sampling and other random
        factors.

        Range: [0, $\infty$]
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
    objective: str = "reg:squarederror"
    reg_lambda: float = 1.0
    seed: int = 5843

    def validate(self, params=None):
        """Checks both the types and the values of all instance
        variables and raises an exception if something is off.

        Args:
            params (dict, optional): A dictionary containing
                the parameters to validate. If not is passed,
                the own parameters will be validated.

        Note:
            This method is called at end of the \_\_init\_\_ constructor
            and every time before the predictor - or a class holding
            it as an instance variable - is sent to the getML engine.
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
