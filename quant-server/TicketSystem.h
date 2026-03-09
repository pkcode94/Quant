#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <mutex>
#include <algorithm>

// ============================================================
// TicketSystem — support tickets for quota / payment requests
//
// Users create tickets to request more storage.
// Admin sets a BTC price, user pays, admin confirms.
// ============================================================

class TicketSystem
{
public:
    struct Message
    {
        std::string from;       // username or "ADMIN"
        std::string text;
        std::string timestamp;
    };

    struct Ticket
    {
        int         ticketId    = 0;
        std::string username;
        std::string subject;
        std::string status;     // "open", "pending_payment", "closed"
        std::string btcAddress;
        double      requiredBtc = 0;
        size_t      requestedBytes = 0;
        std::string createdAt;
        std::vector<Message> messages;
    };

    explicit TicketSystem(const std::string& dataDir)
        : m_dataDir(dataDir), m_nextId(1)
    {
        load();
    }

    int createTicket(const std::string& username, const std::string& subject,
                     const std::string& message)
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        Ticket t;
        t.ticketId  = m_nextId++;
        t.username  = username;
        t.subject   = subject;
        t.status    = "open";
        t.createdAt = nowISO();
        t.messages.push_back({username, message, t.createdAt});
        m_tickets.push_back(t);
        save();
        return t.ticketId;
    }

    bool addReply(int ticketId, const std::string& from, const std::string& text)
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        for (auto& t : m_tickets)
            if (t.ticketId == ticketId)
            {
                t.messages.push_back({from, text, nowISO()});
                save();
                return true;
            }
        return false;
    }

    bool closeTicket(int ticketId)
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        for (auto& t : m_tickets)
            if (t.ticketId == ticketId)
            {
                t.status = "closed";
                save();
                return true;
            }
        return false;
    }

    bool setBtcPayment(int ticketId, const std::string& btcAddress,
                       double requiredBtc, size_t requestedBytes)
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        for (auto& t : m_tickets)
            if (t.ticketId == ticketId)
            {
                t.btcAddress    = btcAddress;
                t.requiredBtc   = requiredBtc;
                t.requestedBytes = requestedBytes;
                t.status        = "pending_payment";
                t.messages.push_back({"ADMIN",
                    "Payment required: " + std::to_string(requiredBtc) + " BTC to " + btcAddress,
                    nowISO()});
                save();
                return true;
            }
        return false;
    }

    const Ticket* findTicket(int ticketId) const
    {
        for (const auto& t : m_tickets)
            if (t.ticketId == ticketId) return &t;
        return nullptr;
    }

    std::vector<Ticket> listForUser(const std::string& username) const
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        std::vector<Ticket> out;
        for (const auto& t : m_tickets)
            if (t.username == username) out.push_back(t);
        return out;
    }

    std::vector<Ticket> listOpen() const
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        std::vector<Ticket> out;
        for (const auto& t : m_tickets)
            if (t.status != "closed") out.push_back(t);
        return out;
    }

    const std::vector<Ticket>& all() const { return m_tickets; }

