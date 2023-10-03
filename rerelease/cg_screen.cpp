// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
#include "cg_local.h"

constexpr int32_t STAT_MINUS      = 10;  // num frame for '-' stats digit
constexpr const char *sb_nums[2][11] =
{
    {   "num_0", "num_1", "num_2", "num_3", "num_4", "num_5",
        "num_6", "num_7", "num_8", "num_9", "num_minus"
    },
    {   "anum_0", "anum_1", "anum_2", "anum_3", "anum_4", "anum_5",
        "anum_6", "anum_7", "anum_8", "anum_9", "anum_minus"
    }
};

constexpr int32_t CHAR_WIDTH    = 16;
constexpr int32_t CONCHAR_WIDTH = 8;

static int32_t font_y_offset;

constexpr rgba_t alt_color { 112, 255, 52, 255 };

static cvar_t *scr_usekfont;

static cvar_t *scr_centertime;
static cvar_t *scr_printspeed;
static cvar_t *cl_notifytime;
static cvar_t *scr_maxlines;
static cvar_t *ui_acc_contrast;
static cvar_t* ui_acc_alttypeface;

// static temp data used for hud
static struct
{
    struct {
        struct {
            char    text[24];
        } table_cells[6];
    } table_rows[11]; // just enough to store 8 levels + header + total (+ one slack)

    size_t column_widths[6];
    int32_t num_rows = 0;
    int32_t num_columns = 0;
} hud_temp;

#include <vector>

// max number of centerprints in the rotating buffer
constexpr size_t MAX_CENTER_PRINTS = 4;

struct cl_bind_t {
    std::string bind;
    std::string purpose;
};

struct cl_centerprint_t {
    std::vector<cl_bind_t> binds; // binds

    std::vector<std::string> lines;
    bool        instant; // don't type out

    size_t      current_line; // current line we're typing out
    size_t      line_count; // byte count to draw on current line
    bool        finished; // done typing it out
    uint64_t    time_tick, time_off; // time to remove at
};

inline bool CG_ViewingLayout(const player_state_t *ps)
{
    return ps->stats[STAT_LAYOUTS] & (LAYOUTS_LAYOUT | LAYOUTS_INVENTORY);
}

inline bool CG_InIntermission(const player_state_t *ps)
{
    return ps->stats[STAT_LAYOUTS] & LAYOUTS_INTERMISSION;
}

inline bool CG_HudHidden(const player_state_t *ps)
{
    return ps->stats[STAT_LAYOUTS] & LAYOUTS_HIDE_HUD;
}

layout_flags_t CG_LayoutFlags(const player_state_t *ps)
{
    return (layout_flags_t) ps->stats[STAT_LAYOUTS];
}

#include <optional>
#include <array>

constexpr size_t MAX_NOTIFY = 8;

struct cl_notify_t {
    std::string     message; // utf8 message
    bool            is_active; // filled or not
    bool            is_chat; // green or not
    uint64_t        time; // rotate us when < CL_Time()
};

// per-splitscreen client hud storage
struct hud_data_t {
    std::array<cl_centerprint_t, MAX_CENTER_PRINTS> centers; // list of centers
    std::optional<size_t> center_index; // current index we're drawing, or unset if none left
    std::array<cl_notify_t, MAX_NOTIFY> notify; // list of notifies
};

static std::array<hud_data_t, MAX_SPLIT_PLAYERS> hud_data;

void CG_ClearCenterprint(int32_t isplit)
{
    hud_data[isplit].center_index = {};
}

void CG_ClearNotify(int32_t isplit)
{
    for (auto &msg : hud_data[isplit].notify)
        msg.is_active = false;
}

// if the top one is expired, cycle the ones ahead backwards (since
// the times are always increasing)
static void CG_Notify_CheckExpire(hud_data_t &data)
{
    while (data.notify[0].is_active && data.notify[0].time < cgi.CL_ClientTime())
    {
        data.notify[0].is_active = false;

        for (size_t i = 1; i < MAX_NOTIFY; i++)
            if (data.notify[i].is_active)
                std::swap(data.notify[i], data.notify[i - 1]);
    }
}

// add notify to list
static void CG_AddNotify(hud_data_t &data, const char *msg, bool is_chat)
{
    size_t i = 0;

    if (scr_maxlines->integer <= 0)
        return;

    const int max = min(MAX_NOTIFY, (size_t)scr_maxlines->integer);

    for (; i < max; i++)
        if (!data.notify[i].is_active)
            break;

    // none left, so expire the topmost one
    if (i == max)
    {
        data.notify[0].time = 0;
        CG_Notify_CheckExpire(data);
        i = max - 1;
    }
    
    data.notify[i].message.assign(msg);
    data.notify[i].is_active = true;
    data.notify[i].is_chat = is_chat;
    data.notify[i].time = cgi.CL_ClientTime() + (cl_notifytime->value * 1000);
}

// draw notifies
static void CG_DrawNotify(int32_t isplit, vrect_t hud_vrect, vrect_t hud_safe, int32_t scale)
{
    auto &data = hud_data[isplit];

    CG_Notify_CheckExpire(data);

    int y;
    
    y = (hud_vrect.y * scale) + hud_safe.y;

    cgi.SCR_SetAltTypeface(ui_acc_alttypeface->integer && true);

    if (ui_acc_contrast->integer)
    {
        for (auto& msg : data.notify)
        {
            if (!msg.is_active || !msg.message.length())
                break;

            vec2_t sz = cgi.SCR_MeasureFontString(msg.message.c_str(), scale);
            sz.x += 10; // extra padding for black bars
            cgi.SCR_DrawColorPic((hud_vrect.x * scale) + hud_safe.x - 5, y, sz.x, 15 * scale, "_white", rgba_black);
            y += 10 * scale;
        }
    }

    y = (hud_vrect.y * scale) + hud_safe.y;
    for (auto &msg : data.notify)
    {
        if (!msg.is_active)
            break;

        cgi.SCR_DrawFontString(msg.message.c_str(), (hud_vrect.x * scale) + hud_safe.x, y, scale, msg.is_chat ? alt_color : rgba_white, true, text_align_t::LEFT);
        y += 10 * scale;
    }

    cgi.SCR_SetAltTypeface(false);

    // draw text input (only the main player can really chat anyways...)
    if (isplit == 0)
    {
        const char *input_msg;
        bool input_team;

        if (cgi.CL_GetTextInput(&input_msg, &input_team))
            cgi.SCR_DrawFontString(G_Fmt("{}: {}", input_team ? "say_team" : "say", input_msg).data(), (hud_vrect.x * scale) + hud_safe.x, y, scale, rgba_white, true, text_align_t::LEFT);
    }
}

