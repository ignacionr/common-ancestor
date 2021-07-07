#pragma once
#include <vector>
#include <string>
#include <string_view>
#include <functional>
#include "abstract_protocol.h"
#include "tree.h"
#include "mini-parser.h"

template <typename repo_t>
struct tree_controller
{
  struct translator_t
  {
    std::function<std::string(typename repo_t::tree_key_t)> to_string;
    std::function<typename repo_t::tree_key_t(std::string const &)> parse;
  };

  tree_controller(repo_t &data, translator_t translator)
      : data_{data}, translator_{translator}
  {
  }

  void common_ancestor(abstract_protocol &proto)
  {
    typename repo_t::tree_key_t tree_id;
    std::string tree_id_string;
    int value1{}, value2{};
    mini_parser p;
    p.set(p.ignore(2, p.read(tree_id_string, p.ignore(1, p.read_int(value1, p.read_int(value2, p.parse_throw))))));
    for (auto c : proto.uri)
    {
      p(c);
    }
    tree_id = translator_.parse(tree_id_string);
    tree t{tree_id};
    auto result{t.find_common_ancestor(data_, value1, value2)};
    proto.reply(std::to_string(result));
  }

  void post_tree(abstract_protocol &proto)
  {
    auto tree_id = tree<typename repo_t::tree_key_t>::parse(data_, proto.body).id();
    proto.reply(translator_.to_string(tree_id));
  }

private:
  repo_t &data_;
  translator_t translator_;
};
