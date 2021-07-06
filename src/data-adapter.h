#include <sqlite3.h>
#include <stdexcept>
#include <string_view>
#include <vector>
#include <array>
#include <iostream>
#include <string>
#include <set>
#include <functional>
#include <numeric>
#include <memory>

#include "version.h"
#if !defined(VERSION)
#define VERSION "test"
#endif

#include "tree.h"

class data_adapter
{
public:
  using node_key_t = std::string;
  using tree_key_t = std::string;

  data_adapter()
  {
    auto rc{sqlite3_open("trees-" VERSION ".db", &db)};
    if (rc != SQLITE_OK)
    {
      throw std::runtime_error("unable to open the database");
    }
    if (version() != VERSION)
    {
      drop_table("config", true);
      drop_table("node", true);
      drop_table("tree", true);

      create_table("config", std::array<std::string_view, 2>{
                                 "item TEXT PRIMARY KEY",
                                 "content TEXT"});
      create_table("tree", std::array<std::string_view, 1>{"id INTEGER PRIMARY KEY"});
      create_table("node", std::array<std::string_view, 9>{
                               "id INTEGER PRIMARY KEY",
                               "value INTEGER",
                               "left INTEGER NULL",
                               "right INTEGER NULL",
                               "node_tree INTEGER",
                               "FOREIGN KEY(node_tree) REFERENCES tree(id)",
                               "FOREIGN KEY(left) REFERENCES node(id)",
                               "FOREIGN KEY(right) REFERENCES node(id)",
                               "UNIQUE (node_tree,value)"});
      upsert("config", "item", "content", "version", VERSION);
    }
  }

  data_adapter(const data_adapter&) = delete;
  data_adapter(data_adapter&&) = delete;

  ~data_adapter()
  {
    if (db)
      sqlite3_close(db);
  }

  node_key_t get_parent_by_id(node_key_t node_id) const
  {
    auto cmd = std::string("SELECT id FROM node WHERE ");
    cmd += " left=";
    cmd += node_id;
    cmd += " OR right=";
    cmd += node_id;

    std::string res;
    exec(cmd,
         [&res](auto values, auto columns)
         {
           res = values.front();
         });
    return res;
  }

  std::string new_tree() const
  {
    std::string tree_id;
    exec("INSERT INTO tree DEFAULT VALUES");
    exec("SELECT last_insert_rowid()", [&tree_id](auto values, auto columns)
         {
           tree_id = values.front();
         });
    if (tree_id.empty())
    {
      throw std::runtime_error("couldn't obtain last tree id");
    }
    return tree_id;
  }

  std::string ensure_node(std::string_view tree_id, int const value) const
  {
    auto const valueStr{std::to_string(value)};
    std::string res;
    auto cmd = std::string("INSERT INTO node (value, node_tree) VALUES(") + valueStr + ",";
    cmd += tree_id;
    cmd += ") "
           "ON CONFLICT(node_tree,value) DO NOTHING";
    exec(cmd);

    cmd = "SELECT id FROM node WHERE node_tree=";
    cmd += tree_id;
    cmd += " AND value=" + valueStr;
    exec(cmd,
         [&res](auto values, auto columns)
         {
           res = values.front();
         });
    return res;
  }

  int get_value_by_id(std::string_view node_id) const
  {
    auto cmd = std::string("SELECT value FROM node WHERE id = ");
    cmd += node_id;
    int res;
    bool found{};
    exec(cmd,
         [&res, &found](auto values, auto columns)
         {
           res = std::atoi(std::string(values.front()).c_str());
           found = true;
         });
    if (!found)
    {
      throw std::runtime_error("Not found.");
    }
    return res;
  }

  std::string get_id_by_value(std::string const &tree_id, int const value) const
  {
    auto const valueStr{std::to_string(value)};
    auto cmd = std::string("SELECT id FROM node WHERE node_tree = ");
    cmd += tree_id;
    cmd += " AND value=";
    cmd += std::to_string(value);
    std::string res;
    exec(cmd,
         [&res](auto values, auto columns)
         {
           res = values.front();
         });
    if (res.empty())
    {
      throw std::runtime_error("Not found.");
    }
    return res;
  }

  void bind_left(std::string_view node, std::string_view left) const
  {
    std::string cmd("UPDATE node SET left = ");
    cmd += left;
    cmd += " WHERE id = ";
    cmd += node;
    exec(cmd);
  }

  void bind_right(std::string_view node, std::string_view right) const
  {
    std::string cmd("UPDATE node SET right = ");
    cmd += right;
    cmd += " WHERE id = ";
    cmd += node;
    exec(cmd);
  }

  std::string version() const
  {
    std::string result;
    try
    {
      exec("SELECT content FROM config WHERE item='version'", [&result](auto values, auto columns)
           { result = values[0]; });
    }
    catch (std::exception const &)
    {
      result = "unknown";
    }
    return result;
  }

private:
  static std::string quoted(std::string_view text)
  {
    std::string result;
    result.reserve(text.size() + 2);
    result.append("'");
    for (auto c : text)
    {
      if (c == '\'')
      {
        result += '\'';
      }
      result += c;
    }
    result += '\'';
    return result;
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

  void drop_table(std::string_view name, bool if_exists) const
  {
    std::string cmd("DROP TABLE ");
    if (if_exists)
      cmd += "IF EXISTS ";
    cmd += name;
    exec(cmd);
  }

  void upsert(std::string_view table,
              std::string_view key_column,
              std::string_view value_column,
              std::string_view key,
              std::string_view new_value) const
  {
    std::string cmd("INSERT INTO ");
    cmd += table;
    cmd += '(';
    cmd += key_column;
    cmd += ',';
    cmd += value_column;
    cmd += ") VALUES (";
    cmd += quoted(key);
    cmd += ',';
    cmd += quoted(new_value);
    cmd += ") ON CONFLICT(";
    cmd += key_column;
    cmd += ") DO UPDATE SET ";
    cmd += value_column;
    cmd += "=excluded.";
    cmd += value_column;
    exec(cmd);
  }

  void exec(std::string_view command) const
  {
    char *pError{};
    // prepare
    sqlite3_stmt *stmt{};
    auto rc = sqlite3_prepare_v2(db, command.data(), command.length(), &stmt, nullptr);
    check_rc(rc, command);
    {
      stmt_hold hs{stmt};
      // step
      do
      {
        rc = sqlite3_step(hs);
        if (rc == SQLITE_ROW)
        {
          // uninteded query?
        }
        else if (rc != SQLITE_DONE)
        {
          check_rc(rc, command);
        }
      } while (rc != SQLITE_DONE);
    }
  }

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
    operator sqlite3_stmt*()
    {
      return held_;
    }

  private:
    sqlite3_stmt *held_;
  };

  template <typename T>
  void exec(std::string_view command, T callback) const
  {
    char *pError{};
    // prepare
    sqlite3_stmt *stmt;
    auto rc = sqlite3_prepare_v2(db, command.data(), command.length(), &stmt, nullptr);
    check_rc(rc, command);
    {
      stmt_hold hs{stmt};
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

  sqlite3 *db{};
};
