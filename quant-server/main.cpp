// ============================================================
// quant-server — Sync server for Quant encrypted database vaults
//
// Zero-knowledge design: all encryption is client-side.
// Server stores opaque blobs, manages auth, quotas, tickets.
//
// Build:   make
// Run:     ./quant-server --port 9443 --data ./server-data
// ============================================================

#include "../Quant/cpp-httplib-master/httplib.h"
#include "../Quant/UserManager.h"
#include "SyncStore.h"
#include "TicketSystem.h"

#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <mutex>
#include <deque>
#include <ctime>

// ---- Server log ----
static std::mutex g_logMutex;
static std::deque<std::string> g_log;
static constexpr int MAX_LOG = 500;

static void slog(const std::string& msg)
{
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream line;
    line << std::put_time(&tm, "%H:%M:%S") << "  " << msg;
    {
        std::lock_guard<std::mutex> lk(g_logMutex);
        g_log.push_back(line.str());
        if (g_log.size() > MAX_LOG) g_log.pop_front();
    }
    std::cout << line.str() << "\n";
}

// ---- Helpers ----

static std::string escHtml(const std::string& s)
{
    std::string o;
    for (char c : s)
    {
        if (c == '<') o += "&lt;";
        else if (c == '>') o += "&gt;";
        else if (c == '&') o += "&amp;";
        else if (c == '"') o += "&quot;";
        else o += c;
    }
    return o;
}

static std::string jsonEsc(const std::string& s)
{
    std::string o;
    for (char c : s)
    {
        if (c == '"')       o += "\\\"";
        else if (c == '\\') o += "\\\\";
        else if (c == '\n') o += "\\n";
        else                o += c;
    }
    return o;
}

static std::string getToken(const httplib::Request& req)
{
    auto it = req.headers.find("Authorization");
    if (it != req.headers.end()) return it->second;
    // Also check cookie
    auto ck = req.headers.find("Cookie");
    if (ck != req.headers.end())
    {
        auto pos = ck->second.find("quant_session=");
        if (pos != std::string::npos)
        {
            auto start = pos + 14;
            auto end = ck->second.find(';', start);
            return ck->second.substr(start, end == std::string::npos ? end : end - start);
        }
    }
    return "";
}

static std::map<std::string, std::string> parseForm(const std::string& body)
{
    std::map<std::string, std::string> m;
    std::istringstream ss(body);
    std::string pair;
    while (std::getline(ss, pair, '&'))
    {
        auto eq = pair.find('=');
        if (eq != std::string::npos)
        {
            auto decode = [](const std::string& s) {
                std::string o;
                for (size_t i = 0; i < s.size(); ++i)
                {
                    if (s[i] == '+') o += ' ';
                    else if (s[i] == '%' && i + 2 < s.size())
                    {
                        auto hv = [](char c) -> int {
                            if (c >= '0' && c <= '9') return c - '0';
                            if (c >= 'a' && c <= 'f') return c - 'a' + 10;
                            if (c >= 'A' && c <= 'F') return c - 'A' + 10;
                            return 0;
                        };
                        o += (char)(hv(s[i + 1]) * 16 + hv(s[i + 2]));
                        i += 2;
                    }
                    else o += s[i];
                }
                return o;
            };
            m[decode(pair.substr(0, eq))] = decode(pair.substr(eq + 1));
        }
    }
    return m;
}

static std::string fv(const std::map<std::string, std::string>& f, const std::string& k)
{
    auto it = f.find(k);
    return (it != f.end()) ? it->second : "";
}

static double fd(const std::map<std::string, std::string>& f, const std::string& k, double def = 0.0)
{
    auto it = f.find(k);
    if (it == f.end() || it->second.empty()) return def;
    try { return std::stod(it->second); } catch (...) { return def; }
}

static int fi(const std::map<std::string, std::string>& f, const std::string& k, int def = 0)
{
    auto it = f.find(k);
    if (it == f.end() || it->second.empty()) return def;
    try { return std::stoi(it->second); } catch (...) { return def; }
}

static std::string fmtBytes(size_t bytes)
{
    std::ostringstream ss;
    if (bytes >= 1024ULL * 1024 * 1024)
        ss << std::fixed << std::setprecision(2) << (bytes / (1024.0 * 1024 * 1024)) << " GB";
    else if (bytes >= 1024 * 1024)
        ss << std::fixed << std::setprecision(2) << (bytes / (1024.0 * 1024)) << " MB";
    else if (bytes >= 1024)
        ss << std::fixed << std::setprecision(1) << (bytes / 1024.0) << " KB";
    else
        ss << bytes << " B";
    return ss.str();
}

// ---- Admin HTML ----

