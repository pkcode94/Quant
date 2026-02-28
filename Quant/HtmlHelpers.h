#pragma once

#include "Trade.h"
#include "ProfitCalculator.h"
#include "MultiHorizonEngine.h"
#include "cpp-httplib-master\httplib.h"

#include <string>
#include <sstream>
#include <iomanip>
#include <map>
#include <cmath>
#include <ctime>

// ---- HTML helpers ----
namespace html {

// Format unix timestamp to "YYYY-MM-DD HH:MM" or "-" if 0.
inline std::string fmtTime(long long ts)
{
    if (ts <= 0) return "-";
    std::time_t tt = static_cast<std::time_t>(ts);
    std::tm tm;
#ifdef _WIN32
    localtime_s(&tm, &tt);
#else
    localtime_r(&tt, &tm);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", &tm);
    return buf;
}

// Format unix timestamp to datetime-local input value "YYYY-MM-DDTHH:MM".
inline std::string fmtDatetimeLocal(long long ts)
{
    if (ts <= 0) return "";
    std::time_t tt = static_cast<std::time_t>(ts);
    std::tm tm;
#ifdef _WIN32
    localtime_s(&tm, &tt);
#else
    localtime_r(&tt, &tm);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M", &tm);
    return buf;
}

// Parse datetime-local string "YYYY-MM-DDTHH:MM" to unix timestamp.
inline long long parseDatetimeLocal(const std::string& s)
{
    if (s.empty()) return 0;
    std::tm tm = {};
    // YYYY-MM-DDTHH:MM
    if (s.size() >= 16 && s[4] == '-' && s[7] == '-' && (s[10] == 'T' || s[10] == ' '))
    {
        try {
            tm.tm_year = std::stoi(s.substr(0, 4)) - 1900;
            tm.tm_mon  = std::stoi(s.substr(5, 2)) - 1;
            tm.tm_mday = std::stoi(s.substr(8, 2));
            tm.tm_hour = std::stoi(s.substr(11, 2));
            tm.tm_min  = std::stoi(s.substr(14, 2));
            tm.tm_isdst = -1;
            return static_cast<long long>(std::mktime(&tm));
        } catch (...) {}
    }
    // fallback: try as raw unix timestamp
    try { return std::stoll(s); } catch (...) {}
    return 0;
}

inline std::string esc(const std::string& s)
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

// Today's date as "YYYY-MM-DD".
inline std::string today()
{
    std::time_t now = std::time(nullptr);
    std::tm tm;
#ifdef _WIN32
    localtime_s(&tm, &now);
#else
    localtime_r(&now, &tm);
#endif
    char buf[16];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d", &tm);
    return buf;
}

inline std::string css()
{
    return
        "<style>"
        "*{box-sizing:border-box;}"
        "body{font-family:'Segoe UI',monospace;background:#0b1426;color:#cbd5e1;margin:0;padding:0;}"
        "nav{background:#0f1b2d;padding:10px 20px;border-bottom:1px solid #1a2744;display:flex;gap:8px;flex-wrap:wrap;}"
        "nav a{color:#7b97c4;text-decoration:none;font-size:0.85em;padding:4px 8px;border-radius:4px;}"
        "nav a:hover{background:#132035;}"
        ".container{max-width:1200px;margin:0 auto;padding:20px;}"
        "h1{color:#c9a44a;margin:0 0 12px 0;}"
        "h2{color:#64748b;border-bottom:1px solid #152238;padding-bottom:5px;}"
        "table{border-collapse:collapse;width:100%;margin:8px 0 18px 0;}"
        "th,td{border:1px solid #152238;padding:5px 8px;text-align:right;font-size:0.85em;}"
        "th{background:#0f1b2d;color:#c9a44a;text-align:left;}"
        "td{background:#0b1426;}tr:hover td{background:#0f1b2d;}"
        ".buy{color:#22c55e;font-weight:bold;}"
        ".sell{color:#ef4444;font-weight:bold;}"
        ".on{color:#22c55e;}.off{color:#475569;}"
        "form.card{background:#0f1b2d;border:1px solid #1a2744;border-radius:8px;padding:12px;margin:8px 0;}"
        "form.card h3{margin:0 0 8px 0;color:#c9a44a;font-size:0.95em;}"
        "label{display:inline-block;min-width:110px;color:#64748b;font-size:0.82em;}"
        "input,select{background:#0b1426;border:1px solid #1a2744;color:#cbd5e1;padding:4px 6px;"
        "border-radius:4px;margin:2px 4px 2px 0;font-family:inherit;font-size:0.85em;}"
        "input:focus,select:focus{border-color:#c9a44a;outline:none;}"
        "button,.btn{background:#166534;color:#fff;border:none;padding:5px 12px;border-radius:4px;"
        "cursor:pointer;font-size:0.82em;font-family:inherit;text-decoration:none;display:inline-block;}"
        "button:hover,.btn:hover{background:#15803d;}"
        ".btn-danger{background:#991b1b;}.btn-danger:hover{background:#ef4444;}"
        ".btn-sm{padding:2px 7px;font-size:0.78em;}"
        ".btn-warn{background:#a16207;}.btn-warn:hover{background:#ca8a04;}"
        ".row{display:flex;gap:12px;flex-wrap:wrap;}"
        ".stat{background:#0f1b2d;border:1px solid #1a2744;border-radius:8px;padding:10px 16px;min-width:140px;}"
        ".stat .lbl{color:#64748b;font-size:0.78em;}"
        ".stat .val{font-size:1.3em;color:#c9a44a;}"
        ".msg{background:#1e3a5f22;border:1px solid #2563eb;padding:6px 10px;border-radius:4px;margin:8px 0;color:#7b97c4;font-size:0.9em;}"
        ".err{background:#ef444422;border:1px solid #ef4444;color:#ef4444;}"
        ".iform{display:inline;}"
        "input[type=number]{width:90px;}"
        "input[type=text]{width:120px;}"
        ".forms-row{display:flex;gap:10px;flex-wrap:wrap;}"
        ".forms-row form.card{flex:1;min-width:280px;}"
        "p.empty{color:#475569;font-style:italic;}"
        ".child-row td{border-left:3px solid #1a2744;}"
        ".child-row td:first-child{padding-left:24px;color:#64748b;}"
        ".child-indent{color:#475569;font-size:0.8em;margin-right:4px;}"
        ".workflow{display:flex;margin:0 0 16px 0;}"
        ".wf-step{flex:1;text-align:center;padding:8px 0;font-size:0.82em;border-bottom:3px solid #152238;color:#475569;}"
        ".wf-step a{color:inherit;text-decoration:none;}"
        ".wf-step.active{border-color:#2563eb;color:#7b97c4;font-weight:bold;}"
        ".wf-step.done{border-color:#16a34a;color:#22c55e;}"
        ".wf-step.done a::before{content:'\2713  ';}"
        ".wf-step.locked{border-color:#152238;color:#1a2744;pointer-events:none;opacity:0.4;}"
        ".wf-step.locked a{color:#1a2744;cursor:default;}"
        ".calc-console{background:#0b1426;border:1px solid #1a2744;border-radius:6px;padding:12px 16px;"
        "margin:12px 0;font-family:'Consolas',monospace;font-size:0.78em;color:#64748b;overflow-x:auto;"
        "white-space:pre-wrap;line-height:1.6;}"
        ".calc-console .hd{color:#c9a44a;font-weight:bold;display:block;margin:6px 0 2px 0;"
        "border-bottom:1px solid #152238;padding-bottom:2px;}"
        ".calc-console .fm{color:#a78bfa;}"
        ".calc-console .vl{color:#c9a44a;}"
        ".calc-console .rs{color:#22c55e;font-weight:bold;}"
        "</style>";
}

inline std::string nav()
{
    return
        "<nav>"
        "<a href='/'>Dashboard</a>"
        "<a href='/trades'>Trades</a>"
        "<a href='/wallet'>Wallet</a>"
        "<a href='/portfolio'>Portfolio</a>"
        "<a href='/dca'>DCA</a>"
        "<a href='/profit'>Profit</a>"
        "<a href='/generate-horizons'>Horizons Gen</a>"
        "<a href='/price-check'>Price Check</a>"
        "<a href='/market-entry'>Entry Calc</a>"
        "<a href='/serial-generator'>Serial Gen</a>"
        "<a href='/exit-strategy'>Exit Calc</a>"
        "<a href='/pending-exits'>Pending Exits</a>"
        "<a href='/entry-points'>Entry Points</a>"
        "<a href='/profit-history'>Profit History</a>"
        "<a href='/params-history'>Params</a>"
        "<a href='/param-models'>Models</a>"
        "<a href='/pnl' style='color:#22c55e;'>&#9654; P&amp;L</a>"
        "<a href='/simulator' style='color:#38bdf8;'>&#9881; Simulator</a>"
        "<a href='/chart' style='color:#c9a44a;font-weight:bold;'>&#9733; Chart</a>"
        "<a href='/premium' style='color:#c9a44a;'>&#9733; Premium</a>"
        "<a href='/admin' style='color:#7b97c4;'>Admin</a>"
        "<a href='/wipe' style='color:#ef4444;'>Wipe</a>"
        "<a href='/logout' style='color:#94a3b8;margin-left:auto;'>Logout</a>"
        "</nav>";
}

inline std::string workflow(int step, bool canHorizons = true, bool canExits = true)
{
    static const char* labels[] = {"1. Market Entry", "2. Generate Horizons", "3. Exit Strategy"};
    static const char* hrefs[]  = {"/market-entry", "/generate-horizons", "/exit-strategy"};
    bool available[] = {true, canHorizons, canExits};
    std::string h = "<div class='workflow'>";
    for (int i = 0; i < 3; ++i)
    {
        const char* cls;
        if (!available[i])      cls = "wf-step locked";
        else if (i == step)     cls = "wf-step active";
        else if (i < step)      cls = "wf-step done";
        else                    cls = "wf-step";
        h += std::string("<div class='") + cls + "'><a href='" + hrefs[i] + "'>" + labels[i] + "</a></div>";
    }
    return h + "</div>";
}

inline std::string traceOverhead(double price, double quantity,
                                 const HorizonParams& p)
{
    std::ostringstream c;
    c << std::fixed << std::setprecision(8);
    double fc  = p.feeSpread * p.feeHedgingCoefficient * p.deltaTime;
    double num = fc * static_cast<double>(p.symbolCount);
    double ppq = (quantity > 0.0) ? price / quantity : 0.0;
    double den = ppq * p.portfolioPump + p.coefficientK;
    double oh  = (den != 0.0) ? num / den : 0.0;
    double surpComp = p.surplusRate * p.feeHedgingCoefficient * p.deltaTime;
    double feeComp  = p.feeSpread * p.feeHedgingCoefficient * p.deltaTime;
    double eo  = oh + surpComp + feeComp;
    c << "<span class='hd'>Overhead</span>"
      << "feeComponent = <span class='fm'>" << p.feeSpread << " &times; " << p.feeHedgingCoefficient << " &times; " << p.deltaTime << "</span>"
      << " = <span class='vl'>" << fc << "</span>\n"
      << "numerator    = <span class='fm'>" << fc << " &times; " << p.symbolCount << "</span>"
      << " = <span class='vl'>" << num << "</span>\n"
      << "pricePerQty  = <span class='fm'>" << price << " / " << quantity << "</span>"
      << " = <span class='vl'>" << ppq << "</span>\n"
      << "denominator  = <span class='fm'>" << ppq << " &times; " << p.portfolioPump << " + " << p.coefficientK << "</span>"
      << " = <span class='vl'>" << den << "</span>\n"
      << "<span class='rs'>overhead</span>     = " << num << " / " << den
      << " = <span class='rs'>" << oh << " (" << (oh * 100.0) << "%)</span>\n"
      << "surplusComp  = <span class='fm'>" << p.surplusRate << " &times; " << p.feeHedgingCoefficient << " &times; " << p.deltaTime << "</span>"
      << " = <span class='vl'>" << surpComp << "</span>\n"
      << "feeTimeComp  = <span class='fm'>" << p.feeSpread << " &times; " << p.feeHedgingCoefficient << " &times; " << p.deltaTime << "</span>"
      << " = <span class='vl'>" << feeComp << "</span>\n"
      << "<span class='rs'>effective</span>    = " << oh << " + " << surpComp << " + " << feeComp
      << " = <span class='rs'>" << eo << " (" << (eo * 100.0) << "%)</span>\n";
    if (p.maxRisk > 0.0)
        c << "maxRisk      = <span class='vl'>" << p.maxRisk << " (" << (p.maxRisk * 100.0) << "%)</span> (TP ceiling per entry)\n";
    if (p.minRisk > 0.0)
        c << "minRisk      = <span class='vl'>" << p.minRisk << " (" << (p.minRisk * 100.0) << "%)</span> (TP floor above BE)\n";
    return c.str();
}

inline std::string traceProfit(const Trade& t, double curPrice,
                               double buyFees, double sellFees,
                               const ProfitResult& r)
{
    std::ostringstream c;
    c << std::fixed << std::setprecision(8);
    bool isBuy = (t.type == TradeType::Buy);
    c << "<span class='hd'>Profit</span>"
      << "trade #" << t.tradeId << " " << t.symbol << " " << (isBuy ? "BUY" : "SELL")
      << "  entry=<span class='vl'>" << t.value << "</span>"
      << "  qty=<span class='vl'>" << t.quantity << "</span>"
      << "  current=<span class='vl'>" << curPrice << "</span>\n\n";
    if (isBuy)
        c << "<span class='fm'>grossProfit</span>  = (current - entry) &times; qty\n"
          << "               = (" << curPrice << " - " << t.value << ") &times; " << t.quantity
          << " = <span class='vl'>" << r.grossProfit << "</span>\n";
    else
        c << "<span class='fm'>grossProfit</span>  = (entry - current) &times; qty\n"
          << "               = (" << t.value << " - " << curPrice << ") &times; " << t.quantity
          << " = <span class='vl'>" << r.grossProfit << "</span>\n";
    double cost = t.value * t.quantity + buyFees;
    c << "<span class='fm'>netProfit</span>    = gross - buyFees - sellFees\n"
      << "               = " << r.grossProfit << " - " << buyFees << " - " << sellFees
      << " = <span class='rs'>" << r.netProfit << "</span>\n"
      << "<span class='fm'>cost</span>         = entry &times; qty + buyFees\n"
      << "               = " << t.value << " &times; " << t.quantity << " + " << buyFees
      << " = <span class='vl'>" << cost << "</span>\n"
      << "<span class='fm'>ROI</span>          = (net / cost) &times; 100\n"
      << "               = (" << r.netProfit << " / " << cost << ") &times; 100"
      << " = <span class='rs'>" << r.roi << "%</span>\n";
    return c.str();
}

inline std::string wrap(const std::string& title, const std::string& body)
{
    return "<!DOCTYPE html><html><head><meta charset='utf-8'>"
           "<meta name='viewport' content='width=device-width,initial-scale=1'>"
           "<title>" + esc(title) + " - Quant</title>" + css() +
           "</head><body>" + nav() +
           "<div class='container'>" + body + "</div></body></html>";
}

inline std::string msgBanner(const httplib::Request& req)
{
    if (!req.has_param("msg")) return "";
    return "<div class='msg'>" + esc(req.get_param_value("msg")) + "</div>";
}

inline std::string errBanner(const httplib::Request& req)
{
    if (!req.has_param("err")) return "";
    return "<div class='msg err'>" + esc(req.get_param_value("err")) + "</div>";
}

} // namespace html