/*
==============
CG_DrawHUDString
==============
*/
static int CG_DrawHUDString (const char *string, int x, int y, int centerwidth, int _xor, int scale, bool shadow = true)
{
    int     margin;
    char    line[1024];
    int     width;
    int     i;

    margin = x;

    while (*string)
    {
        // scan out one line of text from the string
        width = 0;
        while (*string && *string != '\n')
            line[width++] = *string++;
        line[width] = 0;

        vec2_t size;
        
        if (scr_usekfont->integer)
            size = cgi.SCR_MeasureFontString(line, scale);

        if (centerwidth)
        {
            if (!scr_usekfont->integer)
                x = margin + ((centerwidth - width*CONCHAR_WIDTH*scale))/2;
            else
                x = margin + ((centerwidth - size.x))/2;
        }
        else
            x = margin;

        if (!scr_usekfont->integer)
        {
            for (i=0 ; i<width ; i++)
            {
                cgi.SCR_DrawChar (x, y, scale, line[i]^_xor, shadow);
                x += CONCHAR_WIDTH * scale;
            }
        }
        else
        {
            cgi.SCR_DrawFontString(line, x, y - (font_y_offset * scale), scale, _xor ? alt_color : rgba_white, true, text_align_t::LEFT);
            x += size.x;
        }

        if (*string)
        {
            string++;   // skip the \n
            x = margin;
            if (!scr_usekfont->integer)
                y += CONCHAR_WIDTH * scale;
            else
                // TODO
                y += 10 * scale;//size.y;
        }
    }

    return x;
}

// Shamefully stolen from Kex
size_t FindStartOfUTF8Codepoint(const std::string &str, size_t pos)
{
    if(pos >= str.size())
    {
        return std::string::npos;
    }

    for(ptrdiff_t i = pos; i >= 0; i--)
    {
        const char &ch = str[i];

        if((ch & 0x80) == 0)
        {
            // character is one byte
            return i;
        }
        else if((ch & 0xC0) == 0x80)
        {
            // character is part of a multi-byte sequence, keep going
            continue;
        }
        else
        {
            // character is the start of a multi-byte sequence, so stop now
            return i;
        }
    }

    return std::string::npos;
}

size_t FindEndOfUTF8Codepoint(const std::string &str, size_t pos)
{
    if(pos >= str.size())
    {
        return std::string::npos;
    }

    for(size_t i = pos; i < str.size(); i++)
    {
        const char &ch = str[i];

        if((ch & 0x80) == 0)
        {
            // character is one byte
            return i;
        }
        else if((ch & 0xC0) == 0x80)
        {
            // character is part of a multi-byte sequence, keep going
            continue;
        }
        else
        {
            // character is the start of a multi-byte sequence, so stop now
            return i;
        }
    }

    return std::string::npos;
}

void CG_NotifyMessage(int32_t isplit, const char *msg, bool is_chat)
{
    CG_AddNotify(hud_data[isplit], msg, is_chat);
}

// centerprint stuff
static cl_centerprint_t &CG_QueueCenterPrint(int isplit, bool instant)
{
    auto &icl = hud_data[isplit];

    // just use first index
    if (!icl.center_index.has_value() || instant)
    {
        icl.center_index = 0;

        for (size_t i = 1; i < MAX_CENTER_PRINTS; i++)
            icl.centers[i].lines.clear();

        return icl.centers[0];
    }

    // pick the next free index if we can find one
    for (size_t i = 1; i < MAX_CENTER_PRINTS; i++)
    {
        auto &center = icl.centers[(icl.center_index.value() + i) % MAX_CENTER_PRINTS];

        if (center.lines.empty())
            return center;
    }
    
    // none, so update the current one (the new end of buffer)
    // and skip ahead
    auto &center = icl.centers[icl.center_index.value()];
    icl.center_index = (icl.center_index.value() + 1) % MAX_CENTER_PRINTS;
    return center;
}

/*
==============
SCR_CenterPrint

Called for important messages that should stay in the center of the screen
for a few moments
==============
*/
void CG_ParseCenterPrint (const char *str, int isplit, bool instant) // [Sam-KEX] Made 1st param const
{
    const char    *s;
    char    line[64];
    int     i, j, l;

    // handle center queueing
    cl_centerprint_t &center = CG_QueueCenterPrint(isplit, instant);

    center.lines.clear();

    // split the string into lines
    size_t line_start = 0;

    std::string string(str);

    center.binds.clear();

    // [Paril-KEX] pull out bindings. they'll always be at the start
    while (string.compare(0, 6, "%bind:") == 0)
    {
        size_t end_of_bind = string.find_first_of('%', 1);

        if (end_of_bind == std::string::npos)
            break;

        std::string bind = string.substr(6, end_of_bind - 6);

        if (auto purpose_index = bind.find_first_of(':'); purpose_index != std::string::npos)
            center.binds.emplace_back(cl_bind_t { bind.substr(0, purpose_index), bind.substr(purpose_index + 1) });
        else
            center.binds.emplace_back(cl_bind_t { bind });

        string = string.substr(end_of_bind + 1);
    }

    // echo it to the console
    cgi.Com_Print("\n\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n\n");

    s = string.c_str();
    do
    {
        // scan the width of the line
        for (l=0 ; l<40 ; l++)
            if (s[l] == '\n' || !s[l])
                break;
        for (i=0 ; i<(40-l)/2 ; i++)
            line[i] = ' ';

        for (j=0 ; j<l ; j++)
        {
            line[i++] = s[j];
        }

        line[i] = '\n';
        line[i+1] = 0;

        cgi.Com_Print(line);

        while (*s && *s != '\n')
            s++;

        if (!*s)
            break;
        s++;        // skip the \n
    } while (1);
    cgi.Com_Print("\n\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n\n");
    CG_ClearNotify (isplit);

    for (size_t line_end = 0; ; )
    {
        line_end = FindEndOfUTF8Codepoint(string, line_end);

        if (line_end == std::string::npos)
        {
            // final line
            if (line_start < string.size())
                center.lines.emplace_back(string.c_str() + line_start);
            break;
        }
        
        // char part of current line;
        // if newline, end line and cut off
        const char &ch = string[line_end];

        if (ch == '\n')
        {
            if (line_end > line_start)
                center.lines.emplace_back(string.c_str() + line_start, line_end - line_start);
            else
                center.lines.emplace_back();
            line_start = line_end + 1;
            line_end++;
            continue;
        }

         line_end++;
    }

    if (center.lines.empty())
    {
        center.finished = true;
        return;
    }

    center.time_tick = cgi.CL_ClientRealTime() + (scr_printspeed->value * 1000);
    center.instant = instant;
    center.finished = false;
    center.current_line = 0;
    center.line_count = 0;
}

