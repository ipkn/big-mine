#pragma once

#include <boost/variant.hpp>

class User;

namespace field_msg 
{
	struct AddUser
	{
		User* user;
	};

	struct RemoveUser
	{
		User* user;
	};

    struct Flag
    {
        int x;
        int y;
        bool set;
        bool wide;
    };

    struct FlagInternal
    {
        int x;
        int y;
        bool set;
    };

    struct Open
    {
        int x;
        int y;
        bool wide;
    };

    struct OpenInternal
    {
        int x;
        int y;
    };

    struct Broadcast
    {
        int force{0};
    };

	using type = boost::variant<AddUser, RemoveUser, Open, Broadcast, OpenInternal, Flag, FlagInternal>;
}
