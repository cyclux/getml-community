// Copyright 2024 Code17 GmbH
//
// This file is licensed under the Elastic License 2.0 (ELv2).
// Refer to the LICENSE.txt file in the root of the repository
// for details.
//

#ifndef DATABASE_POSTGRES_HPP_
#define DATABASE_POSTGRES_HPP_

#include <libpq-fe.h>

#include <string>
#include <vector>

#include "database/Command.hpp"
#include "database/Connector.hpp"
#include "database/DatabaseParser.hpp"
#include "database/PostgresIterator.hpp"
#include "database/TableContent.hpp"
#include "io/StatementMaker.hpp"
#include "io/io.hpp"

namespace database {

class Postgres : public Connector {
 public:
  Postgres(const typename Command::PostgresOp& _obj, const std::string& _passwd)
      : connection_string_(make_connection_string(_obj, _passwd)),
        time_formats_(_obj.time_formats()) {}

  explicit Postgres(const std::vector<std::string>& _time_formats)
      : time_formats_(_time_formats) {}

  ~Postgres() = default;

 public:
  /// Returns a std::string describing the connection.
  std::string describe() const final;

  /// Returns the names of the table columns.
  std::vector<std::string> get_colnames_from_query(
      const std::string& _table) const final;

  /// Returns the types of the table columns.
  std::vector<io::Datatype> get_coltypes_from_query(
      const std::string& _table,
      const std::vector<std::string>& _colnames) const final;

  /// Returns the content of a table in a format that is compatible
  /// with the DataTables.js server-side processing API.
  TableContent get_content(const std::string& _tname, const std::int32_t _draw,
                           const std::int32_t _start,
                           const std::int32_t _length) final;

  /// Lists the name of the tables held in the database.
  std::vector<std::string> list_tables() final;

  /// Reads a CSV file or another data source into a table.
  void read(const std::string& _table, const size_t _skip,
            io::Reader* _reader) final;

 public:
  /// Returns the dialect of the connector.
  std::string dialect() const final { return "postgres"; }

  /// Drops a table and cleans up, if necessary.
  void drop_table(const std::string& _tname) final {
    const auto tname = io::StatementMaker::handle_schema(_tname, "\"", "\"");
    execute("DROP TABLE \"" + tname + "\";");
  }

  /// Executes an SQL query.
  void execute(const std::string& _sql) final {
    auto conn = make_connection();
    exec(_sql, conn.get());
    if (PQtransactionStatus(conn.get()) == PQTRANS_INTRANS) {
      exec("COMMIT", conn.get());
    }
  }

  /// Returns the names of columns of the table.
  std::vector<std::string> get_colnames_from_table(
      const std::string& _table) const final {
    const auto table = io::StatementMaker::handle_schema(_table, "\"", "\"");
    return get_colnames_from_query("SELECT * FROM \"" + table + "\" LIMIT 0;");
  }

  /// Returns the types of columns of the table.
  std::vector<io::Datatype> get_coltypes_from_table(
      const std::string& _table,
      const std::vector<std::string>& _colnames) const final {
    const auto table = io::StatementMaker::handle_schema(_table, "\"", "\"");
    return get_coltypes_from_query("SELECT * FROM \"" + table + "\" LIMIT 0;",
                                   _colnames);
  }

  /// Returns the number of rows in the table signified by _tname.
  std::int32_t get_nrows(const std::string& _tname) final {
    const auto tname = io::StatementMaker::handle_schema(_tname, "\"", "\"");
    return select({"COUNT(*)"}, tname, "")->get_int();
  }

  /// Returns a shared_ptr containing a PostgresIterator.
  rfl::Ref<Iterator> select(const std::vector<std::string>& _colnames,
                            const std::string& _tname,
                            const std::string& _where) final {
    return rfl::Ref<PostgresIterator>::make(make_connection(), _colnames,
                                            time_formats_, _tname, _where);
  }

  /// Returns a shared_ptr containing a PostgresIterator.
  rfl::Ref<Iterator> select(const std::string& _sql) final {
    return rfl::Ref<PostgresIterator>::make(make_connection(), _sql,
                                            time_formats_);
  }

  /// Returns the time formats used.
  const std::vector<std::string>& time_formats() const { return time_formats_; }

 private:
  /// Makes sure that the colnames of the CSV file match the colnames of the
  /// target table.
  void check_colnames(const std::vector<std::string>& _colnames,
                      io::Reader* _reader);

  /// Returns the io::Datatype associated with a oid.
  io::Datatype interpret_oid(Oid _oid) const;

  /// Prepares a shared ptr to the connection object
  /// Called by the constructor.
  static std::string make_connection_string(
      const typename Command::PostgresOp& _obj, const std::string& _passwd);

 private:
  /// Executes and SQL command given a connection.
  std::shared_ptr<PGresult> exec(const std::string& _sql, PGconn* _conn) const {
    auto raw_ptr = PQexec(_conn, _sql.c_str());

    auto result = std::shared_ptr<PGresult>(raw_ptr, PQclear);

    const auto status = PQresultStatus(result.get());

    if (status != PGRES_COMMAND_OK && status != PGRES_TUPLES_OK) {
      const std::string error_msg = PQresultErrorMessage(result.get());

      throw std::runtime_error("Executing command in postgres failed: " +
                               error_msg);
    }

    return result;
  }

  /// Returns a new connection based on the connection_string_
  std::shared_ptr<PGconn> make_connection() const {
    auto raw_ptr = PQconnectdb(connection_string_.c_str());

    auto conn = std::shared_ptr<PGconn>(raw_ptr, PQfinish);

    if (PQstatus(conn.get()) != CONNECTION_OK) {
      throw std::runtime_error(std::string("Connection to postgres failed:") +
                               PQerrorMessage(conn.get()));
    }

    return conn;
  }

  /// List of all typnames that will be interpreted as double precision.
  static std::vector<std::string> typnames_double_precision() {
    return {"float4", "float8", "_float4", "_float8", "numeric", "_numeric"};
  }

  /// List of all typnames that will be interpreted as int.
  static std::vector<std::string> typnames_int() {
    return {"int8", "int2", "int4", "_int2", "_int4"};
  }

 private:
  /// String containing the meta-information related to the connection.
  std::string connection_string_;

  /// Vector containing the time formats.
  const std::vector<std::string> time_formats_;
};

// ----------------------------------------------------------------------------

}  // namespace database

#endif  // DATABASE_POSTGRES_HPP_
