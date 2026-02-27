#pragma once

#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <mutex>

class AdminConfig
{
public:
    double   premiumPriceMbtc = 5.0;
    int      freeTradeLimit   = 10;
    std::string electrumPath  = "electrum";
    std::string electrumWallet;

    explicit AdminConfig(const std::string& path = "admin_config.json")
        : m_path(path)
    {
        load();
    }

    void load()
    {
        std::ifstream f(m_path);
        if (!f) return;
        std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        premiumPriceMbtc = getNum(content, "premiumPriceMbtc", premiumPriceMbtc);
        freeTradeLimit   = (int)getNum(content, "freeTradeLimit", (double)freeTradeLimit);
        electrumPath     = getStr(content, "electrumPath", electrumPath);
        electrumWallet   = getStr(content, "electrumWallet", electrumWallet);
    }

    void save() const
    {
        std::ofstream f(m_path, std::ios::trunc);
        if (!f) return;
        f << "{\n"
          << "  \"premiumPriceMbtc\": " << premiumPriceMbtc << ",\n"
          << "  \"freeTradeLimit\": " << freeTradeLimit << ",\n"
          << "  \"electrumPath\": \"" << jsonEsc(electrumPath) << "\",\n"
          << "  \"electrumWallet\": \"" << jsonEsc(electrumWallet) << "\"\n"
          << "}\n";
    }

    double premiumPriceBtc() const { return premiumPriceMbtc * 0.001; }

    std::string electrumCmd() const
    {
        std::string cmd = electrumPath;
        if (!electrumWallet.empty())
            cmd += " -w \"" + electrumWallet + "\"";
        return cmd;
    }

private:
    std::string m_path;

    static std::string jsonEsc(const std::string& s)
    {
        std::string o;
        for (char c : s)
        {
            if (c == '"')       o += "\\\"";
            else if (c == '\\') o += "\\\\";
            else                o += c;
        }
        return o;
    }

    static std::string jsonUnesc(const std::string& s)
    {
        std::string o;
        for (size_t i = 0; i < s.size(); ++i)
        {
            if (s[i] == '\\' && i + 1 < s.size()) { ++i; o += s[i]; }
            else o += s[i];
        }
        return o;
    }

    static std::string getStr(const std::string& obj, const std::string& key, const std::string& def)
    {
        std::string needle = "\"" + key + "\"";
        auto pos = obj.find(needle);
        if (pos == std::string::npos) return def;
        pos = obj.find(':', pos + needle.size());
        if (pos == std::string::npos) return def;
        pos++;
        while (pos < obj.size() && (obj[pos] == ' ' || obj[pos] == '\t')) pos++;
        if (pos >= obj.size() || obj[pos] != '"') return def;
        pos++;
        auto end = pos;
        while (end < obj.size() && !(obj[end] == '"' && (end == 0 || obj[end - 1] != '\\'))) end++;
        return jsonUnesc(obj.substr(pos, end - pos));
    }

    static double getNum(const std::string& obj, const std::string& key, double def)
    {
        std::string needle = "\"" + key + "\"";
        auto pos = obj.find(needle);
        if (pos == std::string::npos) return def;
        pos = obj.find(':', pos + needle.size());
        if (pos == std::string::npos) return def;
        pos++;
        while (pos < obj.size() && (obj[pos] == ' ' || obj[pos] == '\t')) pos++;
        try { return std::stod(obj.substr(pos)); } catch (...) { return def; }
    }
};
