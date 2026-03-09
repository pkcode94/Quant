// ============================================================
// quant_mcp.h — Shared MCP request handler (C API)
//
// Processes JSON-RPC 2.0 requests (MCP protocol) against a
// QEngine instance.  Transport-agnostic: callers provide the
// raw JSON request string and receive a raw JSON response.
//
// Used by:
//   quant-mcp   — stdio transport  (desktop CLI)
//   quant-app   — HTTP transport   (Android daemon)
// ============================================================

#ifndef QUANT_MCP_H
#define QUANT_MCP_H

// Forward-declare QEngine so consumers don't need quant_engine.h
// in their include path.  The implementation (quant_mcp.cpp) includes
// the full header.
typedef struct QEngine QEngine;

#ifdef __cplusplus
extern "C" {
#endif

// Initialise the MCP handler.  Loads presets from dbDir/presets.json
// and seeds built-in defaults for any missing presets.
void qmcp_init(const char* dbDir);

// Process a single JSON-RPC request.
// Returns a malloc'd JSON response string, or NULL for notifications
// (which require no response).  Caller must free with qmcp_free().
char* qmcp_handle(QEngine* engine, const char* jsonRpcRequest);

// Free a string returned by qmcp_handle().
void qmcp_free(char* str);

// Shut down the MCP handler (no-op currently, reserved for cleanup).
void qmcp_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif // QUANT_MCP_H
