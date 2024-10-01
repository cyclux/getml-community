// Copyright 2024 Code17 GmbH
//
// This file is licensed under the Elastic License 2.0 (ELv2).
// Refer to the LICENSE.txt file in the root of the repository
// for details.
//

#ifndef COMMANDS_FLOATCOLUMNORFLOATCOLUMNVIEW_HPP_
#define COMMANDS_FLOATCOLUMNORFLOATCOLUMNVIEW_HPP_

#include <rfl/Field.hpp>
#include <rfl/Literal.hpp>
#include <rfl/Ref.hpp>
#include <rfl/TaggedUnion.hpp>
#include <rfl/json/Reader.hpp>
#include <variant>

#include "commands/BooleanColumnView.hpp"
#include "commands/Float.hpp"
#include "commands/StringColumnOrStringColumnView.hpp"

namespace commands {

struct BooleanColumnView;
struct StringColumnOrStringColumnView;

struct FloatColumnOrFloatColumnView {
  /// The command used for arange operations.
  struct FloatArangeOp {
    using Tag = rfl::Literal<"arange">;
    rfl::Field<"start_", Float> start;
    rfl::Field<"stop_", Float> stop;
    rfl::Field<"step_", Float> step;
    rfl::Field<"type_", rfl::Literal<"FloatColumnView">> type;
  };

  /// The command used for transforming string columns to float columns.
  struct FloatAsTSOp {
    using Tag = rfl::Literal<"as_ts">;
    rfl::Field<"operand1_", rfl::Ref<StringColumnOrStringColumnView>> operand1;
    rfl::Field<"time_formats_", std::vector<std::string>> time_formats;
    rfl::Field<"type_", rfl::Literal<"FloatColumnView">> type;
  };

  /// The command used for float binary operations.
  struct FloatBinaryOp {
    using FloatBinaryOpLiteral =
        rfl::Literal<"divides", "fmod", "minus", "multiplies", "plus", "pow">;

    rfl::Field<"operator_", FloatBinaryOpLiteral> op;
    rfl::Field<"operand1_", rfl::Ref<FloatColumnOrFloatColumnView>> operand1;
    rfl::Field<"operand2_", rfl::Ref<FloatColumnOrFloatColumnView>> operand2;
    rfl::Field<"type_", rfl::Literal<"FloatColumnView">> type;
  };

  /// The command used for retrieving float columns from a data frame.
  struct FloatColumnOp {
    using Tag = rfl::Literal<"FloatColumn">;
    rfl::Field<"df_name_", std::string> df_name;
    rfl::Field<"name_", std::string> name;
    rfl::Field<"role_", std::string> role;
    rfl::Field<"type_", rfl::Literal<"FloatColumn">> type;
  };

  /// The command used for float const operations.
  struct FloatConstOp {
    using Tag = rfl::Literal<"const">;
    rfl::Field<"value_", Float> value;
    rfl::Field<"type_", rfl::Literal<"FloatColumnView">> type;
  };

  /// The command used for transforming boolean column views to float columns.
  struct FloatFromBooleanOp {
    using Tag = rfl::Literal<"boolean_as_num">;
    rfl::Field<"operand1_", rfl::Ref<BooleanColumnView>> operand1;
    rfl::Field<"type_", rfl::Literal<"FloatColumnView">> type;
  };

  /// The command used for transforming string columns to float columns.
  struct FloatFromStringOp {
    using Tag = rfl::Literal<"as_num">;
    rfl::Field<"operand1_", rfl::Ref<StringColumnOrStringColumnView>> operand1;
    rfl::Field<"type_", rfl::Literal<"FloatColumnView">> type;
  };

  /// The command used for random operations.
  struct FloatRandomOp {
    using Tag = rfl::Literal<"random">;
    rfl::Field<"seed_", unsigned int> seed;
    rfl::Field<"type_", rfl::Literal<"FloatColumnView">> type;
  };

  /// The command used for rowid operations.
  struct FloatRowidOp {
    using Tag = rfl::Literal<"rowid">;
    rfl::Field<"type_", rfl::Literal<"FloatColumnView">> type;
  };

  /// The command used for float subselection operations.
  struct FloatSubselectionOp {
    using Tag = rfl::Literal<"num_subselection">;
    rfl::Field<"operand1_", rfl::Ref<FloatColumnOrFloatColumnView>> operand1;
    rfl::Field<"operand2_", std::variant<rfl::Ref<FloatColumnOrFloatColumnView>,
                                         rfl::Ref<BooleanColumnView>>>
        operand2;
    rfl::Field<"type_", rfl::Literal<"FloatColumnView">> type;
  };

  /// The command used for float unary operations.
  struct FloatUnaryOp {
    using FloatUnaryOpLiteral =
        rfl::Literal<"abs", "acos", "asin", "atan", "cbrt", "ceil", "cos",
                     "day", "erf", "exp", "floor", "hour", "lgamma", "log",
                     "minute", "month", "round", "second", "sin", "sqrt", "tan",
                     "tgamma", "weekday", "year", "yearday">;

    rfl::Field<"operator_", FloatUnaryOpLiteral> op;
    rfl::Field<"operand1_", rfl::Ref<FloatColumnOrFloatColumnView>> operand1;
    rfl::Field<"type_", rfl::Literal<"FloatColumnView">> type;
  };

  /// The command used for float binary operations.
  struct FloatUpdateOp {
    using Tag = rfl::Literal<"num_update">;
    rfl::Field<"operand1_", rfl::Ref<FloatColumnOrFloatColumnView>> operand1;
    rfl::Field<"operand2_", rfl::Ref<FloatColumnOrFloatColumnView>> operand2;
    rfl::Field<"condition_", rfl::Ref<BooleanColumnView>> condition;
    rfl::Field<"type_", rfl::Literal<"FloatColumnView">> type;
  };

  /// The command used for string with subtoles operations.
  struct FloatWithSubrolesOp {
    using Tag = rfl::Literal<"num_with_subroles">;
    rfl::Field<"subroles_", std::vector<std::string>> subroles;
    rfl::Field<"operand1_", rfl::Ref<FloatColumnOrFloatColumnView>> operand1;
    rfl::Field<"type_", rfl::Literal<"FloatColumnView">> type;
  };

  /// The command used for float with unit operations.
  struct FloatWithUnitOp {
    using Tag = rfl::Literal<"num_with_unit">;
    rfl::Field<"unit_", std::string> unit;
    rfl::Field<"operand1_", rfl::Ref<FloatColumnOrFloatColumnView>> operand1;
    rfl::Field<"type_", rfl::Literal<"FloatColumnView">> type;
  };

  using ReflectionType =
      rfl::TaggedUnion<"operator_", FloatArangeOp, FloatAsTSOp, FloatBinaryOp,
                       FloatConstOp, FloatFromBooleanOp, FloatFromStringOp,
                       FloatRandomOp, FloatRowidOp, FloatSubselectionOp,
                       FloatUnaryOp, FloatUpdateOp, FloatColumnOp,
                       FloatWithSubrolesOp, FloatWithUnitOp>;

  using InputVarType = typename rfl::json::Reader::InputVarType;

  static FloatColumnOrFloatColumnView from_json_obj(const InputVarType& _obj);

  /// Used to break the recursive definition.
  ReflectionType val_;
};

}  // namespace commands

#endif
