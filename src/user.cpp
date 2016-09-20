#include "user.h"
#include "field.h"
#include "field_msg.h"
#include "board.h"
#include <goplus.h>
#include <random>
#include <chrono>
#include <boost/function_output_iterator.hpp>

std::mt19937 gen(std::chrono::system_clock::now().time_since_epoch().count());

User::User(crow::websocket::connection& conn) 
    : conn_(conn)
{
    goplus::go+ [this] { run(); };

    int x = gen() % Board::TotalSize;
    int y = gen() % Board::TotalSize;

    see(x, y);
    conn_.send_text("{\"m\":\"see\",\"x\":"+std::to_string(x_)+",\"y\":"+std::to_string(y_)+"}");
}

User::~User()
{
    for(auto* f:sights_)
    {
        f->c << field_msg::RemoveUser{this};
    }
}

void User::see(int x, int y)
{
    if (x / Board::FieldSize == x_ / Board::FieldSize && y / Board::FieldSize == y_ / Board::FieldSize)
        return;
    auto new_sights = calc_sights(x, y);
    std::set_difference(sights_.begin(), sights_.end(), new_sights.begin(), new_sights.end(), 
            boost::make_function_output_iterator([&](Field* f)
                {
                    f->c << field_msg::RemoveUser{this};
                }));
    std::set_difference(new_sights.begin(), new_sights.end(), sights_.begin(), sights_.end(), 
            boost::make_function_output_iterator([&](Field* f)
                {
                    f->c << field_msg::AddUser{this};
                }));
    sights_ = std::move(new_sights);
    x_ = x;
    y_ = y;
}

std::set<Field*> User::calc_sights(int x, int y)
{
    int x1 = (x-User::Sight) / Board::FieldSize;
    int y1 = (y-User::Sight) / Board::FieldSize;
    int x2 = (x+User::Sight+Board::FieldSize-1) / Board::FieldSize;
    int y2 = (y+User::Sight+Board::FieldSize-1) / Board::FieldSize;
    std::set<Field*> ret;
    for(int fy = y1; fy <= y2; fy++)
    for(int fx = x1; fx <= x2; fx++)
    {
        Field* f = Board::Instance().get(fx,fy);
        if (f)
        {
            ret.insert(f);
        }
    }
    return ret;
}
void User::recv(const std::string& msg, bool is_binary)
{
    if (!is_binary)
    {
        if (msg[0] == '0')
        {
            // open
            std::istringstream is(msg);
            int fx, fy, x, y, wide;
            is >> x >> fx >> fy >> x >> y >> wide;
            Field* f = Board::Instance().get(fx, fy);
            std::cerr<< "!" << fx << ' '<< fy << ' ' << x <<' ' << y <<' ' << f << std::endl;
            if (f)
                f->c << field_msg::Open{x, y, wide != 0};
        }
        else if (msg[0] == '1')
        {
            // flag
            std::istringstream is(msg);
            int fx, fy, x, y, b, wide;
            is >> x >> fx >> fy >> x >> y >> b >> wide;
            Field* f = Board::Instance().get(fx, fy);
            std::cerr<< "F" << fx << ' '<< fy << ' ' << x <<' ' << y <<' ' << f << std::endl;
            if (f)
                f->c << field_msg::Flag{x, y, b != 0, wide != 0};
        }
        else if (msg[0] == '2')
        {
            // see
            std::istringstream is(msg);
            int x, y;
            is >> x >> x >> y;
            see(x, y);
        }
    }
}

void User::run()
{
    while(1)
    {
        std::string msg;
        bool ok;
        c >> std::tie(msg, ok);
        if (!ok)
            break;
        //conn_.send_binary(msg);
    }
}

