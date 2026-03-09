// ============================================================
// quant_ui_jni.cpp — JNI bridge for the virtual router
//
// Java calls QuantUI.get("/path") → gets rendered HTML string
// Java calls QuantUI.post("/path", "key=val&...") → gets redirect
// ============================================================

#include <jni.h>
#include "quant_ui.h"
#include "quant_engine.h"

#include <string>

// Non-static so quant_mcp_jni.cpp (same .so) can share the pointer
QEngine* g_uiEngine = nullptr;

static std::string jstr(JNIEnv* env, jstring s)
{
    if (!s) return "";
    const char* c = env->GetStringUTFChars(s, nullptr);
    std::string r(c);
    env->ReleaseStringUTFChars(s, c);
    return r;
}

extern "C" {

JNIEXPORT void JNICALL
Java_com_quant_ui_QuantUI_nativeInit(JNIEnv* env, jclass, jstring dbDir, jstring appName)
{
    if (g_uiEngine) qe_close(g_uiEngine);
    g_uiEngine = qe_open(jstr(env, dbDir).c_str());
    if (appName) qui_set_app_name(jstr(env, appName).c_str());
}

JNIEXPORT void JNICALL
Java_com_quant_ui_QuantUI_nativeDestroy(JNIEnv*, jclass)
{
    if (g_uiEngine) { qe_close(g_uiEngine); g_uiEngine = nullptr; }
}

JNIEXPORT jobject JNICALL
Java_com_quant_ui_QuantUI_nativeGet(JNIEnv* env, jclass, jstring path, jstring queryString)
{
    if (!g_uiEngine) return nullptr;

    QUIResponse r = qui_get(g_uiEngine,
                            jstr(env, path).c_str(),
                            queryString ? jstr(env, queryString).c_str() : nullptr);

    // Build Java UIResponse object
    jclass cls = env->FindClass("com/quant/ui/UIResponse");
    jmethodID ctor = env->GetMethodID(cls, "<init>", "(ILjava/lang/String;Ljava/lang/String;)V");

    jstring jHtml = r.html ? env->NewStringUTF(r.html) : nullptr;
    jstring jRedir = r.redirect[0] ? env->NewStringUTF(r.redirect) : nullptr;

    jobject resp = env->NewObject(cls, ctor, r.statusCode, jHtml, jRedir);
    qui_free_response(&r);
    return resp;
}

JNIEXPORT jobject JNICALL
Java_com_quant_ui_QuantUI_nativePost(JNIEnv* env, jclass, jstring path, jstring formBody)
{
    if (!g_uiEngine) return nullptr;

    QUIResponse r = qui_post(g_uiEngine,
                             jstr(env, path).c_str(),
                             formBody ? jstr(env, formBody).c_str() : nullptr);

    jclass cls = env->FindClass("com/quant/ui/UIResponse");
    jmethodID ctor = env->GetMethodID(cls, "<init>", "(ILjava/lang/String;Ljava/lang/String;)V");

    jstring jHtml = r.html ? env->NewStringUTF(r.html) : nullptr;
    jstring jRedir = r.redirect[0] ? env->NewStringUTF(r.redirect) : nullptr;

    jobject resp = env->NewObject(cls, ctor, r.statusCode, jHtml, jRedir);
    qui_free_response(&r);
    return resp;
}

} // extern "C"
