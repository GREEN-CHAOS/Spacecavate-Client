/*************************************************************************/
/*  register_types.cpp                                                   */
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

#include "register_types.h"

#include "core/io/file_access_encrypted.h"
#include "core/io/resource_loader.h"
#include "core/os/dir_access.h"
#include "core/os/file_access.h"
#include "gdscript.h"
#include "gdscript_analyzer.h"
#include "gdscript_cache.h"
#include "gdscript_tokenizer.h"

#ifdef TESTS_ENABLED
#include "tests/test_gdscript.h"
#include "tests/test_macros.h"
#endif

GDScriptLanguage *script_language_gd = nullptr;
Ref<ResourceFormatLoaderGDScript> resource_loader_gd;
Ref<ResourceFormatSaverGDScript> resource_saver_gd;
GDScriptCache *gdscript_cache = nullptr;

#ifdef TOOLS_ENABLED

#include "editor/editor_export.h"
#include "editor/editor_node.h"
#include "editor/editor_settings.h"
#include "editor/editor_translation_parser.h"
#include "editor/gdscript_highlighter.h"
#include "editor/gdscript_translation_parser_plugin.h"

#ifndef GDSCRIPT_NO_LSP
#include "core/config/engine.h"
#include "language_server/gdscript_language_server.h"
#endif // !GDSCRIPT_NO_LSP

Ref<GDScriptEditorTranslationParserPlugin> gdscript_translation_parser_plugin;

class EditorExportGDScript : public EditorExportPlugin {
	GDCLASS(EditorExportGDScript, EditorExportPlugin);

public:
	virtual void _export_file(const String &p_path, const String &p_type, const Set<String> &p_features) override {
		int script_mode = EditorExportPreset::MODE_SCRIPT_COMPILED;
		String script_key;

		const Ref<EditorExportPreset> &preset = get_export_preset();

		if (preset.is_valid()) {
			script_mode = preset->get_script_export_mode();
			script_key = preset->get_script_encryption_key().to_lower();
		}

		if (!p_path.ends_with(".gd") || script_mode == EditorExportPreset::MODE_SCRIPT_TEXT) {
			return;
		}

		// TODO: Readd compiled GDScript on export.
		return;
	}
};

static void _editor_init() {
	Ref<EditorExportGDScript> gd_export;
	gd_export.instance();
	EditorExport::get_singleton()->add_export_plugin(gd_export);

#ifdef TOOLS_ENABLED
	Ref<GDScriptSyntaxHighlighter> gdscript_syntax_highlighter;
	gdscript_syntax_highlighter.instance();
	ScriptEditor::get_singleton()->register_syntax_highlighter(gdscript_syntax_highlighter);
#endif

#ifndef GDSCRIPT_NO_LSP
	register_lsp_types();
	GDScriptLanguageServer *lsp_plugin = memnew(GDScriptLanguageServer);
	EditorNode::get_singleton()->add_editor_plugin(lsp_plugin);
	Engine::get_singleton()->add_singleton(Engine::Singleton("GDScriptLanguageProtocol", GDScriptLanguageProtocol::get_singleton()));
#endif // !GDSCRIPT_NO_LSP
}

#endif // TOOLS_ENABLED

void register_gdscript_types() {
	ClassDB::register_class<GDScript>();
	ClassDB::register_virtual_class<GDScriptFunctionState>();

	script_language_gd = memnew(GDScriptLanguage);
	ScriptServer::register_language(script_language_gd);

	resource_loader_gd.instance();
	ResourceLoader::add_resource_format_loader(resource_loader_gd);

	resource_saver_gd.instance();
	ResourceSaver::add_resource_format_saver(resource_saver_gd);

	gdscript_cache = memnew(GDScriptCache);

#ifdef TOOLS_ENABLED
	EditorNode::add_init_callback(_editor_init);

	gdscript_translation_parser_plugin.instance();
	EditorTranslationParser::get_singleton()->add_parser(gdscript_translation_parser_plugin, EditorTranslationParser::STANDARD);
#endif // TOOLS_ENABLED
}

void unregister_gdscript_types() {
	ScriptServer::unregister_language(script_language_gd);

	if (gdscript_cache) {
		memdelete(gdscript_cache);
	}

	if (script_language_gd) {
		memdelete(script_language_gd);
	}

	ResourceLoader::remove_resource_format_loader(resource_loader_gd);
	resource_loader_gd.unref();

	ResourceSaver::remove_resource_format_saver(resource_saver_gd);
	resource_saver_gd.unref();

#ifdef TOOLS_ENABLED
	EditorTranslationParser::get_singleton()->remove_parser(gdscript_translation_parser_plugin, EditorTranslationParser::STANDARD);
	gdscript_translation_parser_plugin.unref();
#endif // TOOLS_ENABLED

	GDScriptParser::cleanup();
	GDScriptAnalyzer::cleanup();
}

#ifdef TESTS_ENABLED
void test_tokenizer() {
	TestGDScript::test(TestGDScript::TestType::TEST_TOKENIZER);
}

void test_parser() {
	TestGDScript::test(TestGDScript::TestType::TEST_PARSER);
}

void test_compiler() {
	TestGDScript::test(TestGDScript::TestType::TEST_COMPILER);
}

void test_bytecode() {
	TestGDScript::test(TestGDScript::TestType::TEST_BYTECODE);
}

REGISTER_TEST_COMMAND("gdscript-tokenizer", &test_tokenizer);
REGISTER_TEST_COMMAND("gdscript-parser", &test_parser);
REGISTER_TEST_COMMAND("gdscript-compiler", &test_compiler);
REGISTER_TEST_COMMAND("gdscript-bytecode", &test_bytecode);
#endif