static void CG_DrawCenterString( const player_state_t *ps, const vrect_t &hud_vrect, const vrect_t &hud_safe, int isplit, int scale, cl_centerprint_t &center)
{
    int32_t y = hud_vrect.y * scale;
    
    if (CG_ViewingLayout(ps))
        y += hud_safe.y;
    else if (center.lines.size() <= 4)
        y += (hud_vrect.height * 0.2f) * scale;
    else
        y += 48 * scale;

    int lineHeight = (scr_usekfont->integer ? 10 : 8) * scale;
    if (ui_acc_alttypeface->integer) lineHeight *= 1.5f;

    // easy!
    if (center.instant)
    {
        for (size_t i = 0; i < center.lines.size(); i++)
        {
            auto &line = center.lines[i];

            cgi.SCR_SetAltTypeface(ui_acc_alttypeface->integer && true);

            if (ui_acc_contrast->integer && line.length())
            {
                vec2_t sz = cgi.SCR_MeasureFontString(line.c_str(), scale);
                sz.x += 10; // extra padding for black bars
                int barY = ui_acc_alttypeface->integer ? y - 8 : y;
                cgi.SCR_DrawColorPic((hud_vrect.x + hud_vrect.width / 2) * scale - (sz.x / 2), barY, sz.x, lineHeight, "_white", rgba_black);
            }
            CG_DrawHUDString(line.c_str(), (hud_vrect.x + hud_vrect.width/2 + -160) * scale, y, (320 / 2) * 2 * scale, 0, scale);

            cgi.SCR_SetAltTypeface(false);

            y += lineHeight;
        }

        for (auto &bind : center.binds)
        {
            y += lineHeight * 2;
            cgi.SCR_DrawBind(isplit, bind.bind.c_str(), bind.purpose.c_str(), (hud_vrect.x + (hud_vrect.width / 2)) * scale, y, scale);
        }

        if (!center.finished)
        {
            center.finished = true;
            center.time_off = cgi.CL_ClientRealTime() + (scr_centertime->value * 1000);
        }

        return;
    }

    // hard and annoying!
    // check if it's time to fetch a new char
    const uint64_t t = cgi.CL_ClientRealTime();

    if (!center.finished)
    {
        if (center.time_tick < t)
        {
            center.time_tick = t + (scr_printspeed->value * 1000);
            center.line_count = FindEndOfUTF8Codepoint(center.lines[center.current_line], center.line_count + 1);

            if (center.line_count == std::string::npos)
            {
                center.current_line++;
                center.line_count = 0;

                if (center.current_line == center.lines.size())
                {
                    center.current_line--;
                    center.finished = true;
                    center.time_off = t + (scr_centertime->value * 1000);
                }
            }
        }
    }

    // smallish byte buffer for single line of data...
    char buffer[256];

    for (size_t i = 0; i < center.lines.size(); i++)
    {
        cgi.SCR_SetAltTypeface(ui_acc_alttypeface->integer && true);

        auto &line = center.lines[i];

        buffer[0] = 0;

        if (center.finished || i != center.current_line)
            Q_strlcpy(buffer, line.c_str(), sizeof(buffer));
        else
            Q_strlcpy(buffer, line.c_str(), min(center.line_count + 1, sizeof(buffer)));

        int blinky_x;

        if (ui_acc_contrast->integer && line.length())
        {
            vec2_t sz = cgi.SCR_MeasureFontString(line.c_str(), scale);
            sz.x += 10; // extra padding for black bars
            int barY = ui_acc_alttypeface->integer ? y - 8 : y;
            cgi.SCR_DrawColorPic((hud_vrect.x + hud_vrect.width / 2) * scale - (sz.x / 2), barY, sz.x, lineHeight, "_white", rgba_black);
        }
        
        if (buffer[0])
            blinky_x = CG_DrawHUDString(buffer, (hud_vrect.x + hud_vrect.width/2 + -160) * scale, y, (320 / 2) * 2 * scale, 0, scale);
        else
            blinky_x = (hud_vrect.width / 2) * scale;

        cgi.SCR_SetAltTypeface(false);

        if (i == center.current_line && !ui_acc_alttypeface->integer)
            cgi.SCR_DrawChar(blinky_x, y, scale, 10 + ((cgi.CL_ClientRealTime() >> 8) & 1), true);

        y += lineHeight;

        if (i == center.current_line)
            break;
    }
}

static void CG_CheckDrawCenterString( const player_state_t *ps, const vrect_t &hud_vrect, const vrect_t &hud_safe, int isplit, int scale )
{
    if (CG_InIntermission(ps))
        return;
    if (!hud_data[isplit].center_index.has_value())
        return;

    auto &data = hud_data[isplit];
    auto &center = data.centers[data.center_index.value()];

    // ran out of center time
    if (center.finished && center.time_off < cgi.CL_ClientRealTime())
    {
        center.lines.clear();

        size_t next_index = (data.center_index.value() + 1) % MAX_CENTER_PRINTS;
        auto &next_center = data.centers[next_index];

        // no more
        if (next_center.lines.empty())
        {
            data.center_index.reset();
            return;
        }

        // buffer rotated; start timer now
        data.center_index = next_index;
        next_center.current_line = next_center.line_count = 0;
    }

    if (!data.center_index.has_value())
        return;

    CG_DrawCenterString( ps, hud_vrect, hud_safe, isplit, scale, data.centers[data.center_index.value()] );
}

/*
==============
CG_DrawString
==============
*/
static void CG_DrawString (int x, int y, int scale, const char *s, bool alt = false, bool shadow = true)
{
    while (*s)
    {
        cgi.SCR_DrawChar (x, y, scale, *s ^ (alt ? 0x80 : 0), shadow);
        x+=8*scale;
        s++;
    }
}

#include <charconv>

/*
==============
CG_DrawField
==============
*/
static void CG_DrawField (int x, int y, int color, int width, int value, int scale)
{
    char    num[16], *ptr;
    int     l;
    int     frame;

    if (width < 1)
        return;

    // draw number string
    if (width > 5)
        width = 5;

    auto result = std::to_chars(num, num + sizeof(num) - 1, value);
    *(result.ptr) = '\0';

    l = (result.ptr - num);

    if (l > width)
        l = width;

    x += (2 + CHAR_WIDTH*(width - l)) * scale;

    ptr = num;
    while (*ptr && l)
    {
        if (*ptr == '-')
            frame = STAT_MINUS;
        else
            frame = *ptr -'0';
        int w, h;
        cgi.Draw_GetPicSize(&w, &h, sb_nums[color][frame]);
        cgi.SCR_DrawPic(x, y, w * scale, h * scale, sb_nums[color][frame]);
        x += CHAR_WIDTH * scale;
        ptr++;
        l--;
    }
}

// [Paril-KEX]
static void CG_DrawTable(int x, int y, uint32_t width, uint32_t height, int32_t scale)
{
    // half left
    int32_t width_pixels = width;
    x -= width_pixels / 2;
    y += CONCHAR_WIDTH * scale;
    // use Y as top though

    int32_t height_pixels = height;
    
    // draw border
    // KEX_FIXME method that requires less chars
    cgi.SCR_DrawChar(x - (CONCHAR_WIDTH * scale), y - (CONCHAR_WIDTH * scale), scale, 18, false);
    cgi.SCR_DrawChar((x + width_pixels), y - (CONCHAR_WIDTH * scale), scale, 20, false);
    cgi.SCR_DrawChar(x - (CONCHAR_WIDTH * scale), y + height_pixels, scale, 24, false);
    cgi.SCR_DrawChar((x + width_pixels), y + height_pixels, scale, 26, false);

    for (int cx = x; cx < x + width_pixels; cx += CONCHAR_WIDTH * scale)
    {
        cgi.SCR_DrawChar(cx, y - (CONCHAR_WIDTH * scale), scale, 19, false);
        cgi.SCR_DrawChar(cx, y + height_pixels, scale, 25, false);
    }

    for (int cy = y; cy < y + height_pixels; cy += CONCHAR_WIDTH * scale)
    {
        cgi.SCR_DrawChar(x - (CONCHAR_WIDTH * scale), cy, scale, 21, false);
        cgi.SCR_DrawChar((x + width_pixels), cy, scale, 23, false);
    }

    cgi.SCR_DrawColorPic(x, y, width_pixels, height_pixels, "_white", { 0, 0, 0, 255 });

    // draw in columns
    for (int i = 0; i < hud_temp.num_columns; i++)
    {
        for (int r = 0, ry = y; r < hud_temp.num_rows; r++, ry += (CONCHAR_WIDTH + font_y_offset) * scale)
        {
            int x_offset = 0;

            // center 
            if (r == 0)
            {
                x_offset = ((hud_temp.column_widths[i]) / 2) -
                    ((cgi.SCR_MeasureFontString(hud_temp.table_rows[r].table_cells[i].text, scale).x) / 2);
            }
            // right align
            else if (i != 0)
            {
                x_offset = (hud_temp.column_widths[i] - cgi.SCR_MeasureFontString(hud_temp.table_rows[r].table_cells[i].text, scale).x);
            }

            //CG_DrawString(x + x_offset, ry, scale, hud_temp.table_rows[r].table_cells[i].text, r == 0, true);
            cgi.SCR_DrawFontString(hud_temp.table_rows[r].table_cells[i].text, x + x_offset, ry - (font_y_offset * scale), scale, r == 0 ? alt_color : rgba_white, true, text_align_t::LEFT);
        }

        x += (hud_temp.column_widths[i] + cgi.SCR_MeasureFontString(" ", 1).x);
    }
}

