#pragma once
#include <vector>
#include <memory>
#include <string>
#include <sstream>
#include <algorithm>
#include "memtree.h"

class mem_adapter{
public:
  using node_key_t = memtree*;
  using tree_key_t = size_t;

  node_key_t get_parent_by_id(node_key_t node_id) const
  {
    return node_id->parent;
  }

  tree_key_t new_tree()
  {
    forest_.push_back(std::vector<std::unique_ptr<memtree>>());
    return forest_.size() - 1;
  }

  node_key_t ensure_node(tree_key_t tree_id, int const value)
  {
    auto result = get_id_by_value(tree_id, value);
    if (!result) {
      auto &t {forest_[tree_id]};
      result = new memtree{nullptr, value};
      t.push_back(std::unique_ptr<memtree>{result});

    }

    return result;
  }

  int get_value_by_id(node_key_t node_id) const
  {
    return node_id->value;
  }

  node_key_t get_id_by_value(tree_key_t tree_id, int const value)
  {
    auto const &t {forest_[tree_id]};
    auto const pos = std::find_if(t.begin(), t.end(), [value](auto const &node){ return node->value == value; });
    node_key_t const result = (pos == t.end()) ? (node_key_t{}) : pos->get();
    return result;
  }

  void bind_left(node_key_t node, node_key_t left) const 
  {
    left->parent = node;
    node->left = left;
  }

  void bind_right(node_key_t node, node_key_t right) const 
  {
    right->parent = node;
    node->right = right;
  }

  std::string str() const 
  {
    std::stringstream ss;
    for (auto &t: forest_) {
      ss << "---Tree---" << std::endl;
      for (auto &n: t) {
        ss << "node value " << n->value;
        if (n->parent) {
          ss << " parent " << n->parent->value;
        }
        if (n->left) {
          ss << " left " << n->left->value;
        }
        if (n->right) {
          ss << " right " << n->right->value;
        }
        ss << std::endl;
      }
    }
    return ss.str();
  }

private:
  std::vector<std::vector<std::unique_ptr<memtree>>> forest_;
};
