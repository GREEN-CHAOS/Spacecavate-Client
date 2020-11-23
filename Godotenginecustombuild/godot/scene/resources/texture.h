/*************************************************************************/
/*  texture.h                                                            */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2020 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2020 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#ifndef TEXTURE_H
#define TEXTURE_H

#include "core/io/resource.h"
#include "core/io/resource_loader.h"
#include "core/math/rect2.h"
#include "core/os/file_access.h"
#include "core/os/mutex.h"
#include "core/os/rw_lock.h"
#include "core/os/thread_safe.h"
#include "scene/resources/curve.h"
#include "scene/resources/gradient.h"
#include "servers/camera_server.h"
#include "servers/rendering_server.h"

class Texture : public Resource {
	GDCLASS(Texture, Resource);

public:
	Texture() {}
};

class Texture2D : public Texture {
	GDCLASS(Texture2D, Texture);
	OBJ_SAVE_TYPE(Texture2D); // Saves derived classes with common type so they can be interchanged.

protected:
	static void _bind_methods();

public:
	virtual int get_width() const = 0;
	virtual int get_height() const = 0;
	virtual Size2 get_size() const;

	virtual bool is_pixel_opaque(int p_x, int p_y) const;

	virtual bool has_alpha() const = 0;

	virtual void draw(RID p_canvas_item, const Point2 &p_pos, const Color &p_modulate = Color(1, 1, 1), bool p_transpose = false) const;
	virtual void draw_rect(RID p_canvas_item, const Rect2 &p_rect, bool p_tile = false, const Color &p_modulate = Color(1, 1, 1), bool p_transpose = false) const;
	virtual void draw_rect_region(RID p_canvas_item, const Rect2 &p_rect, const Rect2 &p_src_rect, const Color &p_modulate = Color(1, 1, 1), bool p_transpose = false, bool p_clip_uv = true) const;
	virtual bool get_rect_region(const Rect2 &p_rect, const Rect2 &p_src_rect, Rect2 &r_rect, Rect2 &r_src_rect) const;

	virtual Ref<Image> get_data() const { return Ref<Image>(); }

	Texture2D();
};

class BitMap;

class ImageTexture : public Texture2D {
	GDCLASS(ImageTexture, Texture2D);
	RES_BASE_EXTENSION("tex");

	mutable RID texture;
	Image::Format format;
	bool mipmaps;
	int w, h;
	Size2 size_override;
	mutable Ref<BitMap> alpha_cache;
	bool image_stored;

protected:
	virtual void reload_from_file() override;

	bool _set(const StringName &p_name, const Variant &p_value);
	bool _get(const StringName &p_name, Variant &r_ret) const;
	void _get_property_list(List<PropertyInfo> *p_list) const;

	void _reload_hook(const RID &p_hook);
	virtual void _resource_path_changed() override;
	static void _bind_methods();

public:
	void create_from_image(const Ref<Image> &p_image);

	Image::Format get_format() const;

	void update(const Ref<Image> &p_image, bool p_immediate = false);
	Ref<Image> get_data() const override;

	int get_width() const override;
	int get_height() const override;

	virtual RID get_rid() const override;

	bool has_alpha() const override;
	virtual void draw(RID p_canvas_item, const Point2 &p_pos, const Color &p_modulate = Color(1, 1, 1), bool p_transpose = false) const override;
	virtual void draw_rect(RID p_canvas_item, const Rect2 &p_rect, bool p_tile = false, const Color &p_modulate = Color(1, 1, 1), bool p_transpose = false) const override;
	virtual void draw_rect_region(RID p_canvas_item, const Rect2 &p_rect, const Rect2 &p_src_rect, const Color &p_modulate = Color(1, 1, 1), bool p_transpose = false, bool p_clip_uv = true) const override;

	bool is_pixel_opaque(int p_x, int p_y) const override;

	void set_size_override(const Size2 &p_size);

