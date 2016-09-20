#pragma once

#include <chrono>
#include <random>

class Field;

class Board
{
public:
    static Board& Instance()
    {
        static Board b;
        return b;
    }
    static constexpr int FieldSize = 20;
    static constexpr int FieldCount = 50;
    static constexpr int TotalSize = FieldCount * FieldSize;
    static constexpr int BombProbD = 16;
    static constexpr int BombProbN = 100;
    static constexpr int TotalBombCount = TotalSize*TotalSize/BombProbN*BombProbD;

    Field* get(int x, int y);

private:
    std::mt19937 gen_{static_cast<unsigned int>(std::chrono::high_resolution_clock::now().time_since_epoch().count())};
    Field* fields_[FieldCount][FieldCount]{};
};
