#pragma once

#include <goplus.h>
#include <set>
#include "crow_all.h"
#include "board.h"

class Field;

class User
{
public:
    goplus::chan<std::string> c{goplus::make_chan<std::string>()};
    User(crow::websocket::connection& conn);
    ~User();

    static std::set<Field*> calc_sights(int x, int y);

    void see(int x, int y);

    void recv(const std::string& msg, bool is_binary);
    void send(const std::string& msg)
    {
        conn_.send_binary(msg);
    }

    void close() 
    {
        c.close();
    }

    void run();

    static constexpr int Sight = 40;
private:
    bool dead_{false};
    int x_{-1000};
    int y_{-1000};
    std::set<Field*> sights_;
    crow::websocket::connection& conn_;
};
