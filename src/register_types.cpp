// Copyright © 2025 Cory Petkovsek, Roope Palmroos, and Contributors.

#include "register_types.h"
#include "terrain_3d.h"
#include "terrain_3d_editor.h"

using namespace godot;

void initialize_terrain3d_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
	ClassDB::register_class<Terrain3D>();
	ClassDB::register_class<Terrain3DAssets>();
	ClassDB::register_class<Terrain3DData>();
	ClassDB::register_class<Terrain3DEditor>();
	ClassDB::register_class<Terrain3DInstancer>();
	ClassDB::register_class<Terrain3DMaterial>();
	ClassDB::register_class<Terrain3DMeshAsset>();
	ClassDB::register_class<Terrain3DRegion>();
	ClassDB::register_class<Terrain3DTextureAsset>();
	ClassDB::register_class<Terrain3DUtil>();
}

void uninitialize_terrain3d_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}
