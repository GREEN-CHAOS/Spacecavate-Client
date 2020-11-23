/*************************************************************************/
/*  debugger_editor_plugin.h                                             */
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

#ifndef DEBUGGER_EDITOR_PLUGIN_H
#define DEBUGGER_EDITOR_PLUGIN_H

#include "editor/editor_plugin.h"

class EditorNode;
class EditorFileServer;
class MenuButton;
class PopupMenu;

class DebuggerEditorPlugin : public EditorPlugin {
	GDCLASS(DebuggerEditorPlugin, EditorPlugin);

private:
	MenuButton *debug_menu;
	EditorFileServer *file_server;
	PopupMenu *instances_menu;

	enum MenuOptions {
		RUN_FILE_SERVER,
		RUN_LIVE_DEBUG,
		RUN_DEBUG_COLLISONS,
		RUN_DEBUG_NAVIGATION,
		RUN_DEPLOY_REMOTE_DEBUG,
		RUN_RELOAD_SCRIPTS,
	};

	void _update_debug_options();
	void _notification(int p_what);
	void _select_run_count(int p_index);
	void _menu_option(int p_option);

public:
	virtual String get_name() const override { return "Debugger"; }
	bool has_main_screen() const override { return false; }

	DebuggerEditorPlugin(EditorNode *p_node, MenuButton *p_menu);
	~DebuggerEditorPlugin();
};

#endif // DEBUGGER_EDITOR_PLUGIN_H
