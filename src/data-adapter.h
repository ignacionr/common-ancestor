#include <sqlite3.h>
#include <stdexcept>
#include <string_view>
#include <vector>
#include <array>
#include <iostream>
#include <string>

#include "version.h"
#if !defined(VERSION)
#define VERSION "please run CMake"
#endif
#include "tree-parser.h"

class data_adapter
{
public:
  data_adapter()
  {
    auto rc{sqlite3_open("trees-" VERSION ".db", &db)};
    if (rc != SQLITE_OK)
    {
      throw std::runtime_error("unable to open the database");
    }
    if (version() != VERSION) {
      drop_table("config", true);
      drop_table("node", true);
      drop_table("tree", true);

      create_table("config", std::array<std::string_view,2>{
        "item TEXT PRIMARY KEY",
        "content TEXT" });
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
          "UNIQUE (node_tree,value)"
      });
      upsert("config", "item", "content", "version", VERSION);
    }
  }
  ~data_adapter()
  {
    if (db)
      sqlite3_close(db);
  }

  std::string ensure_node(std::string const &tree_id, int const value) const
  {
    auto const valueStr {std::to_string(value)};
    std::string res;
    exec(std::string("INSERT INTO node (value, node_tree) VALUES(") + valueStr + "," + tree_id + ") "
      "ON CONFLICT(node_tree,value) DO NOTHING; SELECT id FROM node WHERE node_tree=" + tree_id + " AND value=" + valueStr,
      [&res](auto values, auto columns) {
        res = values.front();
      });
    return res;
  }

  std::string insert_tree(std::string_view text) const
  {
    std::string tree_id;
    exec("INSERT INTO tree DEFAULT VALUES; SELECT last_insert_rowid()", [&tree_id](auto values, auto columns){
      tree_id = values.front();
    });
    if (tree_id.empty()) {
      throw std::runtime_error("couldn't obtain last tree id");
    }
    tree_parser::parse(text, [this, &tree_id](auto node){
      auto this_node = ensure_node(tree_id, node.value);
      std::string additional;
      if (node.left.has_value()) {
        auto left = ensure_node(tree_id, node.left.value());
        additional = " left = " + left;
      }
      if (node.right.has_value()) {
        auto right = ensure_node(tree_id, node.right.value());
        if (!additional.empty()) additional += ',';
        additional += " right = " + right;
      }
      if (!additional.empty()) {
        std::string const cmd {"UPDATE node SET " + additional + " WHERE id=" + this_node};
        exec(cmd);
      }
    });
    return tree_id;
  }

  std::string version() const
  {
    std::string result;
    try {
      exec("SELECT content FROM config WHERE item='version'", [&result](auto values, auto columns) { result = values[0]; });
    }
    catch(std::exception const &) {
      result = "unknown";
    }
    return result;
  }

  static std::string quoted(std::string_view text) 
  {
    std::string result;
    result.reserve(text.size() + 2);
    result.append("'");
    for(auto c: text) {
      if (c == '\'') {
        result += '\'';
      }
      result += c;
    }
    result += '\'';
    return result;
  }

  template<typename T>
  void create_table(std::string_view name, T fields, bool if_not_exists = false) const {
    std::string cmd("CREATE TABLE ");
    if (if_not_exists) cmd += "IF NOT EXISTS ";
    cmd += name;
    cmd += '(';
    bool first {true};
    for (auto const &field: fields) {
      if (first)
        first = false;
      else
        cmd += ',';
      cmd += field;
    }
    cmd += ')';
    exec(cmd);
  }

  void drop_table(std::string_view name, bool if_exists) const {
    std::string cmd("DROP TABLE ");
    if (if_exists) cmd += "IF EXISTS ";
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
    char *pError;
    auto rc = sqlite3_exec(db,
                           std::string(command).c_str(),
                           nullptr,
                           nullptr,
                           &pError);
    if (rc != SQLITE_OK)
    {
      std::string descr(pError);
      sqlite3_free(pError);
      descr += " while executing ";
      descr += command;
      throw std::runtime_error(descr);
    }
    if (pError)
      sqlite3_free(pError);
  }

  template <typename T>
  void exec(std::string_view command, T callback) const
  {
    std::cerr << command << std::endl;
    char *pError;
    auto rc = sqlite3_exec(
        db,
        std::string(command).c_str(),
        +[](void *cb, int col_count, char **paValues, char **paColumnNames)
        {
          std::cerr << "\nresults row" << std::endl;
          auto values = std::vector<std::string_view>(col_count);
          auto columns = std::vector<std::string_view>(col_count);
          for (int col{0}; col < col_count; ++col)
          {
            std::cerr << paColumnNames[col] << "=" << paValues[col] << std::endl;
            values[col] = paValues[col];
            columns[col] = paColumnNames[col];
          }
          (*static_cast<T *>(cb))(values, columns);
          return 0;
        },
        &callback,
        &pError);
    if (rc != SQLITE_OK)
    {
      std::string descr(pError);
      sqlite3_free(pError);
      throw std::runtime_error(descr);
    }
    if (pError)
      sqlite3_free(pError);
  }


private:
  sqlite3 *db{};
};
