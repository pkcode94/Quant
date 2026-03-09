package com.quant.ui;

import android.annotation.SuppressLint;
import android.content.Context;
import android.util.AttributeSet;
import android.webkit.WebResourceRequest;
import android.webkit.WebResourceResponse;
import android.webkit.WebView;
import android.webkit.WebViewClient;

import java.io.ByteArrayInputStream;
import java.nio.charset.StandardCharsets;

/**
 * QuantWebView — WebView that routes all navigation through the
 * C++ virtual router instead of making network requests.
 *
 * All page rendering, form submissions, and redirects happen
 * locally via JNI calls to the engine. No HTTP server needed.
 */
@SuppressLint("SetJavaScriptEnabled")
public class QuantWebView extends WebView {

    private static final String BASE_URL = "app://quant";

    public QuantWebView(Context context) {
        super(context);
        setup();
    }

    public QuantWebView(Context context, AttributeSet attrs) {
        super(context, attrs);
        setup();
    }

    private void setup() {
        setBackgroundColor(0xFF0B1426);
        getSettings().setJavaScriptEnabled(true);
        getSettings().setDomStorageEnabled(true);

        setWebViewClient(new WebViewClient() {
            @Override
            public WebResourceResponse shouldInterceptRequest(WebView view,
                                                               WebResourceRequest request) {
                String url = request.getUrl().toString();
                if (!url.startsWith(BASE_URL)) {
                    return super.shouldInterceptRequest(view, request);
                }

                String path = url.substring(BASE_URL.length());
                if (path.isEmpty()) path = "/";

                String queryString = null;
                int qmark = path.indexOf('?');
                if (qmark >= 0) {
                    queryString = path.substring(qmark + 1);
                    path = path.substring(0, qmark);
                }

                UIResponse response = QuantUI.get(path, queryString);
                if (response == null) {
                    return super.shouldInterceptRequest(view, request);
                }

                if (response.isRedirect()) {
                    final String target = response.redirect;
                    view.post(() -> navigateTo(target));
                    return new WebResourceResponse("text/html", "UTF-8",
                            new ByteArrayInputStream(new byte[0]));
                }

                if (response.html != null) {
                    byte[] data = response.html.getBytes(StandardCharsets.UTF_8);
                    return new WebResourceResponse("text/html", "UTF-8",
                            new ByteArrayInputStream(data));
                }

                return super.shouldInterceptRequest(view, request);
            }

            @Override
            public boolean shouldOverrideUrlLoading(WebView view, WebResourceRequest request) {
                String url = request.getUrl().toString();
                if (url.startsWith(BASE_URL)) {
                    String path = url.substring(BASE_URL.length());
                    if (path.isEmpty()) path = "/";
                    navigateTo(path);
                    return true;
                }
                return false;
            }
        });

        // JavaScript bridge for intercepting form POST submissions
        addJavascriptInterface(new Object() {
            @android.webkit.JavascriptInterface
            public void postForm(String action, String data) {
                post(() -> submitForm(action, data));
            }
        }, "QuantBridge");
    }

    /**
     * Navigate to a virtual path (GET).
     */
    public void navigateTo(String path) {
        String queryString = null;
        int qmark = path.indexOf('?');
        if (qmark >= 0) {
            queryString = path.substring(qmark + 1);
            path = path.substring(0, qmark);
        }

        UIResponse response = QuantUI.get(path, queryString);
        if (response == null) return;

        if (response.isRedirect()) {
            navigateTo(response.redirect);
            return;
        }

        if (response.html != null) {
            final String finalPath = path;
            loadDataWithBaseURL(BASE_URL + finalPath, response.html,
                    "text/html", "UTF-8", null);
        }
    }

    /**
     * Submit a form (POST) to the virtual router.
     */
    public void submitForm(String action, String formData) {
        UIResponse response = QuantUI.post(action, formData);
        if (response == null) return;

        if (response.isRedirect()) {
            navigateTo(response.redirect);
        } else if (response.html != null) {
            loadDataWithBaseURL(BASE_URL + action, response.html,
                    "text/html", "UTF-8", null);
        }
    }

    @Override
    public void loadDataWithBaseURL(String baseUrl, String data,
                                     String mimeType, String encoding,
                                     String historyUrl) {
        // Inject form interception script before </body>
        if (data != null && data.contains("</body>")) {
            String script =
                "<script>" +
                "document.addEventListener('submit', function(e) {" +
                "  e.preventDefault();" +
                "  var form = e.target;" +
                "  var data = new URLSearchParams(new FormData(form)).toString();" +
                "  var action = form.getAttribute('action') || '/';" +
                "  if (action.indexOf('app://') === 0) action = action.replace('app://quant', '');" +
                "  if (action.charAt(0) !== '/') action = '/' + action;" +
                "  QuantBridge.postForm(action, data);" +
                "}, true);" +
                "</script>";
            data = data.replace("</body>", script + "</body>");
        }
        super.loadDataWithBaseURL(baseUrl, data, mimeType, encoding, historyUrl);
    }
}
