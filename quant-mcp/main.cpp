// quant-mcp -- adaptive stdio relay for MCP transport
//
// The relay auto-detects transport on the configured host/port:
// 1) If HTTP /mcp is reachable, it forwards JSON-RPC via POST /mcp.
// 2) Otherwise, it falls back to raw TCP line relay.
//
// Usage: quant-mcp [--host HOST] [--port PORT] [--path PATH]

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>

#include "../Quant/cpp-httplib-master/httplib.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#include <io.h>
#include <fcntl.h>
using SocketHandle = SOCKET;
static constexpr SocketHandle kInvalidSock = INVALID_SOCKET;
inline int sockClose(SocketHandle s) { return closesocket(s); }
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
using SocketHandle = int;
static constexpr SocketHandle kInvalidSock = -1;
inline int sockClose(SocketHandle s) { return close(s); }
#endif

static bool isHttpMcpReachable(const std::string& host, int port, const std::string& path)
{
    httplib::Client cli(host, port);
    cli.set_connection_timeout(1, 0);
    cli.set_read_timeout(1, 0);
    cli.set_write_timeout(1, 0);
    auto resp = cli.Get(path.c_str());
    return static_cast<bool>(resp);
}

static int runHttpRelay(const std::string& host, int port, const std::string& path)
{
    httplib::Client cli(host, port);
    cli.set_read_timeout(30, 0);
    cli.set_write_timeout(30, 0);
    cli.set_connection_timeout(5, 0);

    char buf[8192];
    while (std::fgets(buf, sizeof(buf), stdin))
    {
        std::string line = buf;
        while (!line.empty() && line.back() != '\n' && !std::feof(stdin))
        {
            if (std::fgets(buf, sizeof(buf), stdin)) line += buf;
            else break;
        }

        auto resp = cli.Post(path.c_str(), line, "application/json");
        if (!resp)
        {
            std::fprintf(stderr,
                "[quant-mcp] ERROR: HTTP MCP endpoint unreachable at http://%s:%d%s\n",
                host.c_str(), port, path.c_str());
            return 1;
        }

        if (!resp->body.empty())
        {
            std::fputs(resp->body.c_str(), stdout);
            std::fputc('\n', stdout);
            std::fflush(stdout);
        }
    }
    return 0;
}

static int runTcpRelay(const std::string& host, int port)
{
    SocketHandle sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == kInvalidSock)
    {
        std::fprintf(stderr, "[quant-mcp] socket() failed\n");
        return 1;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<u_short>(port));
    if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) != 1)
    {
        std::fprintf(stderr, "[quant-mcp] invalid host address: %s\n", host.c_str());
        sockClose(sock);
        return 1;
    }

    if (connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0)
    {
        std::fprintf(stderr,
            "[quant-mcp] ERROR: cannot connect TCP MCP at %s:%d\n",
            host.c_str(), port);
        sockClose(sock);
        return 1;
    }

    std::thread reader([&]() {
        char buf[4096];
        while (true)
        {
            int n = recv(sock, buf, sizeof(buf) - 1, 0);
            if (n <= 0) break;
            buf[n] = '\0';
            std::fputs(buf, stdout);
            std::fflush(stdout);
        }
    });
    reader.detach();

    char buf[4096];
    while (std::fgets(buf, sizeof(buf), stdin))
    {
        std::string line = buf;
        while (!line.empty() && line.back() != '\n' && !std::feof(stdin))
            if (std::fgets(buf, sizeof(buf), stdin)) line += buf;

        int total = static_cast<int>(line.size());
        int sent = 0;
        while (sent < total)
        {
            int s = send(sock, line.data() + sent, total - sent, 0);
            if (s <= 0) break;
            sent += s;
        }
        if (sent < total) break;
    }

    sockClose(sock);
    return 0;
}

int main(int argc, char* argv[])
{
    std::string host = "127.0.0.1";
    int port = 9100;
    std::string path = "/mcp";

    for (int i = 1; i < argc; ++i)
    {
        if (std::strcmp(argv[i], "--host") == 0 && i + 1 < argc) host = argv[++i];
        else if (std::strcmp(argv[i], "--port") == 0 && i + 1 < argc) port = std::atoi(argv[++i]);
        else if (std::strcmp(argv[i], "--path") == 0 && i + 1 < argc) path = argv[++i];
        else if (std::strcmp(argv[i], "--db") == 0 && i + 1 < argc) ++i;
    }

#ifdef _WIN32
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
#endif
    std::setvbuf(stdout, nullptr, _IONBF, 0);

    int rc = 0;
    if (isHttpMcpReachable(host, port, path))
        rc = runHttpRelay(host, port, path);
    else
        rc = runTcpRelay(host, port);

#ifdef _WIN32
    WSACleanup();
#endif
    return rc;
}
