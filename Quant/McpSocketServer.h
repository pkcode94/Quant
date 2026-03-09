#pragma once

// ============================================================
// McpSocketServer.h — TCP socket server for MCP JSON-RPC
//
// Runs inside Quant.exe so that quant-mcp (stdio transport)
// relays requests over TCP instead of accessing DB files
// directly.  All MCP activity is logged to the Quant console.
//
// Self-contained: includes a compatibility shim (QEngine,
// qe_* functions) that wraps TradeDatabase directly, plus
// the MCP handler source via unity build.
// ============================================================

// ---- Platform sockets ----

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
using McpSockHandle = SOCKET;
static constexpr McpSockHandle kMcpInvalidSock = INVALID_SOCKET;
inline int  mcpSockClose(McpSockHandle s) { return closesocket(s); }
inline void mcpSockInit()  { WSADATA d; WSAStartup(MAKEWORD(2,2), &d); }
inline void mcpSockClean() { WSACleanup(); }
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
using McpSockHandle = int;
static constexpr McpSockHandle kMcpInvalidSock = -1;
inline int  mcpSockClose(McpSockHandle s) { return close(s); }
inline void mcpSockInit()  {}
inline void mcpSockClean() {}
#endif

#include <thread>
#include <mutex>
#include <string>
#include <iostream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <atomic>
#include <cstring>
#include <algorithm>

// ============================================================
// QEngine + qe_* API and qmcp_* API are already provided by
// Routes_Mcp.h (unity-includes quant_engine.cpp / quant_mcp.cpp).
// McpSocketServer only needs the forward declarations to call them.
// ============================================================

// ============================================================
// Console logging helpers
// ============================================================

static std::string mcpExtractMethod(const std::string& json) {
    auto pos = json.find("\"method\""); if (pos == std::string::npos) return {};
    pos = json.find(':', pos + 8); if (pos == std::string::npos) return {};
    auto q1 = json.find('"', pos + 1); if (q1 == std::string::npos) return {};
    auto q2 = json.find('"', q1 + 1); if (q2 == std::string::npos) return {};
    return json.substr(q1 + 1, q2 - q1 - 1);
}

static std::string mcpExtractToolName(const std::string& json) {
    auto pp = json.find("\"params\""); if (pp == std::string::npos) return {};
    auto np = json.find("\"name\"", pp); if (np == std::string::npos) return {};
    auto c = json.find(':', np + 6); if (c == std::string::npos) return {};
    auto q1 = json.find('"', c + 1); if (q1 == std::string::npos) return {};
    auto q2 = json.find('"', q1 + 1); if (q2 == std::string::npos) return {};
    return json.substr(q1 + 1, q2 - q1 - 1);
}

static std::string mcpTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    char buf[32]; std::strftime(buf, sizeof(buf), "%H:%M:%S", &tm);
    return buf;
}

// ============================================================
// McpSocketServer
// ============================================================

class McpSocketServer
{
    int             m_port;
    std::string     m_dbDir;
    std::atomic<bool> m_running{false};
    McpSockHandle   m_listenSock = kMcpInvalidSock;

public:
    explicit McpSocketServer(int port, const std::string& dbDir)
        : m_port(port), m_dbDir(dbDir) {}

    void run()
    {
        mcpSockInit();

        m_listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (m_listenSock == kMcpInvalidSock)
        { std::cerr << "  [MCP-TCP] ERROR: socket() failed\n"; return; }

        int yes = 1;
        setsockopt(m_listenSock, SOL_SOCKET, SO_REUSEADDR,
                   reinterpret_cast<const char*>(&yes), sizeof(yes));

        sockaddr_in addr{};
        addr.sin_family      = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        addr.sin_port        = htons(static_cast<u_short>(m_port));

        if (bind(m_listenSock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0)
        { std::cerr << "  [MCP-TCP] ERROR: bind() failed on port " << m_port << "\n";
          mcpSockClose(m_listenSock); return; }

        if (listen(m_listenSock, 4) != 0)
        { std::cerr << "  [MCP-TCP] ERROR: listen() failed\n";
          mcpSockClose(m_listenSock); return; }

        m_running = true;
        std::cout << "  [MCP-TCP] listening on localhost:" << m_port << "\n";

        while (m_running)
        {
            sockaddr_in client{}; int cLen = sizeof(client);
            McpSockHandle conn = accept(m_listenSock,
                reinterpret_cast<sockaddr*>(&client),
#ifdef _WIN32
                &cLen);
#else
                reinterpret_cast<socklen_t*>(&cLen));
#endif
            if (conn == kMcpInvalidSock) continue;
            std::cout << "  [MCP-TCP] client connected\n";
            std::thread([this, conn]() { handleClient(conn); }).detach();
        }

        mcpSockClose(m_listenSock);
        mcpSockClean();
    }

    void stop() { m_running = false; }

private:
    void handleClient(McpSockHandle sock)
    {
        QEngine* engine = qe_open(m_dbDir.c_str());
        qmcp_init(m_dbDir.c_str());

        std::string buffer;
        char chunk[4096];

        while (m_running)
        {
            int n = recv(sock, chunk, sizeof(chunk) - 1, 0);
            if (n <= 0) break;
            chunk[n] = '\0';
            buffer += chunk;

            std::string::size_type pos;
            while ((pos = buffer.find('\n')) != std::string::npos)
            {
                std::string line = buffer.substr(0, pos);
                buffer.erase(0, pos + 1);
                while (!line.empty() && line.back() == '\r') line.pop_back();
                if (line.empty()) continue;

                std::string method = mcpExtractMethod(line);
                std::string tool   = mcpExtractToolName(line);
                {
                    std::ostringstream log;
                    log << "  [MCP " << mcpTimestamp() << "] >> " << method;
                    if (!tool.empty()) log << " [" << tool << "]";
                    std::cout << log.str() << "\n";
                }

                char* response = qmcp_handle(engine, line.c_str());
                if (response)
                {
                    std::string resp(response);
                    qmcp_free(response);

                    std::string preview = resp.substr(0, 120);
                    if (resp.size() > 120) preview += "...";
                    std::cout << "  [MCP " << mcpTimestamp() << "] << " << preview << "\n";

                    resp += '\n';
                    int total = (int)resp.size(); int sent = 0;
                    while (sent < total) {
                        int s = send(sock, resp.data() + sent, total - sent, 0);
                        if (s <= 0) break; sent += s;
                    }
                }
                else
                {
                    std::cout << "  [MCP " << mcpTimestamp() << "] << (notification)\n";
                }
            }
        }

        if (engine) qe_close(engine);
        mcpSockClose(sock);
        std::cout << "  [MCP-TCP] client disconnected\n";
    }
};
