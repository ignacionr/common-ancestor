extern "C"
{
#include <mongoose.h>
}
#include <functional>
#include "data-adapter.h"

static void route(struct mg_connection *c, int ev, void *ev_data, void *fn_data)
{
  static data_adapter data;
  if (ev == MG_EV_HTTP_MSG)
  {
    try {
    auto hm{(struct mg_http_message *)ev_data};
      if (mg_http_match_uri(hm, "/version")) {
        mg_http_reply(c, 200, nullptr, data.version().c_str());
      }
      else if (mg_http_match_uri(hm, "/tree")) {
        auto tree_id = data.insert_tree({hm->body.ptr, hm->body.len});
        mg_http_reply(c, 200, nullptr, tree_id.c_str());
      }
      else if (mg_http_match_uri(hm, "/tree/*/common-ancestor/*/*")) {
        std::string tree_id;
        int value1{}, value2{};
        auto end = hm->uri.ptr + hm->uri.len;
        using parser = std::function<void(char)>;
        parser current;
        auto parse_throw = [](char){ throw "parse error"; };
        auto ignore = [&current](size_t count, parser next) {
          return [count, &current, next](char c) mutable {
            if (c == '/' && --count == 0) {
              current = next;
            }
          };
        };
        auto read = [&current](std::string &s, parser next) {
          return [&current, next, &s](char c) {
            if (c == '/') {
              current = next;
            }
            else {
              s += c;
            }
          };
        };
        auto read_int = [&current](int &s, parser next) {
          return [&current, &s, next](char c) {
            if (isdigit(c)) {
              s *= 10;
              s += c - '0';
            }
            else {
              current = next;
            }
          };
        };
        current = ignore(2, read(tree_id, ignore(1, read_int(value1, read_int(value2, parse_throw)))));
        for (auto p {hm->uri.ptr}; p < end; ++p) {
          current(*p);
        }

        mg_http_reply(c, 200, nullptr, std::to_string(data.find_common_ancestor(tree_id, value1, value2)).c_str());
      }
      else {
        mg_http_reply(c, 404, nullptr, "Not found");
      }
    }
    catch(std::exception const &e) {
      mg_http_reply(c, 500, nullptr, e.what());
    }
  }
}

int main()
{
  struct mg_mgr mgr;
  struct mg_connection *c;
  mg_mgr_init(&mgr);
  if ((c = mg_http_listen(&mgr, "http://0.0.0.0:8080", route, nullptr)) == nullptr)
  {
    exit(EXIT_FAILURE);
  }

  // Start infinite event loop
  for (;;)
    mg_mgr_poll(&mgr, 1000);
  mg_mgr_free(&mgr);
  return 0;
}