	virtual void set_path(const String &p_path, bool p_take_over = false) override;

	ImageTexture();
	~ImageTexture();
};

class StreamTexture2D : public Texture2D {
	GDCLASS(StreamTexture2D, Texture2D);

public:
	enum DataFormat {
		DATA_FORMAT_IMAGE,
		DATA_FORMAT_LOSSLESS,
		DATA_FORMAT_LOSSY,
		DATA_FORMAT_BASIS_UNIVERSAL,
	};

	enum {
		FORMAT_VERSION = 1
	};

	enum FormatBits {
		FORMAT_MASK_IMAGE_FORMAT = (1 << 20) - 1,
		FORMAT_BIT_LOSSLESS = 1 << 20,
		FORMAT_BIT_LOSSY = 1 << 21,
		FORMAT_BIT_STREAM = 1 << 22,
		FORMAT_BIT_HAS_MIPMAPS = 1 << 23,
		FORMAT_BIT_DETECT_3D = 1 << 24,
		//FORMAT_BIT_DETECT_SRGB = 1 << 25,
		FORMAT_BIT_DETECT_NORMAL = 1 << 26,
		FORMAT_BIT_DETECT_ROUGNESS = 1 << 27,
	};

private:
	Error _load_data(const String &p_path, int &tw, int &th, int &tw_custom, int &th_custom, Ref<Image> &image, bool &r_request_3d, bool &r_request_normal, bool &r_request_roughness, int &mipmap_limit, int p_size_limit = 0);
	String path_to_file;
	mutable RID texture;
	Image::Format format;
	int w, h;
	mutable Ref<BitMap> alpha_cache;

	virtual void reload_from_file() override;

	static void _requested_3d(void *p_ud);
	static void _requested_roughness(void *p_ud, const String &p_normal_path, RS::TextureDetectRoughnessChannel p_roughness_channel);
	static void _requested_normal(void *p_ud);

protected:
	static void _bind_methods();
	void _validate_property(PropertyInfo &property) const override;

public:
	static Ref<Image> load_image_from_file(FileAccess *p_file, int p_size_limit);

	typedef void (*TextureFormatRequestCallback)(const Ref<StreamTexture2D> &);
	typedef void (*TextureFormatRoughnessRequestCallback)(const Ref<StreamTexture2D> &, const String &p_normal_path, RS::TextureDetectRoughnessChannel p_roughness_channel);

	static TextureFormatRequestCallback request_3d_callback;
	static TextureFormatRoughnessRequestCallback request_roughness_callback;
	static TextureFormatRequestCallback request_normal_callback;

	Image::Format get_format() const;
	Error load(const String &p_path);
	String get_load_path() const;

	int get_width() const override;
	int get_height() const override;
	virtual RID get_rid() const override;

	virtual void set_path(const String &p_path, bool p_take_over) override;

	virtual void draw(RID p_canvas_item, const Point2 &p_pos, const Color &p_modulate = Color(1, 1, 1), bool p_transpose = false) const override;
	virtual void draw_rect(RID p_canvas_item, const Rect2 &p_rect, bool p_tile = false, const Color &p_modulate = Color(1, 1, 1), bool p_transpose = false) const override;
	virtual void draw_rect_region(RID p_canvas_item, const Rect2 &p_rect, const Rect2 &p_src_rect, const Color &p_modulate = Color(1, 1, 1), bool p_transpose = false, bool p_clip_uv = true) const override;

	virtual bool has_alpha() const override;
	bool is_pixel_opaque(int p_x, int p_y) const override;

	virtual Ref<Image> get_data() const override;

	StreamTexture2D();
	~StreamTexture2D();
};

class ResourceFormatLoaderStreamTexture2D : public ResourceFormatLoader {
public:
	virtual RES load(const String &p_path, const String &p_original_path = "", Error *r_error = nullptr, bool p_use_sub_threads = false, float *r_progress = nullptr, bool p_no_cache = false);
	virtual void get_recognized_extensions(List<String> *p_extensions) const;
	virtual bool handles_type(const String &p_type) const;
	virtual String get_resource_type(const String &p_path) const;
};

