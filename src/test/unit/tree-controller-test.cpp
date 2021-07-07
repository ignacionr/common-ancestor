#include <gtest/gtest.h>
#include "../../tree-controller.h"
#include "mem-adapter.h"

std::string id_to_string(size_t id) {
  return "123-" + std::to_string(id);
}

TEST(tree_controller, post_tree)
{
  mem_adapter adapter;
  tree_controller controller(adapter, 
  tree_controller<mem_adapter>::translator_t{id_to_string, [](std::string const &)->size_t{ throw std::runtime_error("unexpected");}});
  std::string reply;
  abstract_protocol proto {
    "uri",
    "[5<10>15]",
    [&reply](auto contents){ reply = contents; }
  };
  controller.post_tree(proto);
  ASSERT_NE(reply, std::string());
  ASSERT_GT(reply.size(), 4);
  ASSERT_EQ(reply.substr(0,4), "123-");
}

TEST(tree_controller, common_ancestor)
{
  mem_adapter adapter;
  auto const tree_id {adapter.new_tree()};
  auto const left_id {adapter.ensure_node(tree_id, 10)};
  auto const center_id {adapter.ensure_node(tree_id, 20)};
  auto const right_id {adapter.ensure_node(tree_id, 30)};
  adapter.bind_left(center_id, left_id);
  adapter.bind_right(center_id, right_id);

  tree_controller controller(adapter, {id_to_string, [](std::string const &src){ return static_cast<size_t>(std::atol(src.c_str()));}});
  std::string reply;
  std::string uri {"/tree/"};
  uri += std::to_string(tree_id);
  uri += "/common-ancestor/10/30";
  abstract_protocol proto {
    uri,
    {},
    [&reply](auto contents){ reply = contents; }
  };
  controller.common_ancestor(proto);
  ASSERT_EQ(reply, "20");
}
