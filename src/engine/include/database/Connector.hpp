// Copyright 2022 The SQLNet Company GmbH
//
// This file is licensed under the Elastic License 2.0 (ELv2).
// Refer to the LICENSE.txt file in the root of the repository
// for details.
//

#ifndef DATABASE_CONNECTOR_HPP_
#define DATABASE_CONNECTOR_HPP_

#include <Poco/JSON/Object.h>

#include <memory>
#include <string>
#include <vector>

#include "database/Iterator.hpp"
#include "io/io.hpp"

namespace database {

class Connector {
 public:
  Connector() {}

  virtual ~Connector() = default;

 public:
  /// Returns a Poco::JSON::Object describing the connection.
  virtual std::string describe() const = 0;

  /// Describes the dialect used by the connector.
  virtual std::string dialect() const = 0;

  /// Drops a table and cleans up, if necessary.
  virtual void drop_table(const std::string& _tname) = 0;

  /// Executes an SQL query.
  virtual void execute(const std::string& _sql) = 0;

  /// Returns the content of a table in a format that is compatible
  /// with the DataTables.js server-side processing API.
  virtual Poco::JSON::Object get_content(const std::string& _tname,
                                         const std::int32_t _draw,
                                         const std::int32_t _start,
                                         const std::int32_t _length) = 0;

  /// Returns the names of the table columns.
  virtual std::vector<std::string> get_colnames(
      const std::string& _table) const = 0;

  /// Returns the types of the table columns.
  virtual std::vector<io::Datatype> get_coltypes(
      const std::string& _table,
      const std::vector<std::string>& _colnames) const = 0;

  /// Returns the number of rows in the table signified by _tname.
  virtual std::int32_t get_nrows(const std::string& _tname) = 0;

  /// Lists the name of the tables held in the database.
  virtual std::vector<std::string> list_tables() = 0;

  /// Reads from a CSV file or another data source.
  virtual void read(const std::string& _table, const size_t _skip,
                    io::Reader* _reader) = 0;

  /// Returns a shared_ptr containing the corresponding iterator.
  virtual std::shared_ptr<Iterator> select(
      const std::vector<std::string>& _colnames, const std::string& _tname,
      const std::string& _where) = 0;

  /// Returns a shared_ptr containing an iterator for the SQL query.
  virtual std::shared_ptr<Iterator> select(const std::string& _sql) = 0;

  /// Returns the time formats used.
  virtual const std::vector<std::string>& time_formats() const = 0;
};

}  // namespace database

#endif  // DATABASE_CONNECTOR_HPP_
