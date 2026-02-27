#pragma once

#include "IdGenerator.h"
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>

// Central symbol registry.
//
// Every symbol gets a unique integer ID.  All subsystems that deal
// with symbols should reference symbols by ID (or by SymbolRef) so
// renames, merges, and cross-referencing work cleanly.
//
// Thread safety: callers must hold their own mutex.

struct SymbolInfo
{
    int         id   = 0;
    std::string name;        // normalised uppercase (e.g. "BTC")
};

class SymbolRegistry
{
public:
    SymbolRegistry() = default;

    // Look up by name.  Returns nullptr if not registered.
    const SymbolInfo* find(const std::string& name) const
    {
        std::string n = normalize(name);
        for (const auto& s : m_symbols)
            if (s.name == n) return &s;
        return nullptr;
    }

    // Look up by ID.
    const SymbolInfo* find(int id) const
    {
        for (const auto& s : m_symbols)
            if (s.id == id) return &s;
        return nullptr;
    }

    // Get-or-create: returns the existing ID or registers a new one.
    int getOrCreate(const std::string& name)
    {
        std::string n = normalize(name);
        auto* existing = find(n);
        if (existing) return existing->id;
        int id = m_idGen.acquire();
        m_idGen.commit(id);
        m_symbols.push_back({ id, n });
        return id;
    }

    // Resolve an ID back to a name.  Returns empty string if unknown.
    std::string nameOf(int id) const
    {
        auto* s = find(id);
        return s ? s->name : std::string{};
    }

    const std::vector<SymbolInfo>& all() const { return m_symbols; }

    // Seed from existing trade data (call once at startup).
    void seed(const std::vector<std::string>& names)
    {
        for (const auto& n : names)
            getOrCreate(n);
    }

    IdGenerator&       idGen()       { return m_idGen; }
    const IdGenerator& idGen() const { return m_idGen; }

private:
    static std::string normalize(const std::string& s)
    {
        std::string out = s;
        std::transform(out.begin(), out.end(), out.begin(),
                       [](unsigned char c) { return std::toupper(c); });
        return out;
    }

    std::vector<SymbolInfo> m_symbols;
    IdGenerator             m_idGen;
};
