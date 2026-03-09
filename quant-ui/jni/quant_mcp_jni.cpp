// ============================================================
// quant_mcp_jni.cpp -- JNI bridge for the MCP request handler
//
// Shares the same QEngine* instance as the UI (g_uiEngine in
// quant_ui_jni.cpp) so the MCP daemon operates on the same
// ledger database.
// ============================================================

#include <jni.h>
#include "quant_mcp.h"
#include "quant_engine.h"

#include <string>

// Engine pointer owned by quant_ui_jni.cpp (same .so)
extern QEngine* g_uiEngine;

static std::string jstr_mcp(JNIEnv* env, jstring s)
{
    if (!s) return "";
    const char* c = env->GetStringUTFChars(s, nullptr);
    std::string r(c);
    env->ReleaseStringUTFChars(s, c);
    return r;
}

extern "C" {

JNIEXPORT void JNICALL
Java_com_quant_mcp_McpBridge_nativeInitMcp(JNIEnv* env, jclass, jstring dbDir)
{
    qmcp_init(jstr_mcp(env, dbDir).c_str());
}

JNIEXPORT jstring JNICALL
Java_com_quant_mcp_McpBridge_nativeHandle(JNIEnv* env, jclass, jstring jsonRequest)
{
    if (!g_uiEngine) return nullptr;
    std::string req = jstr_mcp(env, jsonRequest);
    char* resp = qmcp_handle(g_uiEngine, req.c_str());
    if (!resp) return nullptr;
    jstring result = env->NewStringUTF(resp);
    qmcp_free(resp);
    return result;
}

JNIEXPORT void JNICALL
Java_com_quant_mcp_McpBridge_nativeShutdownMcp(JNIEnv*, jclass)
{
    qmcp_shutdown();
}

} // extern "C"
