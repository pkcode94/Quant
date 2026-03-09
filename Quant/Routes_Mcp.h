#pragma once

#include "AppContext.h"

#include <mutex>
#include <map>
#include <filesystem>
#include <iostream>
#include <exception>
// The .cpp files expect their headers on the include path.  We satisfy
// that by including the headers first, then guarding re-inclusion.
#include "../libquant-engine/include/quant_engine.h"
#include "../libquant-engine/include/quant_engine_types.h"
#include "../libquant-mcp/include/quant_mcp.h"

// Prevent the .cpp files from re-including headers they can't find.
// Note: Quant\QuantMath.h (already included) provides the QuantMath class;
// guard the libquant copy so it is not pulled in a second time.
#define QUANT_ENGINE_H
#define QUANT_ENGINE_TYPES_H
#define LIBQUANT_QUANTMATH_H
#define QUANT_MCP_H

// Now pull in the implementations.
#include "../libquant-engine/src/quant_engine.cpp"
#include "../libquant-mcp/src/quant_mcp.cpp"

// ============================================================
// MCP endpoint for the desktop Quant HTTP server
//
// POST /mcp  -- JSON-RPC 2.0 (MCP Streamable HTTP transport)
// GET  /mcp  -- Status page (browser-friendly)
//
// The MCP handler runs against its own QEngine instance that
// points at the current user's database directory, so MCP
// tools see the same trades, entries, chains, etc.
// ============================================================

inline void registerMcpRoutes(httplib::Server& svr, AppContext& ctx)
{
    // Lazy-init: one QEngine per user, matching their DB dir.
    // For the default single-user case this opens once.
    static std::mutex               mcpMutex;
    static std::map<std::string, QEngine*> mcpEngines;
    static bool mcpInited = false;

    // POST /mcp -- MCP JSON-RPC handler
    svr.Post("/mcp", [&](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");

        try {
            std::lock_guard<std::mutex> lk(mcpMutex);
            auto user = ctx.currentUser(req);
            if (user.empty()) user = UserManager::adminUser();
            auto key  = user;

            auto it = mcpEngines.find(key);
            QEngine* engine = nullptr;
            if (it != mcpEngines.end()) {
                engine = it->second;
            } else {
                std::string dbDir = ctx.users.userDbDir(user);
                std::filesystem::create_directories(dbDir);
                engine = qe_open(dbDir.c_str());
                if (engine) {
                    mcpEngines[key] = engine;
                    if (!mcpInited) {
                        qmcp_init(dbDir.c_str());
                        mcpInited = true;
                    }
                }
            }

            if (!engine) {
                res.set_content("{\"error\":\"Engine not available\"}", "application/json");
                res.status = 500;
                return;
            }

            // Safety check: verify engine is valid
            if (!engine) {
                res.set_content("{\"error\":\"Engine is null\"}", "application/json");
                res.status = 500;
                return;
            }

            char* response = qmcp_handle(engine, req.body.c_str());
            if (response) {
                res.set_content(response, "application/json");
                qmcp_free(response);
            } else {
                res.status = 202; // notification, no response
            }
        } catch (const std::exception& ex) {
            res.set_content(std::string("{\"error\":\"MCP error: ") + ex.what() + "\"}", "application/json");
            res.status = 500;
        } catch (...) {
            res.set_content("{\"error\":\"MCP unknown error\"}", "application/json");
            res.status = 500;
        }
    });

    // GET /mcp -- status page
    svr.Get("/mcp", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(
            "<!DOCTYPE html><html><head><meta charset='utf-8'>"
            "<title>Quant MCP</title>"
            "<style>body{font-family:monospace;background:#0b1426;color:#cbd5e1;padding:20px;}"
            "h1{color:#22c55e;}code{color:#c9a44a;}.ok{color:#22c55e;font-weight:bold;}</style></head><body>"
            "<h1>&#9939; Quant MCP Server</h1>"
            "<p>Status: <span class='ok'>Running</span></p>"
            "<p>Endpoint: <code>POST /mcp</code> (JSON-RPC 2.0)</p>"
            "<p>Transport: Streamable HTTP</p>"
            "<hr style='border-color:#1e293b;'>"
            "<p style='color:#64748b;'>Point your AI assistant at this endpoint. "
            "All Quant Ledger tools are available: portfolio, trades, serial plans, "
            "what-if, chains, and more.</p></body></html>",
            "text/html");
    });

    // OPTIONS /mcp -- CORS preflight
    svr.Options("/mcp", [](const httplib::Request&, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        res.status = 204;
    });
}
