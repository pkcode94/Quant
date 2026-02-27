#pragma once

#include "UserManager.h"
#include "TradeDatabase.h"
#include "AdminConfig.h"
#include "SymbolRegistry.h"
#include "PriceSeries.h"
#include "cpp-httplib-master/httplib.h"

#include <mutex>
#include <map>
#include <memory>
#include <string>

struct AppContext
{
    UserManager& users;
    std::mutex&  dbMutex;
    TradeDatabase& defaultDb;
    AdminConfig& config;
    SymbolRegistry& symbols;
    PriceSeries&    prices;

    // Per-user database cache
    std::map<std::string, std::shared_ptr<TradeDatabase>> userDbs;

    // Get or create the TradeDatabase for a given username
    TradeDatabase& dbFor(const std::string& username)
    {
        auto it = userDbs.find(username);
        if (it != userDbs.end()) return *it->second;
        auto newDb = std::make_shared<TradeDatabase>(users.userDbDir(username));
        userDbs[username] = newDb;
        return *newDb;
    }

    // Resolve the database for the current request's authenticated user
    TradeDatabase& userDb(const httplib::Request& req)
    {
        auto user = currentUser(req);
        if (user.empty()) return defaultDb;
        return dbFor(user);
    }

    // Get the username of the currently logged-in user
    std::string currentUser(const httplib::Request& req) const
    {
        auto token = getSessionToken(req);
        return users.getSessionUser(token);
    }

    // Check if the current user has premium access
    bool isPremium(const httplib::Request& req) const
    {
        auto user = currentUser(req);
        return !user.empty() && (users.isAdmin(user) || users.isPremium(user));
    }

    bool isAdmin(const httplib::Request& req) const
    {
        auto user = currentUser(req);
        return !user.empty() && users.isAdmin(user);
    }

    // Check if user can add more trades (returns true if allowed)
    bool canAddTrade(const httplib::Request& req) const
    {
        if (isPremium(req)) return true;
        int count = (int)defaultDb.loadTrades().size();
        return count < config.freeTradeLimit;
    }

    int tradesRemaining(const httplib::Request& req) const
    {
        if (isPremium(req)) return 999999;
        int count = (int)defaultDb.loadTrades().size();
        int rem = config.freeTradeLimit - count;
        return rem > 0 ? rem : 0;
    }

    // Extract session token from request cookies
    static std::string getSessionToken(const httplib::Request& req)
    {
        if (!req.has_header("Cookie")) return "";
        auto cookies = req.get_header_value("Cookie");
        std::string prefix = std::string(UserManager::cookieName()) + "=";
        auto pos = cookies.find(prefix);
        if (pos == std::string::npos) return "";
        auto start = pos + prefix.size();
        auto end = cookies.find(';', start);
        return cookies.substr(start, end == std::string::npos ? end : end - start);
    }
};
