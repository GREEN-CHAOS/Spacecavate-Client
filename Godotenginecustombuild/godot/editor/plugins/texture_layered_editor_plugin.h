/*************************************************************************/
/*  texture_layered_editor_plugin.h                                      */
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

#ifndef TEXTURE_LAYERED_EDITOR_PLUGIN_H
#define TEXTURE_LAYERED_EDITOR_PLUGIN_H

#include "editor/editor_node.h"
#include "editor/editor_plugin.h"
#include "scene/resources/shader.h"
#include "scene/resources/texture.h"

class TextureLayeredEditor : public Control {
	GDCLASS(TextureLayeredEditor, Control);

	SpinBox *layer;
	Label *info;
	Ref<TextureLayered> texture;

	Ref<Shader> shaders[3];
	Ref<ShaderMaterial> materials[3];

	float x_rot = 0;
	float y_rot = 0;
	Control *texture_rect;

	void _make_shaders();

	void _update_material();
	bool setting;
	void _layer_changed(double) {
		if (!setting) {
			_update_material();
		}
	}

	void _texture_rect_update_area();
	void _texture_rect_draw();

protected:
	void _notification(int p_what);
	void _gui_input(Ref<InputEvent> p_event);
	void _changed_callback(Object *p_changed, const char *p_prop) override;
	static void _bind_methods();

public:
	void edit(Ref<TextureLayered> p_texture);
	TextureLayeredEditor();
	~TextureLayeredEditor();
};

class EditorInspectorPluginLayeredTexture : public EditorInspectorPlugin {
	GDCLASS(EditorInspectorPluginLayeredTexture, EditorInspectorPlugin);

public:
	virtual bool can_handle(Object *p_object) override;
	virtual void parse_begin(Object *p_object) override;
};

class TextureLayeredEditorPlugin : public EditorPlugin {
	GDCLASS(TextureLayeredEditorPlugin, EditorPlugin);

public:
	virtual String get_name() const override { return "TextureLayered"; }

	TextureLayeredEditorPlugin(EditorNode *p_node);
};

#endif // TEXTURE_EDITOR_PLUGIN_H
