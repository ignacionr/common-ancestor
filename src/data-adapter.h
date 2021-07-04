#include <sqlite3.h>
#include <stdexcept>
#include <string_view>
#include <vector>
#include <array>
#include <iostream>
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
          "UNIQUE (value, node_tree)"
      });
      upsert("config", "item", "content", "version", VERSION);
    }
  }
  ~data_adapter()
  {
    if (db)
      sqlite3_close(db);
  }

  int insert_tree(std::string_view text) 
  {
    // [5<10>15][5>7][13<15][11<13>14]
    std::cerr << __FILE__ << ":" << __LINE__ << " body:" << text << std::endl;

    throw std::runtime_error("not implemented");
    return 0;
  }

  std::string version() 
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
  void create_table(std::string_view name, T fields, bool if_not_exists = false) {
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

  void drop_table(std::string_view name, bool if_exists) {
    std::string cmd("DROP TABLE ");
    if (if_exists) cmd += "IF EXISTS ";
    cmd += name;
    exec(cmd);
  }

  void upsert(std::string_view table, 
    std::string_view key_column, 
    std::string_view value_column,
    std::string_view key, 
    std::string_view new_value) 
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

  void exec(std::string_view command)
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
  void exec(std::string_view command, T callback)
  {
    std::cerr << command << std::endl;
    char *pError;
    auto rc = sqlite3_exec(
        db,
        std::string(command).c_str(),
        +[](void *pActualCallback, int col_count, char **paValues, char **paColumnNames)
        {
          auto values = std::vector<std::string_view>(col_count);
          auto columns = std::vector<std::string_view>(col_count);
          for (int col{0}; col < col_count; ++col)
          {
            values[col] = paValues[col];
            columns[col] = paColumnNames[col];
          }
          (*static_cast<T *>(pActualCallback))(values, columns);
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