static std::string adminCss()
{
    return
        "<style>"
        "*{box-sizing:border-box;}"
        "body{font-family:'Courier New',monospace;background:#000;color:#c0c0c0;margin:0;padding:0;}"
        "nav{background:#0a0a0a;padding:8px 12px;border-bottom:1px solid #00e676;display:flex;gap:4px;flex-wrap:wrap;}"
        "nav a{color:#00e676;text-decoration:none;font-size:0.8em;padding:6px 10px;border-radius:3px;}"
        "nav a:hover{background:#0f0f0f;}"
        ".container{max-width:1100px;margin:0 auto;padding:16px;}"
        "h1{color:#00e676;font-size:1.1em;letter-spacing:1px;text-transform:uppercase;margin:0 0 10px 0;}"
        "h2{color:#757575;font-size:0.9em;border-bottom:1px solid #1a1a1a;padding-bottom:4px;}"
        "table{border-collapse:collapse;width:100%;margin:8px 0 16px 0;font-size:0.8em;}"
        "th,td{border:1px solid #1a1a1a;padding:4px 8px;text-align:left;}"
        "th{background:#0a0a0a;color:#ffd600;font-weight:normal;text-transform:uppercase;font-size:0.75em;letter-spacing:0.5px;}"
        "td{background:#000;}tr:hover td{background:#0a0a0a;}"
        "form.card{background:#0a0a0a;border:1px solid #1a1a1a;border-radius:4px;padding:10px;margin:6px 0;}"
        "label{display:inline-block;min-width:100px;color:#757575;font-size:0.78em;}"
        "input,select,textarea{background:#050505;border:1px solid #1a1a1a;color:#c0c0c0;padding:4px 6px;"
        "border-radius:3px;margin:2px 4px 2px 0;font-family:inherit;font-size:0.82em;}"
        "input:focus,textarea:focus{border-color:#00e676;outline:none;}"
        "textarea{width:100%;min-height:60px;resize:vertical;}"
        "button,.btn{background:#004d25;color:#00e676;border:1px solid #00e676;padding:4px 10px;"
        "border-radius:3px;cursor:pointer;font-size:0.78em;font-family:inherit;text-decoration:none;display:inline-block;}"
        "button:hover,.btn:hover{background:#006b35;}"
        ".btn-danger{background:#4a0000;color:#ff1744;border-color:#ff1744;}"
        ".btn-danger:hover{background:#6b0000;}"
        ".stat{display:inline-block;background:#0a0a0a;border:1px solid #1a1a1a;border-radius:4px;padding:8px 14px;margin:4px;}"
        ".stat .lbl{color:#757575;font-size:0.72em;text-transform:uppercase;}"
        ".stat .val{font-size:1.1em;color:#00e676;font-weight:bold;}"
        ".msg{background:#001a0a;border:1px solid #00e676;padding:5px 8px;border-radius:3px;margin:6px 0;color:#00e676;font-size:0.82em;}"
        ".console{background:#050505;border:1px solid #1a1a1a;border-radius:4px;padding:8px;margin:8px 0;"
        "max-height:400px;overflow-y:auto;font-size:0.75em;line-height:1.6;}"
        ".console .line{color:#757575;}.console .line span{color:#00e676;}"
        ".open{color:#00e676;}.pending{color:#ffd600;}.closed{color:#757575;}"
        "</style>";
}

static std::string adminNav()
{
    return
        "<nav>"
        "<a href='/admin'>Dashboard</a>"
        "<a href='/admin/users'>Users</a>"
        "<a href='/admin/tickets'>Tickets</a>"
        "<a href='/admin/console'>Console</a>"
        "</nav>";
}

static std::string adminWrap(const std::string& title, const std::string& body)
{
    return "<!DOCTYPE html><html><head><meta charset='utf-8'>"
           "<meta name='viewport' content='width=device-width,initial-scale=1'>"
           "<title>" + escHtml(title) + " - Quant Server</title>"
           + adminCss() +
           "</head><body>" + adminNav() +
           "<div class='container'>" + body + "</div>"
           "</body></html>";
}

// ---- Public HTML (user-facing, same terminal theme) ----

static std::string publicNav(const std::string& username = "")
{
    std::string h = "<nav>"
        "<a href='/' style='color:#ffd600;font-weight:bold;'>&#9939; QUANT</a>";
    if (username.empty())
    {
        h += "<a href='/login'>Login</a>"
             "<a href='/register'>Register</a>";
    }
    else
    {
        h += "<a href='/dashboard'>" + escHtml(username) + "</a>"
             "<a href='/dashboard/tickets'>Tickets</a>"
             "<form method='POST' action='/logout' style='margin:0;display:inline;'>"
             "<button style='background:none;border:none;color:#ff1744;cursor:pointer;"
             "font-family:inherit;font-size:0.8em;padding:5px 8px;'>Logout</button></form>";
    }
    return h + "</nav>";
}

static std::string publicWrap(const std::string& title, const std::string& body,
                              const std::string& username = "")
{
    return "<!DOCTYPE html><html><head><meta charset='utf-8'>"
           "<meta name='viewport' content='width=device-width,initial-scale=1'>"
           "<title>" + escHtml(title) + " - Quant Sync</title>"
           + adminCss() +
           "</head><body>" + publicNav(username) +
           "<div class='container'>" + body + "</div>"
           "</body></html>";
}

// ============================================================
// Main
// ============================================================

