package com.quant.mcp;

/**
 * McpBridge -- JNI wrapper for the shared MCP request handler.
 *
 * The native side reuses the same QEngine instance as the UI,
 * so MCP tools operate on the live ledger database.
 */
public class McpBridge {

    /** Initialise presets and tool definitions. Call once after QuantUI.init(). */
    public static void init(String dbDir) {
        nativeInitMcp(dbDir);
    }

    /** Process a single JSON-RPC request.  Returns null for notifications. */
    public static String handle(String jsonRpcRequest) {
        return nativeHandle(jsonRpcRequest);
    }

    /** Shut down the MCP handler. */
    public static void shutdown() {
        nativeShutdownMcp();
    }

    // ---- Native methods (implemented in quant_mcp_jni.cpp) ----
    private static native void    nativeInitMcp(String dbDir);
    private static native String  nativeHandle(String jsonRpcRequest);
    private static native void    nativeShutdownMcp();
}
