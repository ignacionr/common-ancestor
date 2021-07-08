#pragma once
#include <sqlite3.h>

struct sqlitedb
{
  struct parameter
  {
    virtual ~parameter() = default;
    virtual void bind(sqlite3_stmt *, int index) const = 0;
  };

  struct int_parameter : public parameter
  {
    int value;
    int_parameter(int v) : value{v} {}
    void bind(sqlite3_stmt *stmt, int index) const override
    {
      auto rc = sqlite3_bind_int(stmt, index, value);
      if (rc != SQLITE_OK)
      {
        throw std::runtime_error("error " + std::to_string(rc) + " index " + std::to_string(index));
      }
    }
  };

  struct string_parameter : public parameter
  {
    std::string value;
    string_parameter(std::string const &v) : value{v} {}
    void bind(sqlite3_stmt *stmt, int index) const override
    {
      auto rc = sqlite3_bind_text(stmt, index, value.data(), value.size(), SQLITE_STATIC);
      if (rc != SQLITE_OK)
      {
        throw std::runtime_error("error " + std::to_string(rc) + " index " + std::to_string(index));
      }
    }
  };

  ~sqlitedb() {
    if (db)
      sqlite3_close(db);
  }

  auto open(std::string const &file) {
    return sqlite3_open(file.c_str(), &db);    
  }

  template <typename T>
  void exec(std::string_view command, T callback, std::vector<parameter *> parameters = {}) const
  {
    // prepare
    sqlite3_stmt *stmt;
    auto rc = sqlite3_prepare_v2(db, command.data(), command.length(), &stmt, nullptr);
    check_rc(rc, command);
    {
      stmt_hold hs{stmt};
      for (int idx{}; idx < parameters.size(); ++idx)
      {
        parameters[idx]->bind(hs, idx + 1);
      }
      size_t const col_count{static_cast<size_t>(sqlite3_column_count(stmt))};
      std::vector<std::string_view> col_names{col_count};
      for (int c{0}; c < col_count; ++c)
      {
        col_names[c] = sqlite3_column_name(hs, c);
      }
      // step
      std::vector<std::string_view> values{col_count};
      do
      {
        rc = sqlite3_step(hs);
        if (rc == SQLITE_ROW)
        {
          for (int c{0}; c < col_count; ++c)
          {
            values[c] = reinterpret_cast<const char *>(sqlite3_column_text(hs, c));
          }
          callback(values, col_names);
        }
        else if (rc != SQLITE_DONE)
        {
          check_rc(rc, command);
        }
      } while (rc != SQLITE_DONE);
    }
  }

  void exec(std::string_view command, std::vector<parameter *> parameters = {}) const
  {
    // prepare
    sqlite3_stmt *stmt;
    auto rc = sqlite3_prepare_v2(db, command.data(), command.length(), &stmt, nullptr);
    check_rc(rc, command);
    {
      stmt_hold hs{stmt};
      for (int idx{}; idx < parameters.size(); ++idx)
      {
        parameters[idx]->bind(hs, idx + 1);
      }
      // step
      do
      {
        rc = sqlite3_step(hs);
        if (rc != SQLITE_DONE)
        {
          check_rc(rc, command);
        }
      } while (rc != SQLITE_DONE);
    }
  }

  void drop_table(std::string_view name, bool if_exists) const
  {
    std::string cmd("DROP TABLE ");
    if (if_exists)
      cmd += "IF EXISTS ";
    cmd += name;
    exec(cmd);
  }

  void upsert_string(std::string_view table,
                     std::string_view key_column,
                     std::string_view value_column,
                     std::string_view key,
                     std::string_view new_value) const
  {
    string_parameter table_parameter{std::string(table)};
    string_parameter key_column_parameter{std::string(key_column)};
    string_parameter value_column_parameter{std::string(value_column)};
    string_parameter key_parameter{std::string(key)};
    string_parameter new_value_parameter{std::string(new_value)};
    exec("INSET INTO ? (?,?) VALUES (?,?) ON CONFLICT(?2) DO UPDATE SET ?3=excluded.?3",
         {&table_parameter, &key_column_parameter, &value_column_parameter, &key_parameter, &new_value_parameter});
  }

  template <typename T>
  void create_table(std::string_view name, T fields, bool if_not_exists = false) const
  {
    std::string cmd("CREATE TABLE ");
    if (if_not_exists)
      cmd += "IF NOT EXISTS ";
    cmd += name;
    cmd += '(';
    bool first{true};
    for (auto const &field : fields)
    {
      if (first)
        first = false;
      else
        cmd += ',';
      cmd += field;
    }
    cmd += ')';
    exec(cmd);
  }


private:
  void check_rc(int rc, std::string_view context, char *pError = nullptr) const
  {
    if (rc != SQLITE_OK)
    {
      std::string descr("error " + std::to_string(rc));
      if (pError)
      {
        descr += (pError);
        sqlite3_free(pError);
      }
      descr += ", ";
      descr += sqlite3_errmsg(db);
      descr += " in the context of ";
      descr += context;
      throw std::runtime_error(descr);
    }
  }
  struct stmt_hold
  {
    stmt_hold(sqlite3_stmt *held) : held_{held} {}
    ~stmt_hold()
    {
      // finalize
      sqlite3_finalize(held_);
    }
    operator sqlite3_stmt *()
    {
      return held_;
    }

  private:
    sqlite3_stmt *held_;
  };

  sqlite3 *db{};
};
