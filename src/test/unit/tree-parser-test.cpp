#include <gtest/gtest.h>
#include "../../tree-parser.h"

TEST(tree_parser, no_text) {
  int times{};
  tree_parser::parse("", [&times](auto){ ++times;});
  EXPECT_EQ(times, 0);
}

TEST(tree_parser, invalid_text) {
  EXPECT_THROW(tree_parser::parse("abc", [](auto){ }), std::runtime_error);
  EXPECT_THROW(tree_parser::parse("[]", [](auto){ }), std::runtime_error);
  EXPECT_THROW(tree_parser::parse("[<<]", [](auto){ }), std::runtime_error);
  EXPECT_THROW(tree_parser::parse("[1<2<3]", [](auto){ }), std::runtime_error);
  EXPECT_THROW(tree_parser::parse("[1>2>3]", [](auto){ }), std::runtime_error);
  EXPECT_THROW(tree_parser::parse("[1<2>3][]", [](auto){ }), std::runtime_error);
  EXPECT_THROW(tree_parser::parse("[][1<2>3]", [](auto){ }), std::runtime_error);
  EXPECT_THROW(tree_parser::parse("[2<2>3]", [](auto){ }), std::runtime_error);
  EXPECT_THROW(tree_parser::parse("[2<3>3]", [](auto){ }), std::runtime_error);
  EXPECT_THROW(tree_parser::parse("[10<8>9]", [](auto){ }), std::runtime_error);
}

TEST(tree_parser, valid_text_1) {
  tree_parser::triplet const expected[] = {
    {5,10,15}
  };
  tree_parser::triplet const *current {expected};
  tree_parser::parse("[5<10>15]", [&current](auto value) {
    EXPECT_EQ(value.left, current->left);
    EXPECT_EQ(value.value, current->value);
    EXPECT_EQ(value.right, current->right);
    ++current;
  });
  EXPECT_EQ(current, expected + sizeof(expected)/ sizeof(*expected));
}


TEST(tree_parser, valid_text_2) {
  tree_parser::triplet const expected[] = {
    {5,10,15},
    {{},10,{}},
    {{}, 5, 7},
    {13, 15, {}},
    {11, 13, 14}
  };
  tree_parser::triplet const *current {expected};
  tree_parser::parse("[5<10>15][<10>][5>7][13<15][11<13>14]", [&current](auto value) {
    EXPECT_EQ(value.left, current->left);
    EXPECT_EQ(value.value, current->value);
    EXPECT_EQ(value.right, current->right);
    ++current;
  });
  EXPECT_EQ(current, expected + sizeof(expected)/ sizeof(*expected));
}
