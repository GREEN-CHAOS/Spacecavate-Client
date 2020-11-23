/*************************************************************************/
/*  gdscript.h                                                           */
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

#ifndef GDSCRIPT_H
#define GDSCRIPT_H

#include "core/debugger/engine_debugger.h"
#include "core/debugger/script_debugger.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/object/script_language.h"
#include "gdscript_function.h"

class GDScriptNativeClass : public Reference {
	GDCLASS(GDScriptNativeClass, Reference);

	StringName name;

protected:
	bool _get(const StringName &p_name, Variant &r_ret) const;
	static void _bind_methods();

public:
	_FORCE_INLINE_ const StringName &get_name() const { return name; }
	Variant _new();
	Object *instance();
	GDScriptNativeClass(const StringName &p_name);
};

class GDScript : public Script {
	GDCLASS(GDScript, Script);
	bool tool;
	bool valid;

	struct MemberInfo {
		int index;
		StringName setter;
		StringName getter;
		MultiplayerAPI::RPCMode rpc_mode;
		GDScriptDataType data_type;
	};

	friend class GDScriptInstance;
	friend class GDScriptFunction;
	friend class GDScriptAnalyzer;
	friend class GDScriptCompiler;
	friend class GDScriptFunctions;
	friend class GDScriptLanguage;

	Ref<GDScriptNativeClass> native;
	Ref<GDScript> base;
	GDScript *_base; //fast pointer access
	GDScript *_owner; //for subclasses

	Set<StringName> members; //members are just indices to the instanced script.
	Map<StringName, Variant> constants;
	Map<StringName, GDScriptFunction *> member_functions;
	Map<StringName, MemberInfo> member_indices; //members are just indices to the instanced script.
	Map<StringName, Ref<GDScript>> subclasses;
	Map<StringName, Vector<StringName>> _signals;
	Vector<ScriptNetData> rpc_functions;
	Vector<ScriptNetData> rpc_variables;

#ifdef TOOLS_ENABLED

	Map<StringName, int> member_lines;

	Map<StringName, Variant> member_default_values;

	List<PropertyInfo> members_cache;
	Map<StringName, Variant> member_default_values_cache;
	Ref<GDScript> base_cache;
	Set<ObjectID> inheriters_cache;
	bool source_changed_cache;
	bool placeholder_fallback_enabled;
	void _update_exports_values(Map<StringName, Variant> &values, List<PropertyInfo> &propnames);

#endif
	Map<StringName, PropertyInfo> member_info;

	GDScriptFunction *implicit_initializer = nullptr;
	GDScriptFunction *initializer = nullptr; //direct pointer to new , faster to locate

	int subclass_count;
	Set<Object *> instances;
	//exported members
	String source;
	String path;
	String name;
	String fully_qualified_name;
	SelfList<GDScript> script_list;

	SelfList<GDScriptFunctionState>::List pending_func_states;

	void _super_implicit_constructor(GDScript *p_script, GDScriptInstance *p_instance, Callable::CallError &r_error);
	GDScriptInstance *_create_instance(const Variant **p_args, int p_argcount, Object *p_owner, bool p_isref, Callable::CallError &r_error);

	void _set_subclass_path(Ref<GDScript> &p_sc, const String &p_path);

#ifdef TOOLS_ENABLED
	Set<PlaceHolderScriptInstance *> placeholders;
	//void _update_placeholder(PlaceHolderScriptInstance *p_placeholder);
	virtual void _placeholder_erased(PlaceHolderScriptInstance *p_placeholder) override;
#endif

#ifdef DEBUG_ENABLED

	Map<ObjectID, List<Pair<StringName, Variant>>> pending_reload_state;

#endif

	bool _update_exports(bool *r_err = nullptr, bool p_recursive_call = false);

	void _save_orphaned_subclasses();
	void _init_rpc_methods_properties();

protected:
	bool _get(const StringName &p_name, Variant &r_ret) const;
	bool _set(const StringName &p_name, const Variant &p_value);
	void _get_property_list(List<PropertyInfo> *p_properties) const;

