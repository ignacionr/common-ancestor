extern "C"
{
#include <mongoose.h>
}
#include <functional>
#include <unordered_map>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include "data-adapter.h"
#include "tree.h"
#include "tree-controller.h"
#include "abstract_protocol.h"

using controller_map_t = std::unordered_map<std::string, std::function<void(abstract_protocol &)>>;

static void route(struct mg_connection *c, int ev, void *ev_data, void *fn_data)
{
  if (ev == MG_EV_HTTP_MSG)
  {
    try
    {
      auto hm{(struct mg_http_message *)ev_data};
      auto cm{reinterpret_cast<controller_map_t const *>(fn_data)};
      abstract_protocol proto {
          {hm->uri.ptr, hm->uri.len},
          {hm->body.ptr, hm->body.len},
          [&c](std::string_view contents)
          {
            mg_http_reply(c, 200, nullptr, std::string(contents).c_str());
          }};
      auto pos = std::find_if(cm->begin(), cm->end(), [hm](auto const &entry) {
        return mg_http_match_uri(hm, entry.first.c_str());
      });
      if (pos != cm->end()) {
        pos->second(proto);
      }
      else {
        static mg_http_serve_opts opts {
          "html", nullptr, nullptr
        };
        mg_http_serve_dir(c, hm, &opts);
      }
    }
    catch (std::exception const &e)
    {
      mg_http_reply(c, 500, nullptr, e.what());
    }
  }
}

int main(int argc, char **argv)
{
  data_adapter data;
  auto using_balancer = 
    std::any_of(argv + 1, argv + argc, [](char *text){ return 0 == std::strncmp("--balancer", text, 11);}) ||
    std::any_of(argv + 1, argv + argc, [](char *text){ return 0 == std::strncmp("-b", text, 3);});
  std::string prefix;
  if (using_balancer) {
    char buff [64];
    gethostname(buff, 64);
    prefix = buff;
    prefix += '-';
  }
  tree_controller tc{data, prefix};
  controller_map_t map {
    {"/tree/*/common-ancestor/*/*", [&tc](auto &proto){tc.common_ancestor(proto);}},
    {"/tree", [&tc](auto &proto){ tc.post_tree(proto); }},
    {"/version", [](auto &proto) { proto.reply(VERSION);}},
  };

  struct mg_mgr mgr;
  struct mg_connection *c;
  mg_mgr_init(&mgr);
  if ((c = mg_http_listen(&mgr, "http://0.0.0.0:8080", route, &map)) == nullptr)
  {
    exit(EXIT_FAILURE);
  }

  // Start infinite event loop
  for (;;)
    mg_mgr_poll(&mgr, 1000);
  mg_mgr_free(&mgr);
  return 0;
}