/*
================
CG_ExecuteLayoutString

================
*/
static void CG_ExecuteLayoutString (const char *s, vrect_t hud_vrect, vrect_t hud_safe, int32_t scale, int32_t playernum, const player_state_t *ps)
{
    int     x, y;
    int     w, h;
    int     hx, hy;
    int     value;
    const char *token;
    int     width;
    int     index;

    if (!s[0])
        return;

    x = hud_vrect.x;
    y = hud_vrect.y;
    width = 3;

    hx = 320 / 2;
    hy = 240 / 2;

    bool flash_frame = (cgi.CL_ClientTime() % 1000) < 500;

    // if non-zero, parse but don't affect state
    int32_t if_depth = 0; // current if statement depth
    int32_t endif_depth = 0; // at this depth, toggle skip_depth
    bool skip_depth = false; // whether we're in a dead stmt or not

    while (s)
    {
        token = COM_Parse (&s);
        if (!strcmp(token, "xl"))
        {
            token = COM_Parse (&s);
            if (!skip_depth)
                x = ((hud_vrect.x + atoi(token)) * scale) + hud_safe.x;
            continue;
        }
        if (!strcmp(token, "xr"))
        {
            token = COM_Parse (&s);
            if (!skip_depth)
                x = ((hud_vrect.x + hud_vrect.width + atoi(token)) * scale) - hud_safe.x;
            continue;
        }
        if (!strcmp(token, "xv"))
        {
            token = COM_Parse (&s);
            if (!skip_depth)
                x = (hud_vrect.x + hud_vrect.width/2 + (atoi(token) - hx)) * scale;
            continue;
        }

        if (!strcmp(token, "yt"))
        {
            token = COM_Parse (&s);
            if (!skip_depth)
                y = ((hud_vrect.y + atoi(token)) * scale) + hud_safe.y;
            continue;
        }
        if (!strcmp(token, "yb"))
        {
            token = COM_Parse (&s);
            if (!skip_depth)
                y = ((hud_vrect.y + hud_vrect.height + atoi(token)) * scale) - hud_safe.y;
            continue;
        }
        if (!strcmp(token, "yv"))
        {
            token = COM_Parse (&s);
            if (!skip_depth)
                y = (hud_vrect.y + hud_vrect.height/2 + (atoi(token) - hy)) * scale;
            continue;
        }

        if (!strcmp(token, "pic"))
        {   // draw a pic from a stat number
            token = COM_Parse (&s);
            if (!skip_depth)
            {
                value = ps->stats[atoi(token)];
                if (value >= MAX_IMAGES)
                    cgi.Com_Error("Pic >= MAX_IMAGES");

                const char *const pic = cgi.get_configstring(CS_IMAGES + value);

                if (pic && *pic)
                {
                    cgi.Draw_GetPicSize (&w, &h, pic);
                    cgi.SCR_DrawPic (x, y, w * scale, h * scale, pic);
                }
            }

            continue;
        }

        if (!strcmp(token, "client"))
        {   // draw a deathmatch client block
            token = COM_Parse (&s);
            if (!skip_depth)
            {
                x = (hud_vrect.x + hud_vrect.width/2 + (atoi(token) - hx)) * scale;
                x += 8 * scale;
            }
            token = COM_Parse (&s);
            if (!skip_depth)
            {
                y = (hud_vrect.y + hud_vrect.height/2 + (atoi(token) - hy)) * scale;
                y += 7 * scale;
            }

            token = COM_Parse (&s);

            if (!skip_depth)
            {
                value = atoi(token);
                if (value >= MAX_CLIENTS || value < 0)
                    cgi.Com_Error("client >= MAX_CLIENTS");
            }

            int score, ping;

            token = COM_Parse (&s);
            if (!skip_depth)
                score = atoi(token);

            token = COM_Parse (&s);
            if (!skip_depth)
            {
                ping = atoi(token);

                if (!scr_usekfont->integer)
                    CG_DrawString (x + 32 * scale, y, scale, cgi.CL_GetClientName(value));
                else
                    cgi.SCR_DrawFontString(cgi.CL_GetClientName(value), x + 32 * scale, y - (font_y_offset * scale), scale, rgba_white, true, text_align_t::LEFT);
                
                if (!scr_usekfont->integer)
                    CG_DrawString (x + 32 * scale, y + 10 * scale, scale, G_Fmt("{}", score).data(), true);
                else
                    cgi.SCR_DrawFontString(G_Fmt("{}", score).data(), x + 32 * scale, y + (10 - font_y_offset) * scale, scale, rgba_white, true, text_align_t::LEFT);

                cgi.SCR_DrawPic(x + 96 * scale, y + 10 * scale, 9 * scale, 9 * scale, "ping");
                
                if (!scr_usekfont->integer)
                    CG_DrawString (x + 73 * scale + 32 * scale, y + 10 * scale, scale, G_Fmt("{}", ping).data());
                else
                    cgi.SCR_DrawFontString (G_Fmt("{}", ping).data(), x + 107 * scale, y + (10 - font_y_offset) * scale, scale, rgba_white, true, text_align_t::LEFT);
            }
            continue;
        }

        if (!strcmp(token, "ctf"))
        {   // draw a ctf client block
            int     score, ping;

            token = COM_Parse (&s);
            if (!skip_depth)
                x = (hud_vrect.x + hud_vrect.width/2 - hx + atoi(token)) * scale;
            token = COM_Parse (&s);
            if (!skip_depth)
                y = (hud_vrect.y + hud_vrect.height/2 - hy + atoi(token)) * scale;

            token = COM_Parse (&s);
            if (!skip_depth)
            {
                value = atoi(token);
                if (value >= MAX_CLIENTS || value < 0)
                    cgi.Com_Error("client >= MAX_CLIENTS");
            }

            token = COM_Parse (&s);
            if (!skip_depth)
                score = atoi(token);

            token = COM_Parse (&s);
            if (!skip_depth)
            {
                ping = atoi(token);
                if (ping > 999)
                    ping = 999;
            }

            token = COM_Parse (&s);

            if (!skip_depth)
            {

                cgi.SCR_DrawFontString (G_Fmt("{}", score).data(), x, y - (font_y_offset * scale), scale, value == playernum ? alt_color : rgba_white, true, text_align_t::LEFT);
                x += 3 * 9 * scale;
                cgi.SCR_DrawFontString (G_Fmt("{}", ping).data(), x, y - (font_y_offset * scale), scale, value == playernum ? alt_color : rgba_white, true, text_align_t::LEFT);
                x += 3 * 9 * scale;
                cgi.SCR_DrawFontString (cgi.CL_GetClientName(value), x, y - (font_y_offset * scale), scale, value == playernum ? alt_color : rgba_white, true, text_align_t::LEFT);

                if (*token)
                {
                    cgi.Draw_GetPicSize(&w, &h, token);
                    cgi.SCR_DrawPic(x - ((w + 2) * scale), y, w * scale, h * scale, token);
                }
            }
            continue;
        }

        if (!strcmp(token, "picn"))
        {   // draw a pic from a name
            token = COM_Parse (&s);
            if (!skip_depth)
            {
                cgi.Draw_GetPicSize(&w, &h, token);
                cgi.SCR_DrawPic(x, y, w * scale, h * scale, token);
            }
            continue;
        }

        if (!strcmp(token, "num"))
        {   // draw a number
            token = COM_Parse (&s);
            if (!skip_depth)
                width = atoi(token);
            token = COM_Parse (&s);
            if (!skip_depth)
            {
                value = ps->stats[atoi(token)];
                CG_DrawField (x, y, 0, width, value, scale);
            }
            continue;
        }
        // [Paril-KEX] special handling for the lives number
        else if (!strcmp(token, "lives_num"))
        {
            token = COM_Parse (&s);
            if (!skip_depth)
            {
                value = ps->stats[atoi(token)];
                CG_DrawField(x, y, value <= 2 ? flash_frame : 0, 1, max(0, value - 2), scale);
            }
        }

        if (!strcmp(token, "hnum"))
        {
            // health number
            if (!skip_depth)
            {
                int     color;

                width = 3;
                value = ps->stats[STAT_HEALTH];
                if (value > 25)
                    color = 0;  // green
                else if (value > 0)
                    color = flash_frame;      // flash
                else
                    color = 1;
                if (ps->stats[STAT_FLASHES] & 1)
                {
                    cgi.Draw_GetPicSize(&w, &h, "field_3");
                    cgi.SCR_DrawPic(x, y, w * scale, h * scale, "field_3");
                }

                CG_DrawField (x, y, color, width, value, scale);
            }
            continue;
        }

        if (!strcmp(token, "anum"))
        {
            // ammo number
            if (!skip_depth)
            {
                int     color;

                width = 3;
                value = ps->stats[STAT_AMMO];

                int32_t min_ammo = cgi.CL_GetWarnAmmoCount(ps->stats[STAT_ACTIVE_WEAPON]);

                if (!min_ammo)
                    min_ammo = 5; // back compat

                if (value > min_ammo)
                    color = 0;  // green
                else if (value >= 0)
                    color = flash_frame;      // flash
                else
                    continue;   // negative number = don't show
                if (ps->stats[STAT_FLASHES] & 4)
                {
                    cgi.Draw_GetPicSize(&w, &h, "field_3");
                    cgi.SCR_DrawPic(x, y, w * scale, h * scale, "field_3");
                }

                CG_DrawField (x, y, color, width, value, scale);
            }
            continue;
        }

        if (!strcmp(token, "rnum"))
        {
            // armor number
            if (!skip_depth)
            {
                int     color;

                width = 3;
                value = ps->stats[STAT_ARMOR];
                if (value < 0)
                    continue;

                color = 0;  // green
                if (ps->stats[STAT_FLASHES] & 2)
                {
                    cgi.Draw_GetPicSize(&w, &h, "field_3");
                    cgi.SCR_DrawPic(x, y, w * scale, h * scale, "field_3");
                }

                CG_DrawField (x, y, color, width, value, scale);
            }
            continue;
        }

        if (!strcmp(token, "stat_string"))
        {
            token = COM_Parse (&s);

            if (!skip_depth)
            {
                index = atoi(token);
                if (index < 0 || index >= MAX_STATS)
                    cgi.Com_Error("Bad stat_string index");
                index = ps->stats[index];

                if (cgi.CL_ServerProtocol() <= PROTOCOL_VERSION_3XX)
                    index = CS_REMAP(index).start / CS_MAX_STRING_LENGTH;

                if (index < 0 || index >= MAX_CONFIGSTRINGS)
                    cgi.Com_Error("Bad stat_string index");
                if (!scr_usekfont->integer)
                    CG_DrawString (x, y, scale, cgi.get_configstring(index));
                else
                    cgi.SCR_DrawFontString(cgi.get_configstring(index), x, y - (font_y_offset * scale), scale, rgba_white, true, text_align_t::LEFT);
            }
            continue;
        }

        if (!strcmp(token, "cstring"))
        {
            token = COM_Parse (&s);
            if (!skip_depth)
                CG_DrawHUDString (token, x, y, hx*2*scale, 0, scale);
            continue;
        }

        if (!strcmp(token, "string"))
        {
            token = COM_Parse (&s);
            if (!skip_depth)
            {
                if (!scr_usekfont->integer)
                    CG_DrawString (x, y, scale, token);
                else
                    cgi.SCR_DrawFontString(token, x, y - (font_y_offset * scale), scale, rgba_white, true, text_align_t::LEFT);
            }
            continue;
        }

        if (!strcmp(token, "cstring2"))
        {
            token = COM_Parse (&s);
            if (!skip_depth)
                CG_DrawHUDString (token, x, y, hx*2*scale, 0x80, scale);
            continue;
        }

        if (!strcmp(token, "string2"))
        {
            token = COM_Parse (&s);
            if (!skip_depth)
            {
                if (!scr_usekfont->integer)
                    CG_DrawString (x, y, scale, token, true);
                else
                    cgi.SCR_DrawFontString(token, x, y - (font_y_offset * scale), scale, alt_color, true, text_align_t::LEFT);
            }
            continue;
        }

        if (!strcmp(token, "if"))
        {
            // if stmt
            token = COM_Parse (&s);

            if_depth++;

            // skip to endif
            if (!skip_depth && !ps->stats[atoi(token)])
            {
                skip_depth = true;
                endif_depth = if_depth;
            }

            continue;
        }

        if (!strcmp(token, "ifgef"))
        {
            // if stmt
            token = COM_Parse (&s);

            if_depth++;

            // skip to endif
            if (!skip_depth && cgi.CL_ServerFrame() < atoi(token))
            {
                skip_depth = true;
                endif_depth = if_depth;
            }

            continue;
        }

        if (!strcmp(token, "endif"))
        {
            if (skip_depth && (if_depth == endif_depth))
                skip_depth = false;

            if_depth--;

            if (if_depth < 0)
                cgi.Com_Error("endif without matching if");

            continue;
        }

        // localization stuff
        if (!strcmp(token, "loc_stat_string"))
        {
            token = COM_Parse (&s);

            if (!skip_depth)
            {
                index = atoi(token);
                if (index < 0 || index >= MAX_STATS)
                    cgi.Com_Error("Bad stat_string index");
                index = ps->stats[index];

                if (cgi.CL_ServerProtocol() <= PROTOCOL_VERSION_3XX)
                    index = CS_REMAP(index).start / CS_MAX_STRING_LENGTH;

                if (index < 0 || index >= MAX_CONFIGSTRINGS)
                    cgi.Com_Error("Bad stat_string index");
                if (!scr_usekfont->integer)
                    CG_DrawString (x, y, scale, cgi.Localize(cgi.get_configstring(index), nullptr, 0));
                else
                    cgi.SCR_DrawFontString(cgi.Localize(cgi.get_configstring(index), nullptr, 0), x, y - (font_y_offset * scale), scale, rgba_white, true, text_align_t::LEFT);
            }
            continue;
        }

        if (!strcmp(token, "loc_stat_rstring"))
        {
            token = COM_Parse (&s);

            if (!skip_depth)
            {
                index = atoi(token);
                if (index < 0 || index >= MAX_STATS)
                    cgi.Com_Error("Bad stat_string index");
                index = ps->stats[index];

                if (cgi.CL_ServerProtocol() <= PROTOCOL_VERSION_3XX)
                    index = CS_REMAP(index).start / CS_MAX_STRING_LENGTH;

                if (index < 0 || index >= MAX_CONFIGSTRINGS)
                    cgi.Com_Error("Bad stat_string index");
                const char *s = cgi.Localize(cgi.get_configstring(index), nullptr, 0);
                if (!scr_usekfont->integer)
                    CG_DrawString (x - (strlen(s) * CONCHAR_WIDTH * scale), y, scale, s);
                else
                {
                    vec2_t size = cgi.SCR_MeasureFontString(s, scale);
                    cgi.SCR_DrawFontString(s, x - size.x, y - (font_y_offset * scale), scale, rgba_white, true, text_align_t::LEFT);
                }
            }
            continue;
        }
        
        if (!strcmp(token, "loc_stat_cstring"))
        {
            token = COM_Parse (&s);

            if (!skip_depth)
            {
                index = atoi(token);
                if (index < 0 || index >= MAX_STATS)
                    cgi.Com_Error("Bad stat_string index");
                index = ps->stats[index];

                if (cgi.CL_ServerProtocol() <= PROTOCOL_VERSION_3XX)
                    index = CS_REMAP(index).start / CS_MAX_STRING_LENGTH;

                if (index < 0 || index >= MAX_CONFIGSTRINGS)
                    cgi.Com_Error("Bad stat_string index");
                CG_DrawHUDString (cgi.Localize(cgi.get_configstring(index), nullptr, 0), x, y, hx*2*scale, 0, scale);
            }
            continue;
        }

        if (!strcmp(token, "loc_stat_cstring2"))
        {
            token = COM_Parse (&s);

            if (!skip_depth)
            {
                index = atoi(token);
                if (index < 0 || index >= MAX_STATS)
                    cgi.Com_Error("Bad stat_string index");
                index = ps->stats[index];

                if (cgi.CL_ServerProtocol() <= PROTOCOL_VERSION_3XX)
                    index = CS_REMAP(index).start / CS_MAX_STRING_LENGTH;

                if (index < 0 || index >= MAX_CONFIGSTRINGS)
                    cgi.Com_Error("Bad stat_string index");
                CG_DrawHUDString (cgi.Localize(cgi.get_configstring(index), nullptr, 0), x, y, hx*2*scale, 0x80, scale);
            }
            continue;
        }

        static char arg_tokens[MAX_LOCALIZATION_ARGS + 1][MAX_TOKEN_CHARS];
        static const char *arg_buffers[MAX_LOCALIZATION_ARGS];

        if (!strcmp(token, "loc_cstring"))
        {
            int32_t num_args = atoi(COM_Parse (&s));

            if (num_args < 0 || num_args >= MAX_LOCALIZATION_ARGS)
                cgi.Com_Error("Bad loc string");

            // parse base
            token = COM_Parse (&s);
            Q_strlcpy(arg_tokens[0], token, sizeof(arg_tokens[0]));

            // parse args
            for (int32_t i = 0; i < num_args; i++)
            {
                token = COM_Parse (&s);
                Q_strlcpy(arg_tokens[1 + i], token, sizeof(arg_tokens[0]));
                arg_buffers[i] = arg_tokens[1 + i];
            }

            if (!skip_depth)
                CG_DrawHUDString (cgi.Localize(arg_tokens[0], arg_buffers, num_args), x, y, hx*2*scale, 0, scale);
            continue;
        }

        if (!strcmp(token, "loc_string"))
        {
            int32_t num_args = atoi(COM_Parse (&s));

            if (num_args < 0 || num_args >= MAX_LOCALIZATION_ARGS)
                cgi.Com_Error("Bad loc string");

            // parse base
            token = COM_Parse (&s);
            Q_strlcpy(arg_tokens[0], token, sizeof(arg_tokens[0]));

            // parse args
            for (int32_t i = 0; i < num_args; i++)
            {
                token = COM_Parse (&s);
                Q_strlcpy(arg_tokens[1 + i], token, sizeof(arg_tokens[0]));
                arg_buffers[i] = arg_tokens[1 + i];
            }
            
            if (!skip_depth)
            {
                if (!scr_usekfont->integer)
                    CG_DrawString (x, y, scale, cgi.Localize(arg_tokens[0], arg_buffers, num_args));
                else
                    cgi.SCR_DrawFontString(cgi.Localize(arg_tokens[0], arg_buffers, num_args), x, y - (font_y_offset * scale), scale, rgba_white, true, text_align_t::LEFT);
            }
            continue;
        }

        if (!strcmp(token, "loc_cstring2"))
        {
            int32_t num_args = atoi(COM_Parse (&s));

            if (num_args < 0 || num_args >= MAX_LOCALIZATION_ARGS)
                cgi.Com_Error("Bad loc string");

            // parse base
            token = COM_Parse (&s);
            Q_strlcpy(arg_tokens[0], token, sizeof(arg_tokens[0]));

            // parse args
            for (int32_t i = 0; i < num_args; i++)
            {
                token = COM_Parse (&s);
                Q_strlcpy(arg_tokens[1 + i], token, sizeof(arg_tokens[0]));
                arg_buffers[i] = arg_tokens[1 + i];
            }
            
            if (!skip_depth)
                CG_DrawHUDString (cgi.Localize(arg_tokens[0], arg_buffers, num_args), x, y, hx*2*scale, 0x80, scale);
            continue;
        }

        if (!strcmp(token, "loc_string2") || !strcmp(token, "loc_rstring2") ||
            !strcmp(token, "loc_string") || !strcmp(token, "loc_rstring"))
        {
            bool green = token[strlen(token) - 1] == '2';
            bool rightAlign = !Q_strncasecmp(token, "loc_rstring", strlen("loc_rstring"));
            int32_t num_args = atoi(COM_Parse (&s));

            if (num_args < 0 || num_args >= MAX_LOCALIZATION_ARGS)
                cgi.Com_Error("Bad loc string");

            // parse base
            token = COM_Parse (&s);
            Q_strlcpy(arg_tokens[0], token, sizeof(arg_tokens[0]));

            // parse args
            for (int32_t i = 0; i < num_args; i++)
            {
                token = COM_Parse (&s);
                Q_strlcpy(arg_tokens[1 + i], token, sizeof(arg_tokens[0]));
                arg_buffers[i] = arg_tokens[1 + i];
            }
            
            if (!skip_depth)
            {
                const char *locStr = cgi.Localize(arg_tokens[0], arg_buffers, num_args);
                int xOffs = 0;
                if (rightAlign)
                {
                    xOffs = scr_usekfont->integer ? cgi.SCR_MeasureFontString(locStr, scale).x : (strlen(locStr) * CONCHAR_WIDTH * scale);
                }

                if (!scr_usekfont->integer)
                    CG_DrawString (x - xOffs, y, scale, locStr, green);
                else
                    cgi.SCR_DrawFontString(locStr, x - xOffs, y - (font_y_offset * scale), scale, green ? alt_color : rgba_white, true, text_align_t::LEFT);
            }
            continue;
        }

        // draw time remaining
        if (!strcmp(token, "time_limit"))
        {
            // end frame
            token = COM_Parse (&s);

            if (!skip_depth)
            {
                int32_t end_frame = atoi(token);

                if (end_frame < cgi.CL_ServerFrame())
                    continue;

                uint64_t remaining_ms = (end_frame - cgi.CL_ServerFrame()) * cgi.frame_time_ms;

                const bool green = true;
                arg_buffers[0] = G_Fmt("{:02}:{:02}", (remaining_ms / 1000) / 60, (remaining_ms / 1000) % 60).data();

                const char *locStr = cgi.Localize("$g_score_time", arg_buffers, 1);
                int xOffs = scr_usekfont->integer ? cgi.SCR_MeasureFontString(locStr, scale).x : (strlen(locStr) * CONCHAR_WIDTH * scale);
                if (!scr_usekfont->integer)
                    CG_DrawString (x - xOffs, y, scale, locStr, green);
                else
                    cgi.SCR_DrawFontString(locStr, x - xOffs, y - (font_y_offset * scale), scale, green ? alt_color : rgba_white, true, text_align_t::LEFT);
            }
        }

        // draw client dogtag
        if (!strcmp(token, "dogtag"))
        {
            token = COM_Parse (&s);
            
            if (!skip_depth)
            {
                value = atoi(token);
                if (value >= MAX_CLIENTS || value < 0)
                    cgi.Com_Error("client >= MAX_CLIENTS");

                const std::string_view path = G_Fmt("/tags/{}", cgi.CL_GetClientDogtag(value));
                cgi.SCR_DrawPic(x, y, 198 * scale, 32 * scale, path.data());
            }
        }

        if (!strcmp(token, "start_table"))
        {
            token = COM_Parse (&s);
            value = atoi(token);

            if (!skip_depth)
            {
                if (value >= q_countof(hud_temp.table_rows[0].table_cells))
                    cgi.Com_Error("table too big");

                hud_temp.num_columns = value;
                hud_temp.num_rows = 1;

                for (int i = 0; i < value; i++)
                    hud_temp.column_widths[i] = 0;
            }

            for (int i = 0; i < value; i++)
            {
                token = COM_Parse (&s);
                if (!skip_depth)
                {
                    token = cgi.Localize(token, nullptr, 0);
                    Q_strlcpy(hud_temp.table_rows[0].table_cells[i].text, token, sizeof(hud_temp.table_rows[0].table_cells[i].text));
                    hud_temp.column_widths[i] = max(hud_temp.column_widths[i], (size_t) cgi.SCR_MeasureFontString(hud_temp.table_rows[0].table_cells[i].text, scale).x);
                }
            }
        }

        if (!strcmp(token, "table_row"))
        {
            token = COM_Parse (&s);
            value = atoi(token);

            if (!skip_depth)
            {
                if (hud_temp.num_rows >= q_countof(hud_temp.table_rows))
                {
                    cgi.Com_Error("table too big");
                    return;
                }
            }
            
            auto &row = hud_temp.table_rows[hud_temp.num_rows];

            for (int i = 0; i < value; i++)
            {
                token = COM_Parse (&s);
                if (!skip_depth)
                {
                    Q_strlcpy(row.table_cells[i].text, token, sizeof(row.table_cells[i].text));
                    hud_temp.column_widths[i] = max(hud_temp.column_widths[i], (size_t) cgi.SCR_MeasureFontString(row.table_cells[i].text, scale).x);
                }
            }
            
            if (!skip_depth)
            {
                for (int i = value; i < hud_temp.num_columns; i++)
                    row.table_cells[i].text[0] = '\0';

                hud_temp.num_rows++;
            }
        }

        if (!strcmp(token, "draw_table"))
        {
            if (!skip_depth)
            {
                // in scaled pixels, incl padding between elements
                uint32_t total_inner_table_width = 0;

                for (int i = 0; i < hud_temp.num_columns; i++)
                {
                    if (i != 0)
                        total_inner_table_width += cgi.SCR_MeasureFontString(" ", scale).x;

                    total_inner_table_width += hud_temp.column_widths[i];
                }

                // in scaled pixels
                uint32_t total_table_height = hud_temp.num_rows * (CONCHAR_WIDTH + font_y_offset) * scale;

                CG_DrawTable(x, y, total_inner_table_width, total_table_height, scale);
            }
        }

        if (!strcmp(token, "stat_pname"))
        {
            token = COM_Parse(&s);

            if (!skip_depth)
            {
                index = atoi(token);
                if (index < 0 || index >= MAX_STATS)
                    cgi.Com_Error("Bad stat_string index");
                index = ps->stats[index] - 1;

                if (!scr_usekfont->integer)
                    CG_DrawString(x, y, scale, cgi.CL_GetClientName(index));
                else
                    cgi.SCR_DrawFontString(cgi.CL_GetClientName(index), x, y - (font_y_offset * scale), scale, rgba_white, true, text_align_t::LEFT);
            }
            continue;
        }

        if (!strcmp(token, "health_bars"))
        {
            if (skip_depth)
                continue;

            const byte *stat = reinterpret_cast<const byte *>(&ps->stats[STAT_HEALTH_BARS]);
            const char *name = cgi.Localize(cgi.get_configstring(CONFIG_HEALTH_BAR_NAME), nullptr, 0);

            CG_DrawHUDString(name, (hud_vrect.x + hud_vrect.width/2 + -160) * scale, y, (320 / 2) * 2 * scale, 0, scale);

            float bar_width = ((hud_vrect.width * scale) - (hud_safe.x * 2)) * 0.50f;
            float bar_height = 4 * scale;

            y += cgi.SCR_FontLineHeight(scale);

            float x = ((hud_vrect.x + (hud_vrect.width * 0.5f)) * scale) - (bar_width * 0.5f);

            // 2 health bars, hardcoded
            for (size_t i = 0; i < 2; i++, stat++)
            {
                if (!(*stat & 0b10000000))
                    continue;

                float percent = (*stat & 0b01111111) / 127.f;

                cgi.SCR_DrawColorPic(x, y, bar_width + scale, bar_height + scale, "_white", rgba_black);

                if (percent > 0)
                    cgi.SCR_DrawColorPic(x, y, bar_width * percent, bar_height, "_white", rgba_red);
                if (percent < 1)
                    cgi.SCR_DrawColorPic(x + (bar_width * percent), y, bar_width * (1.f - percent), bar_height, "_white", { 80, 80, 80, 255 });

                y += bar_height * 3;
            }
        }

        if (!strcmp(token, "story"))
        {
            const char *story_str = cgi.get_configstring(CONFIG_STORY);

            if (!*story_str)
                continue;

            const char *localized = cgi.Localize(story_str, nullptr, 0);
            vec2_t size = cgi.SCR_MeasureFontString(localized, scale);
            float centerx = ((hud_vrect.x + (hud_vrect.width * 0.5f)) * scale);
            float centery = ((hud_vrect.y + (hud_vrect.height * 0.5f)) * scale) - (size.y * 0.5f);

            cgi.SCR_DrawFontString(localized, centerx, centery, scale, rgba_white, true, text_align_t::CENTER);
        }
    }

    if (skip_depth)
        cgi.Com_Error("if with no matching endif");
}

