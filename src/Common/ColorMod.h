// Copyright (c) 2019, The Fandom Gold Project
//
// This file is part of Fandom Gold.
//
// Fandom Gold is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Fandom Gold is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with Fandom Gold.  If not, see <http://www.gnu.org/licenses/>.#include <ostream>

namespace Color {
    enum Code {
	DEFAULT          = 0,
	BOLD             = 1,
	DIM              = 2,
	UNDERLINED       = 3,
	BLINK            = 5,
	REVERSE          = 7,
	HIDDEN           = 8,
	FG_BLACK         = 30,
	FG_RED           = 31,
	FG_GREEN         = 32,
	FG_YELLOW        = 33,
	FG_BLUE          = 34,
	FG_MAGENTA       = 35,
	FG_CYAN          = 36,
	FG_LIGHT_GRAY    = 37,
	FG_DEFAULT       = 39,
	FG_DARK_GRAY     = 90,
	FG_LIGHT_RED     = 91,
	FG_LIGHT_GREEN   = 92,
	FG_LIGHT_YELLOW  = 93,
	FG_LIGHT_BLUE    = 94,
	FG_LIGHT_MAGENTA = 95,
	FG_LIGHT_CYAN    = 96,
	FG_WHITE         = 97,
	BG_RED           = 41,
	BG_GREEN         = 42,
	BG_YELLOW	 = 43,
	BG_BLUE          = 44,
        BG_DARK_GRAY     =100,
        BG_LIGHT_RED     =101,
        BG_LIGHT_GREEN   =102,
        BG_LIGHT_YELLOW  =103,
        BG_LIGHT_BLUE    =104,
        BG_LIGHT_MAGENTA =105,
        BG_LIGHT_CYAN    =106,
        BG_White         =107,
	BG_DEFAULT       = 49
    };
    class Modifier {
        Code code;
    public:
        Modifier(Code pCode) : code(pCode) {}
        friend std::ostream&
        operator<<(std::ostream& os, const Modifier& mod) {
            return os << "\033[" << mod.code << "m";
        }
    };
}
