#pragma once

#include <map>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>

// Time-series price data keyed by symbol.
//
// Stores (timestamp, price) pairs per symbol.  Provides:
//   set()    — insert or overwrite a data point
//   at()     — nearest-neighbour lookup at a given time
//   latest() — most recent price for a symbol
//   range()  — all points in a time window
//
// Thread safety: callers must hold their own mutex.

struct PricePoint
{
    long long timestamp = 0;   // unix seconds
    double    price     = 0.0;
};

class PriceSeries
{
public:
    PriceSeries() = default;

    // Insert or overwrite a price at the given time.
    void set(const std::string& symbol, long long time, double price)
    {
        auto& pts = m_data[symbol];
        // check for exact timestamp
        for (auto& p : pts)
        {
            if (p.timestamp == time) { p.price = price; return; }
        }
        pts.push_back({ time, price });
        std::sort(pts.begin(), pts.end(),
                  [](const PricePoint& a, const PricePoint& b)
                  { return a.timestamp < b.timestamp; });
    }

    // Bulk-set a sorted series (replaces any existing data for that symbol).
    void setSeries(const std::string& symbol, std::vector<PricePoint> pts)
    {
        std::sort(pts.begin(), pts.end(),
                  [](const PricePoint& a, const PricePoint& b)
                  { return a.timestamp < b.timestamp; });
        m_data[symbol] = std::move(pts);
    }

    // Nearest-neighbour price at time t.
    // Returns 0 if no data exists for this symbol.
    double at(const std::string& symbol, long long time) const
    {
        auto it = m_data.find(symbol);
        if (it == m_data.end() || it->second.empty()) return 0.0;
        const auto& pts = it->second;

        // binary search for closest
        auto lb = std::lower_bound(pts.begin(), pts.end(), time,
            [](const PricePoint& p, long long t) { return p.timestamp < t; });

        if (lb == pts.end())
            return pts.back().price;
        if (lb == pts.begin())
            return lb->price;

        auto prev = lb - 1;
        return (std::abs(lb->timestamp - time) < std::abs(prev->timestamp - time))
             ? lb->price : prev->price;
    }

    // Most recent price.
    double latest(const std::string& symbol) const
    {
        auto it = m_data.find(symbol);
        if (it == m_data.end() || it->second.empty()) return 0.0;
        return it->second.back().price;
    }

    // Earliest timestamp across all symbols (0 if empty).
    long long earliest() const
    {
        long long t = 0;
        bool first = true;
        for (const auto& kv : m_data)
            for (const auto& p : kv.second)
                if (first || p.timestamp < t) { t = p.timestamp; first = false; }
        return t;
    }

    // Latest timestamp across all symbols (0 if empty).
    long long latestTime() const
    {
        long long t = 0;
        for (const auto& kv : m_data)
            if (!kv.second.empty() && kv.second.back().timestamp > t)
                t = kv.second.back().timestamp;
        return t;
    }

    // All points in [from, to] for a symbol.
    std::vector<PricePoint> range(const std::string& symbol,
                                  long long from, long long to) const
    {
        std::vector<PricePoint> out;
        auto it = m_data.find(symbol);
        if (it == m_data.end()) return out;
        for (const auto& p : it->second)
            if (p.timestamp >= from && p.timestamp <= to)
                out.push_back(p);
        return out;
    }

    // All symbols that have data.
    std::vector<std::string> symbols() const
    {
        std::vector<std::string> out;
        for (const auto& kv : m_data)
            if (!kv.second.empty())
                out.push_back(kv.first);
        return out;
    }

    bool hasSymbol(const std::string& symbol) const
    {
        auto it = m_data.find(symbol);
        return it != m_data.end() && !it->second.empty();
    }

    const std::map<std::string, std::vector<PricePoint>>& data() const
    {
        return m_data;
    }

    void clear() { m_data.clear(); }

private:
    std::map<std::string, std::vector<PricePoint>> m_data;
};
