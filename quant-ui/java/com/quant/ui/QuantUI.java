package com.quant.ui;

/**
 * QuantUI — Virtual router for the Quant GUI.
 *
 * Renders complete HTML pages from engine state without any HTTP server.
 * Use with a WebView to display pages, intercept form submissions,
 * and route them back through post().
 *
 * Usage:
 *   QuantUI.init(getFilesDir().getAbsolutePath() + "/db", "Quant");
 *
 *   UIResponse r = QuantUI.get("/chains");
 *   if (r.isOk()) {
 *       webView.loadDataWithBaseURL("app://quant", r.html,
 *           "text/html", "UTF-8", null);
 *   }
 *
 *   // On form submit (intercepted by WebViewClient):
 *   UIResponse r = QuantUI.post("/wallet/deposit", "amount=100");
 *   if (r.isRedirect()) {
 *       UIResponse page = QuantUI.get(r.redirect);
 *       webView.loadDataWithBaseURL("app://quant", page.html, ...);
 *   }
 */
public class QuantUI {

    static {
        System.loadLibrary("quant-ui");
    }

    /**
     * Initialize the UI engine.
     * @param dbDir   Path to the database directory
     * @param appName Application name shown in page titles
     */
    public static void init(String dbDir, String appName) {
        nativeInit(dbDir, appName);
    }

    /** Shut down and free resources. */
    public static void destroy() {
        nativeDestroy();
    }

    /**
     * Render a page (GET request).
     * @param path        URL path, e.g. "/chains" or "/wallet"
     * @param queryString Optional query string (without '?'), e.g. "msg=OK"
     * @return UIResponse with HTML or redirect
     */
    public static UIResponse get(String path, String queryString) {
        return nativeGet(path, queryString);
    }

    /** GET without query string. */
    public static UIResponse get(String path) {
        return nativeGet(path, null);
    }

    /**
     * Process a form submission (POST request).
     * @param path     URL path, e.g. "/wallet/deposit"
     * @param formBody URL-encoded form data, e.g. "amount=100"
     * @return UIResponse — typically a redirect (303)
     */
    public static UIResponse post(String path, String formBody) {
        return nativePost(path, formBody);
    }

    // ---- Native methods ----

    private static native void      nativeInit(String dbDir, String appName);
    private static native void      nativeDestroy();
    private static native UIResponse nativeGet(String path, String queryString);
    private static native UIResponse nativePost(String path, String formBody);
}
