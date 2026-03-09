#ifndef QUANT_UI_H
#define QUANT_UI_H

// ============================================================
// quant_ui.h — Virtual router / GUI provider (C API)
//
// Renders HTML pages from engine state without any HTTP
// dependency.  Consumers provide path + optional form data
// and receive complete HTML documents.
//
// Desktop usage (HTTP adapter):
//   svr.Get("(.*)", [&](req, res) {
//       QUIResponse r = qui_get(engine, req.path.c_str(), qs);
//       if (r.statusCode == 303)
//           res.set_redirect(r.redirect, 303);
//       else
//           res.set_content(r.html, "text/html");
//       qui_free_response(&r);
//   });
//
// Android usage (WebView):
//   String html = QuantUI.get("/chains");
//   webView.loadDataWithBaseURL("app://quant", html,
//       "text/html", "UTF-8", null);
//
// All paths mirror the existing web UI routes in Quant/.
// ============================================================

#include "quant_engine.h"

#ifdef __cplusplus
extern "C" {
#endif

// ---- Response ----

typedef struct {
    char*       html;           // allocated HTML (NULL on redirect)
    int         statusCode;     // 200, 303, 404, etc.
    char        redirect[512];  // target URL if statusCode == 303
} QUIResponse;

// ---- Virtual router ----

// Handle a GET request.  queryString may be NULL.
QUIResponse qui_get(QEngine* e, const char* path, const char* queryString);

// Handle a POST request.  formBody is URL-encoded form data.
QUIResponse qui_post(QEngine* e, const char* path, const char* formBody);

// Free the HTML buffer inside a response.
void qui_free_response(QUIResponse* r);

// ---- Theme / branding ----

// Override the page title prefix (default: "Quant").
void qui_set_app_name(const char* name);

// Override the nav bar items.  Pass NULL to use defaults.
// Format: "Label1\tPath1\nLabel2\tPath2\n..."
void qui_set_nav(const char* navSpec);

#ifdef __cplusplus
}
#endif

#endif // QUANT_UI_H
