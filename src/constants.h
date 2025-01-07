// Copyright © 2025 Cory Petkovsek, Roope Palmroos, and Contributors.

#ifndef CONSTANTS_CLASS_H
#define CONSTANTS_CLASS_H

// Constants

#define IS_EDITOR Engine::get_singleton()->is_editor_hint()

#define COLOR_NAN Color(NAN, NAN, NAN, NAN)
#define COLOR_BLACK Color(0.0f, 0.0f, 0.0f, 1.0f)
#define COLOR_WHITE Color(1.0f, 1.0f, 1.0f, 1.0f)
#define COLOR_ROUGHNESS Color(1.0f, 1.0f, 1.0f, 0.5f)
#define COLOR_CHECKED Color(1.f, 1.f, 1.0f, -1.0f)
#define COLOR_NORMAL Color(0.5f, 0.5f, 1.0f, 1.0f)
#define COLOR_CONTROL Color(as_float(enc_auto(true)), 0.f, 0.f, 1.0f)

#define V2(x) Vector2(x, x)
#define V2_ZERO Vector2(0.f, 0.f)
#define V2I_ZERO Vector2i(0, 0)
#define V2_MAX Vector2(FLT_MAX, FLT_MAX)
#define V2I_MAX Vector2i(INT32_MAX, INT32_MAX)
#define V3(x) Vector3(x, x, x)
#define V3_ZERO Vector3(0.f, 0.f, 0.f)
#define V3_MAX Vector3(FLT_MAX, FLT_MAX, FLT_MAX)

// Set class name for logger.h

#define CLASS_NAME() const String __class__ = get_class_static() + \
		String("#") + String::num_uint64(get_instance_id()).right(4);

#define CLASS_NAME_STATIC(p_name) static inline const char *__class__ = p_name;

// Validation macros

#define VOID // a return value for void, to avoid compiler warnings

#define IS_INIT(ret)           \
	if (_terrain == nullptr) { \
		return ret;            \
	}

#define IS_INIT_MESG(mesg, ret) \
	if (_terrain == nullptr) {  \
		TERRAINLOG(ERROR, mesg);       \
		return ret;             \
	}

#define IS_INIT_COND(cond, ret)        \
	if (_terrain == nullptr || cond) { \
		return ret;                    \
	}

#define IS_INIT_COND_MESG(cond, mesg, ret) \
	if (_terrain == nullptr || cond) {     \
		TERRAINLOG(ERROR, mesg);                  \
		return ret;                        \
	}

#define IS_INSTANCER_INIT(ret)                                         \
	if (_terrain == nullptr || _terrain->get_instancer() == nullptr) { \
		return ret;                                                    \
	}

#define IS_INSTANCER_INIT_MESG(mesg, ret)                              \
	if (_terrain == nullptr || _terrain->get_instancer() == nullptr) { \
		TERRAINLOG(ERROR, mesg);                                              \
		return ret;                                                    \
	}

#define IS_DATA_INIT(ret)                                         \
	if (_terrain == nullptr || _terrain->get_data() == nullptr) { \
		return ret;                                               \
	}

#define IS_DATA_INIT_MESG(mesg, ret)                              \
	if (_terrain == nullptr || _terrain->get_data() == nullptr) { \
		TERRAINLOG(ERROR, mesg);                                         \
		return ret;                                               \
	}

#endif // CONSTANTS_CLASS_H