	Variant call(const StringName &p_method, const Variant **p_args, int p_argcount, Callable::CallError &r_error) override;

	static void _bind_methods();

public:
	virtual bool is_valid() const override { return valid; }

	bool inherits_script(const Ref<Script> &p_script) const override;

	const Map<StringName, Ref<GDScript>> &get_subclasses() const { return subclasses; }
	const Map<StringName, Variant> &get_constants() const { return constants; }
	const Set<StringName> &get_members() const { return members; }
	const GDScriptDataType &get_member_type(const StringName &p_member) const {
		CRASH_COND(!member_indices.has(p_member));
		return member_indices[p_member].data_type;
	}
	const Map<StringName, GDScriptFunction *> &get_member_functions() const { return member_functions; }
	const Ref<GDScriptNativeClass> &get_native() const { return native; }
	const String &get_script_class_name() const { return name; }

	virtual bool has_script_signal(const StringName &p_signal) const override;
	virtual void get_script_signal_list(List<MethodInfo> *r_signals) const override;

	bool is_tool() const override { return tool; }
	Ref<GDScript> get_base() const;

	const Map<StringName, MemberInfo> &debug_get_member_indices() const { return member_indices; }
	const Map<StringName, GDScriptFunction *> &debug_get_member_functions() const; //this is debug only
	StringName debug_get_member_by_index(int p_idx) const;

	Variant _new(const Variant **p_args, int p_argcount, Callable::CallError &r_error);
	virtual bool can_instance() const override;

	virtual Ref<Script> get_base_script() const override;

	virtual StringName get_instance_base_type() const override; // this may not work in all scripts, will return empty if so
	virtual ScriptInstance *instance_create(Object *p_this) override;
	virtual PlaceHolderScriptInstance *placeholder_instance_create(Object *p_this) override;
	virtual bool instance_has(const Object *p_this) const override;

	virtual bool has_source_code() const override;
	virtual String get_source_code() const override;
	virtual void set_source_code(const String &p_code) override;
	virtual void update_exports() override;

	virtual Error reload(bool p_keep_state = false) override;

	void set_script_path(const String &p_path) { path = p_path; } //because subclasses need a path too...
	Error load_source_code(const String &p_path);
	Error load_byte_code(const String &p_path);

	Vector<uint8_t> get_as_byte_code() const;

	bool get_property_default_value(const StringName &p_property, Variant &r_value) const override;

	virtual void get_script_method_list(List<MethodInfo> *p_list) const override;
	virtual bool has_method(const StringName &p_method) const override;
	virtual MethodInfo get_method_info(const StringName &p_method) const override;

	virtual void get_script_property_list(List<PropertyInfo> *p_list) const override;

	virtual ScriptLanguage *get_language() const override;

	virtual int get_member_line(const StringName &p_member) const override {
#ifdef TOOLS_ENABLED
		if (member_lines.has(p_member)) {
			return member_lines[p_member];
		}
#endif
		return -1;
	}

	virtual void get_constants(Map<StringName, Variant> *p_constants) override;
	virtual void get_members(Set<StringName> *p_members) override;

	virtual Vector<ScriptNetData> get_rpc_methods() const override;
	virtual uint16_t get_rpc_method_id(const StringName &p_method) const override;
	virtual StringName get_rpc_method(const uint16_t p_rpc_method_id) const override;
	virtual MultiplayerAPI::RPCMode get_rpc_mode_by_id(const uint16_t p_rpc_method_id) const override;
	virtual MultiplayerAPI::RPCMode get_rpc_mode(const StringName &p_method) const override;

	virtual Vector<ScriptNetData> get_rset_properties() const override;
	virtual uint16_t get_rset_property_id(const StringName &p_variable) const override;
	virtual StringName get_rset_property(const uint16_t p_variable_id) const override;
	virtual MultiplayerAPI::RPCMode get_rset_mode_by_id(const uint16_t p_variable_id) const override;
	virtual MultiplayerAPI::RPCMode get_rset_mode(const StringName &p_variable) const override;

#ifdef TOOLS_ENABLED
	virtual bool is_placeholder_fallback_enabled() const override { return placeholder_fallback_enabled; }
#endif

