#include "field.h"
#include "user.h"

#define DEBUG_OPEN

void Field::handle(field_msg::AddUser& m)
{
    std::cerr << "F+ " << x_ << ' ' << y_ << std::endl;
	observers_.insert(m.user);
	encode();
	m.user->send(encoded_);
}

void Field::handle(field_msg::RemoveUser& m)
{
    std::cerr << "F- " << x_ << ' ' << y_ << std::endl;
	observers_.erase(m.user);
}

void Field::handle(field_msg::OpenInternal& m)
{
#ifdef DEBUG_OPEN
    std::cerr << "OpenInteranl " << x_ << ' ' << y_ << ' '  <<m.x << ' ' << m.y << std::endl;
#endif
    open(m.x, m.y);
}

void Field::handle(field_msg::FlagInternal& m)
{
    flag(m.x, m.y, m.set, true);
}

void Field::handle(field_msg::Flag& m)
{
    if (m.x < 0 || m.y < 0 || m.x >= Field::W || m.y >= Field::W)
        return;
    if (m.wide && wide_flag(m.x, m.y) || !m.wide && flag(m.x, m.y, m.set))
    {
#ifdef DEBUG_OPEN
        std::cerr << "Broadcast1 " << x_ << ' ' << y_ << std::endl;
#endif
        broadcast(m.wide);
    }
}

void Field::handle(field_msg::Open& m)
{
    if (m.x < 0 || m.y < 0 || m.x >= Field::W || m.y >= Field::W)
        return;
    if (m.wide && wide_open(m.x, m.y) || !m.wide && open(m.x, m.y))
    {
#ifdef DEBUG_OPEN
        std::cerr << "Broadcast1 " << x_ << ' ' << y_ << std::endl;
#endif
        broadcast(m.wide);
    }
}

void Field::handle(field_msg::Broadcast& m)
{
    broadcast(m.force);
}

void Field::broadcast(int force)
{
#ifdef DEBUG_OPEN
        std::cerr << "CHeck B " << x_ << ' ' << y_ << std::endl;
#endif
    if (!encoding_needed_ && force <= 0)
        return;
#ifdef DEBUG_OPEN
        std::cerr << "Do B " << x_ << ' ' << y_ << std::endl;
#endif
    encode();
    for(int i = 0; i < 4; i ++)
        if (connected_[i])
            connected_[i]->c << field_msg::Broadcast{force-1};
    for(auto& o:observers_)
    {
#ifdef DEBUG_OPEN
        std::cerr << " broadcast to " << o << std::endl;
#endif
        o->send(encoded_);
    }
}

