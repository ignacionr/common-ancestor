extern "C"
{
#include <mongoose.h>
}
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
        mg_http_reply(c, 200, nullptr, std::to_string(tree_id).c_str());
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
