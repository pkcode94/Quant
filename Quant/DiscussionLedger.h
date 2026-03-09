#pragma once
// ============================================================
// DiscussionLedger.h  -  Intelligent Ledger metadata layer
//
// Provides relational mapping between trades and post-hoc
// analysis discussions.  Each Discussion links an arbitrary
// subset of trades to trader notes, GPT analysis, and
// per-trade post-mortem comparisons (actual vs ghost profit).
//
// No ML in-loop.  Intelligence comes from the data structure
// enabling post-hoc analysis.  The GPT analysis field is
// populated externally (e.g. via API or manual entry).
//
// Persistence: JSON files via nanojson3 (json.h), following
// the same patterns as TradeDatabase.
// ============================================================

#include "json.h"
#include "Trade.h"

#include <string>
#include <vector>
#include <map>
#include <set>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <algorithm>
#include <functional>

// ---- Data structures ----

struct PostMortem
{
    double actualProfit   = 0.0;   // realised P&L for this trade
    double ghostProfit    = 0.0;   // hypothetical P&L if alternative was taken
    double ghostEntry     = 0.0;   // alternative entry price considered
    double ghostExit      = 0.0;   // alternative exit price considered
    double ghostQuantity  = 0.0;   // alternative quantity
    std::string notes;             // free-text explanation of the alternative
};

struct DiscussionMessage
{
    std::string author;    // "trader" | "gpt" | any user identifier
    std::string content;   // message body
    std::string timestamp; // ISO 8601
};

struct Discussion
{
    std::string                   discussionId;
    std::string                   title;
    std::string                   createdAt;
    std::string                   updatedAt;
    std::vector<int>              tradeIds;       // subset of trades this discussion covers
    std::string                   traderNotes;    // summary from trader
    std::string                   gptAnalysis;    // summary from GPT
    std::vector<DiscussionMessage> messages;       // full conversation thread
    std::map<int, PostMortem>     postMortems;    // tradeId -> post-mortem analysis
    std::vector<std::string>      tags;           // e.g. "BTC", "swing", "loss"
};

// ---- Ledger manager ----

class DiscussionLedger
{
public:
    explicit DiscussionLedger(const std::string& directory = "db")
        : m_dir(directory)
    {
        std::filesystem::create_directories(m_dir);
        load();
    }

    // ---- Accessors ----

    const std::map<std::string, Discussion>& discussions() const { return m_discussions; }

    const Discussion* find(const std::string& discussionId) const
    {
        auto it = m_discussions.find(discussionId);
        return (it != m_discussions.end()) ? &it->second : nullptr;
    }

    Discussion* find(const std::string& discussionId)
    {
        auto it = m_discussions.find(discussionId);
        return (it != m_discussions.end()) ? &it->second : nullptr;
    }

    // ---- Create / update ----

    // Create a new discussion linked to a subset of trades.
    // Returns the generated discussion ID.
    std::string createDiscussion(const std::string& title,
                                 const std::vector<int>& tradeIds,
                                 const std::string& traderNotes = "")
    {
        Discussion d;
        d.discussionId = generateId();
        d.title        = title;
        d.createdAt    = nowISO();
        d.updatedAt    = d.createdAt;
        d.tradeIds     = tradeIds;
        d.traderNotes  = traderNotes;
        std::string id = d.discussionId;
        m_discussions[id] = std::move(d);
        save();
        return id;
    }

    // Link additional trades to an existing discussion.
    bool linkTrades(const std::string& discussionId, const std::vector<int>& tradeIds)
    {
        auto* d = find(discussionId);
        if (!d) return false;
        for (int id : tradeIds)
            if (std::find(d->tradeIds.begin(), d->tradeIds.end(), id) == d->tradeIds.end())
                d->tradeIds.push_back(id);
        d->updatedAt = nowISO();
        save();
        return true;
    }

    // Unlink trades from a discussion.
    bool unlinkTrades(const std::string& discussionId, const std::vector<int>& tradeIds)
    {
        auto* d = find(discussionId);
        if (!d) return false;
        for (int id : tradeIds)
            std::erase(d->tradeIds, id);
        d->updatedAt = nowISO();
        save();
        return true;
    }

    // Add a message to the conversation thread.
    bool addMessage(const std::string& discussionId,
                    const std::string& author,
                    const std::string& content)
    {
        auto* d = find(discussionId);
        if (!d) return false;
        DiscussionMessage msg;
        msg.author    = author;
        msg.content   = content;
        msg.timestamp = nowISO();
        d->messages.push_back(std::move(msg));
        d->updatedAt = nowISO();
        save();
        return true;
    }

    // Set the GPT analysis text for a discussion.
    bool setGptAnalysis(const std::string& discussionId, const std::string& analysis)
    {
        auto* d = find(discussionId);
        if (!d) return false;
        d->gptAnalysis = analysis;
        d->updatedAt   = nowISO();
        save();
        return true;
    }

