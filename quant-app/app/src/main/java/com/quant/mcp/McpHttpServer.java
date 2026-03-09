package com.quant.mcp;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.InetAddress;
import java.net.NetworkInterface;
import java.net.ServerSocket;
import java.net.Socket;
import java.nio.charset.StandardCharsets;
import java.util.Enumeration;

/**
 * McpHttpServer -- lightweight HTTP server that speaks MCP (JSON-RPC 2.0).
 *
 * Listens on a configurable port and dispatches incoming POST /mcp
 * requests through McpBridge.handle().  AI assistants (Claude, Copilot,
 * etc.) connect to http://&lt;device-ip&gt;:&lt;port&gt;/mcp using the
 * MCP Streamable HTTP transport.
 *
 * A GET on / returns a small status page so users can verify the
 * server is running from a browser.
 */
public class McpHttpServer {

    private static final String TAG = "McpHttpServer";

    private final int port;
    private volatile boolean running;
    private ServerSocket serverSocket;
    private Thread serverThread;

    public McpHttpServer(int port) {
        this.port = port;
    }

    /** Start the server on a background daemon thread. */
    public synchronized void start() {
        if (running) return;
        running = true;
        serverThread = new Thread(this::serve, "mcp-http");
        serverThread.setDaemon(true);
        serverThread.start();
    }

    /** Stop the server and close the listening socket. */
    public synchronized void stop() {
        running = false;
        try {
            if (serverSocket != null && !serverSocket.isClosed())
                serverSocket.close();
        } catch (IOException ignored) {}
    }

    public int getPort() { return port; }
    public boolean isRunning() { return running; }

    /** Return the device's non-loopback IPv4 address, or "127.0.0.1". */
    public static String getLocalIp() {
        try {
            Enumeration<NetworkInterface> ifaces = NetworkInterface.getNetworkInterfaces();
            while (ifaces.hasMoreElements()) {
                NetworkInterface iface = ifaces.nextElement();
                if (iface.isLoopback() || !iface.isUp()) continue;
                Enumeration<InetAddress> addrs = iface.getInetAddresses();
                while (addrs.hasMoreElements()) {
                    InetAddress addr = addrs.nextElement();
                    if (addr instanceof java.net.Inet4Address)
                        return addr.getHostAddress();
                }
            }
        } catch (Exception ignored) {}
        return "127.0.0.1";
    }

    // ---- Internal ----

    private void serve() {
        try {
            serverSocket = new ServerSocket(port);
            while (running) {
                Socket client = serverSocket.accept();
                new Thread(() -> handleClient(client), "mcp-client").start();
            }
        } catch (IOException e) {
            if (running) {
                android.util.Log.e(TAG, "Server error", e);
            }
        }
    }