bool Field::open(int x, int y)
{
#ifdef DEBUG_OPEN
    std::cerr << "try open " << x_ << ' ' << y_ << ' ' << x << ' ' << y << std::endl;
#endif
    if (x < 0)
    {
        if (connected_[L])
            connected_[L]->c << field_msg::OpenInternal{x+W, y};
        return false;
    }
    if (y < 0)
    {
        if (connected_[U])
            connected_[U]->c << field_msg::OpenInternal{x, y+W};
        return false;
    }
    if (x >= W)
    {
        if (connected_[R])
            connected_[R]->c << field_msg::OpenInternal{x-W, y};
        return false;
    }
    if (y >= W)
    {
        if (connected_[D])
            connected_[D]->c << field_msg::OpenInternal{x, y-W};
        return false;
    }

    if (has_bomb_[y+1][x+1])
    {
        // explode !
        return false;
    }
    if (is_open_[x+y*W])
        return false;
    is_open_[x+y*W] = true;
    encoding_needed_ = true;

#ifdef DEBUG_OPEN
    std::cerr << "opening " << x_ << ' ' << y_ << ' ' << x << ' ' << y << std::endl;
#endif
    // if (n_bomb == 0)
    if (
        !has_bomb_[y  ][x  ] &&
        !has_bomb_[y  ][x+2] &&
        !has_bomb_[y+2][x  ] &&
        !has_bomb_[y+2][x+2] &&
        !has_bomb_[y+1][x  ] &&
        !has_bomb_[y+1][x+2] &&
        !has_bomb_[y  ][x+1] &&
        !has_bomb_[y+2][x+1])
    {
#ifdef DEBUG_OPEN
    std::cerr << "open recurse " << x_ << ' ' << y_ << std::endl;
#endif
        open(x-1, y);
        open(x+1, y);

        open(x-1, y-1);
        open(x, y-1);
        open(x+1, y-1);

        open(x-1, y+1);
        open(x, y+1);
        open(x+1, y+1);
    }
    return true;
}
void Field::encode()
{
    if (encoding_needed_)
    {
        encoding_needed_ = false;
        //compress();
        std::vector<char> v(W*W);
        for(int idx = 0 ; idx < W*W; idx++)
        {
            int x = idx % W;
            int y = idx / W;
            if (is_open_[idx])
            {
                if (has_bomb_[y+1][x+1])
                    v[idx] = 11;
                else
                {
                    v[idx] = 
                        (has_bomb_[y  ][x  ]?1:0)+
                        (has_bomb_[y  ][x+2]?1:0)+
                        (has_bomb_[y+2][x  ]?1:0)+
                        (has_bomb_[y+2][x+2]?1:0)+
                        (has_bomb_[y+1][x  ]?1:0)+
                        (has_bomb_[y+1][x+2]?1:0)+
                        (has_bomb_[y  ][x+1]?1:0)+
                        (has_bomb_[y+2][x+1]?1:0);
                }
            }
            else if (has_flag_[y+1][x+1])
            {
                v[idx] = 10;
            }
            else if (has_debris_[idx])
            {
                v[idx] = 12;
            }
            else
            {
                v[idx] = 9;
            }
        }
        v.push_back((char)(x_ & 255));
        v.push_back((char)((x_>>8) & 255));
        v.push_back((char)(y_ & 255));
        v.push_back((char)((y_>>8) & 255));
        auto sz = compressBound(v.size());
        encoded_.resize(sz);
        compress((unsigned char*)&encoded_[0], &sz, (unsigned char*)&v[0], v.size());
        encoded_.resize(sz);
    }
}

bool Field::flag(int x, int y, bool set, bool by_internal)
{
    std::cerr << "F" << x_ << ' ' << y_ << ' ' << x << ' ' << y << ' ' << set <<' ' <<by_internal << std::endl;
    if (x >= 0 && y >= 0 && x < W && y < W)
    {
        int idx = y*W+x;
        if (is_open_[idx])
        {
            std::cerr << "F is_o" << x_ << ' ' << y_ << ' ' << x << ' ' << y << ' ' << set <<' ' <<by_internal << std::endl;
            return false;
        }
        if (has_flag_[y+1][x+1] != set)
        {
            std::cerr << "F diff" << x_ << ' ' << y_ << ' ' << x << ' ' << y << ' ' << set <<' ' <<by_internal << std::endl;
            encoding_needed_ = true;
        }
        else
            return false;
    }
    else if (!by_internal)
    {
        if (x < 0 && connected_[L])
            connected_[L]->c << field_msg::FlagInternal{x+W, y, set};
        if (x >= W && connected_[R])
            connected_[R]->c << field_msg::FlagInternal{x-W, y, set};
        if (y < 0 && connected_[U])
            connected_[U]->c << field_msg::FlagInternal{x, y+W, set};
        if (y >= W && connected_[D])
            connected_[D]->c << field_msg::FlagInternal{x, y-W, set};
        return true;
    }

    if (has_flag_[y+1][x+1] == set)
        return false;
    has_flag_[y+1][x+1] = set;
    if (x == 0 && connected_[L])
    {
        connected_[L]->c << field_msg::FlagInternal{x+W, y, set};
    }
    if (x == W-1 && connected_[R])
    {
        connected_[R]->c << field_msg::FlagInternal{x-W, y, set};
    }
    if (y == 0 && connected_[U])
    {
        connected_[U]->c << field_msg::FlagInternal{x, y+W, set};
    }
    if (y == W-1 && connected_[D])
    {
        connected_[D]->c << field_msg::FlagInternal{x, y-W, set};
    }
    return true;
}