    // Set the trader notes for a discussion.
    bool setTraderNotes(const std::string& discussionId, const std::string& notes)
    {
        auto* d = find(discussionId);
        if (!d) return false;
        d->traderNotes = notes;
        d->updatedAt   = nowISO();
        save();
        return true;
    }

    // Add or update a post-mortem for a specific trade within a discussion.
    bool setPostMortem(const std::string& discussionId, int tradeId, const PostMortem& pm)
    {
        auto* d = find(discussionId);
        if (!d) return false;
        // Ensure the trade is linked
        if (std::find(d->tradeIds.begin(), d->tradeIds.end(), tradeId) == d->tradeIds.end())
            d->tradeIds.push_back(tradeId);
        d->postMortems[tradeId] = pm;
        d->updatedAt = nowISO();
        save();
        return true;
    }

    // Add tags to a discussion.
    bool addTags(const std::string& discussionId, const std::vector<std::string>& tags)
    {
        auto* d = find(discussionId);
        if (!d) return false;
        for (const auto& tag : tags)
            if (std::find(d->tags.begin(), d->tags.end(), tag) == d->tags.end())
                d->tags.push_back(tag);
        d->updatedAt = nowISO();
        save();
        return true;
    }

    // Remove a discussion entirely.
    bool removeDiscussion(const std::string& discussionId)
    {
        auto it = m_discussions.find(discussionId);
        if (it == m_discussions.end()) return false;
        m_discussions.erase(it);
        save();
        return true;
    }

    // ---- Queries ----

    // Find all discussions that reference a given trade ID.
    std::vector<const Discussion*> discussionsForTrade(int tradeId) const
    {
        std::vector<const Discussion*> result;
        for (const auto& [id, d] : m_discussions)
            if (std::find(d.tradeIds.begin(), d.tradeIds.end(), tradeId) != d.tradeIds.end())
                result.push_back(&d);
        return result;
    }

    // Find all discussions matching any of the given tags.
    std::vector<const Discussion*> discussionsByTag(const std::string& tag) const
    {
        std::vector<const Discussion*> result;
        for (const auto& [id, d] : m_discussions)
            if (std::find(d.tags.begin(), d.tags.end(), tag) != d.tags.end())
                result.push_back(&d);
        return result;
    }

    // Get the trade subset descriptor string for a discussion
    // (human-readable identification of which trades are covered).
    static std::string describeTradeSubset(const Discussion& d)
    {
        if (d.tradeIds.empty()) return "(no trades)";
        std::ostringstream ss;
        ss << "trades{";
        for (size_t i = 0; i < d.tradeIds.size(); ++i)
        {
            if (i > 0) ss << ",";
            ss << d.tradeIds[i];
        }
        ss << "}";
        return ss.str();
    }

    // ---- Persistence ----

    void save() const
    {
        njs3::js_array arr;
        for (const auto& [id, d] : m_discussions)
            arr.push_back(serialiseDiscussion(d));
        writeJson(ledgerPath(), njs3::json(std::move(arr)));
    }

    void load()
    {
        m_discussions.clear();
        auto j = readJsonArr(ledgerPath());
        const auto* a = j.as_array();
        if (!a) return;
        for (const auto& item : *a)
        {
            Discussion d = deserialiseDiscussion(item);
            if (!d.discussionId.empty())
                m_discussions[d.discussionId] = std::move(d);
        }
    }

private:
    std::string m_dir;
    std::map<std::string, Discussion> m_discussions;

    std::string ledgerPath() const { return m_dir + "/discussions.json"; }

    // ---- ID generation ----

    static std::string generateId()
    {
        auto now = std::chrono::system_clock::now();
        auto ms  = std::chrono::duration_cast<std::chrono::milliseconds>(
                       now.time_since_epoch()).count();
        static int seq = 0;
        std::ostringstream ss;
        ss << "disc_" << ms << "_" << (seq++);
        return ss.str();
    }

    // ---- Timestamp ----

