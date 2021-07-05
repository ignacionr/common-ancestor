#include <gtest/gtest.h>
#include <optional>
#include <stdexcept>
#include "../../tree.h"
#include "mem-adapter.h"

struct triplet
{
  int value;
  std::optional<int> left, right;
};

TEST(tree, simple_positive)
{
  mem_adapter adapter;
  auto tree_id{adapter.new_tree()};
  tree the_tree{tree_id};
  the_tree.add_node(adapter, triplet{15, 10, 20});
  ASSERT_EQ(15, the_tree.find_common_ancestor(adapter, 10, 20));
}

TEST(tree, positives)
{
  mem_adapter adapter;
  auto tree_id{adapter.new_tree()};
  tree the_tree{tree_id};
  the_tree.add_node(adapter, triplet{15, 10, 20});
  the_tree.add_node(adapter, triplet{10, 5, 11});
  the_tree.add_node(adapter, triplet{11, {}, 12});
  the_tree.add_node(adapter, triplet{12, {}, 13});
  the_tree.add_node(adapter, triplet{13, {}, 14});
  ASSERT_EQ(15, the_tree.find_common_ancestor(adapter, 20, 14));
  ASSERT_EQ(10, the_tree.find_common_ancestor(adapter, 10, 14));
  ASSERT_EQ(11, the_tree.find_common_ancestor(adapter, 11, 14));
  ASSERT_EQ(10, the_tree.find_common_ancestor(adapter, 5, 14));
}

TEST(tree, simple_negative)
{
  mem_adapter adapter;
  auto tree_id{adapter.new_tree()};
  tree the_tree{tree_id};
  the_tree.add_node(adapter, triplet{15, 10, 20});
  the_tree.add_node(adapter, triplet{115, 110, 120});
  EXPECT_THROW(the_tree.find_common_ancestor(adapter, 10, 120), std::runtime_error);
}
