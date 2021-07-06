#pragma once
#include <string_view>
#include <functional>

struct abstract_protocol {
  using reply_t = std::function<void(std::string_view)>;

  std::string_view uri;
  std::string_view body;
  reply_t reply;

  abstract_protocol(std::string_view uri_, std::string_view body_, reply_t r):
    uri{uri_}, body{body_}, reply{r} {};

  abstract_protocol(abstract_protocol&) = delete;
  abstract_protocol(abstract_protocol const&) = delete;
  abstract_protocol(abstract_protocol&&) = delete;
};