private:
    std::string m_dataDir;
    std::vector<Ticket> m_tickets;
    int m_nextId;
    mutable std::mutex m_mutex;

    std::string ticketsPath() const { return m_dataDir + "/tickets.json"; }

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

    static std::string esc(const std::string& s)
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

    void save() const
    {
        std::ofstream f(ticketsPath(), std::ios::trunc);
        if (!f) return;
        f << "[\n";
        for (size_t i = 0; i < m_tickets.size(); ++i)
        {
            const auto& t = m_tickets[i];
            f << "  {\n"
              << "    \"ticketId\": " << t.ticketId << ",\n"
              << "    \"username\": \"" << esc(t.username) << "\",\n"
              << "    \"subject\": \"" << esc(t.subject) << "\",\n"
              << "    \"status\": \"" << esc(t.status) << "\",\n"
              << "    \"btcAddress\": \"" << esc(t.btcAddress) << "\",\n"
              << "    \"requiredBtc\": " << std::fixed << std::setprecision(8) << t.requiredBtc << ",\n"
              << "    \"requestedBytes\": " << t.requestedBytes << ",\n"
              << "    \"createdAt\": \"" << esc(t.createdAt) << "\",\n"
              << "    \"messages\": [\n";
            for (size_t j = 0; j < t.messages.size(); ++j)
            {
                const auto& m = t.messages[j];
                f << "      {\"from\": \"" << esc(m.from) << "\", "
                  << "\"text\": \"" << esc(m.text) << "\", "
                  << "\"timestamp\": \"" << esc(m.timestamp) << "\"}"
                  << (j + 1 < t.messages.size() ? "," : "") << "\n";
            }
            f << "    ]\n"
              << "  }" << (i + 1 < m_tickets.size() ? "," : "") << "\n";
        }
        f << "]\n";
    }

    void load()
    {
        m_tickets.clear();
        m_nextId = 1;
        std::ifstream f(ticketsPath());
        if (!f) return;
        std::string content((std::istreambuf_iterator<char>(f)),
                             std::istreambuf_iterator<char>());
        // Minimal JSON parser — tickets are structured with known fields
        // We parse by finding ticket objects between top-level { }
        size_t pos = 0;
        while ((pos = content.find("\"ticketId\"", pos)) != std::string::npos)
        {
            // Find the enclosing { before this key
            auto objStart = content.rfind('{', pos);
            if (objStart == std::string::npos) break;

            // Find matching } — we need to handle nested { } for messages
            int depth = 1;
            size_t objEnd = objStart + 1;
            while (objEnd < content.size() && depth > 0)
            {
                if (content[objEnd] == '{' || content[objEnd] == '[') depth++;
                else if (content[objEnd] == '}' || content[objEnd] == ']') depth--;
                objEnd++;
            }
            std::string obj = content.substr(objStart, objEnd - objStart);

            Ticket t;
            t.ticketId = extractInt(obj, "ticketId");
            t.username = extractStr(obj, "username");
            t.subject  = extractStr(obj, "subject");
            t.status   = extractStr(obj, "status");
            t.btcAddress = extractStr(obj, "btcAddress");
            auto rb = extractStr(obj, "requiredBtc");
            try { t.requiredBtc = std::stod(rb); } catch (...) {}
            auto reqb = extractStr(obj, "requestedBytes");
            try { t.requestedBytes = std::stoull(reqb); } catch (...) {}
            t.createdAt = extractStr(obj, "createdAt");

            // Parse messages array
            auto msgsStart = obj.find("\"messages\"");
            if (msgsStart != std::string::npos)
            {
                auto arrStart = obj.find('[', msgsStart);
                auto arrEnd   = obj.rfind(']');
                if (arrStart != std::string::npos && arrEnd != std::string::npos)
                {
                    std::string msgs = obj.substr(arrStart, arrEnd - arrStart + 1);
                    size_t mpos = 0;
                    while ((mpos = msgs.find('{', mpos)) != std::string::npos)
                    {
                        auto mend = msgs.find('}', mpos);
                        if (mend == std::string::npos) break;
                        std::string mobj = msgs.substr(mpos, mend - mpos + 1);
                        Message m;
                        m.from      = extractStr(mobj, "from");
                        m.text      = extractStr(mobj, "text");
                        m.timestamp = extractStr(mobj, "timestamp");
                        if (!m.from.empty()) t.messages.push_back(m);
                        mpos = mend + 1;
                    }
                }
            }

            if (t.ticketId > 0) m_tickets.push_back(t);
            if (t.ticketId >= m_nextId) m_nextId = t.ticketId + 1;

            pos = objEnd;
        }
    }

    static std::string extractStr(const std::string& obj, const std::string& key)
    {
        std::string needle = "\"" + key + "\"";
        auto pos = obj.find(needle);
        if (pos == std::string::npos) return "";
        pos = obj.find(':', pos + needle.size());
        if (pos == std::string::npos) return "";
        pos++;
        while (pos < obj.size() && obj[pos] == ' ') pos++;
        if (pos >= obj.size()) return "";
        if (obj[pos] == '"')
        {
            pos++;
            std::string result;
            while (pos < obj.size() && !(obj[pos] == '"' && obj[pos - 1] != '\\'))
            {
                if (obj[pos] == '\\' && pos + 1 < obj.size())
                {
                    char next = obj[++pos];
                    if (next == '"') result += '"';
                    else if (next == '\\') result += '\\';
                    else if (next == 'n') result += '\n';
                    else { result += '\\'; result += next; }
                }
                else result += obj[pos];
                pos++;
            }
            return result;
        }
        auto end = obj.find_first_of(",}\n", pos);
        std::string val = obj.substr(pos, end == std::string::npos ? end : end - pos);
        while (!val.empty() && val.back() == ' ') val.pop_back();
        return val;
    }

    static int extractInt(const std::string& obj, const std::string& key)
    {
        auto s = extractStr(obj, key);
        try { return std::stoi(s); } catch (...) { return 0; }
    }
};