// ---- Form body parser ----
inline std::string urlDec(const std::string& s)
{
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
}

inline std::string urlEnc(const std::string& s)
{
    std::ostringstream o;
    for (unsigned char c : s)
    {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
            o << c;
        else
            o << '%' << "0123456789ABCDEF"[c >> 4] << "0123456789ABCDEF"[c & 0xF];
    }
    return o.str();
}

inline std::map<std::string, std::string> parseForm(const std::string& body)
{
    std::map<std::string, std::string> m;
    std::istringstream ss(body);
    std::string pair;
    while (std::getline(ss, pair, '&'))
    {
        auto eq = pair.find('=');
        if (eq != std::string::npos)
            m[urlDec(pair.substr(0, eq))] = urlDec(pair.substr(eq + 1));
    }
    return m;
}

inline std::string fv(const std::map<std::string, std::string>& f, const std::string& k, const std::string& d = "")
{
    auto it = f.find(k);
    return it != f.end() ? it->second : d;
}

inline double fd(const std::map<std::string, std::string>& f, const std::string& k, double d = 0.0)
{
    auto s = fv(f, k);
    if (s.empty()) return d;
    try { return std::stod(s); } catch (...) { return d; }
}

inline int fi(const std::map<std::string, std::string>& f, const std::string& k, int d = 0)
{
    auto s = fv(f, k);
    if (s.empty()) return d;
    try { return std::stoi(s); } catch (...) { return d; }
}
