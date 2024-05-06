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
from .xgboost_classifier import _validate_xgboost_parameters

# ------------------------------------------------------------------------------


@dataclass(repr=False)
class XGBoostRegressor(_Predictor):
    """Gradient boosting regressor based on [xgboost ](https://xgboost.readthedocs.io/en/latest/).

XGBoost is an implementation of the gradient tree boosting algorithm that
is widely recognized for its efficiency and predictive accuracy.

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

XGBoost implements this general approach by adding two specific components:

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
    booster (string, optional):
        Which base classifier to use.

        Possible values:

        * `gbtree`: normal gradient boosted decision trees
        * `gblinear`: uses a linear model instead of decision trees
        * 'dart': adds dropout to the standard gradient boosting algorithm.
          Please also refer to the remarks on *rate_drop* for further
          explanation on `dart`.

    colsample_bylevel (float, optional):
        Subsample ratio for the columns used, for each level
        inside a tree.

        Note that XGBoost grows its trees level-by-level, not
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

    external_memory (bool, optional):
        When the in_memory flag of the engine is set to False,
        XGBoost can use the external memory functionality.
        This reduces the memory consumption,
        but can also affect the quality of the predictions.
        External memory is deactivated by default and it
        is recommended to only use external memory
        for feature selection.
        When the in_memory flag of the engine is set to True,
        (the default value), XGBoost will never use
        external memory.

    gamma (float, optional):
        Minimum loss reduction required for any update
        to the tree. This means that every potential update
        will first be evaluated for its improvement to the loss
        function. If the improvement exceeds gamma,
        the update will be accepted.

        *Increasing* this hyperparameter reduces the
        likelihood of overfitting.

        Range: [0, $\infty$]

    learning_rate (float, optional):
        Learning rate for the gradient boosting algorithm.
        When a new tree $\\nabla f_{t,i}$ is trained,
        it will be added to the existing trees
        $f_{t-1,i}$. Before doing so, it will be
        multiplied by the *learning_rate*.

        *Decreasing* this hyperparameter reduces the
        likelihood of overfitting.

        Range: [0, 1]

    max_delta_step (float, optional):
        The maximum delta step allowed for the weight estimation
        of each tree.

        *Decreasing* this hyperparameter reduces the
        likelihood of overfitting.

        Range: [0, $\infty$)

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

    normalize_type (string, optional):
        This determines how to normalize trees during 'dart'.

        Possible values:

        * `tree`: a new tree has the same weight as a single
          dropped tree.

        * `forest`: a new tree has the same weight as the sum of
          all dropped trees.

        Please also refer to the remarks on
        *rate_drop* for further explanation.

        Will be ignored if `booster` is not set to `dart`.

    n_jobs (int, optional):
        Number of parallel threads. When set to zero, then
        the optimal number of threads will be inferred automatically.

        Range: [0, $\infty$]

    objective (string, optional):
        Specify the learning task and the corresponding
        learning objective.

        Possible values:

        * `reg:squarederror`
        * `reg:tweedie`

    one_drop (bool, optional):
        If set to True, then at least one tree will always be
        dropped out. Setting this hyperparameter to *true* reduces
        the likelihood of overfitting.

        Please also refer to the remarks on
        *rate_drop* for further explanation.

        Will be ignored if `booster` is not set to 'dart'.

    rate_drop (float, optional):
        Dropout rate for trees - determines the probability
        that a tree will be dropped out. Dropout is an
        algorithm that enjoys considerable popularity in
        the deep learning community. It means that every node can
        be randomly removed during training.

        This approach
        can also be applied to gradient boosting, where it
        means that every tree can be randomly removed with
        a certain probability. Said probability is determined
        by *rate_drop*. Dropout for gradient boosting is
        referred to as the 'dart' algorithm.

        *Increasing* this hyperparameter reduces the
        likelihood of overfitting.

        Will be ignored if `booster` is not set to `dart`.

    reg_alpha (float, optional):
        L1 regularization on the weights.

        *Increasing* this hyperparameter reduces the
        likelihood of overfitting.

        Range: [0, $\infty$]

    reg_lambda (float, optional):
        L2 regularization on the weights. Please refer to
        the introductory remarks to understand how this
        hyperparameter influences your weights.

        *Increasing* this hyperparameter reduces the
        likelihood of overfitting.

        Range: [0, $\infty$]

    sample_type (string, optional):
        Possible values:

        * `uniform`: every tree is equally likely to be dropped
          out

        * `weighted`: the dropout probability will be proportional
          to a tree's weight

        Please also refer to the remarks on
        *rate_drop* for further explanation.

        Will be ignored if `booster` is not set to 'dart'.

    silent (bool, optional):
        In silent mode, XGBoost will not print out information on
        the training progress.

    skip_drop (float, optional):
        Probability of skipping the dropout during a given
        iteration. Please also refer to the remarks on
        *rate_drop* for further explanation.

        *Increasing* this hyperparameter reduces the
        likelihood of overfitting.

        Will be ignored if `booster` is not set to `dart`.

        Range: [0, 1]

    subsample (float, optional):
        Subsample ratio from the training set. This means
        that for every tree a subselection of *samples*
        from the training set will be included into training.
        Please note that this samples *without* replacement -
        the common approach for random forests is to sample
        *with* replace.

        *Decreasing* this hyperparameter reduces the
        likelihood of overfitting.

        Range: (0, 1]
    """

    booster: str = "gbtree"
    colsample_bylevel: float = 1.0
    colsample_bytree: float = 1.0
    early_stopping_rounds: int = 10
    external_memory: bool = False
    gamma: float = 0.0
    learning_rate: float = 0.1
    max_delta_step: float = 0.0
    max_depth: int = 3
    min_child_weights: float = 1.0
    n_estimators: int = 100
    normalize_type: str = "tree"
    num_parallel_tree: int = 1
    n_jobs: int = 1
    objective: str = "reg:squarederror"
    one_drop: bool = False
    rate_drop: float = 0.0
    reg_alpha: float = 0.0
    reg_lambda: float = 1.0
    sample_type: str = "uniform"
    silent: bool = True
    skip_drop: float = 0.0
    subsample: float = 1.0

    # ----------------------------------------------------------------

    def validate(self, params=None):
        """Checks both the types and the values of all instance
        variables and raises an exception if something is off.

        Args:
            params (dict, optional): A dictionary containing
                the parameters to validate. If not is passed,
                the own parameters will be validated.

        Example:
            ```python
            x = getml.predictors.XGBoostRegressor()
            x.gamma = 200
            x.validate()
            ```

        Note:
            This method is called at end of the \_\_init\_\_ constructor
            and every time before the predictor - or a class holding
            it as an instance variable - is sent to the getML engine.
        """

        # ------------------------------------------------------------

        if params is None:
            params = self.__dict__
        else:
            params = {**self.__dict__, **params}

        if not isinstance(params, dict):
            raise ValueError("params must be None or a dictionary!")

        _validate_xgboost_parameters(params)

        # ------------------------------------------------------------

        if params["objective"] not in ["reg:squarederror", "reg:tweedie", "reg:linear"]:
            raise ValueError(
                """'objective' supported in XGBoostRegressor
                                 are 'reg:squarederror', 'reg:tweedie',
                                 and 'reg:linear'"""
            )


# ------------------------------------------------------------------------------
