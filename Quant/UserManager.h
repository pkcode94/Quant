#pragma once

#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <random>
#include <chrono>
#include <algorithm>
#include <iomanip>
#include <cctype>

class UserManager
{
public:
    struct User
    {
        std::string username;
        std::string passwordHash;
        std::string salt;
        std::string email;
        std::string createdAt;
        bool        premium       = false;
        std::string premiumExpiry;
    };

    explicit UserManager(const std::string& baseDir = "users")
        : m_baseDir(baseDir)
    {
        std::filesystem::create_directories(m_baseDir);
        loadUsers();
        loadPendingPayments();
        ensureAdmin();
    }

    static constexpr const char* adminUser() { return "ADMIN"; }
    static constexpr const char* adminPass() { return "02AdminA!12"; }

    bool isAdmin(const std::string& username) const
    {
        return toLower(username) == toLower(std::string(adminUser()));
    }

    // Register a new user. Returns empty string on success, error message on failure.
    std::string registerUser(const std::string& username, const std::string& password, const std::string& email)
    {
        if (username.empty() || username.size() > 32)
            return "Username must be 1-32 characters";
        if (password.size() < 4)
            return "Password must be at least 4 characters";
        for (char c : username)
            if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_')
                return "Username: only letters, digits, underscores";

        std::string lower = toLower(username);
        for (const auto& u : m_users)
            if (toLower(u.username) == lower)
                return "Username already taken";

        User u;
        u.username     = username;
        u.salt         = generateRandom(32);
        u.passwordHash = hashPassword(password, u.salt);
        u.email        = email;
        u.createdAt    = nowISO();
        u.premium      = false;

        m_users.push_back(u);
        std::filesystem::create_directories(userDbDir(username));
        saveUsers();
        return "";
    }

    bool authenticate(const std::string& username, const std::string& password) const
    {
        for (const auto& u : m_users)
            if (u.username == username)
                return u.passwordHash == hashPassword(password, u.salt);
        return false;
    }

    std::string createSession(const std::string& username)
    {
        std::string token = generateRandom(48);
        m_sessions[token] = username;
        return token;
    }

    std::string getSessionUser(const std::string& token) const
    {
        auto it = m_sessions.find(token);
        return (it != m_sessions.end()) ? it->second : "";
    }

    void destroySession(const std::string& token) { m_sessions.erase(token); }

    std::string userDbDir(const std::string& username) const
    {
        return m_baseDir + "/" + toLower(username) + "/db";
    }

    bool isPremium(const std::string& username) const
    {
        for (const auto& u : m_users)
            if (u.username == username) return u.premium;
        return false;
    }

    void setPremium(const std::string& username, bool val, const std::string& expiry = "")
    {
        for (auto& u : m_users)
            if (u.username == username)
            {
                u.premium = val;
                u.premiumExpiry = expiry;
                saveUsers();
                return;
            }
    }

    static constexpr const char* cookieName() { return "quant_session"; }

    const std::vector<User>& users() const { return m_users; }

    // ---- Pending payments ----
    struct PendingPayment
    {
        std::string username;
        std::string btcAddress;
        double      requiredBtc = 0;
        std::string createdAt;
    };

    void addPendingPayment(const PendingPayment& p)
    {
        m_pendingPayments.push_back(p);
        savePendingPayments();
    }

    PendingPayment* findPendingPayment(const std::string& username)
    {
        for (auto& p : m_pendingPayments)
            if (p.username == username) return &p;
        return nullptr;
    }

    void removePendingPayment(const std::string& username)
    {
        m_pendingPayments.erase(
            std::remove_if(m_pendingPayments.begin(), m_pendingPayments.end(),
                [&](const PendingPayment& p) { return p.username == username; }),
            m_pendingPayments.end());
        savePendingPayments();
    }

    const std::vector<PendingPayment>& pendingPayments() const { return m_pendingPayments; }

private:
    std::string m_path;  // unused legacy
    std::string m_baseDir;
    std::vector<User> m_users;
    std::map<std::string, std::string> m_sessions; // token -> username
    std::vector<PendingPayment> m_pendingPayments;

