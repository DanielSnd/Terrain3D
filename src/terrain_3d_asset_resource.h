// Copyright © 2025 Cory Petkovsek, Roope Palmroos, and Contributors.

#ifndef TERRAIN3D_ASSET_RESOURCE_CLASS_H
#define TERRAIN3D_ASSET_RESOURCE_CLASS_H

#include "constants.h"
#include "core/io/resource.h"
#include "scene/resources/texture.h"
class Terrain3DAssets;

// Parent class of Terrain3DMeshAsset and Terrain3DTextureAsset
class Terrain3DAssetResource : public Resource {
	friend class Terrain3DAssets;

public:
	Terrain3DAssetResource() {}
	~Terrain3DAssetResource() {}

	virtual void clear() = 0;
	virtual void set_id(const int p_id) = 0;
	virtual int get_id() const = 0;

protected:
	String _name;
	int _id = 0;
};

#endif // TERRAIN3D_ASSET_RESOURCE_CLASS_H