	GDScript();
	~GDScript();
};

class GDScriptInstance : public ScriptInstance {
	friend class GDScript;
	friend class GDScriptFunction;
	friend class GDScriptFunctions;
	friend class GDScriptCompiler;

	ObjectID owner_id;
	Object *owner;
	Ref<GDScript> script;
#ifdef DEBUG_ENABLED
	Map<StringName, int> member_indices_cache; //used only for hot script reloading
#endif
	Vector<Variant> members;
	bool base_ref;

	SelfList<GDScriptFunctionState>::List pending_func_states;

public:
	virtual Object *get_owner() { return owner; }

	virtual bool set(const StringName &p_name, const Variant &p_value);
	virtual bool get(const StringName &p_name, Variant &r_ret) const;
	virtual void get_property_list(List<PropertyInfo> *p_properties) const;
	virtual Variant::Type get_property_type(const StringName &p_name, bool *r_is_valid = nullptr) const;

	virtual void get_method_list(List<MethodInfo> *p_list) const;
	virtual bool has_method(const StringName &p_method) const;
	virtual Variant call(const StringName &p_method, const Variant **p_args, int p_argcount, Callable::CallError &r_error);

	Variant debug_get_member_by_index(int p_idx) const { return members[p_idx]; }

	virtual void notification(int p_notification);
	String to_string(bool *r_valid);

	virtual Ref<Script> get_script() const;

	virtual ScriptLanguage *get_language();

	void set_path(const String &p_path);

	void reload_members();

	virtual Vector<ScriptNetData> get_rpc_methods() const;
	virtual uint16_t get_rpc_method_id(const StringName &p_method) const;
	virtual StringName get_rpc_method(const uint16_t p_rpc_method_id) const;
	virtual MultiplayerAPI::RPCMode get_rpc_mode_by_id(const uint16_t p_rpc_method_id) const;
	virtual MultiplayerAPI::RPCMode get_rpc_mode(const StringName &p_method) const;

	virtual Vector<ScriptNetData> get_rset_properties() const;
	virtual uint16_t get_rset_property_id(const StringName &p_variable) const;
	virtual StringName get_rset_property(const uint16_t p_variable_id) const;
	virtual MultiplayerAPI::RPCMode get_rset_mode_by_id(const uint16_t p_variable_id) const;
	virtual MultiplayerAPI::RPCMode get_rset_mode(const StringName &p_variable) const;

	GDScriptInstance();
	~GDScriptInstance();
};

class GDScriptLanguage : public ScriptLanguage {
	friend class GDScriptFunctionState;

	static GDScriptLanguage *singleton;

	Variant *_global_array;
	Vector<Variant> global_array;
	Map<StringName, int> globals;
	Map<StringName, Variant> named_globals;

	struct CallLevel {
		Variant *stack;
		GDScriptFunction *function;
		GDScriptInstance *instance;
		int *ip;
		int *line;
	};

	int _debug_parse_err_line;
	String _debug_parse_err_file;
	String _debug_error;
	int _debug_call_stack_pos;
	int _debug_max_call_stack;
	CallLevel *_call_stack;

	void _add_global(const StringName &p_name, const Variant &p_value);

	friend class GDScriptInstance;

	Mutex lock;

	friend class GDScript;

	SelfList<GDScript>::List script_list;
	friend class GDScriptFunction;

	SelfList<GDScriptFunction>::List function_list;
	bool profiling;
	uint64_t script_frame_time;

	Map<String, ObjectID> orphan_subclasses;

public:
	int calls;

	bool debug_break(const String &p_error, bool p_allow_continue = true);
	bool debug_break_parse(const String &p_file, int p_line, const String &p_error);

