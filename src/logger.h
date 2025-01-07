// Copyright © 2025 Cory Petkovsek, Roope Palmroos, and Contributors.

#ifndef LOGGER_CLASS_H
#define LOGGER_CLASS_H

#include "core/string/print_string.h"

class Terrain3DLogger {
private:
    static int debug_level;

public:
    enum AssetType {
		TYPE_TEXTURE,
		TYPE_MESH,
	};
    static inline const int MAX_TEXTURES = 32;
	static inline const int MAX_MESHES = 256;
    static void set_debug_level(int level) { debug_level = level; }
    static int get_debug_level() { return debug_level; }
};
/**
 * Prints warnings, errors, and regular messages to the console.
 * Regular messages are filtered based on the user specified debug level.
 * Warnings and errors always print except in release builds.
 * EXTREME is for continuously called prints like inside snapping
 *
 * Note that in DEBUG mode Godot will crash on quit due to an
 * access violation in editor_log.cpp EditorLog::_process_message().
 * This is most likely caused by us printing messages as Godot is
 * attempting to quit.
 */
#define MESG -2 // Always print
#define WARN -1 // Always print
#define ERROR 0 // Always print
#define INFO 1
#define DEBUG 2
#define EXTREME 3
#ifdef DEBUG_ENABLED
#define TERRAINLOG(level, ...)                                                              \
if (level == ERROR)                                                              \
print_line("ERROR", __VA_ARGS__);   \
else if (level == WARN)                                                          \
print_line("WARNING", __VA_ARGS__); \
else if (Terrain3DLogger::get_debug_level() >= level)                                        \
print_line(__VA_ARGS__);
#else
#define TERRAINLOG(...)
#endif
#endif // LOGGER_CLASS_H