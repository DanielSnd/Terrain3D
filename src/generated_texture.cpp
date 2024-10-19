// Copyright © 2025 Cory Petkovsek, Roope Palmroos, and Contributors.

#include "core/object/ref_counted.h"
#include "servers/rendering_server.h"

#include "generated_texture.h"
#include "logger.h"

///////////////////////////
// Public Functions
///////////////////////////

void GeneratedTexture::_bind_methods()
{
}

void GeneratedTexture::clear()
{
        if (_rid.is_valid()) {
		print_line(EXTREME, "GeneratedTexture freeing ", _rid);
		RenderingServer::get_singleton()->free(_rid);
	}
	if (_image.is_valid()) {
		print_line(EXTREME, "GeneratedTexture unref image", _image);
		_image.unref();
	}
	_rid = RID();
	_dirty = true;
}

RID GeneratedTexture::create(const TypedArray<Image> &p_layers) {
	Vector<Ref<Image>> _layers;
	for (int i = 0; i < p_layers.size(); i++) {
		Ref<Image> img = p_layers[i];
		_layers.push_back(img);
	}
	if (!p_layers.is_empty()) {
		if (Terrain3D::debug_level >= DEBUG) {
			print_line(EXTREME, "RenderingServer creating Texture2DArray, layers size: ", p_layers.size());
			for (int i = 0; i < p_layers.size(); i++) {
				Ref<Image> img = p_layers[i];
				print_line(EXTREME, i, ": ", img, ", empty: ", img->is_empty(), ", size: ", img->get_size(), ", format: ", img->get_format());
			}
		}
		_rid = RenderingServer::get_singleton()->texture_2d_layered_create(_layers, RenderingServer::TEXTURE_LAYERED_2D_ARRAY);
		_dirty = false;
	} else {
		clear();
	}
	return _rid;
}

void GeneratedTexture::update(const Ref<Image> &p_image, const int p_layer) {
	print_line(EXTREME, "RenderingServer updating Texture2DArray at index: ", p_layer);
	RenderingServer::get_singleton()->texture_2d_update(_rid, p_image, p_layer);
}

RID GeneratedTexture::create(const Ref<Image> &p_image) {
	print_line(EXTREME, "RenderingServer creating Texture2D");
	_image = p_image;
	_rid = RenderingServer::get_singleton()->texture_2d_create(_image);
	_dirty = false;
	return _rid;
}

GeneratedTexture::GeneratedTexture()
{
}

GeneratedTexture::~GeneratedTexture()
{
}
