#pragma once

#include <string>
#include <map>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <cstring>
#include <mutex>

// ============================================================
// SyncStore — encrypted blob storage with per-user quotas
//
// The server stores opaque encrypted blobs. It never sees
// plaintext — all crypto is client-side (AES-256-GCM).
//
// Each user gets a directory under vaults/:
//   vaults/{username}/db.enc    — encrypted database
//   vaults/{username}/meta.json — sync metadata
// ============================================================

class SyncStore
{
public:
    struct QuotaInfo
    {
        size_t usedBytes  = 0;
        size_t limitBytes = 5 * 1024 * 1024; // 5 MB default
    };

    struct VaultMeta
    {
        std::string lastSync;
        size_t      sizeBytes = 0;
        std::string checksum;
    };

    explicit SyncStore(const std::string& dataDir)
        : m_dataDir(dataDir)
    {
        std::filesystem::create_directories(vaultsDir());
        loadQuotas();
    }

    void setDefaultQuota(size_t bytes)   { m_defaultQuota = bytes; }
    void setPremiumQuota(size_t bytes)    { m_premiumQuota = bytes; }

    QuotaInfo getQuota(const std::string& username) const
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        auto it = m_quotas.find(toLower(username));
        if (it != m_quotas.end()) return it->second;
        QuotaInfo q;
        q.limitBytes = m_defaultQuota;
        return q;
    }

    void setQuotaLimit(const std::string& username, size_t limitBytes)
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        auto& q = m_quotas[toLower(username)];
        q.limitBytes = limitBytes;
        saveQuotas();
    }

    void ensureQuota(const std::string& username, bool isPremium)
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        auto lower = toLower(username);
        if (m_quotas.find(lower) == m_quotas.end())
        {
            QuotaInfo q;
            q.limitBytes = isPremium ? m_premiumQuota : m_defaultQuota;
            m_quotas[lower] = q;
            saveQuotas();
        }
    }

    // Upload encrypted blob. Returns "" on success, error message on failure.
    std::string upload(const std::string& username, const char* data, size_t size)
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        auto lower = toLower(username);
        auto& q = m_quotas[lower];

        if (size > q.limitBytes)
            return "Blob size (" + std::to_string(size) + " bytes) exceeds quota ("
                 + std::to_string(q.limitBytes) + " bytes)";

        auto dir = userVaultDir(lower);
        std::filesystem::create_directories(dir);

        std::ofstream f(dir + "/db.enc", std::ios::binary | std::ios::trunc);
        if (!f) return "Failed to write vault file";
        f.write(data, size);
        f.close();

        q.usedBytes = size;
        saveQuotas();

        // Write metadata
        std::ofstream mf(dir + "/meta.json", std::ios::trunc);
        mf << "{\n"
           << "  \"lastSync\": \"" << nowISO() << "\",\n"
           << "  \"sizeBytes\": " << size << ",\n"
           << "  \"checksum\": \"" << simpleChecksum(data, size) << "\"\n"
           << "}\n";

        return "";
    }

    // Download encrypted blob. Returns data and sets size. Empty on not found.
    std::string download(const std::string& username)
    {
        auto path = userVaultDir(toLower(username)) + "/db.enc";
        std::ifstream f(path, std::ios::binary);
        if (!f) return "";
        return std::string((std::istreambuf_iterator<char>(f)),
                            std::istreambuf_iterator<char>());
    }

    VaultMeta getMeta(const std::string& username) const
    {
        VaultMeta m;
        auto path = userVaultDir(toLower(username)) + "/meta.json";
        std::ifstream f(path);
        if (!f) return m;
        std::string content((std::istreambuf_iterator<char>(f)),
                             std::istreambuf_iterator<char>());
        m.lastSync  = jsonGetStr(content, "lastSync");
        auto sb     = jsonGetStr(content, "sizeBytes");
        try { m.sizeBytes = std::stoull(sb); } catch (...) {}
        m.checksum  = jsonGetStr(content, "checksum");
        return m;
    }

    size_t totalStorageUsed() const
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        size_t total = 0;
        for (const auto& [k, v] : m_quotas) total += v.usedBytes;
        return total;
    }

    const std::map<std::string, QuotaInfo>& quotas() const { return m_quotas; }

