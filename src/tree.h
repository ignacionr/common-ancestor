#pragma once
#include <string>
#include <string_view>

template<typename TData>
class tree {
public:
  tree(TData *data, std::string_view tree_id): data_{data}, tree_id_{tree_id} {}
  template<typename TCallback>
  void visit_ancestors(int value, TCallback cb) 
  {
    for (auto ancestor = get_id_by_value(value); !ancestor.empty(); ancestor = get_parent_by_id(ancestor)) { 
      if (!cb(ancestor)) break;
    }
  }

  std::string get_id_by_value(int const value) const
  {
    auto const valueStr {std::to_string(value)};
    auto cmd = std::string("SELECT id FROM node WHERE node_tree = ");
    cmd += tree_id_;
    cmd += " AND value=";
    cmd += std::to_string(value);
    std::string res;
    data_->exec( cmd,
      [&res](auto values, auto columns) {
        res = values.front();
      });
    if (res.empty()) {
      throw std::runtime_error("Not found.");
    }
    return res;
  }

  int get_value_by_id(std::string_view node_id) const
  {
    auto cmd = std::string("SELECT value FROM node WHERE id = ");
    cmd += node_id;
    int res;
    bool found{};
    data_->exec( cmd,
      [&res, &found](auto values, auto columns) {
        res = std::atoi(std::string(values.front()).c_str());
        found = true;
      });
    if (!found) {
      throw std::runtime_error("Not found.");
    }
    return res;
  }

  std::string get_parent_by_id(std::string_view node_id) const 
  {
    auto cmd = std::string("SELECT id FROM node WHERE ");
    cmd += " left=";
    cmd += node_id;
    cmd += " OR right=";
    cmd += node_id;

    std::string res;
    data_->exec( cmd,
      [&res](auto values, auto columns) {
        res = values.front();
      });
    return res;
  }

  std::string ensure_node(int const value) const
  {
    auto const valueStr {std::to_string(value)};
    std::string res;
    auto cmd = std::string("INSERT INTO node (value, node_tree) VALUES(") + valueStr + ",";
    cmd += tree_id_;
    cmd += ") "
      "ON CONFLICT(node_tree,value) DO NOTHING; SELECT id FROM node WHERE node_tree=";
    cmd += tree_id_;
    cmd += " AND value=" + valueStr;
    data_->exec( cmd,
      [&res](auto values, auto columns) {
        res = values.front();
      });
    return res;
  }

private:
  TData *data_;
  std::string tree_id_;
};