# Copyright 2022 The SQLNet Company GmbH
#
# This file is licensed under the Elastic License 2.0 (ELv2).
# Refer to the LICENSE.txt file in the root of the repository
# for details.
#


"""
Marks the relationship between joins in :class:`~getml.data.Placeholder`
"""


many_to_many = "many-to-many"
"""Used for one-to-many or many-to-many relationships.

When there is such a relationship, feature learning
is necessary and meaningful. If you mark a join as
a default relationship, but that assumption is violated
for the training data, the pipeline will raise a
warning.
"""

many_to_one = "many-to-one"
"""Used for many-to-one relationships.

If two tables are guaranteed to be in a
many-to-one relationship, then feature
learning is not necessary as they can
simply be joined. If a relationship is
marked many-to-one, but the assumption is
violated, the pipeline will raise
an exception.
"""

one_to_many = "one-to-many"
"""Used for one-to-many or many-to-many relationships.

When there is such a relationship, feature learning
is necessary and meaningful. If you mark a join as
a default relationship, but that assumption is violated
for the training data, the pipeline will raise a
warning.
"""

one_to_one = "one-to-one"
"""Used for one-to-one relationships.

If two tables are guaranteed to be in a
one-to-one relationship, then feature
learning is not necessary as they can
simply be joined. If a relationship is
marked one-to-one, but the assumption is
violated, the pipeline will raise
an exception. If you are unsure whether
you want to use many_to_one or one_to_one,
user many_to_one.
"""

propositionalization = "propositionalization"
"""Used for one-to-many or many-to-many relationships.

The flag means that you want a propositionalization
algorithm to be used for this particular join.
This is recommended when there are very many matches
within the join and normal algorithms would take
too long.
"""

_all_relationships = [
    many_to_many,
    many_to_one,
    one_to_many,
    one_to_one,
    propositionalization,
]