static cvar_t *cl_skipHud;
static cvar_t *cl_paused;

/*
================
CL_DrawInventory
================
*/
constexpr size_t DISPLAY_ITEMS   = 19;

static void CG_DrawInventory(const player_state_t *ps, const std::array<int16_t, MAX_ITEMS> &inventory, vrect_t hud_vrect, int32_t scale)
{
    int     i;
    int     num, selected_num, item;
    int     index[MAX_ITEMS];
    int     x, y;
    int     width, height;
    int     selected;
    int     top;

    selected = ps->stats[STAT_SELECTED_ITEM];

    num = 0;
    selected_num = 0;
    for (i=0 ; i<MAX_ITEMS ; i++) {
        if ( i == selected ) {
            selected_num = num;
        }
        if ( inventory[i] ) {
            index[num] = i;
            num++;
        }
    }

    // determine scroll point
    top = selected_num - DISPLAY_ITEMS/2;
    if (num - top < DISPLAY_ITEMS)
        top = num - DISPLAY_ITEMS;
    if (top < 0)
        top = 0;

    x = hud_vrect.x * scale;
    y = hud_vrect.y * scale;
    width = hud_vrect.width;
    height = hud_vrect.height;

    x += ((width / 2) - (256 / 2)) * scale;
    y += ((height / 2) - (216 / 2)) * scale;

    int pich, picw;
    cgi.Draw_GetPicSize(&picw, &pich, "inventory");
    cgi.SCR_DrawPic(x, y+8*scale, picw * scale, pich * scale, "inventory");

    y += 27 * scale;
    x += 22 * scale;

    for (i=top ; i<num && i < top+DISPLAY_ITEMS ; i++)
    {
        item = index[i];
        if (item == selected) // draw a blinky cursor by the selected item
        {
            if ( (cgi.CL_ClientRealTime() * 10) & 1)
                cgi.SCR_DrawChar(x-8, y, scale, 15, false);
        }

        if (!scr_usekfont->integer)
        {
            CG_DrawString(x, y, scale,
                G_Fmt("{:3} {}", inventory[item],
                    cgi.Localize(cgi.get_configstring(CS_ITEMS + item), nullptr, 0)).data(),
                item == selected, false);
        }
        else
        {
            const char *string = G_Fmt("{}", inventory[item]).data();
            cgi.SCR_DrawFontString(string, x + (216 * scale) - (16 * scale), y - (font_y_offset * scale), scale, (item == selected) ? alt_color : rgba_white, true, text_align_t::RIGHT);

            string = cgi.Localize(cgi.get_configstring(CS_ITEMS + item), nullptr, 0);
            cgi.SCR_DrawFontString(string, x + (16 * scale), y - (font_y_offset * scale), scale, (item == selected) ? alt_color : rgba_white, true, text_align_t::LEFT);
        }
            
        y += 8 * scale;
    }
}

