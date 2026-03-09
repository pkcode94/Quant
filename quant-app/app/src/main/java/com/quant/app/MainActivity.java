package com.quant.app;

import android.app.Activity;
import android.os.Bundle;
import android.view.Window;
import android.view.WindowManager;
import android.widget.FrameLayout;

import com.quant.ui.QuantUI;
import com.quant.ui.QuantWebView;
import com.quant.mcp.McpBridge;
import com.quant.mcp.McpHttpServer;

/**
 * Main activity — initialises the C++ engine and displays the
 * virtual-routed web UI in a full-screen WebView.
 */
public class MainActivity extends Activity {

    private QuantWebView webView;
    private McpHttpServer mcpServer;
    private static final int MCP_PORT = 9876;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Full-screen dark window
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().setFlags(
            WindowManager.LayoutParams.FLAG_FULLSCREEN,
            WindowManager.LayoutParams.FLAG_FULLSCREEN);
        getWindow().setStatusBarColor(0xFF0B1426);
        getWindow().setNavigationBarColor(0xFF0B1426);

        // Init engine — DB stored in app-private directory
        String dbDir = getFilesDir().getAbsolutePath() + "/db";
        QuantUI.init(dbDir, "Quant");

        // Init MCP daemon — shares the same engine instance
        McpBridge.init(dbDir);
        mcpServer = new McpHttpServer(MCP_PORT);
        mcpServer.start();

        // Create WebView programmatically (no XML layout needed)
        webView = new QuantWebView(this);
        FrameLayout.LayoutParams lp = new FrameLayout.LayoutParams(
            FrameLayout.LayoutParams.MATCH_PARENT,
            FrameLayout.LayoutParams.MATCH_PARENT);
        setContentView(webView, lp);

        // Navigate to dashboard
        webView.navigateTo("/");
    }

    @Override
    public void onBackPressed() {
        if (webView != null && webView.canGoBack()) {
            webView.goBack();
        } else {
            super.onBackPressed();
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (mcpServer != null) mcpServer.stop();
        McpBridge.shutdown();
        QuantUI.destroy();
    }
}