class AtlasTexture : public Texture2D {
	GDCLASS(AtlasTexture, Texture2D);
	RES_BASE_EXTENSION("atlastex");

protected:
	Ref<Texture2D> atlas;
	Rect2 region;
	Rect2 margin;
	bool filter_clip;

	static void _bind_methods();

public:
	virtual int get_width() const override;
	virtual int get_height() const override;
	virtual RID get_rid() const override;

	virtual bool has_alpha() const override;

	void set_atlas(const Ref<Texture2D> &p_atlas);
	Ref<Texture2D> get_atlas() const;

	void set_region(const Rect2 &p_region);
	Rect2 get_region() const;

	void set_margin(const Rect2 &p_margin);
	Rect2 get_margin() const;

	void set_filter_clip(const bool p_enable);
	bool has_filter_clip() const;

	virtual void draw(RID p_canvas_item, const Point2 &p_pos, const Color &p_modulate = Color(1, 1, 1), bool p_transpose = false) const override;
	virtual void draw_rect(RID p_canvas_item, const Rect2 &p_rect, bool p_tile = false, const Color &p_modulate = Color(1, 1, 1), bool p_transpose = false) const override;
	virtual void draw_rect_region(RID p_canvas_item, const Rect2 &p_rect, const Rect2 &p_src_rect, const Color &p_modulate = Color(1, 1, 1), bool p_transpose = false, bool p_clip_uv = true) const override;
	virtual bool get_rect_region(const Rect2 &p_rect, const Rect2 &p_src_rect, Rect2 &r_rect, Rect2 &r_src_rect) const override;

	bool is_pixel_opaque(int p_x, int p_y) const override;

	AtlasTexture();
};

class Mesh;

class MeshTexture : public Texture2D {
	GDCLASS(MeshTexture, Texture2D);
	RES_BASE_EXTENSION("meshtex");

	Ref<Texture2D> base_texture;
	Ref<Mesh> mesh;
	Size2i size;

protected:
	static void _bind_methods();

public:
	virtual int get_width() const override;
	virtual int get_height() const override;
	virtual RID get_rid() const override;

	virtual bool has_alpha() const override;

	void set_mesh(const Ref<Mesh> &p_mesh);
	Ref<Mesh> get_mesh() const;

	void set_image_size(const Size2 &p_size);
	Size2 get_image_size() const;

	void set_base_texture(const Ref<Texture2D> &p_texture);
	Ref<Texture2D> get_base_texture() const;

	virtual void draw(RID p_canvas_item, const Point2 &p_pos, const Color &p_modulate = Color(1, 1, 1), bool p_transpose = false) const override;
	virtual void draw_rect(RID p_canvas_item, const Rect2 &p_rect, bool p_tile = false, const Color &p_modulate = Color(1, 1, 1), bool p_transpose = false) const override;
	virtual void draw_rect_region(RID p_canvas_item, const Rect2 &p_rect, const Rect2 &p_src_rect, const Color &p_modulate = Color(1, 1, 1), bool p_transpose = false, bool p_clip_uv = true) const override;
	virtual bool get_rect_region(const Rect2 &p_rect, const Rect2 &p_src_rect, Rect2 &r_rect, Rect2 &r_src_rect) const override;

	bool is_pixel_opaque(int p_x, int p_y) const override;

	MeshTexture();
};

class LargeTexture : public Texture2D {
	GDCLASS(LargeTexture, Texture2D);
	RES_BASE_EXTENSION("largetex");

protected:
	struct Piece {
		Point2 offset;
		Ref<Texture2D> texture;
	};

	Vector<Piece> pieces;
	Size2i size;

	Array _get_data() const;
	void _set_data(const Array &p_array);
	static void _bind_methods();

public:
	virtual int get_width() const override;
	virtual int get_height() const override;
	virtual RID get_rid() const override;