    static std::string nowISO()
    {
        auto now = std::chrono::system_clock::now();
        auto t   = std::chrono::system_clock::to_time_t(now);
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

    // ---- JSON helpers (same pattern as TradeDatabase) ----

    static njs3::json JI(int v)                    { return njs3::json(static_cast<njs3::js_integer>(v)); }
    static njs3::json JD(double v)                 { return njs3::json(static_cast<njs3::js_floating>(v)); }
    static njs3::json JStr(const std::string& v)   { return njs3::json(njs3::js_string(v)); }
    static int         gi(const njs3::json& j, const char* k) { return static_cast<int>(j[k]->get_integer_or(0LL)); }
    static double      gd(const njs3::json& j, const char* k) { return static_cast<double>(j[k]->get_number_or(0.0L)); }
    static std::string gs(const njs3::json& j, const char* k) { return j[k]->get_string_or(njs3::js_string("")); }

    static njs3::json readJsonArr(const std::string& path)
    {
        std::ifstream f(path);
        if (!f) return njs3::json(njs3::js_array{});
        std::string c((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        if (c.empty()) return njs3::json(njs3::js_array{});
        try { return njs3::parse_json(c); }
        catch (...) { return njs3::json(njs3::js_array{}); }
    }

    static void writeJson(const std::string& path, const njs3::json& j)
    {
        std::ofstream f(path, std::ios::trunc);
        if (!f) return;
        f << njs3::serialize_json(j, njs3::json_serialize_option::pretty,
            njs3::json_floating_format_options{std::chars_format::general, 17});
    }

    // ---- Serialisation ----

    static njs3::json serialisePostMortem(const PostMortem& pm)
    {
        njs3::json j(njs3::js_object{});
        j["actualProfit"]  = JD(pm.actualProfit);
        j["ghostProfit"]   = JD(pm.ghostProfit);
        j["ghostEntry"]    = JD(pm.ghostEntry);
        j["ghostExit"]     = JD(pm.ghostExit);
        j["ghostQuantity"] = JD(pm.ghostQuantity);
        j["notes"]         = JStr(pm.notes);
        return j;
    }

    static PostMortem deserialisePostMortem(const njs3::json& j)
    {
        PostMortem pm;
        pm.actualProfit  = gd(j, "actualProfit");
        pm.ghostProfit   = gd(j, "ghostProfit");
        pm.ghostEntry    = gd(j, "ghostEntry");
        pm.ghostExit     = gd(j, "ghostExit");
        pm.ghostQuantity = gd(j, "ghostQuantity");
        pm.notes         = gs(j, "notes");
        return pm;
    }

    static njs3::json serialiseMessage(const DiscussionMessage& msg)
    {
        njs3::json j(njs3::js_object{});
        j["author"]    = JStr(msg.author);
        j["content"]   = JStr(msg.content);
        j["timestamp"] = JStr(msg.timestamp);
        return j;
    }

    static DiscussionMessage deserialiseMessage(const njs3::json& j)
    {
        DiscussionMessage msg;
        msg.author    = gs(j, "author");
        msg.content   = gs(j, "content");
        msg.timestamp = gs(j, "timestamp");
        return msg;
    }

    static njs3::json serialiseDiscussion(const Discussion& d)
    {
        njs3::json j(njs3::js_object{});
        j["discussionId"] = JStr(d.discussionId);
        j["title"]        = JStr(d.title);
        j["createdAt"]    = JStr(d.createdAt);
        j["updatedAt"]    = JStr(d.updatedAt);
        j["traderNotes"]  = JStr(d.traderNotes);
        j["gptAnalysis"]  = JStr(d.gptAnalysis);

        // tradeIds array
        njs3::js_array ids;
        for (int id : d.tradeIds)
            ids.push_back(JI(id));
        j["tradeIds"] = njs3::json(std::move(ids));

        // messages array
        njs3::js_array msgs;
        for (const auto& msg : d.messages)
            msgs.push_back(serialiseMessage(msg));
        j["messages"] = njs3::json(std::move(msgs));

        // postMortems object (keyed by trade ID as string)
        njs3::js_object pms;
        for (const auto& [tid, pm] : d.postMortems)
            pms[std::to_string(tid)] = serialisePostMortem(pm);
        j["postMortems"] = njs3::json(std::move(pms));

        // tags array
        njs3::js_array tagArr;
        for (const auto& tag : d.tags)
            tagArr.push_back(JStr(tag));
        j["tags"] = njs3::json(std::move(tagArr));

        return j;
    }

    static Discussion deserialiseDiscussion(const njs3::json& j)
    {
        Discussion d;
        d.discussionId = gs(j, "discussionId");
        d.title        = gs(j, "title");
        d.createdAt    = gs(j, "createdAt");
        d.updatedAt    = gs(j, "updatedAt");
        d.traderNotes  = gs(j, "traderNotes");
        d.gptAnalysis  = gs(j, "gptAnalysis");

        // tradeIds
        if (auto opt = j["tradeIds"]; opt)
            if (const auto* a = opt->as_array())
                for (const auto& item : *a)
                    if (auto iv = item.as_integer())
                        d.tradeIds.push_back(static_cast<int>(*iv));

        // messages
        if (auto opt = j["messages"]; opt)
            if (const auto* a = opt->as_array())
                for (const auto& item : *a)
                    d.messages.push_back(deserialiseMessage(item));

        // postMortems
        if (auto opt = j["postMortems"]; opt)
            if (const auto* o = opt->as_object())
                for (const auto& [key, val] : *o)
                {
                    int tid = 0;
                    try { tid = std::stoi(std::string(key)); } catch (...) { continue; }
                    d.postMortems[tid] = deserialisePostMortem(val);
                }

        // tags
        if (auto opt = j["tags"]; opt)
            if (const auto* a = opt->as_array())
                for (const auto& item : *a)
                    if (auto sv = item.as_string())
                        d.tags.push_back(std::string(*sv));

        return d;
    }
};
