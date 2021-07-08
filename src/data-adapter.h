#pragma once
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

#include "sqlitedb.h"
#include "tree.h"

class data_adapter
{
public:
  using node_key_t = std::string;
  using tree_key_t = std::string;

  data_adapter()
  {
    auto rc{db_.open("trees-" VERSION ".db")};
    if (rc != SQLITE_OK)
    {
      throw std::runtime_error("unable to open the database");
    }
    if (version() != VERSION)
    {
      db_.drop_table("config", true);
      db_.drop_table("node", true);
      db_.drop_table("tree", true);

      db_.create_table("config", std::array<std::string_view, 2>{
                                 "item TEXT PRIMARY KEY",
                                 "content TEXT"});
      db_.create_table("tree", std::array<std::string_view, 1>{"id INTEGER PRIMARY KEY"});
      db_.create_table("node", std::array<std::string_view, 9>{
                               "id INTEGER PRIMARY KEY",
                               "value INTEGER",
                               "left INTEGER NULL",
                               "right INTEGER NULL",
                               "node_tree INTEGER",
                               "FOREIGN KEY(node_tree) REFERENCES tree(id)",
                               "FOREIGN KEY(left) REFERENCES node(id)",
                               "FOREIGN KEY(right) REFERENCES node(id)",
                               "UNIQUE (node_tree,value)"});
      db_.upsert_string("config", "item", "content", "version", VERSION);
    }
  }

  data_adapter(const data_adapter &) = delete;
  data_adapter(data_adapter &&) = delete;

  node_key_t get_parent_by_id(node_key_t node_id) const
  {
    sqlitedb::string_parameter node_id_param {std::string(node_id)};
    std::string res;
    db_.exec("SELECT id FROM node WHERE left=? OR right=?1",
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
    db_.exec("INSERT INTO tree DEFAULT VALUES");
    db_.exec("SELECT last_insert_rowid()", [&tree_id](auto values, auto columns)
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

    sqlitedb::string_parameter tree_id_param{std::string(tree_id)};
    sqlitedb::int_parameter value_param{value};

    db_.exec(
        "INSERT INTO node (node_tree,value) VALUES(?,?) ON CONFLICT(node_tree,value) DO NOTHING",
        {&tree_id_param, &value_param});

    db_.exec("SELECT id FROM node WHERE node_tree=? AND value=?",
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
    sqlitedb::string_parameter node_id_param {std::string(node_id)};
    bool found{};
    db_.exec("SELECT value FROM node WHERE id = ?",
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
    sqlitedb::string_parameter tree_id_param {std::string{tree_id}};
    sqlitedb::int_parameter value_param {value};
    std::string res;
    db_.exec("SELECT id FROM node WHERE node_tree = ? AND value=?",
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
    sqlitedb::string_parameter node_param{std::string(node)};
    sqlitedb::string_parameter left_param{std::string(left)};
    db_.exec("UPDATE node SET left = ? WHERE id = ?", {&left_param, &node_param});
  }

  void bind_right(std::string_view node, std::string_view right) const
  {
    sqlitedb::string_parameter node_param{std::string(node)};
    sqlitedb::string_parameter right_param{std::string(right)};
    db_.exec("UPDATE node SET right = ? WHERE id = ?", {&right_param, &node_param});
  }

  std::string version() const
  {
    std::string result;
    try
    {
      db_.exec("SELECT content FROM config WHERE item='version'", [&result](auto values, auto columns)
           { result = values[0]; });
    }
    catch (std::exception const &)
    {
      result = "unknown";
    }
    return result;
  }

private:
  sqlitedb db_;
};
