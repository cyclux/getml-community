# Copyright 2022 The SQLNet Company GmbH
#
# This file is licensed under the Elastic License 2.0 (ELv2).
# Refer to the LICENSE.txt file in the root of the repository
# for details.
#


"""
Lists the colnames of a table held in the database.
"""

import json
from typing import Any, Dict, Optional

import getml.communication as comm

from .connection import Connection


def get_colnames(name: str, conn: Optional[Connection] = None) -> list[str]:
    """
    Lists the colnames of a table held in the database.

    Args:
        name:
            The name of the table in the database.

        conn:
            The database connection to be used.
            If you don't explicitly pass a connection,
            the engine will use the default connection.

    Returns:
        A list of strings containing the names of the columns in the table.
    """

    conn = conn or Connection()

    cmd: Dict[str, Any] = {}

    cmd["name_"] = name
    cmd["type_"] = "Database.get_colnames"
    cmd["conn_id_"] = conn.conn_id

    with comm.send_and_get_socket(cmd) as sock:
        msg = comm.recv_string(sock)
        if msg != "Success!":
            sock.close()
            comm.engine_exception_handler(msg)
        arr = json.loads(comm.recv_string(sock))

    return arr
