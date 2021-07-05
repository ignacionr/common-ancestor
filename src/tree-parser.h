#include <string_view>
#include <cctype>
#include <functional>
#include <optional>
#include <unordered_map>
#include <stdexcept>

enum token_type
{
  open_node,
  value,
  right_arrow,
  left_arrow,
  close_node,
  end_of_file,
  invalid
};

struct token
{
  token_type t;
  int token_value;
  
  static token next(std::string_view &text)
  {
    if (text.size() == 0)
    {
      return {token_type::end_of_file};
    }
    token result;
    if (std::isdigit(text.front()))
    {
      result.t = token_type::value;
      result.token_value = text.front() - '0';
      for (text = {text.data() + 1, text.size() - 1}; std::isdigit(text.front()); text = {text.data() + 1, text.size() - 1})
      {
        result.token_value *= 10;
        result.token_value += text.front() - '0';
      }
    }
    else
    {
      switch (text.front())
      {
      case '[':
        result.t = token_type::open_node;
        break;
      case ']':
        result.t = token_type::close_node;
        break;
      case '<':
        result.t = token_type::left_arrow;
        break;
      case '>':
        result.t = token_type::right_arrow;
        break;

      default:
        result.t = token_type::invalid;
        break;
      }
      text = {text.data() + 1, text.size() - 1};
    }
    return result;
  }
};

class tree_parser
{
public:
  struct triplet
  {
    std::optional<int> left;
    int value;
    std::optional<int> right;
  };

  using parse_callback = std::function<void(triplet)>;
  static void parse(std::string_view text, parse_callback callback)
  {
    auto const original{text};
    enum state
    {
      initial,
      left_or_value,
      post_left_or_value,
      in_value,
      post_in_value,
      in_right,
      emit
    } status{initial};
    using machine_move = std::function<state(token const &, triplet &)>;
    static const std::unordered_map<state, std::unordered_map<token_type, machine_move>> mechanism =
        {
            {initial, {
                          {token_type::open_node, [](token const &t, triplet &res)
                           {
                             res = {};
                             return left_or_value;
                           }},
                      }},
            {left_or_value, {
                                {token_type::value, [](token const &t, triplet &res)
                                 {
                                   res.left = t.token_value;
                                   return post_left_or_value;
                                 }},
                                {token_type::left_arrow, [](token const &t, triplet &res)
                                 {
                                   res.left.reset();
                                   return in_value;
                                 }},
                            }},
            {post_left_or_value, {{token_type::close_node, [](token const &t, triplet &res)
                                   {
                                     res.value = res.left.value();
                                     res.left.reset();
                                     res.right.reset();
                                     return emit;
                                   }},
                                  {token_type::left_arrow, [](token const &t, triplet &res)
                                   { return in_value; }},
                                  {token_type::right_arrow, [](token const &t, triplet &res)
                                   {
                                     res.value = res.left.value();
                                     res.left.reset();
                                     return in_right;
                                   }}}},
            {in_value, {
                           {token_type::value, [](token const &t, triplet &res)
                            {
                              res.value = t.token_value;
                              return post_in_value;
                            }},
                       }},
            {post_in_value, {{token_type::close_node, [](token const &t, triplet &res)
                              {
                                res.right.reset();
                                return emit;
                              }},
                             {token_type::right_arrow, [](token const &t, triplet &res)
                              { return in_right; }}}},
            {in_right, {
                           {token_type::value, [](token const &t, triplet &res)
                            {
                              res.right = t.token_value;
                              return in_right;
                            }},
                           {token_type::close_node, [](token const &t, triplet &)
                            { return emit; }},
                       }}};

    triplet res{};
    for (auto t{token::next(text)}; t.t != token_type::end_of_file; t = token::next(text))
    {
      auto const &s{mechanism.at(status)};
      auto pos{s.find(t.t)};
      if (pos == s.end())
      {
        throw std::runtime_error("Unable to parse " + std::string(original) + " at " + std::string(text) + ". Last state was " + std::to_string(status) + " last token type " + std::to_string(t.t));
      }
      status = mechanism.at(status).at(t.t)(t, res);
      if (status == emit)
      {
        if (res.left.has_value() && res.left.value() >= res.value) {
          throw std::runtime_error("The left node must have a lesser value.");
        }
        if (res.right.has_value() && res.right.value() <= res.value) {
          throw std::runtime_error("The right node must have a greater value.");
        }
        callback(res);
        status = initial;
      }
    }
  }
};
