#pragma once
#include "tree-parser.h"

template <typename TTreeKey>
class tree
{
public:
  tree(TTreeKey tree_id) : tree_id_{tree_id} {}

  TTreeKey id() const { return tree_id_; }

  static tree parse(auto &repo, std::string_view text)
  {
    tree t{repo.new_tree()};
    tree_parser::parse(text, [&repo, &t](auto node)
                       {
                         t.add_node(repo, node);
                       });
    return t;
  }

  void visit_ancestors(auto &repo, int value, auto cb) const
  {
    for (auto ancestor = repo.get_id_by_value(tree_id_, value); !ancestor.empty(); ancestor = repo.get_parent_by_id(ancestor))
    {
      if (!cb(ancestor))
        break;
    }
  }

  void add_node(auto &repo, auto node)
  {
    auto this_node = repo.ensure_node(tree_id_, node.value);
    if (node.left.has_value())
    {
      auto left = repo.ensure_node(tree_id_, node.left.value());
      repo.bind_left(this_node, left);
    }
    if (node.right.has_value())
    {
      auto right = repo.ensure_node(tree_id_, node.right.value());
      repo.bind_right(this_node, right);
    }
  }

  template <typename TRepo>
  int find_common_ancestor(TRepo &repo, int v1, int v2) const
  {
    std::set<typename TRepo::node_key_t> ancestors;
    visit_ancestors(repo, v1,
                    [&ancestors](auto id)
                    {
                      ancestors.emplace(id);
                      return true;
                    });
    bool found{};
    int result;
    visit_ancestors(repo, v2,
                    [&ancestors, &result, &repo, this](auto id)
                    {
                      if (ancestors.find(id) != ancestors.end())
                      {
                        result = repo.get_value_by_id(id);
                        return false;
                      }
                      return true;
                    });
    if (!found)
      throw std::runtime_error("No common ancestor.");
    return result;
  }

private:
  TTreeKey tree_id_;
};