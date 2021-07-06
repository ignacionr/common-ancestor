#include <gtest/gtest.h>
#include "../../data-adapter.h"

TEST(data_adapter, will_construct_simple_trees) {
  data_adapter data;
  auto tree {data.new_tree()};
  auto first_node{data.ensure_node(tree, 20)};
  auto left_node{data.ensure_node(tree, 10)};
  auto right_node{data.ensure_node(tree, 30)};
  data.bind_left(first_node, left_node);
  data.bind_right(first_node, right_node);
  EXPECT_EQ(data.get_parent_by_id(left_node), first_node);
  EXPECT_EQ(data.get_parent_by_id(right_node), first_node);
  EXPECT_EQ(data.get_parent_by_id(first_node), std::string());
}
