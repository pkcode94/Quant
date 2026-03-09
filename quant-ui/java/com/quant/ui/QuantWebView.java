package com.quant.ui;

import android.annotation.SuppressLint;
import android.content.Context;
import android.util.AttributeSet;
import android.webkit.WebResourceRequest;
import android.webkit.WebResourceResponse;
import android.webkit.WebView;
import android.webkit.WebViewClient;

import java.io.ByteArrayInputStream;
import java.net.URLEncoder;
import java.nio.charset.StandardCharsets;

/**
 * QuantWebView — WebView that routes all navigation through the
 * virtual router instead of making network requests.
 *
 * Usage in Activity/Fragment:
 *   QuantUI.init(getFilesDir() + "/db", "Quant");
 *   QuantWebView wv = findViewById(R.id.webview);
 *   wv.navigateTo("/");
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
        getSettings().setJavaScriptEnabled(true);
        getSettings().setDomStorageEnabled(true);

        setWebViewClient(new WebViewClient() {
            @Override
            public WebResourceResponse shouldInterceptRequest(WebView view, WebResourceRequest request) {
                String url = request.getUrl().toString();

                // Only intercept our virtual URLs
                if (!url.startsWith(BASE_URL)) {
                    return super.shouldInterceptRequest(view, request);
                }

                String path = url.substring(BASE_URL.length());
                if (path.isEmpty()) path = "/";

                // Split path and query string
                String queryString = null;
                int qmark = path.indexOf('?');
                if (qmark >= 0) {
                    queryString = path.substring(qmark + 1);
                    path = path.substring(0, qmark);
                }

                String method = request.getMethod();
                UIResponse response;

                if ("POST".equalsIgnoreCase(method)) {
                    // For POST, form data would be in the request body.
                    // WebView doesn't easily expose POST body in shouldInterceptRequest,
                    // so we use JavaScript interception (see injectFormHandler).
                    response = QuantUI.post(path, queryString != null ? queryString : "");
                } else {
                    response = QuantUI.get(path, queryString);
                }

                if (response == null) {
                    return super.shouldInterceptRequest(view, request);
                }

                if (response.isRedirect()) {
                    // Schedule navigation on UI thread
                    final String target = response.redirect;
                    view.post(() -> navigateTo(target));
                    // Return empty response to cancel current load
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
        });

        // Inject JavaScript to intercept form POSTs and convert them to GETs
        // with form data in query string (so shouldInterceptRequest can handle them)
        injectFormHandler();
    }

    /**
     * Navigate to a virtual path.
     * @param path e.g. "/" or "/chains" or "/wallet?msg=OK"
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
            loadDataWithBaseURL(BASE_URL + path, response.html,
                    "text/html", "UTF-8", null);
        }
    }

    /**
     * Submit a form to the virtual router.
     * Called from JavaScript form interception.
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

    private void injectFormHandler() {
        // Add a JavaScript interface so forms can call back to Java
        addJavascriptInterface(new Object() {
            @android.webkit.JavascriptInterface
            public void postForm(String action, String data) {
                post(() -> submitForm(action, data));
            }
        }, "QuantBridge");
    }

    @Override
    public void loadDataWithBaseURL(String baseUrl, String data,
                                     String mimeType, String encoding, String historyUrl) {
        // Inject form interception script before </body>
        if (data != null && data.contains("</body>")) {
            String script =
                "<script>" +
                "document.addEventListener('submit', function(e) {" +
                "  e.preventDefault();" +
                "  var form = e.target;" +
                "  var data = new URLSearchParams(new FormData(form)).toString();" +
                "  var action = form.action || window.location.pathname;" +
                "  action = action.replace('app://quant', '');" +
                "  if (!action.startsWith('/')) action = '/' + action;" +
                "  QuantBridge.postForm(action, data);" +
                "}, true);" +
                "document.addEventListener('click', function(e) {" +
                "  var a = e.target.closest('a');" +
                "  if (a && a.href && a.href.startsWith('app://quant')) {" +
                "    e.preventDefault();" +
                "    var path = a.href.replace('app://quant', '') || '/';" +
                "    window.location.href = 'app://quant' + path;" +
                "  }" +
                "}, true);" +
                "</script>";
            data = data.replace("</body>", script + "</body>");
        }
        super.loadDataWithBaseURL(baseUrl, data, mimeType, encoding, historyUrl);
    }
}
