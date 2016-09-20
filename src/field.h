#pragma once

#include <zlib.h>
#include <bitset>
#include <unordered_set>

#include "goplus.h"
#include "board.h"
#include "field_msg.h"

// 0, 1, 2, 3, 4, 5, 6, 7, 8, 9=normal, 10=flag, 11=bomb, 12=debris

namespace bitset_hack
{
template <typename T>
inline T operator |= (T a, T b)
{
    a = (bool)a || (bool)b;
    return a;
}
}

class User;

class Field
{
public:
    static constexpr int W = Board::FieldSize;
    static constexpr int WW = Board::FieldSize+2;
    enum FieldConnectedDirection
    {
        U = 0,
        D = 1,
        L = 2,
        R = 3,
    };

    goplus::chan<field_msg::type> c{goplus::make_chan<field_msg::type>(Board::FieldSize*2)};

    template <typename Gen>
    Field(int x, int y, Gen& g)
        : x_(x), y_(y)
    {
		goplus::go+ [this]{ run(); };
        int n = W*W;
        int k = W*W*Board::BombProbD/Board::BombProbN;
        auto get_bit = [&]{
            auto p = std::generate_canonical<double, 10>(g);
            if (p*n < k)
            {
                n--;
                k--;
                return true;
            }
            else
            {
                n--;
                return false;
            }
        };
        for(int y = 0; y < W; y ++)
        for(int x = 0; x < W; x ++)
        {
            has_bomb_[y+1][x+1] = get_bit();
        }
    }

    void encode();

    bool open(int x, int y);
    bool wide_open(int x, int y);
    bool flag(int x, int y, bool set, bool by_internal = false);
    bool wide_flag(int x, int y);

    void broadcast(int force = 0);


    void connect(int dir, Field* another) noexcept;

    void handle(field_msg::AddUser& m);
    void handle(field_msg::RemoveUser& m);
    void handle(field_msg::Flag& m);
    void handle(field_msg::FlagInternal& m);
    void handle(field_msg::Open& m);
    void handle(field_msg::OpenInternal& m);
    void handle(field_msg::Broadcast& m);

    void run()
    {
        while(1)
        {
            field_msg::type m;
            bool ok;
            c >> std::tie(m, ok);
            if (!ok)
                return;
            boost::apply_visitor([this](auto& msg){this->handle(msg);}, m);
        }
    }
private:
    std::unordered_set<User*> observers_;
    int x_;
    int y_;

    Field* connected_[4]{};
    std::bitset<W*W> is_open_, has_debris_;
    std::bitset<WW> has_bomb_[WW];
    std::bitset<WW> has_flag_[WW];
    bool encoding_needed_{true};
    std::string encoded_;
};
