// Copyright © 2025 Cory Petkovsek, Roope Palmroos, and Contributors.

#ifndef GENERATEDTEXTURE_CLASS_H
#define GENERATEDTEXTURE_CLASS_H

#include "core/io/image.h"
#include "core/object/ref_counted.h"
#include "servers/rendering_server.h"

#include "constants.h"

class GeneratedTexture : public RefCounted {
	GDCLASS(GeneratedTexture, RefCounted);

private:
	RID _rid = RID();
	Ref<Image> _image;
	bool _dirty = false;

protected:
	static void _bind_methods();

public:
	void clear();
	bool is_dirty() const { return _dirty; }
	RID create(const TypedArray<Image> &p_layers);
	void update(const Ref<Image> &p_image, int p_layer);
	RID create(const Ref<Image> &p_image);
	Ref<Image> get_image() const { return _image; }
	RID get_rid() const { return _rid; }

	GeneratedTexture();
	~GeneratedTexture();
};

#endif // GENERATEDTEXTURE_CLASS_H