	_FORCE_INLINE_ void enter_function(GDScriptInstance *p_instance, GDScriptFunction *p_function, Variant *p_stack, int *p_ip, int *p_line) {
		if (Thread::get_main_id() != Thread::get_caller_id()) {
			return; //no support for other threads than main for now
		}

		if (EngineDebugger::get_script_debugger()->get_lines_left() > 0 && EngineDebugger::get_script_debugger()->get_depth() >= 0) {
			EngineDebugger::get_script_debugger()->set_depth(EngineDebugger::get_script_debugger()->get_depth() + 1);
		}

		if (_debug_call_stack_pos >= _debug_max_call_stack) {
			//stack overflow
			_debug_error = "Stack Overflow (Stack Size: " + itos(_debug_max_call_stack) + ")";
			EngineDebugger::get_script_debugger()->debug(this);
			return;
		}

		_call_stack[_debug_call_stack_pos].stack = p_stack;
		_call_stack[_debug_call_stack_pos].instance = p_instance;
		_call_stack[_debug_call_stack_pos].function = p_function;
		_call_stack[_debug_call_stack_pos].ip = p_ip;
		_call_stack[_debug_call_stack_pos].line = p_line;
		_debug_call_stack_pos++;
	}

	_FORCE_INLINE_ void exit_function() {
		if (Thread::get_main_id() != Thread::get_caller_id()) {
			return; //no support for other threads than main for now
		}

		if (EngineDebugger::get_script_debugger()->get_lines_left() > 0 && EngineDebugger::get_script_debugger()->get_depth() >= 0) {
			EngineDebugger::get_script_debugger()->set_depth(EngineDebugger::get_script_debugger()->get_depth() - 1);
		}

		if (_debug_call_stack_pos == 0) {
			_debug_error = "Stack Underflow (Engine Bug)";
			EngineDebugger::get_script_debugger()->debug(this);
			return;
		}

		_debug_call_stack_pos--;
	}

	virtual Vector<StackInfo> debug_get_current_stack_info() {
		if (Thread::get_main_id() != Thread::get_caller_id()) {
			return Vector<StackInfo>();
		}

		Vector<StackInfo> csi;
		csi.resize(_debug_call_stack_pos);
		for (int i = 0; i < _debug_call_stack_pos; i++) {
			csi.write[_debug_call_stack_pos - i - 1].line = _call_stack[i].line ? *_call_stack[i].line : 0;
			if (_call_stack[i].function) {
				csi.write[_debug_call_stack_pos - i - 1].func = _call_stack[i].function->get_name();
				csi.write[_debug_call_stack_pos - i - 1].file = _call_stack[i].function->get_script()->get_path();
			}
		}
		return csi;
	}

	struct {
		StringName _init;
		StringName _notification;
		StringName _set;
		StringName _get;
		StringName _get_property_list;
		StringName _script_source;

	} strings;

	_FORCE_INLINE_ int get_global_array_size() const { return global_array.size(); }
	_FORCE_INLINE_ Variant *get_global_array() { return _global_array; }
	_FORCE_INLINE_ const Map<StringName, int> &get_global_map() const { return globals; }
	_FORCE_INLINE_ const Map<StringName, Variant> &get_named_globals_map() const { return named_globals; }

	_FORCE_INLINE_ static GDScriptLanguage *get_singleton() { return singleton; }

	virtual String get_name() const;

	/* LANGUAGE FUNCTIONS */
	virtual void init();
	virtual String get_type() const;
	virtual String get_extension() const;
	virtual Error execute_file(const String &p_path);
	virtual void finish();