int main(int argc, char* argv[])
{
    int port = 9443;
    std::string dataDir = "server-data";

    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg == "--port" && i + 1 < argc) port = std::stoi(argv[++i]);
        else if (arg == "--data" && i + 1 < argc) dataDir = argv[++i];
    }

    std::filesystem::create_directories(dataDir);

    UserManager   users(dataDir);
    SyncStore     store(dataDir);
    TicketSystem  tickets(dataDir);
    std::mutex    globalMtx;

    httplib::Server svr;

    slog("Quant Sync Server starting on port " + std::to_string(port));
    slog("Data directory: " + dataDir);

    // ================================================================
    // API: Auth
    // ================================================================

    svr.Post("/api/register", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(globalMtx);
        auto f = parseForm(req.body);
        auto err = users.registerUser(fv(f, "username"), fv(f, "password"), fv(f, "email"));
        if (!err.empty())
        {
            res.set_content("{\"error\": \"" + jsonEsc(err) + "\"}", "application/json");
            res.status = 400;
            return;
        }
        store.ensureQuota(fv(f, "username"), false);
        slog("REGISTER: " + fv(f, "username"));
        res.set_content("{\"ok\": true}", "application/json");
    });

    svr.Post("/api/login", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(globalMtx);
        auto f = parseForm(req.body);
        auto user = fv(f, "username");
        auto pass = fv(f, "password");
        if (!users.authenticate(user, pass))
        {
            slog("LOGIN FAILED: " + user);
            res.set_content("{\"error\": \"Invalid credentials\"}", "application/json");
            res.status = 401;
            return;
        }
        auto token = users.createSession(user);
        slog("LOGIN: " + user);
        res.set_content("{\"token\": \"" + token + "\", \"username\": \"" + jsonEsc(user) + "\"}", "application/json");
    });

    svr.Post("/api/logout", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(globalMtx);
        auto token = getToken(req);
        if (!token.empty()) users.destroySession(token);
        res.set_content("{\"ok\": true}", "application/json");
    });

    // ================================================================
    // Public Web Pages (register, login, user dashboard)
    // ================================================================

    // ---- Landing ----
    svr.Get("/", [&](const httplib::Request& req, httplib::Response& res) {
        auto user = users.getSessionUser(getToken(req));
        if (!user.empty()) { res.set_redirect("/dashboard", 303); return; }
        std::string h =
            "<div style='text-align:center;padding:40px 0;'>"
            "<h1 style='font-size:1.6em;letter-spacing:3px;'>&#9939; QUANT SYNC</h1>"
            "<p style='color:#757575;font-size:0.85em;margin:16px 0;'>Zero-knowledge encrypted ledger sync</p>"
            "<div style='margin:20px 0;'>"
            "<a class='btn' href='/login' style='padding:8px 20px;font-size:0.9em;margin:4px;'>Login</a> "
            "<a class='btn' href='/register' style='padding:8px 20px;font-size:0.9em;margin:4px;"
            "background:#0a0a0a;color:#ffd600;border-color:#ffd600;'>Register</a>"
            "</div>"
            "<div style='max-width:400px;margin:30px auto;text-align:left;'>"
            "<div class='stat' style='display:block;margin:6px 0;'>"
            "<div class='lbl'>&#9679; Client-Side Encryption</div>"
            "<div style='color:#757575;font-size:0.75em;'>AES-256-GCM — your data is encrypted before it leaves your device</div></div>"
            "<div class='stat' style='display:block;margin:6px 0;'>"
            "<div class='lbl'>&#9679; Zero Knowledge</div>"
            "<div style='color:#757575;font-size:0.75em;'>Server stores opaque blobs — we cannot read your ledger</div></div>"
            "<div class='stat' style='display:block;margin:6px 0;'>"
            "<div class='lbl'>&#9679; Bitcoin Payments</div>"
            "<div style='color:#757575;font-size:0.75em;'>Upgrade storage via BTC — no third-party payment processors</div></div>"
            "</div>"
            "</div>";
        res.set_content(publicWrap("Home", h), "text/html");
    });

    // ---- Register (GET) ----
    svr.Get("/register", [&](const httplib::Request& req, httplib::Response& res) {
        auto user = users.getSessionUser(getToken(req));
        if (!user.empty()) { res.set_redirect("/dashboard", 303); return; }
        std::string err;
        auto it = req.params.find("err");
        if (it != req.params.end()) err = it->second;
        std::string msg;
        auto mt = req.params.find("msg");
        if (mt != req.params.end()) msg = mt->second;

        std::ostringstream h;
        if (!err.empty()) h << "<div class='msg' style='background:#1a0000;border-color:#ff1744;color:#ff1744;'>" << escHtml(err) << "</div>";
        if (!msg.empty()) h << "<div class='msg'>" << escHtml(msg) << "</div>";
        h << "<div style='max-width:360px;margin:30px auto;'>"
             "<h1>Create Account</h1>"
             "<form class='card' method='POST' action='/register'>"
             "<label>Username</label><br>"
             "<input type='text' name='username' required autocomplete='username' "
             "style='width:100%;margin-bottom:6px;' placeholder='letters, digits, underscores'><br>"
             "<label>Email</label><br>"
             "<input type='text' name='email' style='width:100%;margin-bottom:6px;' placeholder='optional'><br>"
             "<label>Password</label><br>"
             "<input type='password' name='password' required autocomplete='new-password' "
             "style='width:100%;margin-bottom:6px;background:#050505;border:1px solid #1a1a1a;"
             "color:#c0c0c0;padding:4px 6px;border-radius:3px;font-family:inherit;font-size:0.82em;' "
             "placeholder='min 4 characters'><br>"
             "<label>Confirm Password</label><br>"
             "<input type='password' name='password2' required autocomplete='new-password' "
             "style='width:100%;margin-bottom:10px;background:#050505;border:1px solid #1a1a1a;"
             "color:#c0c0c0;padding:4px 6px;border-radius:3px;font-family:inherit;font-size:0.82em;' "
             "placeholder='repeat password'><br>"
             "<button style='width:100%;padding:8px;'>Create Account</button></form>"
             "<p style='color:#757575;font-size:0.78em;text-align:center;margin-top:12px;'>"
             "Already have an account? <a href='/login' style='color:#00e676;'>Login</a></p>"
             "<div style='color:#757575;font-size:0.7em;margin-top:16px;border-top:1px solid #1a1a1a;padding-top:8px;'>"
             "&#9679; Free tier: 5 MB encrypted vault storage<br>"
             "&#9679; Upgrade anytime via Bitcoin</div>"
             "</div>";
        res.set_content(publicWrap("Register", h.str()), "text/html");
    });

    // ---- Register (POST) ----
    svr.Post("/register", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(globalMtx);
        auto f = parseForm(req.body);
        auto user = fv(f, "username");
        auto pass = fv(f, "password");
        auto pass2 = fv(f, "password2");
        auto email = fv(f, "email");

        if (pass != pass2)
        {
            res.set_redirect("/register?err=Passwords+do+not+match", 303);
            return;
        }
        auto err = users.registerUser(user, pass, email);
        if (!err.empty())
        {
            res.set_redirect("/register?err=" + escHtml(err), 303);
            return;
        }
        store.ensureQuota(user, false);
        slog("REGISTER (web): " + user);
        res.set_redirect("/login?msg=Account+created.+You+can+now+login.", 303);
    });

    // ---- Login (GET) ----
    svr.Get("/login", [&](const httplib::Request& req, httplib::Response& res) {
        auto user = users.getSessionUser(getToken(req));
        if (!user.empty()) { res.set_redirect("/dashboard", 303); return; }
        std::string err;
        auto it = req.params.find("err");
        if (it != req.params.end()) err = it->second;
        std::string msg;
        auto mt = req.params.find("msg");
        if (mt != req.params.end()) msg = mt->second;

        std::ostringstream h;
        if (!err.empty()) h << "<div class='msg' style='background:#1a0000;border-color:#ff1744;color:#ff1744;'>" << escHtml(err) << "</div>";
        if (!msg.empty()) h << "<div class='msg'>" << escHtml(msg) << "</div>";
        h << "<div style='max-width:360px;margin:30px auto;'>"
             "<h1>Login</h1>"
             "<form class='card' method='POST' action='/login'>"
             "<label>Username</label><br>"
             "<input type='text' name='username' required autocomplete='username' "
             "style='width:100%;margin-bottom:6px;'><br>"
             "<label>Password</label><br>"
             "<input type='password' name='password' required autocomplete='current-password' "
             "style='width:100%;margin-bottom:10px;background:#050505;border:1px solid #1a1a1a;"
             "color:#c0c0c0;padding:4px 6px;border-radius:3px;font-family:inherit;font-size:0.82em;'><br>"
             "<button style='width:100%;padding:8px;'>Login</button></form>"
             "<p style='color:#757575;font-size:0.78em;text-align:center;margin-top:12px;'>"
             "No account? <a href='/register' style='color:#ffd600;'>Register</a></p>"
             "</div>";
        res.set_content(publicWrap("Login", h.str()), "text/html");
    });

    // ---- Login (POST) ----
    svr.Post("/login", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(globalMtx);
        auto f = parseForm(req.body);
        auto user = fv(f, "username");
        auto pass = fv(f, "password");
        if (!users.authenticate(user, pass))
        {
            slog("LOGIN FAILED (web): " + user);
            res.set_redirect("/login?err=Invalid+username+or+password", 303);
            return;
        }
        auto token = users.createSession(user);
        slog("LOGIN (web): " + user);
        res.set_header("Set-Cookie", "quant_session=" + token + "; Path=/; HttpOnly; SameSite=Strict");
        if (users.isAdmin(user))
            res.set_redirect("/admin", 303);
        else
            res.set_redirect("/dashboard", 303);
    });

    // ---- Logout (POST) ----
    svr.Post("/logout", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(globalMtx);
        auto token = getToken(req);
        if (!token.empty()) users.destroySession(token);
        res.set_header("Set-Cookie", "quant_session=; Path=/; Max-Age=0");
        res.set_redirect("/login?msg=Logged+out", 303);
    });

    // ---- User Dashboard (GET) ----
    svr.Get("/dashboard", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(globalMtx);
        auto user = users.getSessionUser(getToken(req));
        if (user.empty()) { res.set_redirect("/login", 303); return; }
        auto quota = store.getQuota(user);
        auto meta  = store.getMeta(user);
        bool prem  = users.isPremium(user);

        std::ostringstream h;
        h << "<h1>&#9939; " << escHtml(user) << "</h1>"
             "<div style='margin-bottom:12px;'>"
             "<div class='stat'><div class='lbl'>Status</div><div class='val' style='font-size:0.9em;'>"
             << (prem ? "<span style='color:#ffd600;'>PREMIUM</span>" : "FREE") << "</div></div>"
             "<div class='stat'><div class='lbl'>Storage</div><div class='val' style='font-size:0.9em;'>"
             << fmtBytes(quota.usedBytes) << " / " << fmtBytes(quota.limitBytes) << "</div></div>"
             "<div class='stat'><div class='lbl'>Last Sync</div><div class='val' style='font-size:0.9em;'>"
             << (meta.lastSync.empty() ? "Never" : escHtml(meta.lastSync)) << "</div></div>"
             "</div>";

        // Storage bar
        double pct = (quota.limitBytes > 0) ? (100.0 * quota.usedBytes / quota.limitBytes) : 0;
        std::string barColor = (pct > 90) ? "#ff1744" : (pct > 70 ? "#ffd600" : "#00e676");
        h << "<div style='background:#1a1a1a;border-radius:3px;height:8px;margin:8px 0 16px 0;'>"
             "<div style='background:" << barColor << ";height:100%;border-radius:3px;width:"
             << std::min(pct, 100.0) << "%;'></div></div>";

        // Connection info for app
        h << "<form class='card'>"
             "<h2 style='margin-top:0;'>App Connection</h2>"
             "<div style='color:#757575;font-size:0.75em;margin-bottom:6px;'>"
             "Use these settings in the Quant app's Sync page:</div>"
             "<label>Server URL</label><br>"
             "<input type='text' readonly style='width:100%;color:#00e676;cursor:text;' "
             "value='" << "' onclick='this.select();'><br>"
             "<div style='color:#757575;font-size:0.72em;margin-top:4px;'>"
             "Enter your username and password in the app to connect.</div>"
             "</form>";

        // Request storage upgrade
        h << "<form class='card' method='POST' action='/dashboard/ticket'>"
             "<h2 style='margin-top:0;'>Request Storage Upgrade</h2>"
             "<div style='color:#757575;font-size:0.75em;margin-bottom:6px;'>"
             "Create a ticket to request more storage. Payment in Bitcoin.</div>"
             "<label>Subject</label><br>"
             "<input type='text' name='subject' style='width:100%;margin-bottom:6px;' "
             "placeholder='e.g. Request 100MB storage' required><br>"
             "<label>Message</label><br>"
             "<input type='text' name='message' style='width:100%;margin-bottom:8px;' "
             "placeholder='Additional details...'><br>"
             "<button>Create Ticket</button></form>";

        res.set_content(publicWrap("Dashboard", h.str(), user), "text/html");
    });

    // ---- User Tickets (GET) ----
    svr.Get("/dashboard/tickets", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(globalMtx);
        auto user = users.getSessionUser(getToken(req));
        if (user.empty()) { res.set_redirect("/login", 303); return; }
        auto list = tickets.listForUser(user);

        std::ostringstream h;
        h << "<h1>My Tickets</h1>";
        if (list.empty())
        {
            h << "<p style='color:#757575;'>No tickets yet. Create one from your "
                 "<a href='/dashboard' style='color:#00e676;'>dashboard</a>.</p>";
        }
        else
        {
            h << "<table><tr><th>#</th><th>Subject</th><th>Status</th><th>Messages</th>"
                 "<th>BTC Required</th></tr>";
            for (const auto& t : list)
            {
                std::string stCls = (t.status == "open") ? "open"
                    : (t.status == "pending_payment" ? "pending" : "closed");
                h << "<tr>"
                  << "<td><a href='/dashboard/ticket/" << t.ticketId
                  << "' style='color:#00e676;'>" << t.ticketId << "</a></td>"
                  << "<td>" << escHtml(t.subject) << "</td>"
                  << "<td class='" << stCls << "'>" << escHtml(t.status) << "</td>"
                  << "<td>" << t.messages.size() << "</td>";
                if (t.requiredBtc > 0)
                    h << "<td>" << std::fixed << std::setprecision(8) << t.requiredBtc << "</td>";
                else
                    h << "<td>-</td>";
                h << "</tr>";
            }
            h << "</table>";
        }
        h << "<a class='btn' href='/dashboard'>Back</a>";
        res.set_content(publicWrap("Tickets", h.str(), user), "text/html");
    });

    // ---- User Ticket Detail (GET) ----
    svr.Get(R"(/dashboard/ticket/(\d+))", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(globalMtx);
        auto user = users.getSessionUser(getToken(req));
        if (user.empty()) { res.set_redirect("/login", 303); return; }
        int id = std::stoi(req.matches[1]);
        const auto* t = tickets.findTicket(id);
        std::ostringstream h;
        if (!t || t->username != user)
        {
            h << "<h1>Ticket not found</h1>";
        }
        else
        {
            std::string stCls = (t->status == "open") ? "open"
                : (t->status == "pending_payment" ? "pending" : "closed");
            h << "<h1>Ticket #" << t->ticketId << "</h1>"
              << "<p>" << escHtml(t->subject) << " &mdash; "
              << "<span class='" << stCls << "'>" << escHtml(t->status) << "</span></p>";

            // Payment info
            if (t->status == "pending_payment" && t->requiredBtc > 0)
            {
                h << "<div class='stat' style='display:block;margin:8px 0;border-color:#ffd600;'>"
                     "<div class='lbl' style='color:#ffd600;'>Payment Required</div>"
                     "<div style='font-size:0.85em;'>"
                     "<strong style='color:#ffd600;'>" << std::fixed << std::setprecision(8)
                     << t->requiredBtc << " BTC</strong><br>"
                     "<span style='color:#757575;font-size:0.85em;'>Send to: </span>"
                     "<span style='color:#00e676;word-break:break-all;'>" << escHtml(t->btcAddress)
                     << "</span></div></div>";
            }

            // Messages
            for (const auto& m : t->messages)
            {
                bool isAdmin = (m.from == "ADMIN");
                h << "<div style='background:" << (isAdmin ? "#001a0a" : "#0a0a0a")
                  << ";border:1px solid " << (isAdmin ? "#00e676" : "#1a1a1a")
                  << ";border-radius:4px;padding:6px 10px;margin:4px 0;'>"
                  << "<strong style='color:" << (isAdmin ? "#00e676" : "#ffd600") << ";'>"
                  << escHtml(m.from) << "</strong> "
                  << "<span style='color:#757575;font-size:0.72em;'>" << escHtml(m.timestamp) << "</span><br>"
                  << "<span style='font-size:0.85em;'>" << escHtml(m.text) << "</span></div>";
            }

            // Reply form (if not closed)
            if (t->status != "closed")
            {
                h << "<form class='card' method='POST' action='/dashboard/ticket/"
                  << t->ticketId << "/reply'>"
                     "<label>Reply</label><br>"
                     "<input type='text' name='message' style='width:100%;margin-bottom:6px;' "
                     "placeholder='Your message...' required><br>"
                     "<button>Send Reply</button></form>";
            }
        }
        h << "<br><a class='btn' href='/dashboard/tickets'>Back to Tickets</a>";
        res.set_content(publicWrap("Ticket #" + std::to_string(id), h.str(), user), "text/html");
    });

    // ---- User Ticket Reply (POST) ----
    svr.Post(R"(/dashboard/ticket/(\d+)/reply)", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(globalMtx);
        auto user = users.getSessionUser(getToken(req));
        if (user.empty()) { res.set_redirect("/login", 303); return; }
        int id = std::stoi(req.matches[1]);
        auto f = parseForm(req.body);
        tickets.addReply(id, user, fv(f, "message"));
        res.set_redirect("/dashboard/ticket/" + std::to_string(id), 303);
    });

    // ---- User Create Ticket (POST) ----
    svr.Post("/dashboard/ticket", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(globalMtx);
        auto user = users.getSessionUser(getToken(req));
        if (user.empty()) { res.set_redirect("/login", 303); return; }
        auto f = parseForm(req.body);
        int id = tickets.createTicket(user, fv(f, "subject"), fv(f, "message"));
        slog("TICKET (web): #" + std::to_string(id) + " by " + user);
        res.set_redirect("/dashboard/tickets", 303);
    });

    // ================================================================
    // API: Sync
    // ================================================================

    svr.Get("/api/sync/status", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(globalMtx);
        auto user = users.getSessionUser(getToken(req));
        if (user.empty()) { res.status = 401; res.set_content("{\"error\":\"Unauthorized\"}", "application/json"); return; }
        auto meta  = store.getMeta(user);
        auto quota = store.getQuota(user);
        std::ostringstream j;
        j << "{\"lastSync\": \"" << jsonEsc(meta.lastSync) << "\","
          << " \"usedBytes\": " << quota.usedBytes << ","
          << " \"quotaBytes\": " << quota.limitBytes << ","
          << " \"checksum\": \"" << jsonEsc(meta.checksum) << "\"}";
        res.set_content(j.str(), "application/json");
    });

    svr.Post("/api/sync/upload", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(globalMtx);
        auto user = users.getSessionUser(getToken(req));
        if (user.empty()) { res.status = 401; res.set_content("{\"error\":\"Unauthorized\"}", "application/json"); return; }
        if (req.body.empty()) { res.status = 400; res.set_content("{\"error\":\"Empty body\"}", "application/json"); return; }

        auto err = store.upload(user, req.body.data(), req.body.size());
        if (!err.empty())
        {
            slog("UPLOAD REJECTED: " + user + " — " + err);
            res.status = 413;
            res.set_content("{\"error\": \"" + jsonEsc(err) + "\"}", "application/json");
            return;
        }
        auto quota = store.getQuota(user);
        slog("UPLOAD: " + user + " " + fmtBytes(req.body.size()));
        res.set_content("{\"ok\": true, \"usedBytes\": " + std::to_string(quota.usedBytes) + "}", "application/json");
    });

    svr.Get("/api/sync/download", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(globalMtx);
        auto user = users.getSessionUser(getToken(req));
        if (user.empty()) { res.status = 401; res.set_content("{\"error\":\"Unauthorized\"}", "application/json"); return; }
        auto blob = store.download(user);
        if (blob.empty()) { res.status = 404; res.set_content("{\"error\":\"No vault found\"}", "application/json"); return; }
        slog("DOWNLOAD: " + user + " " + fmtBytes(blob.size()));
        res.set_content(blob, "application/octet-stream");
    });

    // ================================================================
    // API: Quota
    // ================================================================

    svr.Get("/api/quota", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(globalMtx);
        auto user = users.getSessionUser(getToken(req));
        if (user.empty()) { res.status = 401; res.set_content("{\"error\":\"Unauthorized\"}", "application/json"); return; }
        auto q = store.getQuota(user);
        std::ostringstream j;
        j << "{\"used\": " << q.usedBytes << ", \"limit\": " << q.limitBytes
          << ", \"usedFmt\": \"" << fmtBytes(q.usedBytes) << "\""
          << ", \"limitFmt\": \"" << fmtBytes(q.limitBytes) << "\"}";
        res.set_content(j.str(), "application/json");
    });

    // ================================================================
    // API: Tickets
    // ================================================================

    svr.Post("/api/ticket/create", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(globalMtx);
        auto user = users.getSessionUser(getToken(req));
        if (user.empty()) { res.status = 401; res.set_content("{\"error\":\"Unauthorized\"}", "application/json"); return; }
        auto f = parseForm(req.body);
        int id = tickets.createTicket(user, fv(f, "subject"), fv(f, "message"));
        slog("TICKET CREATED: #" + std::to_string(id) + " by " + user);
        res.set_content("{\"ticketId\": " + std::to_string(id) + "}", "application/json");
    });

    svr.Get("/api/ticket/list", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(globalMtx);
        auto user = users.getSessionUser(getToken(req));
        if (user.empty()) { res.status = 401; res.set_content("{\"error\":\"Unauthorized\"}", "application/json"); return; }
        auto list = tickets.listForUser(user);
        std::ostringstream j;
        j << "[";
        for (size_t i = 0; i < list.size(); ++i)
        {
            const auto& t = list[i];
            j << "{\"ticketId\":" << t.ticketId
              << ",\"subject\":\"" << jsonEsc(t.subject) << "\""
              << ",\"status\":\"" << jsonEsc(t.status) << "\""
              << ",\"createdAt\":\"" << jsonEsc(t.createdAt) << "\""
              << ",\"btcAddress\":\"" << jsonEsc(t.btcAddress) << "\""
              << ",\"requiredBtc\":" << std::fixed << std::setprecision(8) << t.requiredBtc
              << ",\"messageCount\":" << t.messages.size()
              << "}" << (i + 1 < list.size() ? "," : "");
        }
        j << "]";
        res.set_content(j.str(), "application/json");
    });

    svr.Post(R"(/api/ticket/(\d+)/reply)", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(globalMtx);
        auto user = users.getSessionUser(getToken(req));
        if (user.empty()) { res.status = 401; return; }
        int id = std::stoi(req.matches[1]);
        auto f = parseForm(req.body);
        tickets.addReply(id, user, fv(f, "message"));
        res.set_content("{\"ok\": true}", "application/json");
    });

    // ================================================================
    // Admin Console (HTML)
    // ================================================================

    auto requireAdmin = [&](const httplib::Request& req, httplib::Response& res) -> bool {
        auto token = getToken(req);
        auto user = users.getSessionUser(token);
        if (!users.isAdmin(user))
        {
            // Show login form
            std::string h = "<h1>Admin Login</h1>"
                "<form class='card' method='POST' action='/admin/login'>"
                "<label>Username</label><input type='text' name='username' required><br>"
                "<label>Password</label><input type='password' name='password' required><br>"
                "<button>Login</button></form>";
            res.set_content(adminWrap("Login", h), "text/html");
            return false;
        }
        return true;
    };

    svr.Post("/admin/login", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(globalMtx);
        auto f = parseForm(req.body);
        auto user = fv(f, "username");
        if (!users.authenticate(user, fv(f, "password")) || !users.isAdmin(user))
        {
            res.set_redirect("/admin", 303);
            return;
        }
        auto token = users.createSession(user);
        res.set_header("Set-Cookie", "quant_session=" + token + "; Path=/; HttpOnly; SameSite=Strict");
        slog("ADMIN LOGIN");
        res.set_redirect("/admin", 303);
    });

    svr.Get("/admin", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(globalMtx);
        if (!requireAdmin(req, res)) return;
        auto& ulist = users.users();
        auto openTickets = tickets.listOpen();
        std::ostringstream h;
        h << "<h1>&#9939; Quant Server — Dashboard</h1>"
             "<div>"
             "<div class='stat'><div class='lbl'>Users</div><div class='val'>" << ulist.size() << "</div></div>"
             "<div class='stat'><div class='lbl'>Storage</div><div class='val'>" << fmtBytes(store.totalStorageUsed()) << "</div></div>"
             "<div class='stat'><div class='lbl'>Open Tickets</div><div class='val'>" << openTickets.size() << "</div></div>"
             "</div>";
        h << "<h2>Recent Log</h2><div class='console'>";
        {
            std::lock_guard<std::mutex> ll(g_logMutex);
            auto start = g_log.size() > 20 ? g_log.size() - 20 : 0;
            for (size_t i = start; i < g_log.size(); ++i)
                h << "<div class='line'>" << escHtml(g_log[i]) << "</div>";
        }
        h << "</div>";
        res.set_content(adminWrap("Dashboard", h.str()), "text/html");
    });

    svr.Get("/admin/users", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(globalMtx);
        if (!requireAdmin(req, res)) return;
        auto& ulist = users.users();
        std::ostringstream h;
        h << "<h1>Users (" << ulist.size() << ")</h1>"
             "<table><tr><th>Username</th><th>Email</th><th>Premium</th>"
             "<th>Used</th><th>Quota</th><th>Actions</th></tr>";
        for (const auto& u : ulist)
        {
            auto q = store.getQuota(u.username);
            h << "<tr>"
              << "<td>" << escHtml(u.username) << "</td>"
              << "<td>" << escHtml(u.email) << "</td>"
              << "<td>" << (u.premium ? "<span style='color:#00e676;'>YES</span>" : "no") << "</td>"
              << "<td>" << fmtBytes(q.usedBytes) << "</td>"
              << "<td>" << fmtBytes(q.limitBytes) << "</td>"
              << "<td>"
              << "<form style='display:inline;margin:0;' method='POST' action='/admin/set-quota'>"
                 "<input type='hidden' name='username' value='" << escHtml(u.username) << "'>"
                 "<input type='number' name='quotaMB' style='width:60px;' placeholder='MB' required>"
                 "<button style='padding:2px 6px;'>Set MB</button></form> "
              << "<form style='display:inline;margin:0;' method='POST' action='/admin/set-premium'>"
                 "<input type='hidden' name='username' value='" << escHtml(u.username) << "'>"
                 "<input type='hidden' name='premium' value='" << (u.premium ? "0" : "1") << "'>"
                 "<button class='" << (u.premium ? "btn-danger" : "btn") << "' style='padding:2px 6px;'>"
                 << (u.premium ? "Revoke" : "Grant") << "</button></form>"
              << "</td></tr>";
        }
        h << "</table>";
        res.set_content(adminWrap("Users", h.str()), "text/html");
    });

    svr.Post("/admin/set-quota", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(globalMtx);
        auto f = parseForm(req.body);
        auto user = fv(f, "username");
        int mb = fi(f, "quotaMB");
        store.setQuotaLimit(user, static_cast<size_t>(mb) * 1024 * 1024);
        slog("QUOTA SET: " + user + " → " + std::to_string(mb) + " MB");
        res.set_redirect("/admin/users", 303);
    });

    svr.Post("/admin/set-premium", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(globalMtx);
        auto f = parseForm(req.body);
        auto user = fv(f, "username");
        bool prem = (fv(f, "premium") == "1");
        users.setPremium(user, prem);
        if (prem) store.ensureQuota(user, true);
        slog("PREMIUM " + std::string(prem ? "GRANTED" : "REVOKED") + ": " + user);
        res.set_redirect("/admin/users", 303);
    });

    svr.Get("/admin/tickets", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(globalMtx);
        if (!requireAdmin(req, res)) return;
        auto all = tickets.listOpen();
        std::ostringstream h;
        h << "<h1>Open Tickets (" << all.size() << ")</h1>";
        if (all.empty())
        {
            h << "<p style='color:#757575;'>No open tickets.</p>";
        }
        else
        {
            h << "<table><tr><th>#</th><th>User</th><th>Subject</th><th>Status</th><th>Messages</th><th></th></tr>";
            for (const auto& t : all)
            {
                std::string stCls = (t.status == "open") ? "open" : "pending";
                h << "<tr>"
                  << "<td>" << t.ticketId << "</td>"
                  << "<td>" << escHtml(t.username) << "</td>"
                  << "<td>" << escHtml(t.subject) << "</td>"
                  << "<td class='" << stCls << "'>" << escHtml(t.status) << "</td>"
                  << "<td>" << t.messages.size() << "</td>"
                  << "<td><a class='btn' style='padding:2px 6px;font-size:0.75em;' "
                     "href='/admin/ticket/" << t.ticketId << "'>View</a></td>"
                  << "</tr>";
            }
            h << "</table>";
        }
        res.set_content(adminWrap("Tickets", h.str()), "text/html");
    });

    svr.Get(R"(/admin/ticket/(\d+))", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(globalMtx);
        if (!requireAdmin(req, res)) return;
        int id = std::stoi(req.matches[1]);
        const auto* t = tickets.findTicket(id);
        std::ostringstream h;
        if (!t) { h << "<h1>Ticket not found</h1>"; }
        else
        {
            std::string stCls = (t->status == "open") ? "open" : (t->status == "pending_payment" ? "pending" : "closed");
            h << "<h1>Ticket #" << t->ticketId << " — " << escHtml(t->subject) << "</h1>"
              << "<p>User: <strong>" << escHtml(t->username) << "</strong> | "
              << "Status: <span class='" << stCls << "'>" << escHtml(t->status) << "</span></p>";

            // Messages
            for (const auto& m : t->messages)
            {
                bool isAdmin = (m.from == "ADMIN");
                h << "<div style='background:" << (isAdmin ? "#001a0a" : "#0a0a0a")
                  << ";border:1px solid " << (isAdmin ? "#00e676" : "#1a1a1a")
                  << ";border-radius:4px;padding:6px 10px;margin:4px 0;'>"
                  << "<strong style='color:" << (isAdmin ? "#00e676" : "#ffd600") << ";'>"
                  << escHtml(m.from) << "</strong> "
                  << "<span style='color:#757575;font-size:0.75em;'>" << escHtml(m.timestamp) << "</span><br>"
                  << escHtml(m.text) << "</div>";
            }

            // Reply form
            h << "<form class='card' method='POST' action='/admin/ticket/reply'>"
                 "<input type='hidden' name='ticketId' value='" << t->ticketId << "'>"
                 "<label>Reply</label><br>"
                 "<textarea name='reply' required></textarea><br>"
                 "<button>Send Reply</button></form>";

            // Set BTC payment
            if (t->status != "closed")
            {
                h << "<form class='card' method='POST' action='/admin/ticket/set-btc'>"
                     "<input type='hidden' name='ticketId' value='" << t->ticketId << "'>"
                     "<h2 style='margin-top:0;'>Set Payment</h2>"
                     "<label>BTC Address</label><input type='text' name='btcAddress' style='width:300px;' required><br>"
                     "<label>Required BTC</label><input type='number' name='requiredBtc' step='any' required><br>"
                     "<label>Quota (MB)</label><input type='number' name='quotaMB' required><br>"
                     "<button>Set Payment</button></form>";

                h << "<form style='display:inline;' method='POST' action='/admin/ticket/close'>"
                     "<input type='hidden' name='ticketId' value='" << t->ticketId << "'>"
                     "<button class='btn-danger'>Close Ticket</button></form> ";

                h << "<form style='display:inline;' method='POST' action='/admin/confirm-payment'>"
                     "<input type='hidden' name='username' value='" << escHtml(t->username) << "'>"
                     "<input type='hidden' name='ticketId' value='" << t->ticketId << "'>"
                     "<input type='hidden' name='quotaMB' value='" << (t->requestedBytes / (1024 * 1024)) << "'>"
                     "<button style='background:#004d25;'>Confirm Payment &amp; Set Quota</button></form>";
            }
        }
        h << "<br><a class='btn' href='/admin/tickets'>Back</a>";
        res.set_content(adminWrap("Ticket #" + std::to_string(id), h.str()), "text/html");
    });

    svr.Post("/admin/ticket/reply", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(globalMtx);
        auto f = parseForm(req.body);
        int id = fi(f, "ticketId");
        tickets.addReply(id, "ADMIN", fv(f, "reply"));
        slog("TICKET REPLY: #" + std::to_string(id));
        res.set_redirect("/admin/ticket/" + std::to_string(id), 303);
    });

    svr.Post("/admin/ticket/set-btc", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(globalMtx);
        auto f = parseForm(req.body);
        int id = fi(f, "ticketId");
        int mb = fi(f, "quotaMB");
        tickets.setBtcPayment(id, fv(f, "btcAddress"), fd(f, "requiredBtc"),
                              static_cast<size_t>(mb) * 1024 * 1024);
        slog("BTC PAYMENT SET: ticket #" + std::to_string(id));
        res.set_redirect("/admin/ticket/" + std::to_string(id), 303);
    });

    svr.Post("/admin/ticket/close", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(globalMtx);
        auto f = parseForm(req.body);
        int id = fi(f, "ticketId");
        tickets.closeTicket(id);
        slog("TICKET CLOSED: #" + std::to_string(id));
        res.set_redirect("/admin/tickets", 303);
    });

    svr.Post("/admin/confirm-payment", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(globalMtx);
        auto f = parseForm(req.body);
        auto user = fv(f, "username");
        int mb = fi(f, "quotaMB");
        int ticketId = fi(f, "ticketId");
        if (mb > 0) store.setQuotaLimit(user, static_cast<size_t>(mb) * 1024 * 1024);
        if (ticketId > 0)
        {
            tickets.addReply(ticketId, "ADMIN", "Payment confirmed. Quota set to " + std::to_string(mb) + " MB.");
            tickets.closeTicket(ticketId);
        }
        slog("PAYMENT CONFIRMED: " + user + " → " + std::to_string(mb) + " MB");
        res.set_redirect("/admin/tickets", 303);
    });

    svr.Get("/admin/console", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(globalMtx);
        if (!requireAdmin(req, res)) return;
        std::ostringstream h;
        h << "<h1>Server Console</h1>"
             "<div class='console' id='log'>";
        {
            std::lock_guard<std::mutex> ll(g_logMutex);
            for (const auto& line : g_log)
                h << "<div class='line'>" << escHtml(line) << "</div>";
        }
        h << "</div>"
             "<script>var d=document.getElementById('log');d.scrollTop=d.scrollHeight;</script>";
        res.set_content(adminWrap("Console", h.str()), "text/html");
    });

    // ---- Start ----

    slog("Listening on http://0.0.0.0:" + std::to_string(port));
    slog("Admin console: http://localhost:" + std::to_string(port) + "/admin");
    svr.listen("0.0.0.0", port);
    return 0;
}
