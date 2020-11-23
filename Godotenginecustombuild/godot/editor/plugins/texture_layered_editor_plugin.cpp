/*************************************************************************/
/*  texture_layered_editor_plugin.cpp                                    */
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

#include "texture_layered_editor_plugin.h"

#include "core/config/project_settings.h"
#include "core/io/resource_loader.h"
#include "editor/editor_settings.h"

void TextureLayeredEditor::_gui_input(Ref<InputEvent> p_event) {
	Ref<InputEventMouseMotion> mm = p_event;
	if (mm.is_valid() && mm->get_button_mask() & BUTTON_MASK_LEFT) {
		y_rot += -mm->get_relative().x * 0.01;
		x_rot += mm->get_relative().y * 0.01;
		_update_material();
	}
}

void TextureLayeredEditor::_texture_rect_draw() {
	texture_rect->draw_rect(Rect2(Point2(), texture_rect->get_size()), Color(1, 1, 1, 1));
}

void TextureLayeredEditor::_notification(int p_what) {
	if (p_what == NOTIFICATION_READY) {
		//get_scene()->connect("node_removed",this,"_node_removed");
	}
	if (p_what == NOTIFICATION_RESIZED) {
		_texture_rect_update_area();
	}

	if (p_what == NOTIFICATION_DRAW) {
		Ref<Texture2D> checkerboard = get_theme_icon("Checkerboard", "EditorIcons");
		Size2 size = get_size();

		draw_texture_rect(checkerboard, Rect2(Point2(), size), true);
	}
}

void TextureLayeredEditor::_changed_callback(Object *p_changed, const char *p_prop) {
	if (!is_visible()) {
		return;
	}
	update();
}

void TextureLayeredEditor::_update_material() {
	materials[0]->set_shader_param("layer", layer->get_value());
	materials[2]->set_shader_param("layer", layer->get_value());
	materials[texture->get_layered_type()]->set_shader_param("tex", texture->get_rid());

	Vector3 v(1, 1, 1);
	v.normalize();

	Basis b;
	b.rotate(Vector3(1, 0, 0), x_rot);
	b.rotate(Vector3(0, 1, 0), y_rot);

	materials[1]->set_shader_param("normal", v);
	materials[1]->set_shader_param("rot", b);
	materials[2]->set_shader_param("normal", v);
	materials[2]->set_shader_param("rot", b);

	String format = Image::get_format_name(texture->get_format());

	String text;
	if (texture->get_layered_type() == TextureLayered::LAYERED_TYPE_2D_ARRAY) {
		text = itos(texture->get_width()) + "x" + itos(texture->get_height()) + " (x " + itos(texture->get_layers()) + ")" + format;
	} else if (texture->get_layered_type() == TextureLayered::LAYERED_TYPE_CUBEMAP) {
		text = itos(texture->get_width()) + "x" + itos(texture->get_height()) + " " + format;
	} else if (texture->get_layered_type() == TextureLayered::LAYERED_TYPE_CUBEMAP_ARRAY) {
		text = itos(texture->get_width()) + "x" + itos(texture->get_height()) + " (x " + itos(texture->get_layers() / 6) + ")" + format;
	}

	info->set_text(text);
}

void TextureLayeredEditor::_make_shaders() {
	String shader_2d_array = ""
							 "shader_type canvas_item;\n"
							 "uniform sampler2DArray tex;\n"
							 "uniform float layer;\n"
							 "void fragment() {\n"
							 "  COLOR = textureLod(tex,vec3(UV,layer),0.0);\n"
							 "}";

	shaders[0].instance();
	shaders[0]->set_code(shader_2d_array);

	String shader_cube = ""
						 "shader_type canvas_item;\n"
						 "uniform samplerCube tex;\n"
						 "uniform vec3 normal;\n"
						 "uniform mat3 rot;\n"
						 "void fragment() {\n"
						 "  vec3 n = rot * normalize(vec3(normal.xy*(UV * 2.0 - 1.0),normal.z));\n"
						 "  COLOR = textureLod(tex,n,0.0);\n"
						 "}";

	shaders[1].instance();
	shaders[1]->set_code(shader_cube);

	String shader_cube_array = ""
							   "shader_type canvas_item;\n"
							   "uniform samplerCubeArray tex;\n"
							   "uniform vec3 normal;\n"
							   "uniform mat3 rot;\n"
							   "uniform float layer;\n"
							   "void fragment() {\n"
							   "  vec3 n = rot * normalize(vec3(normal.xy*(UV * 2.0 - 1.0),normal.z));\n"
							   "  COLOR = textureLod(tex,vec4(n,layer),0.0);\n"
							   "}";

	shaders[2].instance();
	shaders[2]->set_code(shader_cube_array);

	for (int i = 0; i < 3; i++) {
		materials[i].instance();
		materials[i]->set_shader(shaders[i]);
	}
}

