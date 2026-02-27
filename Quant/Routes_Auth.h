#pragma once

#include "AppContext.h"
#include "HtmlHelpers.h"
#include <sstream>

inline void registerAuthRoutes(httplib::Server& svr, AppContext& ctx)
{
    // ========== GET /login ==========
    svr.Get("/login", [&](const httplib::Request& req, httplib::Response& res) {
        std::string msg;
        if (req.has_param("error")) msg = req.get_param_value("error");
        if (req.has_param("ok"))    msg = req.get_param_value("ok");

        std::ostringstream pg;
        pg << "<!DOCTYPE html><html><head><meta charset='utf-8'>"
              "<meta name='viewport' content='width=device-width,initial-scale=1'>"
              "<title>Login - Quant</title>"
           << html::css()
           << "<style>"
              ".auth-wrap{display:flex;justify-content:center;align-items:center;min-height:80vh;}"
              ".auth-box{background:#0f1b2d;border:1px solid #1a2744;border-radius:8px;padding:32px;width:340px;}"
              ".auth-box h2{color:#c9a44a;margin-bottom:16px;font-size:1.2em;text-align:center;}"
              ".auth-box label{display:block;color:#64748b;font-size:0.85em;margin:8px 0 3px;}"
              ".auth-box input[type=text],.auth-box input[type=password],.auth-box input[type=email]{"
              "width:100%;padding:8px;background:#0b1426;border:1px solid #1a2744;color:#cbd5e1;"
              "border-radius:4px;font-family:inherit;font-size:0.9em;}"
              ".auth-box input:focus{border-color:#c9a44a;outline:none;}"
              ".auth-box button{width:100%;padding:8px;margin-top:14px;background:#166534;color:#fff;"
              "border:none;border-radius:4px;cursor:pointer;font-family:inherit;font-size:0.9em;}"
              ".auth-box button:hover{background:#15803d;}"
              ".auth-box .link{text-align:center;margin-top:12px;font-size:0.82em;}"
              ".auth-box .link a{color:#7b97c4;}"
              ".auth-msg{padding:6px 10px;border-radius:4px;margin-bottom:8px;font-size:0.82em;text-align:center;}"
              ".auth-msg.err{background:#7f1d1d;color:#fca5a5;border:1px solid #991b1b;}"
              ".auth-msg.ok{background:#14532d;color:#86efac;border:1px solid #166534;}"
              "</style></head><body>";
        pg << "<div class='auth-wrap'><div class='auth-box'>"
              "<h2>&#9733; Quant Login</h2>";
        if (!msg.empty())
        {
            bool isOk = req.has_param("ok");
            pg << "<div class='auth-msg " << (isOk ? "ok" : "err") << "'>"
               << html::esc(msg) << "</div>";
        }
        pg << "<form method='POST' action='/login'>"
              "<label>Username</label>"
              "<input type='text' name='username' required autofocus>"
              "<label>Password</label>"
              "<input type='password' name='password' required>"
              "<button type='submit'>Login</button>"
              "</form>"
              "<div class='link'>No account? <a href='/register'>Register</a></div>"
              "</div></div></body></html>";
        res.set_content(pg.str(), "text/html");
    });

    // ========== POST /login ==========
    svr.Post("/login", [&](const httplib::Request& req, httplib::Response& res) {
        auto f = parseForm(req.body);
        std::string username = fv(f, "username");
        std::string password = fv(f, "password");

        if (!ctx.users.authenticate(username, password))
        {
            res.set_redirect("/login?error=Invalid+username+or+password", 303);
            return;
        }

        std::string token = ctx.users.createSession(username);
        res.set_header("Set-Cookie",
            std::string(UserManager::cookieName()) + "=" + token + "; Path=/; HttpOnly; SameSite=Lax");
        res.set_redirect("/", 303);
    });

    // ========== GET /register ==========
    svr.Get("/register", [&](const httplib::Request& req, httplib::Response& res) {
        std::string msg;
        if (req.has_param("error")) msg = req.get_param_value("error");

        std::ostringstream pg;
        pg << "<!DOCTYPE html><html><head><meta charset='utf-8'>"
              "<meta name='viewport' content='width=device-width,initial-scale=1'>"
              "<title>Register - Quant</title>"
           << html::css()
           << "<style>"
              ".auth-wrap{display:flex;justify-content:center;align-items:center;min-height:80vh;}"
              ".auth-box{background:#0f1b2d;border:1px solid #1a2744;border-radius:8px;padding:32px;width:340px;}"
              ".auth-box h2{color:#c9a44a;margin-bottom:16px;font-size:1.2em;text-align:center;}"
              ".auth-box label{display:block;color:#64748b;font-size:0.85em;margin:8px 0 3px;}"
              ".auth-box input[type=text],.auth-box input[type=password],.auth-box input[type=email]{"
              "width:100%;padding:8px;background:#0b1426;border:1px solid #1a2744;color:#cbd5e1;"
              "border-radius:4px;font-family:inherit;font-size:0.9em;}"
              ".auth-box input:focus{border-color:#c9a44a;outline:none;}"
              ".auth-box button{width:100%;padding:8px;margin-top:14px;background:#1e40af;color:#fff;"
              "border:none;border-radius:4px;cursor:pointer;font-family:inherit;font-size:0.9em;}"
              ".auth-box button:hover{background:#1d4ed8;}"
              ".auth-box .link{text-align:center;margin-top:12px;font-size:0.82em;}"
              ".auth-box .link a{color:#7b97c4;}"
              ".auth-msg.err{background:#7f1d1d;color:#fca5a5;border:1px solid #991b1b;"
              "padding:6px 10px;border-radius:4px;margin-bottom:8px;font-size:0.82em;text-align:center;}"
              "</style></head><body>";
        pg << "<div class='auth-wrap'><div class='auth-box'>"
              "<h2>&#9733; Create Account</h2>";
        if (!msg.empty())
            pg << "<div class='auth-msg err'>" << html::esc(msg) << "</div>";
        pg << "<form method='POST' action='/register'>"
              "<label>Username</label>"
              "<input type='text' name='username' required autofocus pattern='[A-Za-z0-9_]+' "
              "title='Letters, digits, underscores only'>"
              "<label>Email (optional)</label>"
              "<input type='email' name='email'>"
              "<label>Password (min 4 chars)</label>"
              "<input type='password' name='password' required minlength='4'>"
              "<label>Confirm Password</label>"
              "<input type='password' name='password2' required minlength='4'>"
              "<button type='submit'>Register</button>"
              "</form>"
              "<div class='link'>Already registered? <a href='/login'>Login</a></div>"
              "</div></div></body></html>";
        res.set_content(pg.str(), "text/html");
    });

    // ========== POST /register ==========
    svr.Post("/register", [&](const httplib::Request& req, httplib::Response& res) {
        auto f = parseForm(req.body);
        std::string username  = fv(f, "username");
        std::string email     = fv(f, "email");
        std::string password  = fv(f, "password");
        std::string password2 = fv(f, "password2");

        if (password != password2)
        {
            res.set_redirect("/register?error=Passwords+do+not+match", 303);
            return;
        }

        std::string err = ctx.users.registerUser(username, password, email);
        if (!err.empty())
        {
            res.set_redirect("/register?error=" + urlEnc(err), 303);
            return;
        }

        res.set_redirect("/login?ok=Account+created.+Please+login.", 303);
    });

    // ========== GET /logout ==========
    svr.Get("/logout", [&](const httplib::Request& req, httplib::Response& res) {
        auto token = AppContext::getSessionToken(req);
        if (!token.empty())
            ctx.users.destroySession(token);
        res.set_header("Set-Cookie",
            std::string(UserManager::cookieName()) + "=; Path=/; Max-Age=0; HttpOnly; SameSite=Lax");
        res.set_redirect("/login?ok=Logged+out", 303);
    });
}
