// Copyright 2022 The SQLNet Company GmbH
//
// This file is licensed under the Elastic License 2.0 (ELv2).
// Refer to the LICENSE.txt file in the root of the repository
// for details.
//

#include "database/Postgres.hpp"

#include "database/CSVBuffer.hpp"
#include "json/json.hpp"

namespace database {

void Postgres::check_colnames(const std::vector<std::string>& _colnames,
                              io::Reader* _reader) {
  const auto csv_colnames = _reader->colnames();

  if (csv_colnames.size() != _colnames.size()) {
    throw std::runtime_error("Wrong number of columns. Expected " +
                             std::to_string(_colnames.size()) + ", saw " +
                             std::to_string(csv_colnames.size()) + ".");
  }

  for (size_t i = 0; i < _colnames.size(); ++i) {
    if (csv_colnames.at(i) != _colnames.at(i)) {
      throw std::runtime_error("Column " + std::to_string(i + 1) +
                               " has wrong name. Expected '" + _colnames.at(i) +
                               "', saw '" + csv_colnames.at(i) + "'.");
    }
  }
}

// ----------------------------------------------------------------------------

std::string Postgres::describe() const {
  const auto description =
      fct::make_field<"connection_string">(connection_string_) *
      fct::make_field<"dialect">(dialect());
  return json::to_json(description);
}

// ----------------------------------------------------------------------------

std::vector<std::string> Postgres::get_colnames(
    const std::string& _table) const {
  const auto table = io::StatementMaker::handle_schema(_table, "\"", "\"");

  const std::string sql = "SELECT * FROM \"" + table + "\" LIMIT 0";

  const auto connection = make_connection();

  const auto result = exec(sql, connection.get());

  const int num_cols = PQnfields(result.get());

  auto colnames = std::vector<std::string>(num_cols);

  for (int i = 0; i < num_cols; ++i) {
    colnames[i] = PQfname(result.get(), i);
  }

  return colnames;
}

// ----------------------------------------------------------------------------

std::vector<io::Datatype> Postgres::get_coltypes(
    const std::string& _table,
    const std::vector<std::string>& _colnames) const {
  const auto table = io::StatementMaker::handle_schema(_table, "\"", "\"");

  const std::string sql = "SELECT * FROM \"" + table + "\" LIMIT 0";

  const auto connection = make_connection();

  const auto result = exec(sql, connection.get());

  const int num_cols = PQnfields(result.get());

  auto coltypes = std::vector<io::Datatype>(num_cols);

  for (int i = 0; i < num_cols; ++i) {
    const auto oid = PQftype(result.get(), i);

    coltypes[i] = interpret_oid(oid);
  }

  return coltypes;
}

// ----------------------------------------------------------------------------

Poco::JSON::Object Postgres::get_content(const std::string& _tname,
                                         const std::int32_t _draw,
                                         const std::int32_t _start,
                                         const std::int32_t _length) {
  const auto nrows = get_nrows(_tname);

  const auto colnames = get_colnames(_tname);

  const auto ncols = colnames.size();

  Poco::JSON::Object obj;

  obj.set("draw", _draw);

  obj.set("recordsTotal", nrows);

  obj.set("recordsFiltered", nrows);

  if (nrows == 0) {
    obj.set("data", Poco::JSON::Array());
    return obj;
  }

  if (_length < 0) {
    throw std::runtime_error("length must be positive!");
  }

  if (_start < 0) {
    throw std::runtime_error("start must be positive!");
  }

  if (_start >= nrows) {
    throw std::runtime_error("start must be smaller than number of rows!");
  }

  const auto begin = _start;

  const auto end = (_start + _length > nrows) ? nrows : _start + _length;

  auto iterator = std::make_shared<PostgresIterator>(
      make_connection(), colnames, time_formats_, _tname, "", begin, end);

  Poco::JSON::Array data;

  for (auto i = begin; i < end; ++i) {
    Poco::JSON::Array row;

    for (size_t j = 0; j < ncols; ++j) {
      row.add(iterator->get_string());
    }

    data.add(row);
  }

  obj.set("data", data);

  return obj;
}

// ----------------------------------------------------------------------------

io::Datatype Postgres::interpret_oid(Oid _oid) const {
  const std::string sql =
      "SELECT typname FROM pg_type WHERE oid=" + std::to_string(_oid) + ";";

  auto connection = make_connection();

  const auto result = exec(sql, connection.get());

  if (PQntuples(result.get()) == 0) {
    throw std::runtime_error("Type for oid " + std::to_string(_oid) +
                             " not known!");
  }

  const std::string typname = PQgetvalue(result.get(), 0, 0);

  auto typnames = typnames_double_precision();

  if (std::find(typnames.begin(), typnames.end(), typname) != typnames.end()) {
    return io::Datatype::double_precision;
  }

  typnames = typnames_int();

  if (std::find(typnames.begin(), typnames.end(), typname) != typnames.end()) {
    return io::Datatype::integer;
  }

  return io::Datatype::string;
}

// ----------------------------------------------------------------------------

std::vector<std::string> Postgres::list_tables() {
  auto iterator = std::make_shared<PostgresIterator>(
      make_connection(), std::vector<std::string>({"table_name"}),
      time_formats_, "information_schema.tables", "table_schema='public'");

  auto tnames = std::vector<std::string>(0);

  while (!iterator->end()) {
    tnames.push_back(iterator->get_string());
  }

  return tnames;
}

// ----------------------------------------------------------------------------

std::string Postgres::make_connection_string(
    const typename Command::PostgresOp& _obj, const std::string& _passwd) {
  const auto& host = _obj.get<"host_">();

  const auto& hostaddr = _obj.get<"hostaddr_">();

  const auto port = _obj.get<"port_">();

  const auto& dbname = _obj.get<"dbname_">();

  const auto& user = _obj.get<"user_">();

  std::string connection_string;

  if (host) {
    connection_string += std::string("host=") + *host + " ";
  }

  if (hostaddr) {
    connection_string += std::string("hostaddr=") + *hostaddr + " ";
  }

  connection_string += std::string("port=") + std::to_string(port) + " ";

  connection_string += std::string("dbname=") + dbname + " ";

  connection_string += std::string("user=") + user + " ";

  connection_string += std::string("password=") + _passwd;

  return connection_string;
}

// ----------------------------------------------------------------------------

void Postgres::read(const std::string& _table, const size_t _skip,
                    io::Reader* _reader) {
  const std::vector<std::string> colnames = get_colnames(_table);

  const std::vector<io::Datatype> coltypes = get_coltypes(_table, colnames);

  assert_true(colnames.size() == coltypes.size());

  check_colnames(colnames, _reader);

  size_t line_count = 0;

  for (size_t i = 0; i < _skip; ++i) {
    _reader->next_line();
    ++line_count;
  }

  const auto table = io::StatementMaker::handle_schema(_table, "\"", "\"");

  const auto copy_statement = std::string("COPY \"") + table +
                              std::string("\" FROM STDIN DELIMITER '") +
                              _reader->sep() + std::string("' CSV QUOTE '") +
                              _reader->quotechar() + std::string("';");

  auto conn = make_connection();

  auto res = PQexec(conn.get(), copy_statement.c_str());

  if (!res) {
    throw std::runtime_error(PQerrorMessage(conn.get()));
  }

  PQgetResult(conn.get());

  try {
    while (!_reader->eof()) {
      const std::vector<std::string> line = _reader->next_line();

      ++line_count;

      if (line.size() == 0) {
        continue;
      } else if (line.size() != coltypes.size()) {
        std::cout << "Corrupted line: " << line_count << ". Expected "
                  << colnames.size() << " fields, saw " << line.size() << "."
                  << std::endl;

        continue;
      }

      const std::string buffer = CSVBuffer::make_buffer(
          line, coltypes, _reader->sep(), _reader->quotechar(), false, false);

      const auto success = PQputCopyData(conn.get(), buffer.c_str(),
                                         static_cast<int>(buffer.size()));

      if (success != 1) {
        throw std::runtime_error("Write error in line " +
                                 std::to_string(line_count) + ".");
      }
    }
  } catch (std::exception& e) {
    PQputCopyEnd(conn.get(), e.what());

    throw std::runtime_error(e.what());
  }

  if (PQputCopyEnd(conn.get(), NULL) == -1) {
    throw std::runtime_error(PQerrorMessage(conn.get()));
  }

  PQgetResult(conn.get());
}

// ----------------------------------------------------------------------------
}  // namespace database