	virtual bool has_alpha() const override;

	int add_piece(const Point2 &p_offset, const Ref<Texture2D> &p_texture);
	void set_piece_offset(int p_idx, const Point2 &p_offset);
	void set_piece_texture(int p_idx, const Ref<Texture2D> &p_texture);

	void set_size(const Size2 &p_size);
	void clear();

	int get_piece_count() const;
	Vector2 get_piece_offset(int p_idx) const;
	Ref<Texture2D> get_piece_texture(int p_idx) const;
	Ref<Image> to_image() const;

	virtual void draw(RID p_canvas_item, const Point2 &p_pos, const Color &p_modulate = Color(1, 1, 1), bool p_transpose = false) const override;
	virtual void draw_rect(RID p_canvas_item, const Rect2 &p_rect, bool p_tile = false, const Color &p_modulate = Color(1, 1, 1), bool p_transpose = false) const override;
	virtual void draw_rect_region(RID p_canvas_item, const Rect2 &p_rect, const Rect2 &p_src_rect, const Color &p_modulate = Color(1, 1, 1), bool p_transpose = false, bool p_clip_uv = true) const override;

	bool is_pixel_opaque(int p_x, int p_y) const override;

	LargeTexture();
};

class TextureLayered : public Texture {
	GDCLASS(TextureLayered, Texture);

protected:
	static void _bind_methods();

public:
	enum LayeredType {
		LAYERED_TYPE_2D_ARRAY,
		LAYERED_TYPE_CUBEMAP,
		LAYERED_TYPE_CUBEMAP_ARRAY
	};

	virtual Image::Format get_format() const = 0;
	virtual LayeredType get_layered_type() const = 0;
	virtual int get_width() const = 0;
	virtual int get_height() const = 0;
	virtual int get_layers() const = 0;
	virtual bool has_mipmaps() const = 0;
	virtual Ref<Image> get_layer_data(int p_layer) const = 0;
};

VARIANT_ENUM_CAST(TextureLayered::LayeredType)

class ImageTextureLayered : public TextureLayered {
	GDCLASS(ImageTextureLayered, TextureLayered);

	LayeredType layered_type;

	mutable RID texture;
	Image::Format format;

	int width;
	int height;
	int layers;
	bool mipmaps;

	Error _create_from_images(const Array &p_images);

	Array _get_images() const;

protected:
	static void _bind_methods();

public:
	virtual Image::Format get_format() const override;
	virtual int get_width() const override;
	virtual int get_height() const override;
	virtual int get_layers() const override;
	virtual bool has_mipmaps() const override;
	virtual LayeredType get_layered_type() const override;

	Error create_from_images(Vector<Ref<Image>> p_images);
	void update_layer(const Ref<Image> &p_image, int p_layer);
	virtual Ref<Image> get_layer_data(int p_layer) const override;

	virtual RID get_rid() const override;
	virtual void set_path(const String &p_path, bool p_take_over = false) override;

	ImageTextureLayered(LayeredType p_layered_type);
	~ImageTextureLayered();
};

class Texture2DArray : public ImageTextureLayered {
	GDCLASS(Texture2DArray, ImageTextureLayered)
public:
	Texture2DArray() :
			ImageTextureLayered(LAYERED_TYPE_2D_ARRAY) {}
};

class Cubemap : public ImageTextureLayered {
	GDCLASS(Cubemap, ImageTextureLayered);

public:
	Cubemap() :
			ImageTextureLayered(LAYERED_TYPE_CUBEMAP) {}
};

class CubemapArray : public ImageTextureLayered {
	GDCLASS(CubemapArray, ImageTextureLayered);

public:
	CubemapArray() :
			ImageTextureLayered(LAYERED_TYPE_CUBEMAP_ARRAY) {}
};

