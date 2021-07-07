#pragma once
#include <vector>
#include <string>
#include <string_view>
#include "abstract_protocol.h"
#include "tree.h"
#include "mini-parser.h"

template <typename repo_t>
struct tree_controller
{
  tree_controller(repo_t &data, std::string_view tree_prefix) 
  : data_{data}, tree_prefix_{tree_prefix} 
  {
  }

  void common_ancestor(abstract_protocol &proto)
  {
    std::string tree_id;
    int value1{}, value2{};
    mini_parser p;
    p.set(p.ignore(2, p.read(tree_id, p.ignore(1, p.read_int(value1, p.read_int(value2, p.parse_throw))))));
    for (auto c : proto.uri)
    {
      p(c);
    }
    tree t{tree_id};
    auto result {t.find_common_ancestor(data_, value1, value2)};
    proto.reply(std::to_string(result));
  }

  void post_tree(abstract_protocol &proto)
  {
    auto tree_id = tree<std::string>::parse(data_, proto.body).id();
    proto.reply(tree_prefix_ + tree_id);
  }

private:
  repo_t &data_;
  std::string tree_prefix_;
};