void TextureLayeredEditor::_texture_rect_update_area() {
	Size2 size = get_size();
	int tex_width = texture->get_width() * size.height / texture->get_height();
	int tex_height = size.height;

	if (tex_width > size.width) {
		tex_width = size.width;
		tex_height = texture->get_height() * tex_width / texture->get_width();
	}

	// Prevent the texture from being unpreviewable after the rescale, so that we can still see something
	if (tex_height <= 0) {
		tex_height = 1;
	}
	if (tex_width <= 0) {
		tex_width = 1;
	}

	int ofs_x = (size.width - tex_width) / 2;
	int ofs_y = (size.height - tex_height) / 2;

	texture_rect->set_position(Vector2(ofs_x, ofs_y));
	texture_rect->set_size(Vector2(tex_width, tex_height));
}

void TextureLayeredEditor::edit(Ref<TextureLayered> p_texture) {
	if (!texture.is_null()) {
		texture->remove_change_receptor(this);
	}

	texture = p_texture;

	if (!texture.is_null()) {
		if (shaders[0].is_null()) {
			_make_shaders();
		}

		texture->add_change_receptor(this);
		update();
		texture_rect->set_material(materials[texture->get_layered_type()]);
		setting = true;
		if (texture->get_layered_type() == TextureLayered::LAYERED_TYPE_2D_ARRAY) {
			layer->set_max(texture->get_layers() - 1);
			layer->set_value(0);
			layer->show();
		} else if (texture->get_layered_type() == TextureLayered::LAYERED_TYPE_CUBEMAP_ARRAY) {
			layer->set_max(texture->get_layers() / 6 - 1);
			layer->set_value(0);
			layer->show();
		} else {
			layer->hide();
		}
		x_rot = 0;
		y_rot = 0;
		_update_material();
		setting = false;
		_texture_rect_update_area();
	} else {
		hide();
	}
}

void TextureLayeredEditor::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_gui_input"), &TextureLayeredEditor::_gui_input);
	ClassDB::bind_method(D_METHOD("_layer_changed"), &TextureLayeredEditor::_layer_changed);
}

TextureLayeredEditor::TextureLayeredEditor() {
	set_texture_repeat(TextureRepeat::TEXTURE_REPEAT_ENABLED);
	set_custom_minimum_size(Size2(1, 150));
	texture_rect = memnew(Control);
	texture_rect->connect("draw", callable_mp(this, &TextureLayeredEditor::_texture_rect_draw));
	texture_rect->set_mouse_filter(MOUSE_FILTER_IGNORE);
	add_child(texture_rect);

	layer = memnew(SpinBox);
	layer->set_step(1);
	layer->set_max(100);
	add_child(layer);
	layer->set_anchor(MARGIN_RIGHT, 1);
	layer->set_anchor(MARGIN_LEFT, 1);
	layer->set_h_grow_direction(GROW_DIRECTION_BEGIN);
	layer->set_modulate(Color(1, 1, 1, 0.8));
	info = memnew(Label);
	add_child(info);
	info->set_anchor(MARGIN_RIGHT, 1);
	info->set_anchor(MARGIN_LEFT, 1);
	info->set_anchor(MARGIN_BOTTOM, 1);
	info->set_anchor(MARGIN_TOP, 1);
	info->set_h_grow_direction(GROW_DIRECTION_BEGIN);
	info->set_v_grow_direction(GROW_DIRECTION_BEGIN);
	info->add_theme_color_override("font_color", Color(1, 1, 1, 1));
	info->add_theme_color_override("font_color_shadow", Color(0, 0, 0, 0.5));
	info->add_theme_color_override("font_color_shadow", Color(0, 0, 0, 0.5));
	info->add_theme_constant_override("shadow_as_outline", 1);
	info->add_theme_constant_override("shadow_offset_x", 2);
	info->add_theme_constant_override("shadow_offset_y", 2);

	setting = false;
	layer->connect("value_changed", Callable(this, "_layer_changed"));
}

TextureLayeredEditor::~TextureLayeredEditor() {
	if (!texture.is_null()) {
		texture->remove_change_receptor(this);
	}
}

//
bool EditorInspectorPluginLayeredTexture::can_handle(Object *p_object) {
	return Object::cast_to<TextureLayered>(p_object) != nullptr;
}

void EditorInspectorPluginLayeredTexture::parse_begin(Object *p_object) {
	TextureLayered *texture = Object::cast_to<TextureLayered>(p_object);
	if (!texture) {
		return;
	}
	Ref<TextureLayered> m(texture);

	TextureLayeredEditor *editor = memnew(TextureLayeredEditor);
	editor->edit(m);
	add_custom_control(editor);
}

TextureLayeredEditorPlugin::TextureLayeredEditorPlugin(EditorNode *p_node) {
	Ref<EditorInspectorPluginLayeredTexture> plugin;
	plugin.instance();
	add_inspector_plugin(plugin);
}