    private void handleClient(Socket client) {
        try {
            client.setSoTimeout(30000);
            BufferedReader in = new BufferedReader(
                new InputStreamReader(client.getInputStream(), StandardCharsets.UTF_8));
            OutputStream out = client.getOutputStream();

            // ---- Read HTTP request line ----
            String requestLine = in.readLine();
            if (requestLine == null) { client.close(); return; }

            String method = "";
            String path = "/";
            int sp1 = requestLine.indexOf(' ');
            if (sp1 > 0) {
                method = requestLine.substring(0, sp1);
                int sp2 = requestLine.indexOf(' ', sp1 + 1);
                if (sp2 > sp1) path = requestLine.substring(sp1 + 1, sp2);
                else           path = requestLine.substring(sp1 + 1);
            }

            // ---- Read headers ----
            int contentLength = 0;
            String headerLine;
            while ((headerLine = in.readLine()) != null && !headerLine.isEmpty()) {
                if (headerLine.regionMatches(true, 0, "Content-Length:", 0, 15)) {
                    contentLength = Integer.parseInt(headerLine.substring(15).trim());
                }
            }

            // ---- Handle request ----
            if ("OPTIONS".equalsIgnoreCase(method)) {
                // CORS preflight
                String resp = "HTTP/1.1 204 No Content\r\n"
                    + corsHeaders()
                    + "Content-Length: 0\r\n\r\n";
                out.write(resp.getBytes(StandardCharsets.UTF_8));
                out.flush();
            } else if ("GET".equalsIgnoreCase(method)) {
                // Status page
                String body = statusHtml();
                String resp = "HTTP/1.1 200 OK\r\n"
                    + corsHeaders()
                    + "Content-Type: text/html; charset=utf-8\r\n"
                    + "Content-Length: " + body.getBytes(StandardCharsets.UTF_8).length + "\r\n"
                    + "\r\n" + body;
                out.write(resp.getBytes(StandardCharsets.UTF_8));
                out.flush();
            } else if ("POST".equalsIgnoreCase(method)
                       && (path.equals("/mcp") || path.equals("/"))) {
                // MCP JSON-RPC request
                char[] buf = new char[contentLength];
                int read = 0;
                while (read < contentLength) {
                    int n = in.read(buf, read, contentLength - read);
                    if (n < 0) break;
                    read += n;
                }
                String jsonReq = new String(buf, 0, read);
                String jsonResp = McpBridge.handle(jsonReq);

                if (jsonResp == null) {
                    // Notification -- no response body
                    String resp = "HTTP/1.1 202 Accepted\r\n"
                        + corsHeaders()
                        + "Content-Length: 0\r\n\r\n";
                    out.write(resp.getBytes(StandardCharsets.UTF_8));
                } else {
                    byte[] body = jsonResp.getBytes(StandardCharsets.UTF_8);
                    String resp = "HTTP/1.1 200 OK\r\n"
                        + corsHeaders()
                        + "Content-Type: application/json\r\n"
                        + "Content-Length: " + body.length + "\r\n"
                        + "\r\n";
                    out.write(resp.getBytes(StandardCharsets.UTF_8));
                    out.write(body);
                }
                out.flush();
            } else {
                String body = "{\"error\":\"Not Found\"}";
                String resp = "HTTP/1.1 404 Not Found\r\n"
                    + corsHeaders()
                    + "Content-Type: application/json\r\n"
                    + "Content-Length: " + body.length() + "\r\n"
                    + "\r\n" + body;
                out.write(resp.getBytes(StandardCharsets.UTF_8));
                out.flush();
            }

            client.close();
        } catch (Exception e) {
            try { client.close(); } catch (IOException ignored) {}
        }
    }

    private static String corsHeaders() {
        return "Access-Control-Allow-Origin: *\r\n"
             + "Access-Control-Allow-Methods: POST, GET, OPTIONS\r\n"
             + "Access-Control-Allow-Headers: Content-Type\r\n";
    }

    private String statusHtml() {
        String ip = getLocalIp();
        return "<!DOCTYPE html><html><head><meta charset='utf-8'>"
             + "<meta name='viewport' content='width=device-width,initial-scale=1'>"
             + "<title>Quant MCP</title>"
             + "<style>body{font-family:monospace;background:#000;color:#c0c0c0;padding:20px;}"
             + "h1{color:#00e676;font-size:1.2em;}code{color:#ffd600;}"
             + ".ok{color:#00e676;font-weight:bold;}</style></head><body>"
             + "<h1>&#9939; Quant MCP Server</h1>"
             + "<p>Status: <span class='ok'>Running</span></p>"
             + "<p>Endpoint: <code>http://" + ip + ":" + port + "/mcp</code></p>"
             + "<p>Transport: Streamable HTTP (POST JSON-RPC 2.0)</p>"
             + "<hr style='border-color:#1a1a1a;'>"
             + "<p style='font-size:0.85em;color:#757575;'>"
             + "Point your AI assistant at the endpoint above. "
             + "The server exposes all Quant Ledger tools: portfolio, trades, "
             + "serial plans, what-if, chains, and more.</p>"
             + "</body></html>";
    }
}
