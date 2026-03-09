// ============================================================
// quant_mcp.cpp — Shared MCP request handler
//
// Transport-agnostic JSON-RPC 2.0 handler for MCP.
// Callers provide raw JSON request strings and receive
// raw JSON response strings.  All tool implementations
// and preset management live here.
//
// Used by quant-mcp (stdio) and quant-app (HTTP daemon).
// ============================================================

#ifndef QUANT_MCP_H
#include "quant_mcp.h"
#endif
#ifndef QUANT_ENGINE_H
#include "quant_engine.h"
#endif
#ifndef QUANT_ENGINE_TYPES_H
#include "quant_engine_types.h"
#endif
#ifndef LIBQUANT_QUANTMATH_H
#include "quantmath.h"
#endif

#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <cstring>
#include <cstdlib>
#include <random>
#include <chrono>
#include <algorithm>
#include <iomanip>
#include <fstream>

// ============================================================
// Minimal JSON helpers (self-contained, no nanojson dependency)
//
// MCP messages are small objects — we only need to parse
// incoming JSON-RPC requests and build response objects.
// ============================================================

static std::string jEsc(const std::string& s)
{
    std::string o;
    for (char c : s)
    {
        if (c == '"')       o += "\\\"";
        else if (c == '\\') o += "\\\\";
        else if (c == '\n') o += "\\n";
        else if (c == '\r') o += "\\r";
        else if (c == '\t') o += "\\t";
        else                o += c;
    }
    return o;
}

static std::string jStr(const std::string& s)  { return "\"" + jEsc(s) + "\""; }
static std::string jInt(int v)                  { return std::to_string(v); }
static std::string jLong(long long v)           { return std::to_string(v); }
static std::string jBool(bool v)                { return v ? "true" : "false"; }

static std::string jDbl(double v)
{
    std::ostringstream ss;
    ss << std::setprecision(10) << v;
    return ss.str();
}

// Build a JSON object from key-value pairs
class JObj
{
    std::vector<std::pair<std::string, std::string>> m_kv;
public:
    JObj& add(const std::string& k, const std::string& rawJson)
    {
        m_kv.push_back({k, rawJson});
        return *this;
    }
    std::string str() const
    {
        std::string o = "{";
        for (size_t i = 0; i < m_kv.size(); ++i)
        {
            if (i > 0) o += ",";
            o += jStr(m_kv[i].first) + ":" + m_kv[i].second;
        }
        return o + "}";
    }
};

// Build a JSON array from raw JSON strings
class JArr
{
    std::vector<std::string> m_items;
public:
    JArr& add(const std::string& rawJson) { m_items.push_back(rawJson); return *this; }
    std::string str() const
    {
        std::string o = "[";
        for (size_t i = 0; i < m_items.size(); ++i)
        {
            if (i > 0) o += ",";
            o += m_items[i];
        }
        return o + "]";
    }
    bool empty() const { return m_items.empty(); }
    size_t size() const { return m_items.size(); }
};

// ============================================================
// Minimal JSON parser (enough for JSON-RPC requests)
// ============================================================

static void skipWs(const std::string& s, size_t& i)
{
    while (i < s.size() && (s[i] == ' ' || s[i] == '\t' || s[i] == '\n' || s[i] == '\r')) ++i;
}

static std::string parseString(const std::string& s, size_t& i)
{
    if (i >= s.size() || s[i] != '"') return "";
    ++i;
    std::string out;
    while (i < s.size() && s[i] != '"')
    {
        if (s[i] == '\\' && i + 1 < s.size())
        {
            ++i;
            if      (s[i] == '"')  out += '"';
            else if (s[i] == '\\') out += '\\';
            else if (s[i] == 'n')  out += '\n';
            else if (s[i] == 'r')  out += '\r';
            else if (s[i] == 't')  out += '\t';
            else                   { out += '\\'; out += s[i]; }
        }
        else out += s[i];
        ++i;
    }
    if (i < s.size()) ++i; // skip closing "
    return out;
}

// Simple recursive-descent: returns a map of key -> raw JSON string
// Only parses one level of object. Nested objects are returned as raw strings.
static std::map<std::string, std::string> parseObj(const std::string& s, size_t& i)
{
    std::map<std::string, std::string> result;
    skipWs(s, i);
    if (i >= s.size() || s[i] != '{') return result;
    ++i;
    while (i < s.size())
    {
        skipWs(s, i);
        if (s[i] == '}') { ++i; break; }
        if (s[i] == ',') { ++i; continue; }
        std::string key = parseString(s, i);
        skipWs(s, i);
        if (i < s.size() && s[i] == ':') ++i;
        skipWs(s, i);
        // Capture raw value
        size_t start = i;
        if (s[i] == '"')
        {
            parseString(s, i); // advance past string
            result[key] = s.substr(start, i - start);
        }
        else if (s[i] == '{')
        {
            int depth = 0;
            while (i < s.size())
            {
                if (s[i] == '{') ++depth;
                else if (s[i] == '}') { --depth; if (depth == 0) { ++i; break; } }
                else if (s[i] == '"') { parseString(s, i); continue; }
                ++i;
            }
            result[key] = s.substr(start, i - start);
        }
        else if (s[i] == '[')
        {
            int depth = 0;
            while (i < s.size())
            {
                if (s[i] == '[') ++depth;
                else if (s[i] == ']') { --depth; if (depth == 0) { ++i; break; } }
                else if (s[i] == '"') { parseString(s, i); continue; }
                ++i;
            }
            result[key] = s.substr(start, i - start);
        }
        else
        {
            // number, bool, null
            while (i < s.size() && s[i] != ',' && s[i] != '}' && s[i] != ' ' &&
                   s[i] != '\n' && s[i] != '\r' && s[i] != '\t') ++i;
            result[key] = s.substr(start, i - start);
        }
    }
    return result;
}

static std::string getStr(const std::map<std::string, std::string>& m, const std::string& k, const std::string& def = "")
{
    auto it = m.find(k);
    if (it == m.end()) return def;
    const auto& v = it->second;
    if (v.size() >= 2 && v.front() == '"' && v.back() == '"')
    {
        size_t pos = 0;
        return parseString(v, pos);
    }
    return v;
}

static double getDbl(const std::map<std::string, std::string>& m, const std::string& k, double def = 0.0)
{
    auto it = m.find(k);
    if (it == m.end()) return def;
    try { return std::stod(it->second); } catch (...) { return def; }
}

static int getInt(const std::map<std::string, std::string>& m, const std::string& k, int def = 0)
{
    auto it = m.find(k);
    if (it == m.end()) return def;
    try { return std::stoi(it->second); } catch (...) { return def; }
}

static bool getBool(const std::map<std::string, std::string>& m, const std::string& k, bool def = false)
{
    auto it = m.find(k);
    if (it == m.end()) return def;
    return it->second == "true";
}

static std::map<std::string, std::string> getObj(const std::map<std::string, std::string>& m, const std::string& k)
{
    auto it = m.find(k);
    if (it == m.end()) return {};
    size_t pos = 0;
    return parseObj(it->second, pos);
}

// ============================================================
// Confirmation tokens for write operations
// ============================================================

struct PendingAction
{
    std::string token;
    std::string action; // "buy" | "sell" | "fill_entry"
    std::string symbol;
    double      price    = 0;
    double      qty      = 0;
    int         entryId  = 0;
    int         exitId   = 0;
    std::string description;
};

static std::map<std::string, PendingAction> g_pending;

static std::string genToken()
{
    static thread_local std::mt19937 rng(
        static_cast<unsigned>(std::chrono::steady_clock::now().time_since_epoch().count()));
    static const char chars[] = "abcdefghijklmnopqrstuvwxyz0123456789";
    std::uniform_int_distribution<int> dist(0, sizeof(chars) - 2);
    std::string s;
    for (int i = 0; i < 24; ++i) s += chars[dist(rng)];
    return s;
}

// ============================================================
// Parameter presets (file-backed, user-editable)
//
// Stored as presets.json in the --db directory.
// Seeded with built-in defaults on first run.
// Managed via save_preset / delete_preset MCP tools,
// or by hand-editing the JSON file.
// ============================================================

using Args = std::map<std::string, std::string>;

struct Preset
{
    std::string description;
    Args        params;         // key -> raw JSON value
};

static std::map<std::string, Preset> g_presets;
static std::string g_dbDir;

static std::string presetsFilePath()
{
    return g_dbDir + "/presets.json";
}

static void savePresetsToFile()
{
    JObj root;
    for (const auto& [name, preset] : g_presets)
    {
        JObj obj;
        obj.add("description", jStr(preset.description));
        for (const auto& [k, v] : preset.params)
            obj.add(k, v);
        root.add(name, obj.str());
    }
    std::ofstream f(presetsFilePath());
    if (f.is_open())
        f << root.str();
}

