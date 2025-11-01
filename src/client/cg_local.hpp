// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

// cg_local.hpp (Client Game)
// This is the primary header for the client-side game module (cgame). It
// includes the shared `bg_local.hpp` header and defines the global interfaces
// for communication between the client game logic and the main engine.
//
// Key Responsibilities:
// - Defines the `cgi` (cgame_import_t) global, which provides function pointers
//   for the cgame module to call into the main engine (e.g., rendering, sound,
//   cvar access).
// - Defines the `cglobals` (cgame_export_t) global, which the cgame module
//   populates with function pointers for the engine to call into it (e.g.,
//   Init, Shutdown, DrawHUD).
// - Provides convenience macros for accessing timing information like
//   SERVER_TICK_RATE and FRAME_TIME_MS.

#pragma once

#include "../shared/bg_local.hpp"

extern cgame_import_t cgi;
extern cgame_export_t cglobals;

#define SERVER_TICK_RATE cgi.tickRate // in hz
#define FRAME_TIME_S cgi.frameTimeSec
#define FRAME_TIME_MS cgi.frameTimeMs