class StreamTextureLayered : public TextureLayered {
	GDCLASS(StreamTextureLayered, TextureLayered);

public:
	enum DataFormat {
		DATA_FORMAT_IMAGE,
		DATA_FORMAT_LOSSLESS,
		DATA_FORMAT_LOSSY,
		DATA_FORMAT_BASIS_UNIVERSAL,
	};

	enum {
		FORMAT_VERSION = 1
	};

	enum FormatBits {
		FORMAT_MASK_IMAGE_FORMAT = (1 << 20) - 1,
		FORMAT_BIT_LOSSLESS = 1 << 20,
		FORMAT_BIT_LOSSY = 1 << 21,
		FORMAT_BIT_STREAM = 1 << 22,
		FORMAT_BIT_HAS_MIPMAPS = 1 << 23,
	};

private:
	Error _load_data(const String &p_path, Vector<Ref<Image>> &images, int &mipmap_limit, int p_size_limit = 0);
	String path_to_file;
	mutable RID texture;
	Image::Format format;
	int w, h, layers;
	bool mipmaps;
	LayeredType layered_type;

	virtual void reload_from_file() override;

protected:
	static void _bind_methods();
	void _validate_property(PropertyInfo &property) const override;

public:
	Image::Format get_format() const override;
	Error load(const String &p_path);
	String get_load_path() const;
	virtual LayeredType get_layered_type() const override;

	int get_width() const override;
	int get_height() const override;
	int get_layers() const override;
	virtual bool has_mipmaps() const override;
	virtual RID get_rid() const override;

	virtual void set_path(const String &p_path, bool p_take_over) override;

	virtual Ref<Image> get_layer_data(int p_layer) const override;

	StreamTextureLayered(LayeredType p_layered_type);
	~StreamTextureLayered();
};

class StreamTexture2DArray : public StreamTextureLayered {
	GDCLASS(StreamTexture2DArray, StreamTextureLayered)
public:
	StreamTexture2DArray() :
			StreamTextureLayered(LAYERED_TYPE_2D_ARRAY) {}
};

class StreamCubemap : public StreamTextureLayered {
	GDCLASS(StreamCubemap, StreamTextureLayered);

public:
	StreamCubemap() :
			StreamTextureLayered(LAYERED_TYPE_CUBEMAP) {}
};

class StreamCubemapArray : public StreamTextureLayered {
	GDCLASS(StreamCubemapArray, StreamTextureLayered);

public:
	StreamCubemapArray() :
			StreamTextureLayered(LAYERED_TYPE_CUBEMAP_ARRAY) {}
};

class ResourceFormatLoaderStreamTextureLayered : public ResourceFormatLoader {
public:
	virtual RES load(const String &p_path, const String &p_original_path = "", Error *r_error = nullptr, bool p_use_sub_threads = false, float *r_progress = nullptr, bool p_no_cache = false);
	virtual void get_recognized_extensions(List<String> *p_extensions) const;
	virtual bool handles_type(const String &p_type) const;
	virtual String get_resource_type(const String &p_path) const;
};

class Texture3D : public Texture {
	GDCLASS(Texture3D, Texture);

protected:
	static void _bind_methods();

	TypedArray<Image> _get_data() const;

public:
	virtual Image::Format get_format() const = 0;
	virtual int get_width() const = 0;
	virtual int get_height() const = 0;
	virtual int get_depth() const = 0;
	virtual bool has_mipmaps() const = 0;
	virtual Vector<Ref<Image>> get_data() const = 0;
};

class ImageTexture3D : public Texture3D {
	GDCLASS(ImageTexture3D, Texture3D);

	mutable RID texture;

	Image::Format format = Image::FORMAT_MAX;
	int width = 1;
	int height = 1;
	int depth = 1;
	bool mipmaps = false;

protected:
	static void _bind_methods();

