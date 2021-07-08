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
      upsert_string("config", "item", "content", "version", VERSION);
    }
  }

  data_adapter(const data_adapter &) = delete;
  data_adapter(data_adapter &&) = delete;

  ~data_adapter()
  {
    if (db)
      sqlite3_close(db);
  }

  node_key_t get_parent_by_id(node_key_t node_id) const
  {
    string_parameter node_id_param {std::string(node_id)};
    std::string res;
    exec("SELECT id FROM node WHERE left=? OR right=?1",
         [&res](auto values, auto columns)
         {
           res = values.front();
         },
         { &node_id_param });
    return res;
  }

  std::string new_tree() const
  {
    std::string tree_id;
    exec("INSERT INTO tree DEFAULT VALUES");
    exec("SELECT last_insert_rowid()", [&tree_id](auto values, auto columns)
         { tree_id = values.front(); });
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

    string_parameter tree_id_param{std::string(tree_id)};
    int_parameter value_param{value};

    exec(
        "INSERT INTO node (node_tree,value) VALUES(?,?) ON CONFLICT(node_tree,value) DO NOTHING",
        {&tree_id_param, &value_param});

    exec("SELECT id FROM node WHERE node_tree=? AND value=?",
         [&res](auto values, auto columns)
         {
           res = values.front();
         },
         {&tree_id_param, &value_param});
    return res;
  }

  int get_value_by_id(std::string_view node_id) const
  {
    int res;
    string_parameter node_id_param {std::string(node_id)};
    bool found{};
    exec("SELECT value FROM node WHERE id = ?",
         [&res, &found](auto values, auto columns)
         {
           res = std::atoi(std::string(values.front()).c_str());
           found = true;
         },
         { & node_id_param });
    if (!found)
    {
      throw std::runtime_error("Not found.");
    }
    return res;
  }

  std::string get_id_by_value(std::string const &tree_id, int const value) const
  {
    string_parameter tree_id_param {std::string{tree_id}};
    int_parameter value_param {value};
    std::string res;
    exec("SELECT id FROM node WHERE node_tree = ? AND value=?",
         [&res](auto values, auto columns)
         {
           res = values.front();
         },
         { &tree_id_param, &value_param});
    if (res.empty())
    {
      throw std::runtime_error("Not found.");
    }
    return res;
  }

  void bind_left(std::string_view node, std::string_view left) const
  {
    string_parameter node_param{std::string(node)};
    string_parameter left_param{std::string(left)};
    exec("UPDATE node SET left = ? WHERE id = ?", {&left_param, &node_param});
  }

  void bind_right(std::string_view node, std::string_view right) const
  {
    string_parameter node_param{std::string(node)};
    string_parameter right_param{std::string(right)};
    exec("UPDATE node SET right = ? WHERE id = ?", {&right_param, &node_param});
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

  struct parameter
  {
    virtual ~parameter() = default;
    virtual void bind(sqlite3_stmt *, int index) const = 0;
  };

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

private:
  sqlite3 *db{};
};