extern uint64_t cgame_init_time;

void CG_DrawHUD (int32_t isplit, const cg_server_data_t *data, vrect_t hud_vrect, vrect_t hud_safe, int32_t scale, int32_t playernum, const player_state_t *ps)
{
    if (cgi.CL_InAutoDemoLoop())
    {
        if (cl_paused->integer) return; // demo is paused, menu is open

        uint64_t time = cgi.CL_ClientRealTime() - cgame_init_time;
        if (time < 20000 && 
            (time % 4000) < 2000)
            cgi.SCR_DrawFontString(cgi.Localize("$m_eou_press_button", nullptr, 0), hud_vrect.width * 0.5f * scale, (hud_vrect.height - 64.f) * scale, scale, rgba_green, true, text_align_t::CENTER);
        return;
    }

    // draw HUD
    if (!cl_skipHud->integer && !(ps->stats[STAT_LAYOUTS] & LAYOUTS_HIDE_HUD))
        CG_ExecuteLayoutString(cgi.get_configstring(CS_STATUSBAR), hud_vrect, hud_safe, scale, playernum, ps);

    // draw centerprint string
    CG_CheckDrawCenterString(ps, hud_vrect, hud_safe, isplit, scale);

    // draw notify
    CG_DrawNotify(isplit, hud_vrect, hud_safe, scale);

    // svc_layout still drawn with hud off
    if (ps->stats[STAT_LAYOUTS] & LAYOUTS_LAYOUT)
        CG_ExecuteLayoutString(data->layout, hud_vrect, hud_safe, scale, playernum, ps);

    // inventory too
    if (ps->stats[STAT_LAYOUTS] & LAYOUTS_INVENTORY)
        CG_DrawInventory(ps, data->inventory, hud_vrect, scale);
}