	Error _create(Image::Format p_format, int p_width, int p_height, int p_depth, bool p_mipmaps, const TypedArray<Image> &p_data);
	void _update(const TypedArray<Image> &p_data);

public:
	virtual Image::Format get_format() const override;
	virtual int get_width() const override;
	virtual int get_height() const override;
	virtual int get_depth() const override;
	virtual bool has_mipmaps() const override;

	Error create(Image::Format p_format, int p_width, int p_height, int p_depth, bool p_mipmaps, const Vector<Ref<Image>> &p_data);
	void update(const Vector<Ref<Image>> &p_data);
	virtual Vector<Ref<Image>> get_data() const override;

	virtual RID get_rid() const override;
	virtual void set_path(const String &p_path, bool p_take_over = false) override;

	ImageTexture3D();
	~ImageTexture3D();
};

class StreamTexture3D : public Texture3D {
	GDCLASS(StreamTexture3D, Texture3D);

public:
	enum DataFormat {
		DATA_FORMAT_IMAGE,
		DATA_FORMAT_LOSSLESS,
		DATA_FORMAT_LOSSY,
		DATA_FORMAT_BASIS_UNIVERSAL,
	};

	enum {
		FORMAT_VERSION = 1
	};

	enum FormatBits {
		FORMAT_MASK_IMAGE_FORMAT = (1 << 20) - 1,
		FORMAT_BIT_LOSSLESS = 1 << 20,
		FORMAT_BIT_LOSSY = 1 << 21,
		FORMAT_BIT_STREAM = 1 << 22,
		FORMAT_BIT_HAS_MIPMAPS = 1 << 23,
	};

private:
	Error _load_data(const String &p_path, Vector<Ref<Image>> &r_data, Image::Format &r_format, int &r_width, int &r_height, int &r_depth, bool &r_mipmaps);
	String path_to_file;
	mutable RID texture;
	Image::Format format;
	int w, h, d;
	bool mipmaps;

	virtual void reload_from_file() override;

protected:
	static void _bind_methods();
	void _validate_property(PropertyInfo &property) const override;

public:
	Image::Format get_format() const override;
	Error load(const String &p_path);
	String get_load_path() const;

	int get_width() const override;
	int get_height() const override;
	int get_depth() const override;
	virtual bool has_mipmaps() const override;
	virtual RID get_rid() const override;

	virtual void set_path(const String &p_path, bool p_take_over) override;

	virtual Vector<Ref<Image>> get_data() const override;

	StreamTexture3D();
	~StreamTexture3D();
};

class ResourceFormatLoaderStreamTexture3D : public ResourceFormatLoader {
public:
	virtual RES load(const String &p_path, const String &p_original_path = "", Error *r_error = nullptr, bool p_use_sub_threads = false, float *r_progress = nullptr, bool p_no_cache = false);
	virtual void get_recognized_extensions(List<String> *p_extensions) const;
	virtual bool handles_type(const String &p_type) const;
	virtual String get_resource_type(const String &p_path) const;
};

class CurveTexture : public Texture2D {
	GDCLASS(CurveTexture, Texture2D);
	RES_BASE_EXTENSION("curvetex")

private:
	mutable RID _texture;
	Ref<Curve> _curve;
	int _width;

	void _update();

protected:
	static void _bind_methods();

public:
	void set_width(int p_width);
	int get_width() const override;

	void ensure_default_setup(float p_min = 0, float p_max = 1);

	void set_curve(Ref<Curve> p_curve);
	Ref<Curve> get_curve() const;

	virtual RID get_rid() const override;

	virtual int get_height() const override { return 1; }
	virtual bool has_alpha() const override { return false; }

	CurveTexture();
	~CurveTexture();
};
/*
	enum CubeMapSide {
		CUBEMAP_LEFT,
		CUBEMAP_RIGHT,
		CUBEMAP_BOTTOM,
		CUBEMAP_TOP,
		CUBEMAP_FRONT,
		CUBEMAP_BACK,
	};

*/
//VARIANT_ENUM_CAST( Texture::CubeMapSide );

