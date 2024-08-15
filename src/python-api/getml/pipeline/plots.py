# Copyright 2022 The SQLNet Company GmbH
#
# This file is licensed under the Elastic License 2.0 (ELv2).
# Refer to the LICENSE.txt file in the root of the repository
# for details.
#


"""
Custom class for handling the plots of a pipeline.
"""

import json
from typing import Any, Dict, Tuple

import numpy as np

import getml.communication as comm


class Plots:
    """
    Custom class for handling the
    plots generated by the pipeline.

    Args:
        name:
            The id of the pipeline the plots are associated with.

    ??? example
        ```python
        recall, precision = my_pipeline.plots.precision_recall_curve()
        fpr, tpr = my_pipeline.plots.roc_curve()
        ```

    """

    # ----------------------------------------------------------------

    def __init__(self, name: str) -> None:
        if not isinstance(name, str):
            raise ValueError("'name' must be a str.")

        self.name = name

    # ------------------------------------------------------------

    def lift_curve(self, target_num: int = 0) -> Tuple[np.ndarray, np.ndarray]:
        """
        Returns the data for the lift curve, as displayed in the getML Monitor.

        This requires that you call
        [`score`][getml.Pipeline.score] first. The data used
        for the curve will always be the data from the *last* time
        you called [`score`][getml.Pipeline.score].

        Args:
            target_num:
                Indicates for which target you want to plot the lift
                curve. (Pipelines can have more than one target.)

        Returns:
            The first array is the proportion of samples, usually displayed on the x-axis.
            The second array is the lift, usually displayed on the y-axis.
        """

        cmd: Dict[str, Any] = {}

        cmd["type_"] = "Pipeline.lift_curve"
        cmd["name_"] = self.name

        cmd["target_num_"] = target_num

        with comm.send_and_get_socket(cmd) as sock:
            msg = comm.recv_string(sock)
            if msg != "Success!":
                comm.handle_engine_exception(msg)
            msg = comm.recv_string(sock)

        json_obj = json.loads(msg)

        return (np.asarray(json_obj["proportion_"]), np.asarray(json_obj["lift_"]))

    # ------------------------------------------------------------

    def precision_recall_curve(
        self, target_num: int = 0
    ) -> Tuple[np.ndarray, np.ndarray]:
        """
        Returns the data for the precision-recall curve, as displayed in the getML
        Monitor.

        This requires that you call
        [`score`][getml.Pipeline.score] first. The data used
        for the curve will always be the data from the *last* time
        you called [`score`][getml.Pipeline.score].

        Args:
            target_num:
                Indicates for which target you want to plot the lift
                curve. (Pipelines can have more than one target.)

        Returns:
            The first array is the recall (a.k.a. true positive rate), usually displayed on the x-axis.
            The second array is the precision, usually displayed on the y-axis.
        """

        cmd: Dict[str, Any] = {}

        cmd["type_"] = "Pipeline.precision_recall_curve"
        cmd["name_"] = self.name

        cmd["target_num_"] = target_num

        with comm.send_and_get_socket(cmd) as sock:
            msg = comm.recv_string(sock)
            if msg != "Success!":
                comm.handle_engine_exception(msg)
            msg = comm.recv_string(sock)

        json_obj = json.loads(msg)

        return (np.asarray(json_obj["tpr_"]), np.asarray(json_obj["precision_"]))

    # ------------------------------------------------------------

    def roc_curve(self, target_num: int = 0) -> Tuple[np.ndarray, np.ndarray]:
        """
        Returns the data for the ROC curve, as displayed in the getML Monitor.

        This requires that you call
        [`score`][getml.Pipeline.score] first. The data used
        for the curve will always be the data from the *last* time
        you called [`score`][getml.Pipeline.score].

        Args:
            target_num:
                Indicates for which target you want to plot the lift
                curve. (Pipelines can have more than one target.)

        Returns:
            The first array is the false positive rate, usually displayed on the x-axis.
            The second array is the true positive rate, usually displayed on the y-axis.
        """

        cmd: Dict[str, Any] = {}

        cmd["type_"] = "Pipeline.roc_curve"
        cmd["name_"] = self.name

        cmd["target_num_"] = target_num

        with comm.send_and_get_socket(cmd) as sock:
            msg = comm.recv_string(sock)
            if msg != "Success!":
                comm.handle_engine_exception(msg)
            msg = comm.recv_string(sock)

        json_obj = json.loads(msg)

        return (np.asarray(json_obj["fpr_"]), np.asarray(json_obj["tpr_"]))
