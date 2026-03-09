package com.quant.ui;

/**
 * QuantUI — Virtual router for the Quant GUI.
 *
 * Renders complete HTML pages from engine state without any HTTP server.
 * All C++ math and persistence runs natively via the NDK.
 *
 * Usage:
 *   QuantUI.init(dbDir, "Quant");
 *   UIResponse r = QuantUI.get("/chains");
 *   if (r.isOk()) { webView.loadData(r.html, ...); }
 */
public class QuantUI {

    static {
        System.loadLibrary("quant-native");
    }

    public static void init(String dbDir, String appName) {
        nativeInit(dbDir, appName);
    }

    public static void destroy() {
        nativeDestroy();
    }

    public static UIResponse get(String path, String queryString) {
        return nativeGet(path, queryString);
    }

    public static UIResponse get(String path) {
        return nativeGet(path, null);
    }

    public static UIResponse post(String path, String formBody) {
        return nativePost(path, formBody);
    }

    // Native methods — implemented in quant_ui_jni.cpp
    private static native void       nativeInit(String dbDir, String appName);
    private static native void       nativeDestroy();
    private static native UIResponse nativeGet(String path, String queryString);
    private static native UIResponse nativePost(String path, String formBody);
}