class GradientTexture : public Texture2D {
	GDCLASS(GradientTexture, Texture2D);

public:
	struct Point {
		float offset;
		Color color;
		bool operator<(const Point &p_ponit) const {
			return offset < p_ponit.offset;
		}
	};

private:
	Ref<Gradient> gradient;
	bool update_pending;
	RID texture;
	int width;

	void _queue_update();
	void _update();

protected:
	static void _bind_methods();

public:
	void set_gradient(Ref<Gradient> p_gradient);
	Ref<Gradient> get_gradient() const;

	void set_width(int p_width);
	int get_width() const override;

	virtual RID get_rid() const override { return texture; }
	virtual int get_height() const override { return 1; }
	virtual bool has_alpha() const override { return true; }

	virtual Ref<Image> get_data() const override;

	GradientTexture();
	virtual ~GradientTexture();
};

class ProxyTexture : public Texture2D {
	GDCLASS(ProxyTexture, Texture2D);

private:
	mutable RID proxy_ph;
	mutable RID proxy;
	Ref<Texture2D> base;

protected:
	static void _bind_methods();

public:
	void set_base(const Ref<Texture2D> &p_texture);
	Ref<Texture2D> get_base() const;

	virtual int get_width() const override;
	virtual int get_height() const override;
	virtual RID get_rid() const override;

	virtual bool has_alpha() const override;

	ProxyTexture();
	~ProxyTexture();
};

class AnimatedTexture : public Texture2D {
	GDCLASS(AnimatedTexture, Texture2D);

	//use readers writers lock for this, since its far more times read than written to
	RWLock *rw_lock;

public:
	enum {
		MAX_FRAMES = 256
	};

private:
	RID proxy_ph;
	RID proxy;

	struct Frame {
		Ref<Texture2D> texture;
		float delay_sec;

		Frame() {
			delay_sec = 0;
		}
	};

	Frame frames[MAX_FRAMES];
	int frame_count;
	int current_frame;
	bool pause;
	bool oneshot;
	float fps;

	float time;

	uint64_t prev_ticks;

	void _update_proxy();

protected:
	static void _bind_methods();
	void _validate_property(PropertyInfo &property) const override;

public:
	void set_frames(int p_frames);
	int get_frames() const;

	void set_current_frame(int p_frame);
	int get_current_frame() const;

	void set_pause(bool p_pause);
	bool get_pause() const;

	void set_oneshot(bool p_oneshot);
	bool get_oneshot() const;

	void set_frame_texture(int p_frame, const Ref<Texture2D> &p_texture);
	Ref<Texture2D> get_frame_texture(int p_frame) const;

	void set_frame_delay(int p_frame, float p_delay_sec);
	float get_frame_delay(int p_frame) const;

	void set_fps(float p_fps);
	float get_fps() const;

	virtual int get_width() const override;
	virtual int get_height() const override;
	virtual RID get_rid() const override;

	virtual bool has_alpha() const override;

	virtual Ref<Image> get_data() const override;

	bool is_pixel_opaque(int p_x, int p_y) const override;

	AnimatedTexture();
	~AnimatedTexture();
};

class CameraTexture : public Texture2D {
	GDCLASS(CameraTexture, Texture2D);

private:
	int camera_feed_id;
	CameraServer::FeedImage which_feed;

protected:
	static void _bind_methods();

public:
	virtual int get_width() const override;
	virtual int get_height() const override;
	virtual RID get_rid() const override;
	virtual bool has_alpha() const override;

	virtual void set_flags(uint32_t p_flags);
	virtual uint32_t get_flags() const;

	virtual Ref<Image> get_data() const override;

	void set_camera_feed_id(int p_new_id);
	int get_camera_feed_id() const;

	void set_which_feed(CameraServer::FeedImage p_which);
	CameraServer::FeedImage get_which_feed() const;

	void set_camera_active(bool p_active);
	bool get_camera_active() const;

	CameraTexture();
	~CameraTexture();
};

#endif
