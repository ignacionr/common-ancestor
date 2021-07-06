#include <gtest/gtest.h>
#include "../../mini-parser.h"

TEST(mini_parser, uri) {
  std::string_view src {"/tree/2/common-ancestor/11/14"};
  std::string tree_id;
  int value1{}, value2{};
  mini_parser p;
  p.set(p.ignore(2, p.read(tree_id, p.ignore(1, p.read_int(value1, p.read_int(value2, p.parse_throw))))));
  for (auto c : src)
  {
    p(c);
  }
  EXPECT_EQ(tree_id, "2");
  EXPECT_EQ(value1, 11);
  EXPECT_EQ(value2, 14);
}
