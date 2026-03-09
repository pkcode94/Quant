#pragma once

#include "AppContext.h"
#include "HtmlHelpers.h"
#include "QuantMath.h"
#include <mutex>
#include <cmath>
#include <algorithm>
#include <set>
#include <sstream>

inline void registerChainManagerRoutes(httplib::Server& svr, AppContext& ctx)
{
    auto& db = ctx.defaultDb;
    auto& dbMutex = ctx.dbMutex;

    // ========== GET /chains — list all chains ==========
    svr.Get("/chains", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(dbMutex);
        auto chains = db.loadManagedChains();
        auto entries = db.loadEntryPoints();

        std::ostringstream h;
        h << std::fixed << std::setprecision(8);
        h << html::msgBanner(req) << html::errBanner(req);
        h << "<h1>&#9939; Chain Manager</h1>"
             "<div style='color:#64748b;font-size:0.82em;margin-bottom:12px;'>"
             "Manage multi-cycle investment chains. Track entry progress against price.</div>";

        h << "<a class='btn' href='/chains/new' style='margin-bottom:16px;display:inline-block;'>+ New Chain</a>";

        if (chains.empty())
        {
            h << "<div class='msg'>No chains defined yet. Create one above.</div>";
        }
        else
        {
            h << "<table><tr>"
                 "<th>ID</th><th>Name</th><th>Symbol</th><th>Status</th>"
                 "<th>Cycle</th><th>Capital</th><th>Entries</th><th>Filled</th>"
                 "<th>Actions</th></tr>";

            for (const auto& c : chains)
            {
                // Count total entries and filled entries for this chain
                int totalEntries = 0, filledEntries = 0;
                for (const auto& cy : c.cycles)
                    for (int eid : cy.entryIds)
                    {
                        ++totalEntries;
                        for (const auto& ep : entries)
                            if (ep.entryId == eid && ep.traded)
                            { ++filledEntries; break; }
                    }

                std::string statusClass = c.active ? "buy" : (c.theoretical ? "" : "sell");
                std::string statusText  = c.active ? "ACTIVE" : (c.theoretical ? "THEORETICAL" : "INACTIVE");

                h << "<tr>"
                  << "<td>" << c.chainId << "</td>"
                  << "<td><a href='/chains/" << c.chainId << "'>" << html::esc(c.name) << "</a></td>"
                  << "<td>" << html::esc(c.symbol) << "</td>"
                  << "<td class='" << statusClass << "'>" << statusText << "</td>"
                  << "<td>" << c.currentCycle << "</td>"
                  << "<td>" << c.capital << "</td>"
                  << "<td>" << totalEntries << "</td>"
                  << "<td>" << filledEntries << " / " << totalEntries << "</td>"
                  << "<td>"
                  << "<a class='btn' href='/chains/" << c.chainId << "' style='font-size:0.8em;padding:3px 8px;'>View</a> "
                  << "<a class='btn' href='/chains/" << c.chainId << "/edit' style='font-size:0.8em;padding:3px 8px;'>Edit</a> "
                  << "<form method='POST' action='/chains/" << c.chainId << "/delete' style='display:inline;'>"
                  << "<button style='font-size:0.8em;padding:3px 8px;background:#dc2626;' "
                  << "onclick='return confirm(\"Delete chain " << c.chainId << "?\")'>Delete</button></form>"
                  << "</td></tr>";
            }
            h << "</table>";
        }
        res.set_content(html::wrap("Chain Manager", h.str()), "text/html");
    });

    // ========== GET /chains/new — create chain form ==========
    svr.Get("/chains/new", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(dbMutex);
        double walBal = db.loadWalletBalance();
        auto entries = db.loadEntryPoints();

        std::ostringstream h;
        h << std::fixed << std::setprecision(8);
        h << "<h1>New Chain</h1>"
             "<form class='card' method='POST' action='/chains/new'>"
             "<label>Name</label><input type='text' name='name' required><br>"
             "<label>Symbol</label><input type='text' name='symbol' required><br>"
             "<label>Starting Capital</label><input type='number' name='capital' step='any' value='" << walBal << "'><br>"
             "<label>Savings Rate</label><input type='number' name='savingsRate' step='any' value='0'><br>"
             "<label>Notes</label><textarea name='notes' rows='3' style='width:100%;'></textarea><br>"
             "<label>Type</label><select name='theoretical'>"
             "<option value='1'>Theoretical</option>"
             "<option value='0'>Live</option></select><br>";

        // Let user attach existing entry points
        if (!entries.empty())
        {
            h << "<h3>Attach Entry Points (Cycle 0)</h3>"
                 "<div style='max-height:200px;overflow-y:auto;border:1px solid #1a2744;padding:8px;border-radius:4px;'>";
            for (const auto& ep : entries)
            {
                h << "<label style='display:block;font-size:0.85em;'>"
                  << "<input type='checkbox' name='entryId' value='" << ep.entryId << "'> "
                  << "#" << ep.entryId << " " << html::esc(ep.symbol)
                  << " @ " << ep.entryPrice
                  << (ep.traded ? " [FILLED]" : " [PENDING]")
                  << "</label>";
            }
            h << "</div>";
        }

        h << "<br><button>Create Chain</button></form>"
             "<br><a class='btn' href='/chains'>Back</a>";
        res.set_content(html::wrap("New Chain", h.str()), "text/html");
    });

    // ========== POST /chains/new — create chain ==========
    svr.Post("/chains/new", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(dbMutex);
        auto f = parseForm(req.body);

        TradeDatabase::ManagedChain c;
        c.chainId      = db.nextChainId();
        c.name         = fv(f, "name");
        c.symbol       = normalizeSymbol(fv(f, "symbol"));
        c.capital      = fd(f, "capital");
        c.savingsRate  = fd(f, "savingsRate");
        c.notes        = fv(f, "notes");
        c.theoretical  = (fv(f, "theoretical") == "1");
        c.active       = !c.theoretical;
        c.currentCycle = 0;

        // Collect attached entry IDs
        TradeDatabase::ManagedChain::CycleEntries ce;
        ce.cycle = 0;
        // Parse multi-valued checkboxes from form body
        {
            std::istringstream ss(req.body);
            std::string token;
            while (std::getline(ss, token, '&'))
            {
                auto eq = token.find('=');
                if (eq == std::string::npos) continue;
                std::string key = token.substr(0, eq);
                std::string val = token.substr(eq + 1);
                if (key == "entryId")
                    try { ce.entryIds.push_back(std::stoi(val)); } catch (...) {}
            }
        }
        if (!ce.entryIds.empty())
            c.cycles.push_back(ce);

        // Timestamp
        {
            auto now = std::chrono::system_clock::now();
            auto t = std::chrono::system_clock::to_time_t(now);
            std::tm tm{};
#ifdef _WIN32
            localtime_s(&tm, &t);
#else
            localtime_r(&t, &tm);
#endif
            std::ostringstream ts;
            ts << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S");
            c.createdAt = ts.str();
        }

        auto chains = db.loadManagedChains();
        chains.push_back(c);
        db.saveManagedChains(chains);

        res.set_redirect("/chains/" + std::to_string(c.chainId) + "?msg=Chain+created", 303);
    });

    // ========== GET /chains/:id — view chain detail + progress ==========
    svr.Get(R"(/chains/(\d+))", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(dbMutex);
        int chainId = std::stoi(req.matches[1]);
        auto chains = db.loadManagedChains();
        auto entries = db.loadEntryPoints();

        const TradeDatabase::ManagedChain* chain = nullptr;
        for (const auto& c : chains)
            if (c.chainId == chainId) { chain = &c; break; }

        if (!chain)
        {
            res.set_redirect("/chains?err=Chain+not+found", 303);
            return;
        }

        // Get current price for progress check
        double currentPrice = 0.0;
        if (ctx.prices.hasSymbol(chain->symbol))
        {
            const auto& pts = ctx.prices.data().at(chain->symbol);
            if (!pts.empty()) currentPrice = pts.back().price;
        }

        std::ostringstream h;
        h << std::fixed << std::setprecision(8);
        h << html::msgBanner(req) << html::errBanner(req);

        std::string statusClass = chain->active ? "buy" : (chain->theoretical ? "" : "sell");
        std::string statusText  = chain->active ? "ACTIVE" : (chain->theoretical ? "THEORETICAL" : "INACTIVE");

        h << "<h1>&#9939; " << html::esc(chain->name) << "</h1>"
          << "<div class='row'>"
             "<div class='stat'><div class='lbl'>Symbol</div><div class='val'>" << html::esc(chain->symbol) << "</div></div>"
             "<div class='stat'><div class='lbl'>Status</div><div class='val " << statusClass << "'>" << statusText << "</div></div>"
             "<div class='stat'><div class='lbl'>Cycle</div><div class='val'>" << chain->currentCycle << "</div></div>"
             "<div class='stat'><div class='lbl'>Capital</div><div class='val'>" << chain->capital << "</div></div>"
             "<div class='stat'><div class='lbl'>Savings</div><div class='val'>" << chain->totalSavings << "</div></div>"
             "<div class='stat'><div class='lbl'>Savings Rate</div><div class='val'>" << (chain->savingsRate * 100) << "%</div></div>";
        if (currentPrice > 0)
            h << "<div class='stat'><div class='lbl'>Current Price</div><div class='val'>" << currentPrice << "</div></div>";
        h << "</div>";

        if (!chain->notes.empty())
            h << "<div style='color:#94a3b8;font-size:0.85em;margin:8px 0;'>" << html::esc(chain->notes) << "</div>";

        // --- Entry progress per cycle ---
        for (const auto& cy : chain->cycles)
        {
            h << "<h2>Cycle " << cy.cycle
              << " <form method='POST' action='/chains/" << chainId << "/delete-cycle' style='display:inline;'>"
              << "<input type='hidden' name='cycle' value='" << cy.cycle << "'>"
              << "<button style='font-size:0.7em;padding:2px 8px;background:#dc2626;vertical-align:middle;' "
              << "onclick='return confirm(\"Delete cycle " << cy.cycle << "?\")'>&#10005; Remove Cycle</button></form>"
              << " <a class='btn' href='/chains/" << chainId << "/create-entry?cycle=" << cy.cycle
              << "' style='font-size:0.7em;padding:2px 8px;vertical-align:middle;'>+ Add Entry</a>"
              << "</h2>";
            if (cy.entryIds.empty())
            {
                h << "<div style='color:#64748b;'>No entries in this cycle.</div>";
                continue;
            }

            h << "<table><tr><th>#</th><th>Entry Price</th><th>Qty</th><th>Cost</th>"
                 "<th>TP</th><th>SL</th><th>Status</th><th>Progress</th><th></th></tr>";

            for (int eid : cy.entryIds)
            {
                const TradeDatabase::EntryPoint* ep = nullptr;
                for (const auto& e : entries)
                    if (e.entryId == eid) { ep = &e; break; }

                if (!ep)
                {
                    h << "<tr><td>" << eid << "</td><td colspan='8' style='color:#dc2626;'>Entry not found</td></tr>";
                    continue;
                }

                double cost = QuantMath::cost(ep->entryPrice, ep->fundingQty);
                std::string status = ep->traded ? "FILLED" : "PENDING";
                std::string statusCls = ep->traded ? "buy" : "";

                // Progress: how close is current price to this entry?
                std::string progress;
                if (currentPrice > 0 && !ep->traded)
                {
                    double dist = ((currentPrice - ep->entryPrice) / ep->entryPrice) * 100.0;
                    if (currentPrice <= ep->entryPrice)
                        progress = "<span class='buy'>FILLABLE (" + std::to_string(dist).substr(0, 6) + "%)</span>";
                    else
                        progress = std::to_string(dist).substr(0, 6) + "% above";
                }
                else if (ep->traded)
                {
                    // Check TP progress
                    if (currentPrice > 0 && ep->exitTakeProfit > 0)
                    {
                        double tpDist = ((currentPrice - ep->exitTakeProfit) / ep->exitTakeProfit) * 100.0;
                        if (currentPrice >= ep->exitTakeProfit)
                            progress = "<span class='buy'>TP HIT</span>";
                        else
                            progress = std::to_string(tpDist).substr(0, 6) + "% to TP";
                    }
                    else
                        progress = "Filled";
                }
                else
                    progress = "-";

                h << "<tr>"
                  << "<td>" << ep->entryId << "</td>"
                  << "<td>" << ep->entryPrice << "</td>"
                  << "<td>" << ep->fundingQty << "</td>"
                  << "<td>" << cost << "</td>"
                  << "<td class='buy'>" << ep->exitTakeProfit << "</td>"
                  << "<td class='sell'>" << ep->exitStopLoss << "</td>"
                  << "<td class='" << statusCls << "'>" << status << "</td>"
                  << "<td>" << progress << "</td>"
                  << "<td><a href='/chains/" << chainId << "/edit-entry?entryId=" << ep->entryId
                  << "&cycle=" << cy.cycle << "' style='font-size:0.7em;' title='Edit entry'>&#9998;</a> "
                  << "<form method='POST' action='/chains/" << chainId << "/remove-entry' style='margin:0;display:inline;'>"
                  << "<input type='hidden' name='cycle' value='" << cy.cycle << "'>"
                  << "<input type='hidden' name='entryId' value='" << ep->entryId << "'>"
                  << "<button style='font-size:0.7em;padding:1px 6px;background:#dc2626;' title='Remove from cycle'>&#10005;</button>"
                  << "</form></td>"
                  << "</tr>";
            }
            h << "</table>";
        }

        // --- Actions ---
        h << "<div style='margin-top:16px;'>";

        // Activate / Deactivate
        if (chain->theoretical)
        {
            h << "<form method='POST' action='/chains/" << chainId << "/activate' style='display:inline;'>"
                 "<button class='btn' style='background:#16a34a;'>&#10003; Activate (Go Live)</button></form> ";
        }
        else if (chain->active)
        {
            h << "<form method='POST' action='/chains/" << chainId << "/deactivate' style='display:inline;'>"
                 "<button class='btn' style='background:#eab308;color:#000;'>Pause</button></form> ";
        }
        else
        {
            h << "<form method='POST' action='/chains/" << chainId << "/activate' style='display:inline;'>"
                 "<button class='btn' style='background:#16a34a;'>Reactivate</button></form> ";
        }

        h << "<a class='btn' href='/chains/" << chainId << "/edit'>Edit</a> "
             "<a class='btn' href='/chains/" << chainId << "/add-cycle'>+ Add Cycle</a> "
             "</div>";

        h << "<br><a class='btn' href='/chains'>Back to Chains</a>";
        res.set_content(html::wrap("Chain: " + chain->name, h.str()), "text/html");
    });

    // ========== GET /chains/:id/edit — edit chain form ==========
    svr.Get(R"(/chains/(\d+)/edit)", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(dbMutex);
        int chainId = std::stoi(req.matches[1]);
        auto chains = db.loadManagedChains();

        const TradeDatabase::ManagedChain* chain = nullptr;
        for (const auto& c : chains)
            if (c.chainId == chainId) { chain = &c; break; }

        if (!chain) { res.set_redirect("/chains?err=Chain+not+found", 303); return; }

        std::ostringstream h;
        h << std::fixed << std::setprecision(8);
        h << "<h1>Edit Chain #" << chainId << "</h1>"
             "<form class='card' method='POST' action='/chains/" << chainId << "/edit'>"
             "<label>Name</label><input type='text' name='name' value='" << html::esc(chain->name) << "' required><br>"
             "<label>Symbol</label><input type='text' name='symbol' value='" << html::esc(chain->symbol) << "' required><br>"
             "<label>Starting Capital</label><input type='number' name='capital' step='any' value='" << chain->capital << "'><br>"
             "<label>Savings Rate</label><input type='number' name='savingsRate' step='any' value='" << chain->savingsRate << "'><br>"
             "<label>Current Cycle</label><input type='number' name='currentCycle' value='" << chain->currentCycle << "'><br>"
             "<label>Total Savings</label><input type='number' name='totalSavings' step='any' value='" << chain->totalSavings << "'><br>"
             "<label>Notes</label><textarea name='notes' rows='3' style='width:100%;'>" << html::esc(chain->notes) << "</textarea><br>"
             "<label>Type</label><select name='theoretical'>"
             "<option value='1'" << (chain->theoretical ? " selected" : "") << ">Theoretical</option>"
             "<option value='0'" << (!chain->theoretical ? " selected" : "") << ">Live</option></select><br>"
             "<button>Save Changes</button></form>"
             "<br><a class='btn' href='/chains/" << chainId << "'>Back</a>";
        res.set_content(html::wrap("Edit Chain", h.str()), "text/html");
    });

    // ========== POST /chains/:id/edit — save edits ==========
    svr.Post(R"(/chains/(\d+)/edit)", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(dbMutex);
        int chainId = std::stoi(req.matches[1]);
        auto f = parseForm(req.body);
        auto chains = db.loadManagedChains();

        for (auto& c : chains)
            if (c.chainId == chainId)
            {
                c.name         = fv(f, "name");
                c.symbol       = normalizeSymbol(fv(f, "symbol"));
                c.capital      = fd(f, "capital");
                c.savingsRate  = fd(f, "savingsRate");
                c.currentCycle = fi(f, "currentCycle");
                c.totalSavings = fd(f, "totalSavings");
                c.notes        = fv(f, "notes");
                c.theoretical  = (fv(f, "theoretical") == "1");
                if (!c.theoretical && !c.active) c.active = true;
                break;
            }

        db.saveManagedChains(chains);
        res.set_redirect("/chains/" + std::to_string(chainId) + "?msg=Chain+updated", 303);
    });

    // ========== POST /chains/:id/delete — remove chain ==========
    svr.Post(R"(/chains/(\d+)/delete)", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(dbMutex);
        int chainId = std::stoi(req.matches[1]);
        auto chains = db.loadManagedChains();
        chains.erase(std::remove_if(chains.begin(), chains.end(),
            [chainId](const TradeDatabase::ManagedChain& c) { return c.chainId == chainId; }),
            chains.end());
        db.saveManagedChains(chains);
        res.set_redirect("/chains?msg=Chain+deleted", 303);
    });

    // ========== POST /chains/:id/activate — confirm chain ==========
    svr.Post(R"(/chains/(\d+)/activate)", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(dbMutex);
        int chainId = std::stoi(req.matches[1]);
        auto chains = db.loadManagedChains();
        for (auto& c : chains)
            if (c.chainId == chainId)
            { c.active = true; c.theoretical = false; break; }
        db.saveManagedChains(chains);
        res.set_redirect("/chains/" + std::to_string(chainId) + "?msg=Chain+activated", 303);
    });

    // ========== POST /chains/:id/deactivate — pause chain ==========
    svr.Post(R"(/chains/(\d+)/deactivate)", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(dbMutex);
        int chainId = std::stoi(req.matches[1]);
        auto chains = db.loadManagedChains();
        for (auto& c : chains)
            if (c.chainId == chainId)
            { c.active = false; break; }
        db.saveManagedChains(chains);
        res.set_redirect("/chains/" + std::to_string(chainId) + "?msg=Chain+paused", 303);
    });

    // ========== POST /chains/:id/delete-cycle — remove an entire cycle ==========
    svr.Post(R"(/chains/(\d+)/delete-cycle)", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(dbMutex);
        int chainId = std::stoi(req.matches[1]);
        auto f = parseForm(req.body);
        int cycle = fi(f, "cycle");

        auto chains = db.loadManagedChains();
        for (auto& c : chains)
            if (c.chainId == chainId)
            {
                c.cycles.erase(
                    std::remove_if(c.cycles.begin(), c.cycles.end(),
                        [cycle](const TradeDatabase::ManagedChain::CycleEntries& ce) { return ce.cycle == cycle; }),
                    c.cycles.end());
                break;
            }
        db.saveManagedChains(chains);
        res.set_redirect("/chains/" + std::to_string(chainId) + "?msg=Cycle+" + std::to_string(cycle) + "+deleted", 303);
    });

    // ========== GET /chains/:id/add-cycle — add entries to a new cycle ==========
    svr.Get(R"(/chains/(\d+)/add-cycle)", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(dbMutex);
        int chainId = std::stoi(req.matches[1]);
        auto chains = db.loadManagedChains();
        auto entries = db.loadEntryPoints();

        const TradeDatabase::ManagedChain* chain = nullptr;
        for (const auto& c : chains)
            if (c.chainId == chainId) { chain = &c; break; }

        if (!chain) { res.set_redirect("/chains?err=Chain+not+found", 303); return; }

        int nextCycle = chain->cycles.empty() ? 0 : static_cast<int>(chain->cycles.size());

        // Collect entry IDs already in this chain
        std::set<int> usedIds;
        for (const auto& cy : chain->cycles)
            for (int id : cy.entryIds)
                usedIds.insert(id);

        std::ostringstream h;
        h << std::fixed << std::setprecision(8);
        h << "<h1>Add Cycle to " << html::esc(chain->name) << "</h1>"
             "<form class='card' method='POST' action='/chains/" << chainId << "/add-cycle'>"
             "<label>Cycle Number</label><input type='number' name='cycle' value='" << nextCycle << "'><br>";

        // Filter to matching symbol, exclude already-used entries
        bool hasEntries = false;
        h << "<div style='max-height:400px;overflow-y:auto;border:1px solid #1a2744;padding:8px;border-radius:4px;'>";
        for (const auto& ep : entries)
        {
            if (usedIds.count(ep.entryId)) continue;
            hasEntries = true;
            h << "<label style='display:block;font-size:0.85em;'>"
              << "<input type='checkbox' name='entryId' value='" << ep.entryId << "'> "
              << "#" << ep.entryId << " " << html::esc(ep.symbol)
              << " @ " << ep.entryPrice
              << " qty=" << ep.fundingQty
              << (ep.traded ? " [FILLED]" : " [PENDING]")
              << "</label>";
        }
        if (!hasEntries)
            h << "<div style='color:#64748b;'>No available entry points to add."
                 "<br>Create entries via <a href='/serial-generator' style='color:#38bdf8;'>Serial Generator</a> "
                 "and save them first.</div>";
        h << "</div>";
        h << "<br><button" << (hasEntries ? "" : " disabled") << ">Add to Chain</button></form>"
             "<br><a class='btn' href='/chains/" << chainId << "'>Back</a>";
        res.set_content(html::wrap("Add Cycle", h.str()), "text/html");
    });

    // ========== POST /chains/:id/add-cycle — save new cycle ==========
    svr.Post(R"(/chains/(\d+)/add-cycle)", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(dbMutex);
        int chainId = std::stoi(req.matches[1]);
        auto f = parseForm(req.body);
        auto chains = db.loadManagedChains();

        int cycle = fi(f, "cycle");

        // Parse multi-valued entryId checkboxes
        TradeDatabase::ManagedChain::CycleEntries ce;
        ce.cycle = cycle;
        {
            std::istringstream ss(req.body);
            std::string token;
            while (std::getline(ss, token, '&'))
            {
                auto eq = token.find('=');
                if (eq == std::string::npos) continue;
                std::string key = token.substr(0, eq);
                std::string val = token.substr(eq + 1);
                if (key == "entryId")
                    try { ce.entryIds.push_back(std::stoi(val)); } catch (...) {}
            }
        }

        for (auto& c : chains)
            if (c.chainId == chainId)
            {
                c.cycles.push_back(ce);
                break;
            }

        db.saveManagedChains(chains);
        res.set_redirect("/chains/" + std::to_string(chainId) + "?msg=Cycle+" + std::to_string(cycle) + "+added", 303);
    });

    // ========== POST /chains/:id/remove-entry — remove an entry from a cycle ==========
    svr.Post(R"(/chains/(\d+)/remove-entry)", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(dbMutex);
        int chainId = std::stoi(req.matches[1]);
        auto f = parseForm(req.body);
        int cycle   = fi(f, "cycle");
        int entryId = fi(f, "entryId");

        auto chains = db.loadManagedChains();
        for (auto& c : chains)
            if (c.chainId == chainId)
            {
                for (auto& cy : c.cycles)
                    if (cy.cycle == cycle)
                    {
                        cy.entryIds.erase(
                            std::remove(cy.entryIds.begin(), cy.entryIds.end(), entryId),
                            cy.entryIds.end());
                        break;
                    }
                break;
            }

        db.saveManagedChains(chains);
        res.set_redirect("/chains/" + std::to_string(chainId) + "?msg=Entry+removed", 303);
    });

    // ========== GET /chains/:id/edit-entry — edit an entry point ==========
    svr.Get(R"(/chains/(\d+)/edit-entry)", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(dbMutex);
        int chainId = std::stoi(req.matches[1]);
        int entryId = 0, cycle = 0;
        if (req.has_param("entryId")) entryId = std::stoi(req.get_param_value("entryId"));
        if (req.has_param("cycle"))   cycle   = std::stoi(req.get_param_value("cycle"));

        auto entries = db.loadEntryPoints();
        const TradeDatabase::EntryPoint* ep = nullptr;
        for (const auto& e : entries)
            if (e.entryId == entryId) { ep = &e; break; }

        if (!ep) { res.set_redirect("/chains/" + std::to_string(chainId) + "?err=Entry+not+found", 303); return; }

        std::ostringstream h;
        h << std::fixed << std::setprecision(8);
        h << "<h1>Edit Entry #" << entryId << "</h1>"
             "<form class='card' method='POST' action='/chains/" << chainId << "/edit-entry'>"
             "<input type='hidden' name='entryId' value='" << entryId << "'>"
             "<input type='hidden' name='cycle' value='" << cycle << "'>"
             "<label>Symbol</label><input type='text' name='symbol' value='" << html::esc(ep->symbol) << "'><br>"
             "<label>Entry Price</label><input type='number' name='entryPrice' step='any' value='" << ep->entryPrice << "'><br>"
             "<label>Quantity</label><input type='number' name='fundingQty' step='any' value='" << ep->fundingQty << "'><br>"
             "<label>Funding</label><input type='number' name='funding' step='any' value='" << ep->funding << "'><br>"
             "<label>Break Even</label><input type='number' name='breakEven' step='any' value='" << ep->breakEven << "'><br>"
             "<label>Effective OH</label><input type='number' name='effectiveOverhead' step='any' value='" << ep->effectiveOverhead << "'><br>"
             "<label>Exit TP (price/unit)</label><input type='number' name='exitTakeProfit' step='any' value='" << ep->exitTakeProfit << "'><br>"
             "<label>Exit SL (price/unit)</label><input type='number' name='exitStopLoss' step='any' value='" << ep->exitStopLoss << "'><br>"
             "<label>SL Fraction</label><input type='number' name='stopLossFraction' step='any' value='" << ep->stopLossFraction << "'><br>"
             "<label>Direction</label><select name='isShort'>"
             "<option value='0'" << (!ep->isShort ? " selected" : "") << ">LONG</option>"
             "<option value='1'" << (ep->isShort ? " selected" : "") << ">SHORT</option></select><br>"
             "<label>Status</label><select name='traded'>"
             "<option value='0'" << (!ep->traded ? " selected" : "") << ">PENDING</option>"
             "<option value='1'" << (ep->traded ? " selected" : "") << ">FILLED</option></select><br>"
             "<button>Save Entry</button></form>"
             "<br><a class='btn' href='/chains/" << chainId << "'>Back</a>";
        res.set_content(html::wrap("Edit Entry", h.str()), "text/html");
    });

    // ========== POST /chains/:id/edit-entry — save entry edits ==========
    svr.Post(R"(/chains/(\d+)/edit-entry)", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(dbMutex);
        int chainId = std::stoi(req.matches[1]);
        auto f = parseForm(req.body);
        int entryId = fi(f, "entryId");

        auto entries = db.loadEntryPoints();
        for (auto& ep : entries)
            if (ep.entryId == entryId)
            {
                ep.symbol            = normalizeSymbol(fv(f, "symbol"));
                ep.entryPrice        = fd(f, "entryPrice");
                ep.fundingQty        = fd(f, "fundingQty");
                ep.funding           = fd(f, "funding");
                ep.breakEven         = fd(f, "breakEven");
                ep.effectiveOverhead = fd(f, "effectiveOverhead");
                ep.exitTakeProfit    = fd(f, "exitTakeProfit");
                ep.exitStopLoss      = fd(f, "exitStopLoss");
                ep.stopLossFraction  = fd(f, "stopLossFraction");
                ep.isShort           = (fv(f, "isShort") == "1");
                ep.traded            = (fv(f, "traded") == "1");
                ep.stopLossActive    = (ep.stopLossFraction > 0.0);
                break;
            }
        db.saveEntryPoints(entries);
        res.set_redirect("/chains/" + std::to_string(chainId) + "?msg=Entry+" + std::to_string(entryId) + "+updated", 303);
    });

    // ========== GET /chains/:id/create-entry — form to create a new entry inline ==========
    svr.Get(R"(/chains/(\d+)/create-entry)", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(dbMutex);
        int chainId = std::stoi(req.matches[1]);
        int cycle = 0;
        if (req.has_param("cycle")) cycle = std::stoi(req.get_param_value("cycle"));

        auto chains = db.loadManagedChains();
        std::string symbol;
        for (const auto& c : chains)
            if (c.chainId == chainId) { symbol = c.symbol; break; }

        std::ostringstream h;
        h << std::fixed << std::setprecision(8);
        h << "<h1>Create Entry for Cycle " << cycle << "</h1>"
             "<form class='card' method='POST' action='/chains/" << chainId << "/create-entry'>"
             "<input type='hidden' name='cycle' value='" << cycle << "'>"
             "<label>Symbol</label><input type='text' name='symbol' value='" << html::esc(symbol) << "'><br>"
             "<label>Entry Price</label><input type='number' name='entryPrice' step='any' required><br>"
             "<label>Quantity</label><input type='number' name='fundingQty' step='any' required><br>"
             "<label>Funding</label><input type='number' name='funding' step='any' required><br>"
             "<label>Break Even</label><input type='number' name='breakEven' step='any' value='0'><br>"
             "<label>Effective OH</label><input type='number' name='effectiveOverhead' step='any' value='0'><br>"
             "<label>Exit TP (price/unit)</label><input type='number' name='exitTakeProfit' step='any' value='0'><br>"
             "<label>Exit SL (price/unit)</label><input type='number' name='exitStopLoss' step='any' value='0'><br>"
             "<label>SL Fraction</label><input type='number' name='stopLossFraction' step='any' value='0'><br>"
             "<label>Direction</label><select name='isShort'>"
             "<option value='0'>LONG</option><option value='1'>SHORT</option></select><br>"
             "<button>Create Entry</button></form>"
             "<br><a class='btn' href='/chains/" << chainId << "'>Back</a>";
        res.set_content(html::wrap("Create Entry", h.str()), "text/html");
    });

    // ========== POST /chains/:id/create-entry — save new entry and attach to cycle ==========
    svr.Post(R"(/chains/(\d+)/create-entry)", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(dbMutex);
        int chainId = std::stoi(req.matches[1]);
        auto f = parseForm(req.body);
        int cycle = fi(f, "cycle");

        // Create the entry point
        TradeDatabase::EntryPoint ep;
        ep.entryId           = db.nextEntryId();
        ep.symbol            = normalizeSymbol(fv(f, "symbol"));
        ep.levelIndex        = 0;
        ep.entryPrice        = fd(f, "entryPrice");
        ep.fundingQty        = fd(f, "fundingQty");
        ep.funding           = fd(f, "funding");
        ep.breakEven         = fd(f, "breakEven");
        ep.effectiveOverhead = fd(f, "effectiveOverhead");
        ep.exitTakeProfit    = fd(f, "exitTakeProfit");
        ep.exitStopLoss      = fd(f, "exitStopLoss");
        ep.stopLossFraction  = fd(f, "stopLossFraction");
        ep.isShort           = (fv(f, "isShort") == "1");
        ep.traded            = false;
        ep.linkedTradeId     = -1;
        ep.stopLossActive    = (ep.stopLossFraction > 0.0);

        // Save entry point
        auto entries = db.loadEntryPoints();
        entries.push_back(ep);
        db.saveEntryPoints(entries);

        // Attach to the chain cycle
        auto chains = db.loadManagedChains();
        for (auto& c : chains)
            if (c.chainId == chainId)
            {
                bool found = false;
                for (auto& cy : c.cycles)
                    if (cy.cycle == cycle)
                    { cy.entryIds.push_back(ep.entryId); found = true; break; }
                if (!found)
                {
                    TradeDatabase::ManagedChain::CycleEntries ce;
                    ce.cycle = cycle;
                    ce.entryIds.push_back(ep.entryId);
                    c.cycles.push_back(ce);
                }
                break;
            }
        db.saveManagedChains(chains);

        res.set_redirect("/chains/" + std::to_string(chainId) + "?msg=Entry+" + std::to_string(ep.entryId) + "+created", 303);
    });
}