    std::string usersPath() const { return m_baseDir + "/users.json"; }

    // ---- JSON persistence ----

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

    static std::string jsonUnesc(const std::string& s)
    {
        std::string o;
        for (size_t i = 0; i < s.size(); ++i)
        {
            if (s[i] == '\\' && i + 1 < s.size())
            {
                char next = s[++i];
                if (next == '"')       o += '"';
                else if (next == '\\') o += '\\';
                else if (next == 'n')  o += '\n';
                else { o += '\\'; o += next; }
            }
            else o += s[i];
        }
        return o;
    }

    static std::string jsonGetStr(const std::string& obj, const std::string& key)
    {
        std::string needle = "\"" + key + "\"";
        auto pos = obj.find(needle);
        if (pos == std::string::npos) return "";
        pos = obj.find(':', pos + needle.size());
        if (pos == std::string::npos) return "";
        pos++;
        while (pos < obj.size() && (obj[pos] == ' ' || obj[pos] == '\t' || obj[pos] == '\n' || obj[pos] == '\r')) pos++;
        if (pos >= obj.size()) return "";
        if (obj[pos] == '"')
        {
            pos++;
            auto end = pos;
            while (end < obj.size() && !(obj[end] == '"' && obj[end - 1] != '\\')) end++;
            return jsonUnesc(obj.substr(pos, end - pos));
        }
        auto end = obj.find_first_of(",}\n\r", pos);
        std::string val = obj.substr(pos, end == std::string::npos ? end : end - pos);
        while (!val.empty() && std::isspace(static_cast<unsigned char>(val.back()))) val.pop_back();
        return val;
    }

    void saveUsers() const
    {
        std::ofstream f(usersPath(), std::ios::trunc);
        if (!f) return;
        f << "[\n";
        for (size_t i = 0; i < m_users.size(); ++i)
        {
            const auto& u = m_users[i];
            f << "  {\n"
              << "    \"username\": \""     << jsonEsc(u.username)     << "\",\n"
              << "    \"passwordHash\": \"" << jsonEsc(u.passwordHash) << "\",\n"
              << "    \"salt\": \""         << jsonEsc(u.salt)         << "\",\n"
              << "    \"email\": \""        << jsonEsc(u.email)        << "\",\n"
              << "    \"createdAt\": \""    << jsonEsc(u.createdAt)    << "\",\n"
              << "    \"premium\": "        << (u.premium ? "true" : "false") << ",\n"
              << "    \"premiumExpiry\": \"" << jsonEsc(u.premiumExpiry) << "\"\n"
              << "  }" << (i + 1 < m_users.size() ? "," : "") << "\n";
        }
        f << "]\n";
    }

    void loadUsers()
    {
        m_users.clear();
        std::ifstream f(usersPath());
        if (!f) return;
        std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());

