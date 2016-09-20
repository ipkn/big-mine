#include "board.h"
#include "field.h"

Field* Board::get(int x, int y)
{
    if (x < 0 || y < 0 || x >= FieldCount || y >= FieldCount)
        return nullptr;
    if (fields_[y][x] == nullptr)
    {
        fields_[y][x] = new Field(x, y, gen_);
        if (y > 0 && fields_[y-1][x])
            fields_[y][x]->connect(Field::U, fields_[y-1][x]);
        if (x > 0 && fields_[y][x-1])
            fields_[y][x]->connect(Field::L, fields_[y][x-1]);
        if (y < FieldCount-1 && fields_[y+1][x])
            fields_[y][x]->connect(Field::D, fields_[y+1][x]);
        if (x < FieldCount-1 && fields_[y][x+1])
            fields_[y][x]->connect(Field::R, fields_[y][x+1]);
    }
    return fields_[y][x];
}
