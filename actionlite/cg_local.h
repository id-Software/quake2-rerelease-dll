// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

// g_local.h -- local definitions for game module
#pragma once

#include "bg_local.h"

extern cgame_import_t cgi;
extern cgame_export_t cglobals;

#define SERVER_TICK_RATE cgi.tick_rate // in hz
#define FRAME_TIME_S cgi.frame_time_s
#define FRAME_TIME_MS cgi.frame_time_ms