        size_t pos = 0;
        while ((pos = content.find('{', pos)) != std::string::npos)
        {
            auto end = content.find('}', pos);
            if (end == std::string::npos) break;
            std::string obj = content.substr(pos, end - pos + 1);

            User u;
            u.username     = jsonGetStr(obj, "username");
            u.passwordHash = jsonGetStr(obj, "passwordHash");
            u.salt         = jsonGetStr(obj, "salt");
            u.email        = jsonGetStr(obj, "email");
            u.createdAt    = jsonGetStr(obj, "createdAt");
            u.premium      = jsonGetStr(obj, "premium") == "true";
            u.premiumExpiry = jsonGetStr(obj, "premiumExpiry");

            if (!u.username.empty())
                m_users.push_back(u);

            pos = end + 1;
        }
    }

    // ---- Hashing ----

    static std::string hashPassword(const std::string& password, const std::string& salt)
    {
        std::string input = salt + password + salt;
        size_t h1 = 0x6a09e667, h2 = 0xbb67ae85, h3 = 0x3c6ef372, h4 = 0xa54ff53a;
        for (int round = 0; round < 5000; ++round)
        {
            h1 = std::hash<std::string>{}(input + std::to_string(h1) + std::to_string(round));
            h2 = std::hash<std::string>{}(input + std::to_string(h2) + std::to_string(h1));
            h3 = std::hash<std::string>{}(input + std::to_string(h3) + std::to_string(h2));
            h4 = std::hash<std::string>{}(input + std::to_string(h4) + std::to_string(h3));
        }
        std::ostringstream ss;
        ss << std::hex << std::setfill('0')
           << std::setw(16) << h1 << std::setw(16) << h2
           << std::setw(16) << h3 << std::setw(16) << h4;
        return ss.str();
    }

    // ---- Utilities ----

    static std::string generateRandom(int length)
    {
        static const char chars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
        static thread_local std::mt19937 rng(
            static_cast<unsigned>(std::chrono::steady_clock::now().time_since_epoch().count()));
        std::uniform_int_distribution<int> dist(0, sizeof(chars) - 2);
        std::string s;
        s.reserve(length);
        for (int i = 0; i < length; ++i) s += chars[dist(rng)];
        return s;
    }

    static std::string toLower(const std::string& s)
    {
        std::string r = s;
        for (auto& c : r) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return r;
    }

    static std::string nowISO()
    {
        auto now = std::chrono::system_clock::now();
        auto t = std::chrono::system_clock::to_time_t(now);
        std::tm tm{};
#ifdef _WIN32
        localtime_s(&tm, &t);
#else
        localtime_r(&t, &tm);
#endif
        std::ostringstream ss;
        ss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S");
        return ss.str();
    }

    // ---- Admin seed ----

    void ensureAdmin()
    {
        std::string lower = toLower(std::string(adminUser()));
        for (const auto& u : m_users)
            if (toLower(u.username) == lower) return;
        User u;
        u.username     = adminUser();
        u.salt         = generateRandom(32);
        u.passwordHash = hashPassword(std::string(adminPass()), u.salt);
        u.createdAt    = nowISO();
        u.premium      = true;
        m_users.push_back(u);
        std::filesystem::create_directories(userDbDir(u.username));
        saveUsers();
    }

    // ---- Pending payment persistence ----

    std::string pendingPath() const { return m_baseDir + "/pending_payments.json"; }

    void savePendingPayments() const
    {
        std::ofstream f(pendingPath(), std::ios::trunc);
        if (!f) return;
        f << "[\n";
        for (size_t i = 0; i < m_pendingPayments.size(); ++i)
        {
            const auto& p = m_pendingPayments[i];
            f << "  {\n"
              << "    \"username\": \""    << jsonEsc(p.username)   << "\",\n"
              << "    \"btcAddress\": \""  << jsonEsc(p.btcAddress) << "\",\n"
              << "    \"requiredBtc\": "   << std::fixed << std::setprecision(8) << p.requiredBtc << ",\n"
              << "    \"createdAt\": \""   << jsonEsc(p.createdAt)  << "\"\n"
              << "  }" << (i + 1 < m_pendingPayments.size() ? "," : "") << "\n";
        }
        f << "]\n";
    }

    void loadPendingPayments()
    {
        m_pendingPayments.clear();
        std::ifstream f(pendingPath());
        if (!f) return;
        std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        size_t pos = 0;
        while ((pos = content.find('{', pos)) != std::string::npos)
        {
            auto end = content.find('}', pos);
            if (end == std::string::npos) break;
            std::string obj = content.substr(pos, end - pos + 1);
            PendingPayment p;
            p.username    = jsonGetStr(obj, "username");
            p.btcAddress  = jsonGetStr(obj, "btcAddress");
            std::string rb = jsonGetStr(obj, "requiredBtc");
            try { p.requiredBtc = std::stod(rb); } catch (...) { p.requiredBtc = 0; }
            p.createdAt   = jsonGetStr(obj, "createdAt");
            if (!p.username.empty())
                m_pendingPayments.push_back(p);
            pos = end + 1;
        }
    }
};