	/* EDITOR FUNCTIONS */
	virtual void get_reserved_words(List<String> *p_words) const;
	virtual void get_comment_delimiters(List<String> *p_delimiters) const;
	virtual void get_string_delimiters(List<String> *p_delimiters) const;
	virtual String _get_processed_template(const String &p_template, const String &p_base_class_name) const;
	virtual Ref<Script> get_template(const String &p_class_name, const String &p_base_class_name) const;
	virtual bool is_using_templates();
	virtual void make_template(const String &p_class_name, const String &p_base_class_name, Ref<Script> &p_script);
	virtual bool validate(const String &p_script, int &r_line_error, int &r_col_error, String &r_test_error, const String &p_path = "", List<String> *r_functions = nullptr, List<ScriptLanguage::Warning> *r_warnings = nullptr, Set<int> *r_safe_lines = nullptr) const;
	virtual Script *create_script() const;
	virtual bool has_named_classes() const;
	virtual bool supports_builtin_mode() const;
	virtual bool can_inherit_from_file() { return true; }
	virtual int find_function(const String &p_function, const String &p_code) const;
	virtual String make_function(const String &p_class, const String &p_name, const PackedStringArray &p_args) const;
	virtual Error complete_code(const String &p_code, const String &p_path, Object *p_owner, List<ScriptCodeCompletionOption> *r_options, bool &r_forced, String &r_call_hint);
#ifdef TOOLS_ENABLED
	virtual Error lookup_code(const String &p_code, const String &p_symbol, const String &p_path, Object *p_owner, LookupResult &r_result);
#endif
	virtual String _get_indentation() const;
	virtual void auto_indent_code(String &p_code, int p_from_line, int p_to_line) const;
	virtual void add_global_constant(const StringName &p_variable, const Variant &p_value);
	virtual void add_named_global_constant(const StringName &p_name, const Variant &p_value);
	virtual void remove_named_global_constant(const StringName &p_name);

	/* DEBUGGER FUNCTIONS */

	virtual String debug_get_error() const;
	virtual int debug_get_stack_level_count() const;
	virtual int debug_get_stack_level_line(int p_level) const;
	virtual String debug_get_stack_level_function(int p_level) const;
	virtual String debug_get_stack_level_source(int p_level) const;
	virtual void debug_get_stack_level_locals(int p_level, List<String> *p_locals, List<Variant> *p_values, int p_max_subitems = -1, int p_max_depth = -1);
	virtual void debug_get_stack_level_members(int p_level, List<String> *p_members, List<Variant> *p_values, int p_max_subitems = -1, int p_max_depth = -1);
	virtual ScriptInstance *debug_get_stack_level_instance(int p_level);
	virtual void debug_get_globals(List<String> *p_globals, List<Variant> *p_values, int p_max_subitems = -1, int p_max_depth = -1);
	virtual String debug_parse_stack_level_expression(int p_level, const String &p_expression, int p_max_subitems = -1, int p_max_depth = -1);

	virtual void reload_all_scripts();
	virtual void reload_tool_script(const Ref<Script> &p_script, bool p_soft_reload);

	virtual void frame();

	virtual void get_public_functions(List<MethodInfo> *p_functions) const;
	virtual void get_public_constants(List<Pair<String, Variant>> *p_constants) const;

	virtual void profiling_start();
	virtual void profiling_stop();

	virtual int profiling_get_accumulated_data(ProfilingInfo *p_info_arr, int p_info_max);
	virtual int profiling_get_frame_data(ProfilingInfo *p_info_arr, int p_info_max);

	/* LOADER FUNCTIONS */

	virtual void get_recognized_extensions(List<String> *p_extensions) const;

	/* GLOBAL CLASSES */

	virtual bool handles_global_class_type(const String &p_type) const;
	virtual String get_global_class_name(const String &p_path, String *r_base_type = nullptr, String *r_icon_path = nullptr) const;

	void add_orphan_subclass(const String &p_qualified_name, const ObjectID &p_subclass);
	Ref<GDScript> get_orphan_subclass(const String &p_qualified_name);

	GDScriptLanguage();
	~GDScriptLanguage();
};

class ResourceFormatLoaderGDScript : public ResourceFormatLoader {
public:
	virtual RES load(const String &p_path, const String &p_original_path = "", Error *r_error = nullptr, bool p_use_sub_threads = false, float *r_progress = nullptr, bool p_no_cache = false);
	virtual void get_recognized_extensions(List<String> *p_extensions) const;
	virtual bool handles_type(const String &p_type) const;
	virtual String get_resource_type(const String &p_path) const;
	virtual void get_dependencies(const String &p_path, List<String> *p_dependencies, bool p_add_types = false);
};

class ResourceFormatSaverGDScript : public ResourceFormatSaver {
public:
	virtual Error save(const String &p_path, const RES &p_resource, uint32_t p_flags = 0);
	virtual void get_recognized_extensions(const RES &p_resource, List<String> *p_extensions) const;
	virtual bool recognize(const RES &p_resource) const;
};

#endif // GDSCRIPT_H