private:
    std::string m_dataDir;
    std::map<std::string, QuotaInfo> m_quotas;
    size_t m_defaultQuota = 5 * 1024 * 1024;    // 5 MB
    size_t m_premiumQuota = 50 * 1024 * 1024;   // 50 MB
    mutable std::mutex m_mutex;

    std::string vaultsDir() const { return m_dataDir + "/vaults"; }
    std::string userVaultDir(const std::string& lower) const { return vaultsDir() + "/" + lower; }
    std::string quotasPath() const { return m_dataDir + "/quotas.json"; }

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

    static std::string simpleChecksum(const char* data, size_t size)
    {
        size_t h = 0x811c9dc5;
        for (size_t i = 0; i < size; ++i)
            h = (h ^ static_cast<unsigned char>(data[i])) * 0x01000193;
        std::ostringstream ss;
        ss << std::hex << std::setw(16) << std::setfill('0') << h;
        return ss.str();
    }

    static std::string jsonGetStr(const std::string& obj, const std::string& key)
    {
        std::string needle = "\"" + key + "\"";
        auto pos = obj.find(needle);
        if (pos == std::string::npos) return "";
        pos = obj.find(':', pos + needle.size());
        if (pos == std::string::npos) return "";
        pos++;
        while (pos < obj.size() && (obj[pos] == ' ' || obj[pos] == '\t')) pos++;
        if (pos >= obj.size()) return "";
        if (obj[pos] == '"')
        {
            pos++;
            auto end = obj.find('"', pos);
            return (end != std::string::npos) ? obj.substr(pos, end - pos) : "";
        }
        auto end = obj.find_first_of(",}\n", pos);
        std::string val = obj.substr(pos, end == std::string::npos ? end : end - pos);
        while (!val.empty() && val.back() == ' ') val.pop_back();
        return val;
    }

    void saveQuotas() const
    {
        std::ofstream f(quotasPath(), std::ios::trunc);
        if (!f) return;
        f << "{\n";
        bool first = true;
        for (const auto& [user, q] : m_quotas)
        {
            if (!first) f << ",\n";
            f << "  \"" << user << "\": {"
              << "\"usedBytes\": " << q.usedBytes << ", "
              << "\"limitBytes\": " << q.limitBytes << "}";
            first = false;
        }
        f << "\n}\n";
    }

    void loadQuotas()
    {
        m_quotas.clear();
        std::ifstream f(quotasPath());
        if (!f) return;
        std::string content((std::istreambuf_iterator<char>(f)),
                             std::istreambuf_iterator<char>());
        // Simple parser for {"user": {"usedBytes": N, "limitBytes": N}, ...}
        size_t pos = 0;
        while (pos < content.size())
        {
            auto keyStart = content.find('"', pos);
            if (keyStart == std::string::npos) break;
            auto keyEnd = content.find('"', keyStart + 1);
            if (keyEnd == std::string::npos) break;
            std::string key = content.substr(keyStart + 1, keyEnd - keyStart - 1);

            auto objStart = content.find('{', keyEnd);
            if (objStart == std::string::npos) break;
            auto objEnd = content.find('}', objStart);
            if (objEnd == std::string::npos) break;
            std::string obj = content.substr(objStart, objEnd - objStart + 1);

            QuotaInfo q;
            auto ub = jsonGetStr(obj, "usedBytes");
            auto lb = jsonGetStr(obj, "limitBytes");
            try { q.usedBytes = std::stoull(ub); } catch (...) {}
            try { q.limitBytes = std::stoull(lb); } catch (...) {}
            m_quotas[key] = q;

            pos = objEnd + 1;
        }
    }
};
