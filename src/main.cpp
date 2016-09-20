#include "crow_all.h"
#include <goplus.h>

#include "user.h"

using namespace goplus;

int main()
{
    crow::SimpleApp app;
    crow::logger::setLogLevel(crow::LogLevel::Debug);

    CROW_ROUTE(app, "/")
    ([&]{
        auto t = crow::mustache::load_text("index.html");
        return t;
    });

    CROW_ROUTE(app, "/static/<path>")
    ([&](const std::string& path){
        if (path.find("..") != path.npos)
            return std::string("");
        return crow::mustache::load_text(path);
    });

    CROW_ROUTE(app, "/ws")
        .websocket()
        .onopen([](crow::websocket::connection& conn){
                auto u = new User(conn);
                conn.userdata(u);

				CROW_LOG_INFO << "new user "  << u;

                })
        .onmessage([](crow::websocket::connection& conn, const std::string& msg, bool is_binary){
                User* u = (User*) conn.userdata();
				CROW_LOG_INFO << "user msg received";
                u->recv(msg, is_binary);
                })
        .onclose([](crow::websocket::connection& conn, const std::string& reason){
                User* u = (User*) conn.userdata();
                u->close();
                delete u;
                });

    app.port(40080).multithreaded().run();
    return 0;
}