/*
================
CG_TouchPics

================
*/
void CG_TouchPics()
{
    for (auto &nums : sb_nums)
        for (auto &str : nums)
            cgi.Draw_RegisterPic(str);

    cgi.Draw_RegisterPic("inventory");

    font_y_offset = (cgi.SCR_FontLineHeight(1) - CONCHAR_WIDTH) / 2;
}

void CG_InitScreen()
{
    cl_paused = cgi.cvar("paused", "0", CVAR_NOFLAGS);
    cl_skipHud = cgi.cvar("cl_skipHud", "0", CVAR_ARCHIVE);
    scr_usekfont = cgi.cvar("scr_usekfont", "1", CVAR_NOFLAGS);

    scr_centertime  = cgi.cvar ("scr_centertime", "5.0",  CVAR_ARCHIVE); // [Sam-KEX] Changed from 2.5
    scr_printspeed  = cgi.cvar ("scr_printspeed", "0.04", CVAR_NOFLAGS); // [Sam-KEX] Changed from 8
    cl_notifytime   = cgi.cvar ("cl_notifytime", "5.0",   CVAR_ARCHIVE);
    scr_maxlines    = cgi.cvar ("scr_maxlines", "4",      CVAR_ARCHIVE);
    ui_acc_contrast = cgi.cvar ("ui_acc_contrast", "0",   CVAR_NOFLAGS);
    ui_acc_alttypeface = cgi.cvar("ui_acc_alttypeface", "0", CVAR_NOFLAGS);

    hud_data = {};
}