void Field::connect(int dir, Field* another) noexcept
{
    using namespace bitset_hack;
    connected_[dir] = another;
    another->connected_[dir^1] = this;
    switch(dir)
    {
        case U:
            {
                another->has_bomb_[W+1] |= has_bomb_[1];
                has_bomb_[0] |= another->has_bomb_[W];
                if (another->connected_[L])
                    another->connected_[L]->has_bomb_[W+1][W+1] |= has_bomb_[1][1];
                if (another->connected_[R])
                    another->connected_[R]->has_bomb_[W+1][0] |= has_bomb_[1][W];
            }
            break;
        case D:
            {
                has_bomb_[W+1] |= another->has_bomb_[1];
                another->has_bomb_[0] |= has_bomb_[W];
                if (another->connected_[L])
                    another->connected_[L]->has_bomb_[0][W+1] |= has_bomb_[W][1];
                if (another->connected_[R])
                    another->connected_[R]->has_bomb_[0][0] |= has_bomb_[W][W];
            }
            break;
        case L:
            {
                for(int i = 0; i < WW; i ++)
                {
                    another->has_bomb_[i][W+1] |= has_bomb_[i][1];
                    has_bomb_[i][0] = another->has_bomb_[i][W];
                }
                if (another->connected_[U])
                    another->connected_[U]->has_bomb_[W+1][W+1] |= has_bomb_[1][1];
                if (another->connected_[D])
                    another->connected_[D]->has_bomb_[0][W+1] |= has_bomb_[W][1];
            }
            break;
        case R:
            {
                for(int i = 0; i < WW; i ++)
                {
                    has_bomb_[i][W+1] |= another->has_bomb_[i][1];
                    another->has_bomb_[i][0] = has_bomb_[i][W];
                }
                if (another->connected_[U])
                    another->connected_[U]->has_bomb_[W+1][0] |= has_bomb_[1][W];
                if (another->connected_[D])
                    another->connected_[D]->has_bomb_[0][0] |= has_bomb_[W][W];
            }
            break;
    }
}

