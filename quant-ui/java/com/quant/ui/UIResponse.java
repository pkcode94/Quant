package com.quant.ui;

/**
 * UIResponse — result from the virtual router.
 *
 * Either html is non-null (render it) or redirect is non-null (navigate there).
 */
public class UIResponse {
    public final int    statusCode;
    public final String html;
    public final String redirect;

    public UIResponse(int statusCode, String html, String redirect) {
        this.statusCode = statusCode;
        this.html       = html;
        this.redirect   = redirect;
    }

    public boolean isRedirect() {
        return statusCode == 303 && redirect != null;
    }

    public boolean isOk() {
        return statusCode == 200 && html != null;
    }
}
