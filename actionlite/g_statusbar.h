// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

#include <sstream>

// easy statusbar wrapper
struct statusbar_t
{
	std::stringstream sb;
	
	inline auto &yb(int32_t offset) { sb << "yb " << offset << ' '; return *this; }
	inline auto &yt(int32_t offset) { sb << "yt " << offset << ' '; return *this; }
	inline auto &yv(int32_t offset) { sb << "yv " << offset << ' '; return *this; }
	inline auto &xl(int32_t offset) { sb << "xl " << offset << ' '; return *this; }
	inline auto &xr(int32_t offset) { sb << "xr " << offset << ' '; return *this; }
	inline auto &xv(int32_t offset) { sb << "xv " << offset << ' '; return *this; }

	inline auto &ifstat(player_stat_t stat) { sb << "if " << stat << ' '; return *this; }
	inline auto &endifstat() { sb << "endif "; return *this; }

	inline auto &pic(player_stat_t stat) { sb << "pic " << stat << ' '; return *this; }
	inline auto &picn(const char *icon) { sb << "picn " << icon << ' '; return *this; }

	inline auto &anum() { sb << "anum "; return *this; }
	inline auto &rnum() { sb << "rnum "; return *this; }
	inline auto &hnum() { sb << "hnum "; return *this; }
	inline auto &num(int32_t width, player_stat_t stat) { sb << "num " << width << ' ' << stat << ' '; return *this; }
	
	inline auto &loc_stat_string(player_stat_t stat) { sb << "loc_stat_string " << stat << ' '; return *this; }
	inline auto &loc_stat_rstring(player_stat_t stat) { sb << "loc_stat_rstring " << stat << ' '; return *this; }
	inline auto &stat_string(player_stat_t stat) { sb << "stat_string " << stat << ' '; return *this; }
	inline auto &loc_stat_cstring2(player_stat_t stat) { sb << "loc_stat_cstring2 " << stat << ' '; return *this; }
	inline auto &string2(const char *str)
	{
		if (str[0] != '"' && (strchr(str, ' ') || strchr(str, '\n')))
			sb << "string2 \"" << str << "\" ";
		else
			sb << "string2 " << str << ' ';
		return *this;
	}
	inline auto &string(const char *str)
	{
		if (str[0] != '"' && (strchr(str, ' ') || strchr(str, '\n')))
			sb << "string \"" << str << "\" ";
		else
			sb << "string " << str << ' ';
		return *this;
	}
	inline auto &loc_rstring(const char *str)
	{
		if (str[0] != '"' && (strchr(str, ' ') || strchr(str, '\n')))
			sb << "loc_rstring 0 \"" << str << "\" ";
		else
			sb << "loc_rstring 0 " << str << ' ';
		return *this;
	}

	inline auto &lives_num(player_stat_t stat) { sb << "lives_num " << stat << ' '; return *this; }
	inline auto &stat_pname(player_stat_t stat) { sb << "stat_pname " << stat << ' '; return *this; }

	inline auto &health_bars() { sb << "health_bars "; return *this; }
	inline auto &story() { sb << "story "; return *this; }
};