bool Field::wide_open(int x, int y)
{
    int n_bomb = 
        (has_bomb_[y  ][x  ]?1:0)+
        (has_bomb_[y  ][x+2]?1:0)+
        (has_bomb_[y+2][x  ]?1:0)+
        (has_bomb_[y+2][x+2]?1:0)+
        (has_bomb_[y+1][x  ]?1:0)+
        (has_bomb_[y+1][x+2]?1:0)+
        (has_bomb_[y  ][x+1]?1:0)+
        (has_bomb_[y+2][x+1]?1:0);
    int n_flag = 
        (has_flag_[y  ][x  ]?1:0)+
        (has_flag_[y  ][x+2]?1:0)+
        (has_flag_[y+2][x  ]?1:0)+
        (has_flag_[y+2][x+2]?1:0)+
        (has_flag_[y+1][x  ]?1:0)+
        (has_flag_[y+1][x+2]?1:0)+
        (has_flag_[y  ][x+1]?1:0)+
        (has_flag_[y+2][x+1]?1:0);
    std::cerr << (has_flag_[y  ][x  ]?1:0) <<
        (has_flag_[y  ][x+2]?1:0)<<
        (has_flag_[y+2][x  ]?1:0)<<
        (has_flag_[y+2][x+2]?1:0)<<
        (has_flag_[y+1][x  ]?1:0)<<
        (has_flag_[y+1][x+2]?1:0)<<
        (has_flag_[y  ][x+1]?1:0)<<
        (has_flag_[y+2][x+1]?1:0);
    std::cerr << "wo " << n_flag << ' ' << n_bomb << std::endl;
    if (n_flag != n_bomb) 
    {
        return false;
    }

    if (!has_flag_[y+1][x]) open(x-1,y);
    if (!has_flag_[y+1][x+2]) open(x+1,y);

    if (!has_flag_[y][x]) open(x-1,y-1);
    if (!has_flag_[y][x+1]) open(x,y-1);
    if (!has_flag_[y][x+2]) open(x+1,y-1);

    if (!has_flag_[y+2][x]) open(x-1,y+1);
    if (!has_flag_[y+2][x+1]) open(x,y+1);
    if (!has_flag_[y+2][x+2]) open(x+1,y+1);
    return true;
}
bool Field::wide_flag(int x, int y)
{
    auto quick_is_open_i = [&](Field* f, int x, int y,auto quick_is_open)->bool
    {
        if (x < 0)
            return !f->connected_[L] || quick_is_open(f->connected_[L], x+Field::W, y, quick_is_open);
        if (y < 0)
            return !f->connected_[U] || quick_is_open(f->connected_[U], x, y+Field::W, quick_is_open);
        if (x >= Field::W)
            return !f->connected_[R] || quick_is_open(f->connected_[R], x-Field::W, y, quick_is_open);
        if (y >= Field::W)
            return !f->connected_[D] || quick_is_open(f->connected_[D], x, y-Field::W, quick_is_open);
        return (bool)f->is_open_[x+y*Field::W];
    };
    auto quick_is_open = [&](int x, int y) { return quick_is_open_i(this,x,y,quick_is_open_i); };
    int n_open = 
        (quick_is_open(x-1,y-1)?1:0)+
        (quick_is_open(x-1,y  )?1:0)+
        (quick_is_open(x-1,y+1)?1:0)+
        (quick_is_open(x  ,y-1)?1:0)+
        (quick_is_open(x  ,y+1)?1:0)+
        (quick_is_open(x+1,y-1)?1:0)+
        (quick_is_open(x+1,y  )?1:0)+
        (quick_is_open(x+1,y+1)?1:0);
    int n_bomb = 
        (has_bomb_[y  ][x  ]?1:0)+
        (has_bomb_[y  ][x+2]?1:0)+
        (has_bomb_[y+2][x  ]?1:0)+
        (has_bomb_[y+2][x+2]?1:0)+
        (has_bomb_[y+1][x  ]?1:0)+
        (has_bomb_[y+1][x+2]?1:0)+
        (has_bomb_[y  ][x+1]?1:0)+
        (has_bomb_[y+2][x+1]?1:0);
    int n_flag = 
        (has_flag_[y  ][x  ]?1:0)+
        (has_flag_[y  ][x+2]?1:0)+
        (has_flag_[y+2][x  ]?1:0)+
        (has_flag_[y+2][x+2]?1:0)+
        (has_flag_[y+1][x  ]?1:0)+
        (has_flag_[y+1][x+2]?1:0)+
        (has_flag_[y  ][x+1]?1:0)+
        (has_flag_[y+2][x+1]?1:0);
    std::cerr << 8-n_open << ' ' << n_bomb << ' ' << n_flag << std::endl;
    if (8-n_open != n_bomb || 8-n_open == n_flag) 
    {
        return false;
    }

    if (!has_flag_[y+1][x]) flag(x-1,y,true);
    if (!has_flag_[y+1][x+2]) flag(x+1,y,true);

    if (!has_flag_[y][x]) flag(x-1,y-1,true);
    if (!has_flag_[y][x+1]) flag(x,y-1,true);
    if (!has_flag_[y][x+2]) flag(x+1,y-1,true);

    if (!has_flag_[y+2][x]) flag(x-1,y+1,true);
    if (!has_flag_[y+2][x+1]) flag(x,y+1,true);
    if (!has_flag_[y+2][x+2]) flag(x+1,y+1,true);
    return true;
}
