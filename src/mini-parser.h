#pragma once
#include <functional>
#include <string>

struct mini_parser
{
  using parser = std::function<void(char)>;

  parser ignore(size_t count, parser const &next)
  {
    return [count, this, next](char c) mutable
    {
      if (c == '/' && --count == 0)
      {
        set(next);
      }
    };
  };
  parser read(std::string &s, parser const &next)
  {
    return [this, next, &s](char c)
    {
      if (c == '/')
      {
        set(next);
      }
      else
      {
        s += c;
      }
    };
  };
  parser read_int(int &s, parser const &next)
  {
    return [this, &s, next](char c)
    {
      if (isdigit(c))
      {
        s *= 10;
        s += c - '0';
      }
      else
      {
        set(next);
      }
    };
  };

  void set(parser const &p) { current_ = p; }

  static void parse_throw (char)
  { throw "parse error"; };

  void operator()(char c) {
    current_(c);
  }

private:
  parser current_;
};