static void loadPresetsFromFile()
{
    std::ifstream f(presetsFilePath());
    if (!f.is_open()) return;
    std::string content((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());
    if (content.empty()) return;
    size_t pos = 0;
    auto root = parseObj(content, pos);
    g_presets.clear();
    for (const auto& [name, rawJson] : root)
    {
        size_t p2 = 0;
        auto inner = parseObj(rawJson, p2);
        Preset preset;
        preset.description = getStr(inner, "description");
        inner.erase("description");
        preset.params = std::move(inner);
        g_presets[name] = std::move(preset);
    }
}

static void seedDefaultPresets()
{
    auto add = [](const std::string& name, const std::string& desc,
                  std::initializer_list<std::pair<std::string, std::string>> kv) {
        if (g_presets.count(name)) return;
        Preset p;
        p.description = desc;
        for (const auto& e : kv)
            p.params[e.first] = e.second;
        g_presets[name] = std::move(p);
    };

    add("conservative",
        "Low-risk: tight levels, sigmoid-weighted funding near current price, "
        "stop-losses enabled, high savings rate.",
        {{"levels","4"}, {"exitLevels","4"}, {"risk","0.15"}, {"steepness","6"},
         {"exitSteepness","4"}, {"exitFraction","1"}, {"exitRisk","0.5"},
         {"feeSpread","0.001"}, {"surplusRate","0.015"}, {"savingsRate","0.1"},
         {"maxRisk","0.03"}, {"minRisk","0.005"},
         {"generateStopLosses","true"}, {"stopLossFraction","1"},
         {"feeHedging","1.5"}, {"coefficientK","0"}});

    add("moderate",
        "Balanced: medium level count, uniform-ish funding, moderate surplus and savings.",
        {{"levels","6"}, {"exitLevels","6"}, {"risk","0.5"}, {"steepness","6"},
         {"exitSteepness","4"}, {"exitFraction","1"}, {"exitRisk","0.5"},
         {"feeSpread","0.001"}, {"surplusRate","0.02"}, {"savingsRate","0.05"},
         {"maxRisk","0.05"}, {"minRisk","0.01"},
         {"generateStopLosses","false"}, {"stopLossFraction","1"},
         {"feeHedging","1"}, {"coefficientK","0"}});

    add("aggressive",
        "High-risk: many deep levels, inverse-sigmoid funding, low savings, no stop-losses.",
        {{"levels","10"}, {"exitLevels","10"}, {"risk","0.85"}, {"steepness","8"},
         {"exitSteepness","5"}, {"exitFraction","1"}, {"exitRisk","0.5"},
         {"feeSpread","0.001"}, {"surplusRate","0.03"}, {"savingsRate","0.02"},
         {"maxRisk","0.1"}, {"minRisk","0.01"},
         {"generateStopLosses","false"}, {"stopLossFraction","1"},
         {"feeHedging","1"}, {"coefficientK","0"}});

    add("scalper",
        "Quick-turnaround: few levels, narrow spread, small surplus, fast exits.",
        {{"levels","3"}, {"exitLevels","3"}, {"risk","0.3"}, {"steepness","4"},
         {"exitSteepness","3"}, {"exitFraction","1"}, {"exitRisk","0.5"},
         {"feeSpread","0.0005"}, {"surplusRate","0.01"}, {"savingsRate","0.02"},
         {"maxRisk","0.02"}, {"minRisk","0.005"},
         {"generateStopLosses","true"}, {"stopLossFraction","0.5"},
         {"feeHedging","1"}, {"coefficientK","0"}});

    add("dca",
        "Dollar-cost averaging: many levels, uniform funding, steep sigmoid for wide coverage.",
        {{"levels","12"}, {"exitLevels","12"}, {"risk","0.5"}, {"steepness","10"},
         {"exitSteepness","4"}, {"exitFraction","1"}, {"exitRisk","0.5"},
         {"feeSpread","0.001"}, {"surplusRate","0.02"}, {"savingsRate","0.05"},
         {"maxRisk","0.05"}, {"minRisk","0.01"},
         {"generateStopLosses","false"}, {"stopLossFraction","1"},
         {"feeHedging","1"}, {"coefficientK","0"}});

    add("chain",
        "Chain-optimized: hedging 2x, additive K=1, 10% savings for multi-cycle reinvestment.",
        {{"levels","4"}, {"exitLevels","4"}, {"risk","0.5"}, {"steepness","6"},
         {"exitSteepness","4"}, {"exitFraction","1"}, {"exitRisk","0.5"},
         {"feeSpread","0.001"}, {"surplusRate","0.02"}, {"savingsRate","0.1"},
         {"maxRisk","0.05"}, {"minRisk","0.01"},
         {"generateStopLosses","false"}, {"stopLossFraction","1"},
         {"feeHedging","2"}, {"coefficientK","1"}});

    add("short",
        "Short-selling: inverse entry direction, stop-losses above entry.",
        {{"levels","6"}, {"exitLevels","6"}, {"risk","0.5"}, {"steepness","6"}, {"isShort","true"},
         {"exitSteepness","4"}, {"exitFraction","1"}, {"exitRisk","0.5"},
         {"feeSpread","0.001"}, {"surplusRate","0.02"}, {"savingsRate","0.05"},
         {"maxRisk","0.05"}, {"minRisk","0.01"},
         {"generateStopLosses","true"}, {"stopLossFraction","1"},
         {"feeHedging","1"}, {"coefficientK","0"}});
}

// Apply preset defaults into an Args map — explicit args take priority
static Args applyPreset(const Args& args)
{
    std::string presetName = getStr(args, "preset");
    if (presetName.empty()) return args;

    auto it = g_presets.find(presetName);
    if (it == g_presets.end()) return args;

    Args merged = args;
    for (const auto& [key, val] : it->second.params)
    {
        if (merged.find(key) == merged.end())
            merged[key] = val;
    }
    return merged;
}

// ============================================================
// MCP Tool definitions
// ============================================================

struct ToolParam
{
    std::string name;
    std::string type;        // "string" | "number" | "integer" | "boolean"
    std::string description;
    bool        required = false;
};

struct ToolDef
{
    std::string name;
    std::string description;
    std::vector<ToolParam> params;
};

static std::vector<ToolDef> buildToolDefs()
{
    std::vector<ToolDef> tools;

    tools.push_back({"list_presets",
        "List all available parameter presets for serial plan generation, chain planning, "
        "and what-if analysis. Each preset provides tuned defaults for levels, risk, steepness, "
        "fees, surplus, savings, and stop-loss settings. Use a preset name with generate_serial_plan, "
        "what_if, or compare_plans to avoid specifying every parameter.",
        {}});

    tools.push_back({"save_preset",
        "Create or update a named parameter preset. Provide the preset name and any strategy "
        "parameters to store. When updating an existing preset, only the provided parameters are "
        "changed (existing params are preserved). The preset is saved to presets.json.",
        {{"name", "string", "Preset name (required, e.g. 'my_btc_strategy')", true},
         {"description", "string", "Human-readable description of this preset", false},
         {"levels", "integer", "Number of entry levels", false},
         {"exitLevels", "integer", "Number of exit levels per entry (0 = same as levels)", false},
         {"steepness", "number", "Entry sigmoid steepness", false},
         {"risk", "number", "Risk coefficient 0-1 (0=conservative, 1=aggressive)", false},
         {"isShort", "boolean", "Short selling mode", false},
         {"feeSpread", "number", "Fee/spread rate", false},
         {"feeHedging", "number", "Fee hedging coefficient", false},
         {"deltaTime", "number", "Time delta between trades", false},
         {"symbolCount", "integer", "Number of symbols in portfolio", false},
         {"coefficientK", "number", "Additive offset K", false},
         {"surplusRate", "number", "Surplus profit rate above breakeven", false},
         {"savingsRate", "number", "Fraction of profit to save", false},
         {"maxRisk", "number", "Maximum risk factor for exit horizon", false},
         {"minRisk", "number", "Minimum risk factor", false},
         {"exitRisk", "number", "Exit risk coefficient", false},
         {"exitFraction", "number", "Fraction of position to sell per exit", false},
         {"exitSteepness", "number", "Exit sigmoid steepness", false},
         {"generateStopLosses", "boolean", "Generate stop-loss levels", false},
         {"stopLossFraction", "number", "Stop-loss fraction of position", false},
         {"rangeAbove", "number", "Price range above current price", false},
         {"rangeBelow", "number", "Price range below current price", false},
         {"rangeAbovePerDt", "number", "Range-above drift rate per deltaTime (widens upside per cycle)", false},
         {"rangeBelowPerDt", "number", "Range-below drift rate per deltaTime (widens downside per cycle)", false}}});

    tools.push_back({"delete_preset",
        "Delete a parameter preset by name. The change is saved to presets.json.",
        {{"name", "string", "Preset name to delete", true}}});

    tools.push_back({"portfolio_status",
        "Get wallet balance, deployed capital, and all open positions with remaining quantities.",
        {}});

    tools.push_back({"list_trades",
        "List all trades (buys and sells) in the ledger.",
        {}});

    tools.push_back({"list_entries",
        "List all entry points with their fill status, TP/SL levels, and funding.",
        {}});

    tools.push_back({"list_exits",
        "List all exit strategy points with TP/SL prices, quantities, and execution status.",
        {}});

    tools.push_back({"list_chains",
        "List all managed chains with their status, capital, cycle, and savings.",
        {}});

    tools.push_back({"price_check",
        "Check a symbol at a given price. Returns fillable entries, triggered exits (TP/SL), "
        "open position P&L, and active chain events.",
        {{"symbol", "string", "The trading symbol (e.g. BTC, ETH)", true},
         {"price", "number", "Current market price", true}}});

    tools.push_back({"calculate_profit",
        "Calculate P&L for a specific trade at a given current price.",
        {{"tradeId", "integer", "The trade ID", true},
         {"currentPrice", "number", "Current market price for P&L calculation", true}}});

    tools.push_back({"generate_serial_plan",
        "Generate a serial entry plan with sigmoid-distributed price levels, "
        "funding allocation, take-profits, and stop-losses. "
        "Use 'preset' to load defaults from a named preset (see list_presets); "
        "any explicit parameter overrides the preset value.",
        {{"preset", "string", "Named preset (conservative, moderate, aggressive, scalper, dca, chain, short). Provides defaults for levels, risk, fees, etc.", false},
         {"currentPrice", "number", "Current market price", true},
         {"quantity", "number", "Base quantity per level (default 1)", false},
         {"levels", "integer", "Number of entry levels (default from preset or 4)", false},
         {"availableFunds", "number", "Total funds to distribute", true},
         {"risk", "number", "Risk coefficient 0-1 (0=conservative, 1=aggressive)", false},
         {"steepness", "number", "Entry sigmoid steepness (default 6)", false},
         {"exitLevels", "integer", "Number of exit levels per entry (default=levels)", false},
         {"exitFraction", "number", "Fraction of position to sell per exit (default 1)", false},
         {"exitSteepness", "number", "Exit sigmoid steepness (default 4)", false},
         {"feeSpread", "number", "Fee/spread rate (default 0.001)", false},
         {"surplusRate", "number", "Surplus profit rate above breakeven (default 0.02)", false},
         {"savingsRate", "number", "Fraction of profit to save (default 0.05)", false},
         {"isShort", "boolean", "Short selling plan (default false)", false},
         {"generateStopLosses", "boolean", "Generate SL levels (default false)", false},
         {"stopLossFraction", "number", "SL fraction of position (default 1)", false},
         {"rangeAbove", "number", "Price range above current price (0 = auto)", false},
         {"rangeBelow", "number", "Price range below current price (0 = auto)", false},
         {"rangeAbovePerDt", "number", "Range-above drift rate per deltaTime (widens upside per cycle)", false},
         {"rangeBelowPerDt", "number", "Range-below drift rate per deltaTime (widens downside per cycle)", false}}});

    tools.push_back({"compute_overhead",
        "Compute raw and effective overhead (real cost of a trade including fees, "
        "hedging, time decay, opportunity cost).",
        {{"price", "number", "Asset price per unit", true},
         {"quantity", "number", "Trade quantity (default 1)", false},
         {"feeSpread", "number", "Fee spread rate (default 0.001)", false},
         {"feeHedging", "number", "Fee hedging coefficient (default 1)", false},
         {"deltaTime", "number", "Time delta (default 1)", false},
         {"symbolCount", "integer", "Number of symbols in portfolio (default 1)", false},
         {"availableFunds", "number", "Available funds for denominator", true},
         {"coefficientK", "number", "Additive offset K (default 0)", false},
         {"surplusRate", "number", "Surplus rate (default 0.02)", false}}});

    tools.push_back({"what_if",
        "Hypothetical trade analysis: compute entries, exits, overhead, and breakeven "
        "for a potential trade WITHOUT modifying the database. "
        "Use 'preset' to load defaults from a named preset (see list_presets).",
        {{"preset", "string", "Named preset (conservative, moderate, aggressive, scalper, dca, chain, short)", false},
         {"symbol", "string", "Symbol", true},
         {"price", "number", "Entry price", true},
         {"quantity", "number", "Quantity (default 1)", false},
         {"availableFunds", "number", "Available funds", true},
         {"levels", "integer", "Entry levels (default 4)", false},
         {"risk", "number", "Risk 0-1 (default 0.5)", false},
         {"feeSpread", "number", "Fee rate (default 0.001)", false},
         {"surplusRate", "number", "Surplus (default 0.02)", false}}});

    tools.push_back({"position_risk_summary",
        "Aggregate risk summary: total exposure per symbol, weighted average entry, "
        "unrealized P&L at given prices, max drawdown if all stop-losses hit.",
        {{"prices", "string", "Comma-separated symbol:price pairs, e.g. 'BTC:67400,ETH:2650'", true}}});

    tools.push_back({"compare_plans",
        "Generate two serial plans with different parameters side by side. "
        "Use 'preset_a'/'preset_b' to load named presets for each plan; "
        "explicit levels_a/risk_a/levels_b/risk_b override preset values.",
        {{"currentPrice", "number", "Current price", true},
         {"availableFunds", "number", "Funds to deploy", true},
         {"preset_a", "string", "Named preset for plan A (see list_presets)", false},
         {"levels_a", "integer", "Levels for plan A (default from preset or 4)", false},
         {"risk_a", "number", "Risk for plan A (default from preset or 0.5)", false},
         {"preset_b", "string", "Named preset for plan B (see list_presets)", false},
         {"levels_b", "integer", "Levels for plan B (default from preset or 4)", false},
         {"risk_b", "number", "Risk for plan B (default from preset or 0.5)", false},
         {"quantity", "number", "Quantity (default 1)", false},
         {"feeSpread", "number", "Fee spread (default 0.001)", false}}});

    tools.push_back({"execute_buy",
        "Request to execute a buy trade (debits wallet). Returns a confirmation token "
        "that must be passed to confirm_execution.",
        {{"symbol", "string", "Symbol to buy", true},
         {"price", "number", "Buy price per unit", true},
         {"quantity", "number", "Quantity to buy", true},
         {"buyFee", "number", "Buy fee (default 0)", false}}});

    tools.push_back({"execute_sell",
        "Request to execute a sell (credits wallet). Returns a confirmation token.",
        {{"symbol", "string", "Symbol to sell", true},
         {"price", "number", "Sell price per unit", true},
         {"quantity", "number", "Quantity to sell", true},
         {"sellFee", "number", "Sell fee (default 0)", false}}});

    tools.push_back({"fill_entry",
        "Request to fill a pending entry point (execute buy at entry price, "
        "auto-create exit points). Returns confirmation token.",
        {{"entryId", "integer", "Entry point ID to fill", true}}});

    tools.push_back({"execute_exit",
        "Request to execute a pending exit point (sell at TP or SL price). "
        "Returns confirmation token.",
        {{"exitId", "integer", "Exit point ID to execute", true},
         {"atStopLoss", "boolean", "Execute at SL instead of TP (default false)", false}}});

    tools.push_back({"confirm_execution",
        "Confirm a previously requested trade execution using its confirmation token.",
        {{"token", "string", "Confirmation token from execute_buy/sell/fill_entry/execute_exit", true}}});

    tools.push_back({"wallet_deposit",
        "Deposit funds into the wallet.",
        {{"amount", "number", "Amount to deposit", true}}});

    tools.push_back({"wallet_withdraw",
        "Withdraw funds from the wallet.",
        {{"amount", "number", "Amount to withdraw", true}}});

    return tools;
}

// ============================================================
// Tool implementations
// ============================================================

static std::string toolListPresets(QEngine*, const Args&)
{
    JArr arr;
    for (const auto& [name, preset] : g_presets)
    {
        JObj o;
        o.add("name", jStr(name))
         .add("description", jStr(preset.description));
        for (const auto& [k, v] : preset.params)
            o.add(k, v);
        arr.add(o.str());
    }
    return arr.str();
}

static std::string toolSavePreset(QEngine*, const Args& a)
{
    std::string name = getStr(a, "name");
    if (name.empty())
        return "{\"error\":\"Preset name is required\"}";

    bool isNew = (g_presets.find(name) == g_presets.end());
    Preset& preset = g_presets[name];

    auto descIt = a.find("description");
    if (descIt != a.end())
        preset.description = getStr(a, "description");
    else if (isNew)
        preset.description = "Custom preset";

    for (const auto& [k, v] : a)
    {
        if (k == "name" || k == "description") continue;
        preset.params[k] = v;
    }

    savePresetsToFile();

    JObj r;
    r.add("saved", jBool(true))
     .add("name", jStr(name))
     .add("isNew", jBool(isNew))
     .add("paramCount", jInt(static_cast<int>(preset.params.size())));
    return r.str();
}

static std::string toolDeletePreset(QEngine*, const Args& a)
{
    std::string name = getStr(a, "name");
    if (name.empty())
        return "{\"error\":\"Preset name is required\"}";

    auto it = g_presets.find(name);
    if (it == g_presets.end())
        return "{\"error\":\"Preset not found\"}";

    g_presets.erase(it);
    savePresetsToFile();

    JObj r;
    r.add("deleted", jBool(true)).add("name", jStr(name));
    return r.str();
}

static std::string toolPortfolioStatus(QEngine* e, const Args&)
{
    QWalletInfo w = qe_wallet_info(e);
    int n = qe_trade_count(e);
    std::vector<QTrade> trades(n > 0 ? n : 1);
    n = qe_trade_list(e, trades.data(), n);

    JObj wallet;
    wallet.add("liquid", jDbl(w.balance))
          .add("deployed", jDbl(w.deployed))
          .add("total", jDbl(w.total));

    JArr positions;
    for (int i = 0; i < n; ++i)
    {
        const auto& t = trades[i];
        if (t.type != Q_BUY) continue;
        double sold = qe_trade_sold_qty(e, t.tradeId);
        double rem = t.quantity - sold;
        if (rem <= 0) continue;
        JObj pos;
        pos.add("tradeId", jInt(t.tradeId))
           .add("symbol", jStr(t.symbol))
           .add("entryPrice", jDbl(t.value))
           .add("quantity", jDbl(t.quantity))
           .add("sold", jDbl(sold))
           .add("remaining", jDbl(rem))
           .add("cost", jDbl(t.value * rem));
        positions.add(pos.str());
    }

    JObj result;
    result.add("wallet", wallet.str())
          .add("openPositions", positions.str())
          .add("totalTrades", jInt(n));
    return result.str();
}

static std::string toolListTrades(QEngine* e, const Args&)
{
    int n = qe_trade_count(e);
    std::vector<QTrade> trades(n > 0 ? n : 1);
    n = qe_trade_list(e, trades.data(), n);

    JArr arr;
    for (int i = 0; i < n; ++i)
    {
        const auto& t = trades[i];
        JObj o;
        o.add("tradeId", jInt(t.tradeId))
         .add("symbol", jStr(t.symbol))
         .add("type", jStr(t.type == Q_BUY ? "BUY" : "SELL"))
         .add("price", jDbl(t.value))
         .add("quantity", jDbl(t.quantity))
         .add("buyFees", jDbl(t.buyFees))
         .add("sellFees", jDbl(t.sellFees))
         .add("timestamp", jLong(t.timestamp));
        if (t.type == Q_BUY)
        {
            double sold = qe_trade_sold_qty(e, t.tradeId);
            o.add("sold", jDbl(sold))
             .add("remaining", jDbl(t.quantity - sold));
        }
        arr.add(o.str());
    }
    return arr.str();
}

static std::string toolListEntries(QEngine* e, const Args&)
{
    int n = qe_entry_count(e);
    std::vector<QEntryPoint> entries(n > 0 ? n : 1);
    n = qe_entry_list(e, entries.data(), n);

    JArr arr;
    for (int i = 0; i < n; ++i)
    {
        const auto& ep = entries[i];
        JObj o;
        o.add("entryId", jInt(ep.entryId))
         .add("symbol", jStr(ep.symbol))
         .add("levelIndex", jInt(ep.levelIndex))
         .add("entryPrice", jDbl(ep.entryPrice))
         .add("breakEven", jDbl(ep.breakEven))
         .add("funding", jDbl(ep.funding))
         .add("fundingQty", jDbl(ep.fundingQty))
         .add("effectiveOverhead", jDbl(ep.effectiveOverhead))
         .add("isShort", jBool(ep.isShort != 0))
         .add("traded", jBool(ep.traded != 0))
         .add("linkedTradeId", jInt(ep.linkedTradeId))
         .add("exitTakeProfit", jDbl(ep.exitTakeProfit))
         .add("exitStopLoss", jDbl(ep.exitStopLoss));
        arr.add(o.str());
    }
    return arr.str();
}

static std::string toolListExits(QEngine* e, const Args&)
{
    int n = qe_exitpt_count(e);
    std::vector<QExitPointData> exits(n > 0 ? n : 1);
    n = qe_exitpt_list(e, exits.data(), n);

    JArr arr;
    for (int i = 0; i < n; ++i)
    {
        const auto& xp = exits[i];
        JObj o;
        o.add("exitId", jInt(xp.exitId))
         .add("tradeId", jInt(xp.tradeId))
         .add("symbol", jStr(xp.symbol))
         .add("levelIndex", jInt(xp.levelIndex))
         .add("tpPrice", jDbl(xp.tpPrice))
         .add("slPrice", jDbl(xp.slPrice))
         .add("sellQty", jDbl(xp.sellQty))
         .add("sellFraction", jDbl(xp.sellFraction))
         .add("slActive", jBool(xp.slActive != 0))
         .add("executed", jBool(xp.executed != 0))
         .add("linkedSellId", jInt(xp.linkedSellId));
        arr.add(o.str());
    }
    return arr.str();
}

static std::string toolListChains(QEngine* e, const Args&)
{
    int n = qe_chain_count(e);
    std::vector<QManagedChain> chains(n > 0 ? n : 1);
    n = qe_chain_list(e, chains.data(), n);

    JArr arr;
    for (int i = 0; i < n; ++i)
    {
        const auto& c = chains[i];
        JObj o;
        o.add("chainId", jInt(c.chainId))
         .add("name", jStr(c.name))
         .add("symbol", jStr(c.symbol))
         .add("active", jBool(c.active != 0))
         .add("theoretical", jBool(c.theoretical != 0))
         .add("currentCycle", jInt(c.currentCycle))
         .add("capital", jDbl(c.capital))
         .add("totalSavings", jDbl(c.totalSavings))
         .add("savingsRate", jDbl(c.savingsRate));
        arr.add(o.str());
    }
    return arr.str();
}

static std::string toolPriceCheck(QEngine* e, const Args& a)
{
    std::string sym = getStr(a, "symbol");
    double price    = getDbl(a, "price");
    if (sym.empty() || price <= 0)
        return "{\"error\":\"symbol and price required\"}";

    QWalletInfo w = qe_wallet_info(e);

    // Fillable entries
    int nE = qe_entry_count(e);
    std::vector<QEntryPoint> entries(nE > 0 ? nE : 1);
    nE = qe_entry_list(e, entries.data(), nE);

    JArr fillable;
    for (int i = 0; i < nE; ++i)
    {
        const auto& ep = entries[i];
        if (ep.traded) continue;
        if (std::string(ep.symbol) != sym) continue;
        bool fill = ep.isShort ? (price >= ep.entryPrice) : (price <= ep.entryPrice);
        if (!fill) continue;
        double cost = ep.entryPrice * ep.fundingQty;
        JObj o;
        o.add("entryId", jInt(ep.entryId))
         .add("entryPrice", jDbl(ep.entryPrice))
         .add("qty", jDbl(ep.fundingQty))
         .add("cost", jDbl(cost))
         .add("canAfford", jBool(cost <= w.balance))
         .add("tp", jDbl(ep.exitTakeProfit))
         .add("sl", jDbl(ep.exitStopLoss));
        fillable.add(o.str());
    }

    // Triggered exits
    int nX = qe_exitpt_count(e);
    std::vector<QExitPointData> exits(nX > 0 ? nX : 1);
    nX = qe_exitpt_list(e, exits.data(), nX);

    int nT = qe_trade_count(e);
    std::vector<QTrade> trades(nT > 0 ? nT : 1);
    nT = qe_trade_list(e, trades.data(), nT);

    JArr triggered;
    for (int i = 0; i < nX; ++i)
    {
        const auto& xp = exits[i];
        if (xp.executed) continue;
        if (std::string(xp.symbol) != sym) continue;
        bool tpHit = (xp.tpPrice > 0 && price >= xp.tpPrice);
        bool slHit = (xp.slActive && xp.slPrice > 0 && price <= xp.slPrice);
        if (!tpHit && !slHit) continue;

        double entryPrice = 0;
        for (int ti = 0; ti < nT; ++ti)
            if (trades[ti].tradeId == xp.tradeId) { entryPrice = trades[ti].value; break; }
        double pnl = (price - entryPrice) * xp.sellQty;

        JObj o;
        o.add("exitId", jInt(xp.exitId))
         .add("tradeId", jInt(xp.tradeId))
         .add("tpHit", jBool(tpHit))
         .add("slHit", jBool(slHit))
         .add("tpPrice", jDbl(xp.tpPrice))
         .add("slPrice", jDbl(xp.slPrice))
         .add("sellQty", jDbl(xp.sellQty))
         .add("pnl", jDbl(pnl));
        triggered.add(o.str());
    }

    // Open positions P&L
    JArr positions;
    for (int i = 0; i < nT; ++i)
    {
        const auto& t = trades[i];
        if (t.type != Q_BUY || std::string(t.symbol) != sym) continue;
        double sold = qe_trade_sold_qty(e, t.tradeId);
        double rem = t.quantity - sold;
        if (rem <= 0) continue;
        double pnl = (price - t.value) * rem;
        double roi = (t.value > 0) ? ((price - t.value) / t.value * 100.0) : 0;
        JObj o;
        o.add("tradeId", jInt(t.tradeId))
         .add("entryPrice", jDbl(t.value))
         .add("remaining", jDbl(rem))
         .add("unrealizedPnl", jDbl(pnl))
         .add("roi", jDbl(roi));
        positions.add(o.str());
    }

    // Active chains
    int nC = qe_chain_count(e);
    std::vector<QManagedChain> chains(nC > 0 ? nC : 1);
    nC = qe_chain_list(e, chains.data(), nC);

    JArr chainEvents;
    for (int i = 0; i < nC; ++i)
    {
        const auto& ch = chains[i];
        if (!ch.active || std::string(ch.symbol) != sym) continue;
        JObj o;
        o.add("chainId", jInt(ch.chainId))
         .add("name", jStr(ch.name))
         .add("cycle", jInt(ch.currentCycle))
         .add("capital", jDbl(ch.capital));
        chainEvents.add(o.str());
    }

    JObj result;
    result.add("symbol", jStr(sym))
          .add("price", jDbl(price))
          .add("walletBalance", jDbl(w.balance))
          .add("fillableEntries", fillable.str())
          .add("triggeredExits", triggered.str())
          .add("openPositions", positions.str())
          .add("activeChains", chainEvents.str());
    return result.str();
}

static std::string toolCalculateProfit(QEngine* e, const Args& a)
{
    int tradeId    = getInt(a, "tradeId");
    double curPrice = getDbl(a, "currentPrice");

    QTrade t{};
    if (!qe_trade_get(e, tradeId, &t))
        return "{\"error\":\"Trade not found\"}";

    QProfitResult pr = qe_profit_calculate(e, tradeId, curPrice);
    JObj result;
    result.add("tradeId", jInt(tradeId))
          .add("symbol", jStr(t.symbol))
          .add("entryPrice", jDbl(t.value))
          .add("quantity", jDbl(t.quantity))
          .add("currentPrice", jDbl(curPrice))
          .add("grossProfit", jDbl(pr.grossProfit))
          .add("netProfit", jDbl(pr.netProfit))
          .add("roi", jDbl(pr.roi));
    return result.str();
}

static std::string toolGenerateSerialPlan(QEngine*, const Args& a)
{
    Args pa = applyPreset(a);
    std::string usedPreset = getStr(a, "preset");

    QSerialParams p{};
    p.currentPrice    = getDbl(pa, "currentPrice");
    p.quantity        = getDbl(pa, "quantity", 1.0);
    p.levels          = getInt(pa, "levels", 4);
    p.exitLevels      = getInt(pa, "exitLevels", p.levels);
    p.steepness       = getDbl(pa, "steepness", 6.0);
    p.risk            = getDbl(pa, "risk", 0.5);
    p.availableFunds  = getDbl(pa, "availableFunds");
    p.feeSpread       = getDbl(pa, "feeSpread", 0.001);
    p.feeHedgingCoefficient = getDbl(pa, "feeHedging", 1.0);
    p.deltaTime       = getDbl(pa, "deltaTime", 1.0);
    p.symbolCount     = getInt(pa, "symbolCount", 1);
    p.coefficientK    = getDbl(pa, "coefficientK", 0.0);
    p.surplusRate     = getDbl(pa, "surplusRate", 0.02);
    p.exitFraction    = getDbl(pa, "exitFraction", 1.0);
    p.exitSteepness   = getDbl(pa, "exitSteepness", 4.0);
    p.exitRisk        = getDbl(pa, "exitRisk", 0.5);
    p.maxRisk         = getDbl(pa, "maxRisk", 0.5);
    p.minRisk         = getDbl(pa, "minRisk", 0.0);
    p.isShort         = getBool(pa, "isShort") ? 1 : 0;
    p.generateStopLosses = getBool(pa, "generateStopLosses") ? 1 : 0;
    p.stopLossFraction = getDbl(pa, "stopLossFraction", 1.0);
    p.savingsRate      = getDbl(pa, "savingsRate", 0.05);
    p.rangeAbove       = getDbl(pa, "rangeAbove", 0.0);
    p.rangeBelow       = getDbl(pa, "rangeBelow", 0.0);
    p.rangeAbovePerDt  = getDbl(pa, "rangeAbovePerDt", 0.0);
    p.rangeBelowPerDt  = getDbl(pa, "rangeBelowPerDt", 0.0);
    p.autoRange        = (p.rangeAbove == 0.0 && p.rangeBelow == 0.0) ? 1 : 0;

    std::vector<QSerialEntry> entries(p.levels > 0 ? p.levels * 4 : 64);
    QSerialPlanSummary sum = qe_serial_generate(&p, entries.data(), static_cast<int>(entries.size()));

    JArr entryArr;
    for (int i = 0; i < sum.entryCount && i < static_cast<int>(entries.size()); ++i)
    {
        const auto& se = entries[i];
        JObj o;
        o.add("level", jInt(se.index))
         .add("entryPrice", jDbl(se.entryPrice))
         .add("breakEven", jDbl(se.breakEven))
         .add("discount", jDbl(se.discountPct))
         .add("funding", jDbl(se.funding))
         .add("fundingQty", jDbl(se.fundQty))
         .add("tpPrice", jDbl(se.tpUnit))
         .add("tpTotal", jDbl(se.tpTotal))
         .add("tpGross", jDbl(se.tpGross))
         .add("slPrice", jDbl(se.slUnit))
         .add("slLoss", jDbl(se.slLoss));
        entryArr.add(o.str());
    }

    // Cycle compute
    QCycleResult cr = qe_cycle_compute(&p);

    JObj result;
    if (!usedPreset.empty())
        result.add("preset", jStr(usedPreset));
    result.add("overhead", jDbl(sum.overhead))
          .add("effectiveOverhead", jDbl(sum.effectiveOH))
          .add("totalFunding", jDbl(sum.totalFunding))
          .add("totalTpGross", jDbl(sum.totalTpGross))
          .add("entryCount", jInt(sum.entryCount))
          .add("entries", entryArr.str())
          .add("cycle", JObj()
               .add("totalCost", jDbl(cr.totalCost))
               .add("totalRevenue", jDbl(cr.totalRevenue))
               .add("totalFees", jDbl(cr.totalFees))
               .add("grossProfit", jDbl(cr.grossProfit))
               .add("savingsAmount", jDbl(cr.savingsAmount))
               .add("reinvestAmount", jDbl(cr.reinvestAmount))
               .add("nextCycleFunds", jDbl(cr.nextCycleFunds)).str());
    return result.str();
}

static std::string toolComputeOverhead(QEngine*, const Args& a)
{
    double price  = getDbl(a, "price");
    double qty    = getDbl(a, "quantity", 1.0);
    double fs     = getDbl(a, "feeSpread", 0.001);
    double fh     = getDbl(a, "feeHedging", 1.0);
    double dt     = getDbl(a, "deltaTime", 1.0);
    int    ns     = getInt(a, "symbolCount", 1);
    double funds  = getDbl(a, "availableFunds");
    double K      = getDbl(a, "coefficientK", 0.0);
    double s      = getDbl(a, "surplusRate", 0.02);

    double oh = qe_overhead(price, qty, fs, fh, dt, ns, funds, K, 0);
    double eo = qe_effective_overhead(oh, s, fs, fh, dt);

    JObj result;
    result.add("rawOverhead", jDbl(oh))
          .add("effectiveOverhead", jDbl(eo))
          .add("breakEven", jDbl(price * (1.0 + oh)))
          .add("tpTarget", jDbl(price * (1.0 + eo)));
    return result.str();
}

static std::string toolWhatIf(QEngine*, const Args& a)
{
    Args pa = applyPreset(a);
    std::string usedPreset = getStr(a, "preset");

    std::string sym = getStr(pa, "symbol");
    double price    = getDbl(pa, "price");
    double qty      = getDbl(pa, "quantity", 1.0);
    double funds    = getDbl(pa, "availableFunds");
    int    levels   = getInt(pa, "levels", 4);
    int    exLevels = getInt(pa, "exitLevels", levels);
    double risk     = getDbl(pa, "risk", 0.5);
    double fs       = getDbl(pa, "feeSpread", 0.001);
    double sr       = getDbl(pa, "surplusRate", 0.02);
    double fh       = getDbl(pa, "feeHedging", 1.0);
    double K        = getDbl(pa, "coefficientK", 0.0);
    double steep    = getDbl(pa, "steepness", 6.0);
    double exFrac   = getDbl(pa, "exitFraction", 1.0);
    double exSteep  = getDbl(pa, "exitSteepness", 4.0);
    double exRisk   = getDbl(pa, "exitRisk", 0.5);
    double maxR     = getDbl(pa, "maxRisk", 0.5);
    double savRate  = getDbl(pa, "savingsRate", 0.05);
    bool   isShort  = getBool(pa, "isShort");
    bool   genSL    = getBool(pa, "generateStopLosses");
    double slFrac   = getDbl(pa, "stopLossFraction", 1.0);

    double oh = qe_overhead(price, qty, fs, fh, 1.0, 1, funds, K, 0);
    double eo = qe_effective_overhead(oh, sr, fs, fh, 1.0);

    QSerialParams p{};
    p.currentPrice = price; p.quantity = qty; p.levels = levels;
    p.exitLevels = exLevels; p.steepness = steep; p.risk = risk;
    p.availableFunds = funds; p.feeSpread = fs; p.surplusRate = sr;
    p.exitFraction = exFrac; p.exitSteepness = exSteep; p.exitRisk = exRisk;
    p.maxRisk = maxR; p.savingsRate = savRate;
    p.feeHedgingCoefficient = fh; p.coefficientK = K;
    p.isShort = isShort ? 1 : 0;
    p.generateStopLosses = genSL ? 1 : 0;
    p.stopLossFraction = slFrac;
    p.rangeAbove      = getDbl(pa, "rangeAbove", 0.0);
    p.rangeBelow      = getDbl(pa, "rangeBelow", 0.0);
    p.rangeAbovePerDt = getDbl(pa, "rangeAbovePerDt", 0.0);
    p.rangeBelowPerDt = getDbl(pa, "rangeBelowPerDt", 0.0);
    p.autoRange       = (p.rangeAbove == 0.0 && p.rangeBelow == 0.0) ? 1 : 0;

    std::vector<QSerialEntry> entries(levels * 4);
    QSerialPlanSummary sum = qe_serial_generate(&p, entries.data(), static_cast<int>(entries.size()));
    QCycleResult cr = qe_cycle_compute(&p);

    JArr entryArr;
    for (int i = 0; i < sum.entryCount && i < static_cast<int>(entries.size()); ++i)
    {
        const auto& se = entries[i];
        JObj o;
        o.add("level", jInt(se.index))
         .add("entry", jDbl(se.entryPrice))
         .add("breakEven", jDbl(se.breakEven))
         .add("funding", jDbl(se.funding))
         .add("qty", jDbl(se.fundQty))
         .add("tp", jDbl(se.tpUnit))
         .add("sl", jDbl(se.slUnit));
        entryArr.add(o.str());
    }

    JObj result;
    if (!usedPreset.empty())
        result.add("preset", jStr(usedPreset));
    result.add("symbol", jStr(sym))
          .add("hypothetical", jBool(true))
          .add("entryPrice", jDbl(price))
          .add("rawOverhead", jDbl(oh))
          .add("effectiveOverhead", jDbl(eo))
          .add("breakEven", jDbl(price * (1.0 + oh)))
          .add("totalFunding", jDbl(sum.totalFunding))
          .add("expectedGrossProfit", jDbl(cr.grossProfit))
          .add("expectedReinvest", jDbl(cr.reinvestAmount))
          .add("entries", entryArr.str());
    return result.str();
}

static std::string toolPositionRiskSummary(QEngine* e, const Args& a)
{
    // Parse "BTC:67400,ETH:2650"
    std::string pricesStr = getStr(a, "prices");
    std::map<std::string, double> priceMap;
    std::istringstream ss(pricesStr);
    std::string token;
    while (std::getline(ss, token, ','))
    {
        auto colon = token.find(':');
        if (colon != std::string::npos)
            priceMap[token.substr(0, colon)] = std::stod(token.substr(colon + 1));
    }

    int nT = qe_trade_count(e);
    std::vector<QTrade> trades(nT > 0 ? nT : 1);
    nT = qe_trade_list(e, trades.data(), nT);

    int nX = qe_exitpt_count(e);
    std::vector<QExitPointData> exits(nX > 0 ? nX : 1);
    nX = qe_exitpt_list(e, exits.data(), nX);

    // Per-symbol aggregation
    struct SymbolAgg {
        double totalQty = 0, weightedEntry = 0, totalCost = 0;
        double unrealizedPnl = 0, maxSlLoss = 0;
        int openCount = 0;
    };
    std::map<std::string, SymbolAgg> agg;

    for (int i = 0; i < nT; ++i)
    {
        const auto& t = trades[i];
        if (t.type != Q_BUY) continue;
        double sold = qe_trade_sold_qty(e, t.tradeId);
        double rem = t.quantity - sold;
        if (rem <= 0) continue;
        std::string sym = t.symbol;
        auto& s = agg[sym];
        s.totalQty += rem;
        s.weightedEntry += t.value * rem;
        s.totalCost += t.value * rem;
        s.openCount++;

        auto pit = priceMap.find(sym);
        if (pit != priceMap.end())
            s.unrealizedPnl += (pit->second - t.value) * rem;

        // SL drawdown for this trade
        for (int xi = 0; xi < nX; ++xi)
        {
            if (exits[xi].tradeId != t.tradeId || exits[xi].executed) continue;
            if (exits[xi].slActive && exits[xi].slPrice > 0)
                s.maxSlLoss += (t.value - exits[xi].slPrice) * exits[xi].sellQty;
        }
    }

    JArr arr;
    for (auto& [sym, s] : agg)
    {
        double avgEntry = s.totalQty > 0 ? s.weightedEntry / s.totalQty : 0;
        JObj o;
        o.add("symbol", jStr(sym))
         .add("openPositions", jInt(s.openCount))
         .add("totalQuantity", jDbl(s.totalQty))
         .add("weightedAvgEntry", jDbl(avgEntry))
         .add("totalExposure", jDbl(s.totalCost))
         .add("unrealizedPnl", jDbl(s.unrealizedPnl))
         .add("maxStopLossDrawdown", jDbl(s.maxSlLoss));
        arr.add(o.str());
    }
    return arr.str();
}

static std::string toolComparePlans(QEngine*, const Args& a)
{
    double price = getDbl(a, "currentPrice");
    double funds = getDbl(a, "availableFunds");
    double qty   = getDbl(a, "quantity", 1.0);
    double fs    = getDbl(a, "feeSpread", 0.001);

    auto makePlan = [&](const std::string& presetName, int levels, double risk) -> std::string {
        // Build a mini Args with the preset and overrides
        Args planArgs;
        if (!presetName.empty())
            planArgs["preset"] = "\"" + presetName + "\"";
        if (levels > 0)
            planArgs["levels"] = std::to_string(levels);
        if (risk >= 0)
            planArgs["risk"] = std::to_string(risk);
        Args pa = applyPreset(planArgs);

        QSerialParams p{};
        p.currentPrice = price; p.quantity = qty;
        p.levels       = getInt(pa, "levels", 4);
        p.exitLevels   = getInt(pa, "exitLevels", p.levels);
        p.steepness    = getDbl(pa, "steepness", 6.0);
        p.risk         = getDbl(pa, "risk", 0.5);
        p.availableFunds = funds;
        p.feeSpread    = getDbl(pa, "feeSpread", fs);
        p.surplusRate  = getDbl(pa, "surplusRate", 0.02);
        p.exitFraction = getDbl(pa, "exitFraction", 1.0);
        p.exitSteepness = getDbl(pa, "exitSteepness", 4.0);
        p.exitRisk     = getDbl(pa, "exitRisk", 0.5);
        p.maxRisk      = getDbl(pa, "maxRisk", 0.5);
        p.autoRange    = 1;
        p.savingsRate  = getDbl(pa, "savingsRate", 0.05);
        p.feeHedgingCoefficient = getDbl(pa, "feeHedging", 1.0);
        p.coefficientK = getDbl(pa, "coefficientK", 0.0);
        p.generateStopLosses = getBool(pa, "generateStopLosses") ? 1 : 0;
        p.stopLossFraction   = getDbl(pa, "stopLossFraction", 1.0);
        p.isShort      = getBool(pa, "isShort") ? 1 : 0;

        std::vector<QSerialEntry> entries(p.levels * 4);
        QSerialPlanSummary sum = qe_serial_generate(&p, entries.data(), static_cast<int>(entries.size()));
        QCycleResult cr = qe_cycle_compute(&p);

        JObj o;
        if (!presetName.empty())
            o.add("preset", jStr(presetName));
        o.add("levels", jInt(p.levels))
         .add("risk", jDbl(p.risk))
         .add("overhead", jDbl(sum.overhead))
         .add("effectiveOH", jDbl(sum.effectiveOH))
         .add("totalFunding", jDbl(sum.totalFunding))
         .add("totalTpGross", jDbl(sum.totalTpGross))
         .add("grossProfit", jDbl(cr.grossProfit))
         .add("reinvest", jDbl(cr.reinvestAmount))
         .add("entryCount", jInt(sum.entryCount));
        return o.str();
    };

    std::string presetA = getStr(a, "preset_a");
    std::string presetB = getStr(a, "preset_b");

    JObj result;
    result.add("planA", makePlan(presetA, getInt(a, "levels_a", -1), getDbl(a, "risk_a", -1)))
          .add("planB", makePlan(presetB, getInt(a, "levels_b", -1), getDbl(a, "risk_b", -1)));
    return result.str();
}

static std::string toolExecuteBuy(QEngine* e, const Args& a)
{
    std::string sym = getStr(a, "symbol");
    double price    = getDbl(a, "price");
    double qty      = getDbl(a, "quantity");
    double fee      = getDbl(a, "buyFee", 0.0);

    QWalletInfo w = qe_wallet_info(e);
    double cost = price * qty + fee;

    PendingAction pa;
    pa.token = genToken();
    pa.action = "buy";
    pa.symbol = sym;
    pa.price = price;
    pa.qty = qty;
    pa.description = "Buy " + std::to_string(qty) + " " + sym + " @ " + std::to_string(price)
                   + " (cost: " + std::to_string(cost) + ", wallet: " + std::to_string(w.balance) + ")";

    g_pending[pa.token] = pa;

    JObj result;
    result.add("confirmationRequired", jBool(true))
          .add("token", jStr(pa.token))
          .add("action", jStr("buy"))
          .add("symbol", jStr(sym))
          .add("price", jDbl(price))
          .add("quantity", jDbl(qty))
          .add("totalCost", jDbl(cost))
          .add("walletBalance", jDbl(w.balance))
          .add("canAfford", jBool(cost <= w.balance))
          .add("description", jStr(pa.description));
    return result.str();
}

static std::string toolExecuteSell(QEngine* e, const Args& a)
{
    std::string sym = getStr(a, "symbol");
    double price    = getDbl(a, "price");
    double qty      = getDbl(a, "quantity");
    double fee      = getDbl(a, "sellFee", 0.0);

    PendingAction pa;
    pa.token = genToken();
    pa.action = "sell";
    pa.symbol = sym;
    pa.price = price;
    pa.qty = qty;
    pa.description = "Sell " + std::to_string(qty) + " " + sym + " @ " + std::to_string(price);

    g_pending[pa.token] = pa;

    JObj result;
    result.add("confirmationRequired", jBool(true))
          .add("token", jStr(pa.token))
          .add("action", jStr("sell"))
          .add("description", jStr(pa.description));
    return result.str();
}

static std::string toolFillEntry(QEngine* e, const Args& a)
{
    int entryId = getInt(a, "entryId");
    QEntryPoint ep{};
    if (!qe_entry_get(e, entryId, &ep))
        return "{\"error\":\"Entry not found\"}";
    if (ep.traded)
        return "{\"error\":\"Entry already filled\"}";

    double cost = ep.entryPrice * ep.fundingQty;
    QWalletInfo w = qe_wallet_info(e);

    PendingAction pa;
    pa.token = genToken();
    pa.action = "fill_entry";
    pa.entryId = entryId;
    pa.symbol = ep.symbol;
    pa.price = ep.entryPrice;
    pa.qty = ep.fundingQty;
    pa.description = "Fill entry #" + std::to_string(entryId) + ": Buy "
                   + std::to_string(ep.fundingQty) + " " + std::string(ep.symbol)
                   + " @ " + std::to_string(ep.entryPrice);

    g_pending[pa.token] = pa;

    JObj result;
    result.add("confirmationRequired", jBool(true))
          .add("token", jStr(pa.token))
          .add("entryId", jInt(entryId))
          .add("symbol", jStr(ep.symbol))
          .add("entryPrice", jDbl(ep.entryPrice))
          .add("qty", jDbl(ep.fundingQty))
          .add("cost", jDbl(cost))
          .add("walletBalance", jDbl(w.balance))
          .add("canAfford", jBool(cost <= w.balance))
          .add("tp", jDbl(ep.exitTakeProfit))
          .add("sl", jDbl(ep.exitStopLoss))
          .add("description", jStr(pa.description));
    return result.str();
}

static std::string toolExecuteExit(QEngine* e, const Args& a)
{
    int exitId   = getInt(a, "exitId");
    bool atSl    = getBool(a, "atStopLoss");

    int total = qe_exitpt_count(e);
    std::vector<QExitPointData> all(total > 0 ? total : 1);
    total = qe_exitpt_list(e, all.data(), total);

    QExitPointData* xp = nullptr;
    for (int i = 0; i < total; ++i)
        if (all[i].exitId == exitId) { xp = &all[i]; break; }
    if (!xp) return "{\"error\":\"Exit not found\"}";
    if (xp->executed) return "{\"error\":\"Exit already executed\"}";

    double price = atSl ? xp->slPrice : xp->tpPrice;

    PendingAction pa;
    pa.token = genToken();
    pa.action = "sell";
    pa.exitId = exitId;
    pa.symbol = xp->symbol;
    pa.price = price;
    pa.qty = xp->sellQty;
    pa.description = std::string(atSl ? "Execute SL" : "Execute TP") + " on exit X"
                   + std::to_string(exitId) + ": Sell " + std::to_string(xp->sellQty)
                   + " " + std::string(xp->symbol) + " @ " + std::to_string(price);

    g_pending[pa.token] = pa;

    JObj result;
    result.add("confirmationRequired", jBool(true))
          .add("token", jStr(pa.token))
          .add("exitId", jInt(exitId))
          .add("atStopLoss", jBool(atSl))
          .add("price", jDbl(price))
          .add("sellQty", jDbl(xp->sellQty))
          .add("description", jStr(pa.description));
    return result.str();
}

#ifndef QUANT_ENGINE_COPYSTR_DEFINED
#define QUANT_ENGINE_COPYSTR_DEFINED
static void copyStr(char* dst, size_t sz, const std::string& src)
{
    size_t len = std::min(src.size(), sz - 1);
    std::memcpy(dst, src.c_str(), len);
    dst[len] = '\0';
}
#endif

static std::string toolConfirmExecution(QEngine* e, const Args& a)
{
    std::string tok = getStr(a, "token");
    auto it = g_pending.find(tok);
    if (it == g_pending.end())
        return "{\"error\":\"Invalid or expired confirmation token\"}";

    PendingAction pa = it->second;
    g_pending.erase(it);

    if (pa.action == "buy")
    {
        int id = qe_execute_buy(e, pa.symbol.c_str(), pa.price, pa.qty, 0);
        if (id < 0) return "{\"error\":\"Buy failed — insufficient wallet balance\"}";
        JObj r;
        r.add("success", jBool(true)).add("tradeId", jInt(id)).add("action", jStr("buy"));
        return r.str();
    }
    else if (pa.action == "sell")
    {
        int id = qe_execute_sell(e, pa.symbol.c_str(), pa.price, pa.qty, 0);
        if (id < 0) return "{\"error\":\"Sell failed\"}";
        // If this was an exit execution, mark it
        if (pa.exitId > 0)
        {
            int total = qe_exitpt_count(e);
            std::vector<QExitPointData> all(total > 0 ? total : 1);
            total = qe_exitpt_list(e, all.data(), total);
            for (int i = 0; i < total; ++i)
                if (all[i].exitId == pa.exitId)
                {
                    all[i].executed = 1;
                    all[i].linkedSellId = id;
                    qe_exitpt_update(e, &all[i]);
                    break;
                }
        }
        JObj r;
        r.add("success", jBool(true)).add("tradeId", jInt(id)).add("action", jStr("sell"));
        return r.str();
    }
    else if (pa.action == "fill_entry")
    {
        int tradeId = qe_execute_buy(e, pa.symbol.c_str(), pa.price, pa.qty, 0);
        if (tradeId < 0) return "{\"error\":\"Buy failed — insufficient wallet balance\"}";

        // Mark entry as traded
        QEntryPoint ep{};
        if (qe_entry_get(e, pa.entryId, &ep))
        {
            ep.traded = 1;
            ep.linkedTradeId = tradeId;
            qe_entry_update(e, &ep);

            // Auto-create exit point from entry TP/SL
            if (ep.exitTakeProfit > 0)
            {
                QExitPointData xp{};
                xp.tradeId = tradeId;
                copyStr(xp.symbol, sizeof(xp.symbol), pa.symbol);
                xp.tpPrice = ep.exitTakeProfit;
                xp.slPrice = ep.exitStopLoss;
                xp.sellQty = pa.qty;
                xp.slActive = (ep.exitStopLoss > 0) ? 1 : 0;
                qe_exitpt_add(e, &xp);
            }
        }
        JObj r;
        r.add("success", jBool(true))
         .add("tradeId", jInt(tradeId))
         .add("entryId", jInt(pa.entryId))
         .add("action", jStr("fill_entry"));
        return r.str();
    }
    return "{\"error\":\"Unknown action\"}";
}

static std::string toolWalletDeposit(QEngine* e, const Args& a)
{
    double amt = getDbl(a, "amount");
    if (amt <= 0) return "{\"error\":\"Amount must be positive\"}";
    qe_wallet_deposit(e, amt);
    QWalletInfo w = qe_wallet_info(e);
    JObj r;
    r.add("deposited", jDbl(amt)).add("newBalance", jDbl(w.balance));
    return r.str();
}

static std::string toolWalletWithdraw(QEngine* e, const Args& a)
{
    double amt = getDbl(a, "amount");
    if (amt <= 0) return "{\"error\":\"Amount must be positive\"}";
    QWalletInfo w = qe_wallet_info(e);
    if (amt > w.balance) return "{\"error\":\"Insufficient balance\"}";
    qe_wallet_withdraw(e, amt);
    w = qe_wallet_info(e);
    JObj r;
    r.add("withdrawn", jDbl(amt)).add("newBalance", jDbl(w.balance));
    return r.str();
}

// ============================================================
// Tool dispatcher
// ============================================================

using ToolFn = std::function<std::string(QEngine*, const Args&)>;

static std::map<std::string, ToolFn> buildDispatch()
{
    std::map<std::string, ToolFn> d;
    d["list_presets"]           = toolListPresets;
    d["save_preset"]            = toolSavePreset;
    d["delete_preset"]          = toolDeletePreset;
    d["portfolio_status"]      = toolPortfolioStatus;
    d["list_trades"]           = toolListTrades;
    d["list_entries"]          = toolListEntries;
    d["list_exits"]            = toolListExits;
    d["list_chains"]           = toolListChains;
    d["price_check"]           = toolPriceCheck;
    d["calculate_profit"]      = toolCalculateProfit;
    d["generate_serial_plan"]  = toolGenerateSerialPlan;
    d["compute_overhead"]      = toolComputeOverhead;
    d["what_if"]               = toolWhatIf;
    d["position_risk_summary"] = toolPositionRiskSummary;
    d["compare_plans"]         = toolComparePlans;
    d["execute_buy"]           = toolExecuteBuy;
    d["execute_sell"]          = toolExecuteSell;
    d["fill_entry"]            = toolFillEntry;
    d["execute_exit"]          = toolExecuteExit;
    d["confirm_execution"]     = toolConfirmExecution;
    d["wallet_deposit"]        = toolWalletDeposit;
    d["wallet_withdraw"]       = toolWalletWithdraw;
    return d;
}

// ============================================================
// MCP Protocol (JSON-RPC 2.0 over stdio)
// ============================================================

static std::string buildToolsListJson(const std::vector<ToolDef>& tools)
{
    JArr arr;
    for (const auto& t : tools)
    {
        JObj schema;
        schema.add("type", jStr("object"));

        JObj props;
        JArr req;
        for (const auto& p : t.params)
        {
            JObj prop;
            prop.add("type", jStr(p.type))
                .add("description", jStr(p.description));
            props.add(p.name, prop.str());
            if (p.required)
                req.add(jStr(p.name));
        }
        schema.add("properties", props.str());
        if (!req.empty())
            schema.add("required", req.str());

        JObj tool;
        tool.add("name", jStr(t.name))
            .add("description", jStr(t.description))
            .add("inputSchema", schema.str());
        arr.add(tool.str());
    }
    return arr.str();
}

static std::string buildResult(const std::string& id, const std::string& resultJson)
{
    JObj resp;
    resp.add("jsonrpc", jStr("2.0"))
        .add("id", id)
        .add("result", resultJson);
    return resp.str();
}

static std::string buildError(const std::string& id, int code, const std::string& msg)
{
    JObj err;
    err.add("code", jInt(code)).add("message", jStr(msg));
    JObj resp;
    resp.add("jsonrpc", jStr("2.0"))
        .add("id", id)
        .add("error", err.str());
    return resp.str();
}

// ============================================================
// Internal dispatch
// ============================================================

static std::string g_toolsJson;
static std::map<std::string, ToolFn> g_dispatch;
static bool g_initialised = false;

static void ensureInit()
{
    if (g_initialised) return;
    g_initialised = true;
    auto tools = buildToolDefs();
    g_dispatch  = buildDispatch();
    g_toolsJson = buildToolsListJson(tools);
}

static std::string handleRequest(QEngine* engine, const std::string& body)
{
    ensureInit();

    size_t pos = 0;
    auto req = parseObj(body, pos);

    std::string method = getStr(req, "method");
    std::string id     = req.count("id") ? req["id"] : "null";
    auto params        = getObj(req, "params");

    if (method == "initialize")
    {
        JObj caps;
        caps.add("tools", JObj().add("listChanged", jBool(false)).str());

        JObj serverInfo;
        serverInfo.add("name", jStr("quant-mcp"))
                  .add("version", jStr("1.0.0"));

        JObj result;
        result.add("protocolVersion", jStr("2024-11-05"))
              .add("capabilities", caps.str())
              .add("serverInfo", serverInfo.str());
        return buildResult(id, result.str());
    }
    else if (method == "notifications/initialized")
    {
        return ""; // no response for notifications
    }
    else if (method == "tools/list")
    {
        JObj result;
        result.add("tools", g_toolsJson);
        return buildResult(id, result.str());
    }
    else if (method == "tools/call")
    {
        std::string toolName = getStr(params, "name");
        auto args = getObj(params, "arguments");

        auto it = g_dispatch.find(toolName);
        if (it == g_dispatch.end())
            return buildError(id, -32601, "Unknown tool: " + toolName);

        std::string output;
        try {
            output = it->second(engine, args);
        } catch (const std::exception& ex) {
            output = "{\"error\":\"" + jEsc(ex.what()) + "\"}";
        }

        JObj textContent;
        textContent.add("type", jStr("text"))
                   .add("text", jStr(output));

        JObj result;
        result.add("content", JArr().add(textContent.str()).str());
        return buildResult(id, result.str());
    }
    else if (method == "resources/list")
    {
        JArr resources;
        resources.add(JObj()
            .add("uri", jStr("quant://portfolio/summary"))
            .add("name", jStr("Portfolio Summary"))
            .add("description", jStr("Current wallet balance and all open positions"))
            .add("mimeType", jStr("application/json")).str());

        JObj result;
        result.add("resources", resources.str());
        return buildResult(id, result.str());
    }
    else if (method == "resources/read")
    {
        std::string uri = getStr(params, "uri");

        if (uri == "quant://portfolio/summary")
        {
            std::string content = toolPortfolioStatus(engine, {});
            JObj textContent;
            textContent.add("uri", jStr(uri))
                       .add("mimeType", jStr("application/json"))
                       .add("text", jStr(content));
            JObj result;
            result.add("contents", JArr().add(textContent.str()).str());
            return buildResult(id, result.str());
        }
        return buildError(id, -32602, "Unknown resource: " + uri);
    }
    else if (method == "ping")
    {
        return buildResult(id, "{}");
    }

    if (id != "null")
        return buildError(id, -32601, "Method not found: " + method);
    return "";
}

// ============================================================
// C API
// ============================================================

extern "C" {

void qmcp_init(const char* dbDir)
{
    g_dbDir = dbDir ? dbDir : "db";
    loadPresetsFromFile();
    size_t before = g_presets.size();
    seedDefaultPresets();
    if (g_presets.size() != before)
        savePresetsToFile();
    ensureInit();
}

char* qmcp_handle(QEngine* engine, const char* jsonRpcRequest)
{
    if (!engine || !jsonRpcRequest) return nullptr;
    std::string result = handleRequest(engine, std::string(jsonRpcRequest));
    if (result.empty()) return nullptr;
    char* buf = (char*)std::malloc(result.size() + 1);
    if (buf) std::memcpy(buf, result.c_str(), result.size() + 1);
    return buf;
}

void qmcp_free(char* str)
{
    std::free(str);
}

void qmcp_shutdown()
{
    g_initialised = false;
    g_dispatch.clear();
    g_toolsJson.clear();
}

} // extern "C"
