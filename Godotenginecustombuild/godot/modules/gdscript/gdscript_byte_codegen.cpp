/*************************************************************************/
/*  gdscript_byte_codegen.cpp                                            */
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

#include "gdscript_byte_codegen.h"

#include "core/debugger/engine_debugger.h"
#include "gdscript.h"

uint32_t GDScriptByteCodeGenerator::add_parameter(const StringName &p_name, bool p_is_optional, const GDScriptDataType &p_type) {
#ifdef TOOLS_ENABLED
	function->arg_names.push_back(p_name);
#endif
	function->_argument_count++;
	function->argument_types.push_back(p_type);
	if (p_is_optional) {
		if (function->_default_arg_count == 0) {
			append(GDScriptFunction::OPCODE_JUMP_TO_DEF_ARGUMENT);
		}
		function->default_arguments.push_back(opcodes.size());
		function->_default_arg_count++;
	}

	return add_local(p_name, p_type);
}

uint32_t GDScriptByteCodeGenerator::add_local(const StringName &p_name, const GDScriptDataType &p_type) {
	int stack_pos = increase_stack();
	add_stack_identifier(p_name, stack_pos);
	return stack_pos;
}

uint32_t GDScriptByteCodeGenerator::add_local_constant(const StringName &p_name, const Variant &p_constant) {
	int index = add_or_get_constant(p_constant);
	local_constants[p_name] = index;
	return index;
}

uint32_t GDScriptByteCodeGenerator::add_or_get_constant(const Variant &p_constant) {
	if (constant_map.has(p_constant)) {
		return constant_map[p_constant];
	}
	int index = constant_map.size();
	constant_map[p_constant] = index;
	return index;
}

uint32_t GDScriptByteCodeGenerator::add_or_get_name(const StringName &p_name) {
	return get_name_map_pos(p_name);
}

uint32_t GDScriptByteCodeGenerator::add_temporary() {
	current_temporaries++;
	return increase_stack();
}

void GDScriptByteCodeGenerator::pop_temporary() {
	current_stack_size--;
	current_temporaries--;
}

void GDScriptByteCodeGenerator::start_parameters() {}

void GDScriptByteCodeGenerator::end_parameters() {
	function->default_arguments.invert();
}

void GDScriptByteCodeGenerator::write_start(GDScript *p_script, const StringName &p_function_name, bool p_static, MultiplayerAPI::RPCMode p_rpc_mode, const GDScriptDataType &p_return_type) {
	function = memnew(GDScriptFunction);
	debug_stack = EngineDebugger::is_active();

	function->name = p_function_name;
	function->_script = p_script;
	function->source = p_script->get_path();

#ifdef DEBUG_ENABLED
	function->func_cname = (String(function->source) + " - " + String(p_function_name)).utf8();
	function->_func_cname = function->func_cname.get_data();
#endif

	function->_static = p_static;
	function->return_type = p_return_type;
	function->rpc_mode = p_rpc_mode;
	function->_argument_count = 0;
}

GDScriptFunction *GDScriptByteCodeGenerator::write_end() {
	append(GDScriptFunction::OPCODE_END, 0);

	if (constant_map.size()) {
		function->_constant_count = constant_map.size();
		function->constants.resize(constant_map.size());
		function->_constants_ptr = function->constants.ptrw();
		const Variant *K = nullptr;
		while ((K = constant_map.next(K))) {
			int idx = constant_map[*K];
			function->constants.write[idx] = *K;
		}
	} else {
		function->_constants_ptr = nullptr;
		function->_constant_count = 0;
	}

	if (name_map.size()) {
		function->global_names.resize(name_map.size());
		function->_global_names_ptr = &function->global_names[0];
		for (Map<StringName, int>::Element *E = name_map.front(); E; E = E->next()) {
			function->global_names.write[E->get()] = E->key();
		}
		function->_global_names_count = function->global_names.size();

	} else {
		function->_global_names_ptr = nullptr;
		function->_global_names_count = 0;
	}

	if (opcodes.size()) {
		function->code = opcodes;
		function->_code_ptr = &function->code[0];
		function->_code_size = opcodes.size();

	} else {
		function->_code_ptr = nullptr;
		function->_code_size = 0;
	}

	if (function->default_arguments.size()) {
		function->_default_arg_count = function->default_arguments.size();
		function->_default_arg_ptr = &function->default_arguments[0];
	} else {
		function->_default_arg_count = 0;
		function->_default_arg_ptr = nullptr;
	}

	if (operator_func_map.size()) {
		function->operator_funcs.resize(operator_func_map.size());
		function->_operator_funcs_count = function->operator_funcs.size();
		function->_operator_funcs_ptr = function->operator_funcs.ptr();
		for (const Map<Variant::ValidatedOperatorEvaluator, int>::Element *E = operator_func_map.front(); E; E = E->next()) {
			function->operator_funcs.write[E->get()] = E->key();
		}
	} else {
		function->_operator_funcs_count = 0;
		function->_operator_funcs_ptr = nullptr;
	}

	if (setters_map.size()) {
		function->setters.resize(setters_map.size());
		function->_setters_count = function->setters.size();
		function->_setters_ptr = function->setters.ptr();
		for (const Map<Variant::ValidatedSetter, int>::Element *E = setters_map.front(); E; E = E->next()) {
			function->setters.write[E->get()] = E->key();
		}
	} else {
		function->_setters_count = 0;
		function->_setters_ptr = nullptr;
	}

	if (getters_map.size()) {
		function->getters.resize(getters_map.size());
		function->_getters_count = function->getters.size();
		function->_getters_ptr = function->getters.ptr();
		for (const Map<Variant::ValidatedGetter, int>::Element *E = getters_map.front(); E; E = E->next()) {
			function->getters.write[E->get()] = E->key();
		}
	} else {
		function->_getters_count = 0;
		function->_getters_ptr = nullptr;
	}

	if (keyed_setters_map.size()) {
		function->keyed_setters.resize(keyed_setters_map.size());
		function->_keyed_setters_count = function->keyed_setters.size();
		function->_keyed_setters_ptr = function->keyed_setters.ptr();
		for (const Map<Variant::ValidatedKeyedSetter, int>::Element *E = keyed_setters_map.front(); E; E = E->next()) {
			function->keyed_setters.write[E->get()] = E->key();
		}
	} else {
		function->_keyed_setters_count = 0;
		function->_keyed_setters_ptr = nullptr;
	}

	if (keyed_getters_map.size()) {
		function->keyed_getters.resize(keyed_getters_map.size());
		function->_keyed_getters_count = function->keyed_getters.size();
		function->_keyed_getters_ptr = function->keyed_getters.ptr();
		for (const Map<Variant::ValidatedKeyedGetter, int>::Element *E = keyed_getters_map.front(); E; E = E->next()) {
			function->keyed_getters.write[E->get()] = E->key();
		}
	} else {
		function->_keyed_getters_count = 0;
		function->_keyed_getters_ptr = nullptr;
	}

	if (indexed_setters_map.size()) {
		function->indexed_setters.resize(indexed_setters_map.size());
		function->_indexed_setters_count = function->indexed_setters.size();
		function->_indexed_setters_ptr = function->indexed_setters.ptr();
		for (const Map<Variant::ValidatedIndexedSetter, int>::Element *E = indexed_setters_map.front(); E; E = E->next()) {
			function->indexed_setters.write[E->get()] = E->key();
		}
	} else {
		function->_indexed_setters_count = 0;
		function->_indexed_setters_ptr = nullptr;
	}

	if (indexed_getters_map.size()) {
		function->indexed_getters.resize(indexed_getters_map.size());
		function->_indexed_getters_count = function->indexed_getters.size();
		function->_indexed_getters_ptr = function->indexed_getters.ptr();
		for (const Map<Variant::ValidatedIndexedGetter, int>::Element *E = indexed_getters_map.front(); E; E = E->next()) {
			function->indexed_getters.write[E->get()] = E->key();
		}
	} else {
		function->_indexed_getters_count = 0;
		function->_indexed_getters_ptr = nullptr;
	}

	if (builtin_method_map.size()) {
		function->builtin_methods.resize(builtin_method_map.size());
		function->_builtin_methods_ptr = function->builtin_methods.ptr();
		function->_builtin_methods_count = builtin_method_map.size();
		for (const Map<Variant::ValidatedBuiltInMethod, int>::Element *E = builtin_method_map.front(); E; E = E->next()) {
			function->builtin_methods.write[E->get()] = E->key();
		}
	} else {
		function->_builtin_methods_ptr = nullptr;
		function->_builtin_methods_count = 0;
	}

	if (constructors_map.size()) {
		function->constructors.resize(constructors_map.size());
		function->_constructors_ptr = function->constructors.ptr();
		function->_constructors_count = constructors_map.size();
		for (const Map<Variant::ValidatedConstructor, int>::Element *E = constructors_map.front(); E; E = E->next()) {
			function->constructors.write[E->get()] = E->key();
		}
	} else {
		function->_constructors_ptr = nullptr;
		function->_constructors_count = 0;
	}

	if (method_bind_map.size()) {
		function->methods.resize(method_bind_map.size());
		function->_methods_ptr = function->methods.ptrw();
		function->_methods_count = method_bind_map.size();
		for (const Map<MethodBind *, int>::Element *E = method_bind_map.front(); E; E = E->next()) {
			function->methods.write[E->get()] = E->key();
		}
	} else {
		function->_methods_ptr = nullptr;
		function->_methods_count = 0;
	}

	if (debug_stack) {
		function->stack_debug = stack_debug;
	}
	function->_stack_size = stack_max;
	function->_instruction_args_size = instr_args_max;
	function->_ptrcall_args_size = ptrcall_max;

	ended = true;
	return function;
}

#ifdef DEBUG_ENABLED
void GDScriptByteCodeGenerator::set_signature(const String &p_signature) {
	function->profile.signature = p_signature;
}
#endif

void GDScriptByteCodeGenerator::set_initial_line(int p_line) {
	function->_initial_line = p_line;
}

#define HAS_BUILTIN_TYPE(m_var) \
	(m_var.type.has_type && m_var.type.kind == GDScriptDataType::BUILTIN)

#define IS_BUILTIN_TYPE(m_var, m_type) \
	(m_var.type.has_type && m_var.type.kind == GDScriptDataType::BUILTIN && m_var.type.builtin_type == m_type)

void GDScriptByteCodeGenerator::write_operator(const Address &p_target, Variant::Operator p_operator, const Address &p_left_operand, const Address &p_right_operand) {
	if (HAS_BUILTIN_TYPE(p_left_operand) && HAS_BUILTIN_TYPE(p_right_operand)) {
		// Gather specific operator.
		Variant::ValidatedOperatorEvaluator op_func = Variant::get_validated_operator_evaluator(p_operator, p_left_operand.type.builtin_type, p_right_operand.type.builtin_type);

		append(GDScriptFunction::OPCODE_OPERATOR_VALIDATED, 3);
		append(p_left_operand);
		append(p_right_operand);
		append(p_target);
		append(op_func);
		return;
	}

	// No specific types, perform variant evaluation.
	append(GDScriptFunction::OPCODE_OPERATOR, 3);
	append(p_left_operand);
	append(p_right_operand);
	append(p_target);
	append(p_operator);
}

void GDScriptByteCodeGenerator::write_type_test(const Address &p_target, const Address &p_source, const Address &p_type) {
	append(GDScriptFunction::OPCODE_EXTENDS_TEST, 3);
	append(p_source);
	append(p_type);
	append(p_target);
}

void GDScriptByteCodeGenerator::write_type_test_builtin(const Address &p_target, const Address &p_source, Variant::Type p_type) {
	append(GDScriptFunction::OPCODE_IS_BUILTIN, 3);
	append(p_source);
	append(p_target);
	append(p_type);
}

void GDScriptByteCodeGenerator::write_and_left_operand(const Address &p_left_operand) {
	append(GDScriptFunction::OPCODE_JUMP_IF_NOT, 1);
	append(p_left_operand);
	logic_op_jump_pos1.push_back(opcodes.size());
	append(0); // Jump target, will be patched.
}

void GDScriptByteCodeGenerator::write_and_right_operand(const Address &p_right_operand) {
	append(GDScriptFunction::OPCODE_JUMP_IF_NOT, 1);
	append(p_right_operand);
	logic_op_jump_pos2.push_back(opcodes.size());
	append(0); // Jump target, will be patched.
}

void GDScriptByteCodeGenerator::write_end_and(const Address &p_target) {
	// If here means both operands are true.
	append(GDScriptFunction::OPCODE_ASSIGN_TRUE, 1);
	append(p_target);
	// Jump away from the fail condition.
	append(GDScriptFunction::OPCODE_JUMP, 0);
	append(opcodes.size() + 3);
	// Here it means one of operands is false.
	patch_jump(logic_op_jump_pos1.back()->get());
	patch_jump(logic_op_jump_pos2.back()->get());
	logic_op_jump_pos1.pop_back();
	logic_op_jump_pos2.pop_back();
	append(GDScriptFunction::OPCODE_ASSIGN_FALSE, 0);
	append(p_target);
}

void GDScriptByteCodeGenerator::write_or_left_operand(const Address &p_left_operand) {
	append(GDScriptFunction::OPCODE_JUMP_IF, 1);
	append(p_left_operand);
	logic_op_jump_pos1.push_back(opcodes.size());
	append(0); // Jump target, will be patched.
}

void GDScriptByteCodeGenerator::write_or_right_operand(const Address &p_right_operand) {
	append(GDScriptFunction::OPCODE_JUMP_IF, 1);
	append(p_right_operand);
	logic_op_jump_pos2.push_back(opcodes.size());
	append(0); // Jump target, will be patched.
}

void GDScriptByteCodeGenerator::write_end_or(const Address &p_target) {
	// If here means both operands are false.
	append(GDScriptFunction::OPCODE_ASSIGN_FALSE, 1);
	append(p_target);
	// Jump away from the success condition.
	append(GDScriptFunction::OPCODE_JUMP, 0);
	append(opcodes.size() + 3);
	// Here it means one of operands is false.
	patch_jump(logic_op_jump_pos1.back()->get());
	patch_jump(logic_op_jump_pos2.back()->get());
	logic_op_jump_pos1.pop_back();
	logic_op_jump_pos2.pop_back();
	append(GDScriptFunction::OPCODE_ASSIGN_TRUE, 1);
	append(p_target);
}

void GDScriptByteCodeGenerator::write_start_ternary(const Address &p_target) {
	ternary_result.push_back(p_target);
}

void GDScriptByteCodeGenerator::write_ternary_condition(const Address &p_condition) {
	append(GDScriptFunction::OPCODE_JUMP_IF_NOT, 1);
	append(p_condition);
	ternary_jump_fail_pos.push_back(opcodes.size());
	append(0); // Jump target, will be patched.
}

void GDScriptByteCodeGenerator::write_ternary_true_expr(const Address &p_expr) {
	append(GDScriptFunction::OPCODE_ASSIGN, 2);
	append(ternary_result.back()->get());
	append(p_expr);
	// Jump away from the false path.
	append(GDScriptFunction::OPCODE_JUMP, 0);
	ternary_jump_skip_pos.push_back(opcodes.size());
	append(0);
	// Fail must jump here.
	patch_jump(ternary_jump_fail_pos.back()->get());
	ternary_jump_fail_pos.pop_back();
}

void GDScriptByteCodeGenerator::write_ternary_false_expr(const Address &p_expr) {
	append(GDScriptFunction::OPCODE_ASSIGN, 2);
	append(ternary_result.back()->get());
	append(p_expr);
}

void GDScriptByteCodeGenerator::write_end_ternary() {
	patch_jump(ternary_jump_skip_pos.back()->get());
	ternary_jump_skip_pos.pop_back();
}

void GDScriptByteCodeGenerator::write_set(const Address &p_target, const Address &p_index, const Address &p_source) {
	if (HAS_BUILTIN_TYPE(p_target)) {
		if (IS_BUILTIN_TYPE(p_index, Variant::INT) && Variant::get_member_validated_indexed_setter(p_target.type.builtin_type)) {
			// Use indexed setter instead.
			Variant::ValidatedIndexedSetter setter = Variant::get_member_validated_indexed_setter(p_target.type.builtin_type);
			append(GDScriptFunction::OPCODE_SET_INDEXED_VALIDATED, 3);
			append(p_target);
			append(p_index);
			append(p_source);
			append(setter);
			return;
		} else if (Variant::get_member_validated_keyed_setter(p_target.type.builtin_type)) {
			Variant::ValidatedKeyedSetter setter = Variant::get_member_validated_keyed_setter(p_target.type.builtin_type);
			append(GDScriptFunction::OPCODE_SET_KEYED_VALIDATED, 3);
			append(p_target);
			append(p_index);
			append(p_source);
			append(setter);
			return;
		}
	}

	append(GDScriptFunction::OPCODE_SET_KEYED, 3);
	append(p_target);
	append(p_index);
	append(p_source);
}

void GDScriptByteCodeGenerator::write_get(const Address &p_target, const Address &p_index, const Address &p_source) {
	if (HAS_BUILTIN_TYPE(p_source)) {
		if (IS_BUILTIN_TYPE(p_index, Variant::INT) && Variant::get_member_validated_indexed_getter(p_source.type.builtin_type)) {
			// Use indexed getter instead.
			Variant::ValidatedIndexedGetter getter = Variant::get_member_validated_indexed_getter(p_source.type.builtin_type);
			append(GDScriptFunction::OPCODE_GET_INDEXED_VALIDATED, 3);
			append(p_source);
			append(p_index);
			append(p_target);
			append(getter);
			return;
		} else if (Variant::get_member_validated_keyed_getter(p_source.type.builtin_type)) {
			Variant::ValidatedKeyedGetter getter = Variant::get_member_validated_keyed_getter(p_source.type.builtin_type);
			append(GDScriptFunction::OPCODE_GET_KEYED_VALIDATED, 3);
			append(p_source);
			append(p_index);
			append(p_target);
			append(getter);
			return;
		}
	}
	append(GDScriptFunction::OPCODE_GET_KEYED, 3);
	append(p_source);
	append(p_index);
	append(p_target);
}

void GDScriptByteCodeGenerator::write_set_named(const Address &p_target, const StringName &p_name, const Address &p_source) {
	if (HAS_BUILTIN_TYPE(p_target) && Variant::get_member_validated_setter(p_target.type.builtin_type, p_name)) {
		Variant::ValidatedSetter setter = Variant::get_member_validated_setter(p_target.type.builtin_type, p_name);
		append(GDScriptFunction::OPCODE_SET_NAMED_VALIDATED, 2);
		append(p_target);
		append(p_source);
		append(setter);
		return;
	}
	append(GDScriptFunction::OPCODE_SET_NAMED, 2);
	append(p_target);
	append(p_source);
	append(p_name);
}

void GDScriptByteCodeGenerator::write_get_named(const Address &p_target, const StringName &p_name, const Address &p_source) {
	if (HAS_BUILTIN_TYPE(p_source) && Variant::get_member_validated_getter(p_source.type.builtin_type, p_name)) {
		Variant::ValidatedGetter getter = Variant::get_member_validated_getter(p_source.type.builtin_type, p_name);
		append(GDScriptFunction::OPCODE_GET_NAMED_VALIDATED, 2);
		append(p_source);
		append(p_target);
		append(getter);
		return;
	}
	append(GDScriptFunction::OPCODE_GET_NAMED, 2);
	append(p_source);
	append(p_target);
	append(p_name);
}

void GDScriptByteCodeGenerator::write_set_member(const Address &p_value, const StringName &p_name) {
	append(GDScriptFunction::OPCODE_SET_MEMBER, 1);
	append(p_value);
	append(p_name);
}

void GDScriptByteCodeGenerator::write_get_member(const Address &p_target, const StringName &p_name) {
	append(GDScriptFunction::OPCODE_GET_MEMBER, 1);
	append(p_target);
	append(p_name);
}

void GDScriptByteCodeGenerator::write_assign(const Address &p_target, const Address &p_source) {
	if (p_target.type.has_type && !p_source.type.has_type) {
		// Typed assignment.
		switch (p_target.type.kind) {
			case GDScriptDataType::BUILTIN: {
				append(GDScriptFunction::OPCODE_ASSIGN_TYPED_BUILTIN, 2);
				append(p_target);
				append(p_source);
				append(p_target.type.builtin_type);
			} break;
			case GDScriptDataType::NATIVE: {
				int class_idx = GDScriptLanguage::get_singleton()->get_global_map()[p_target.type.native_type];
				class_idx |= (GDScriptFunction::ADDR_TYPE_GLOBAL << GDScriptFunction::ADDR_BITS);
				append(GDScriptFunction::OPCODE_ASSIGN_TYPED_NATIVE, 3);
				append(p_target);
				append(p_source);
				append(class_idx);
			} break;
			case GDScriptDataType::SCRIPT:
			case GDScriptDataType::GDSCRIPT: {
				Variant script = p_target.type.script_type;
				int idx = get_constant_pos(script);
				idx |= (GDScriptFunction::ADDR_TYPE_LOCAL_CONSTANT << GDScriptFunction::ADDR_BITS);

				append(GDScriptFunction::OPCODE_ASSIGN_TYPED_SCRIPT, 3);
				append(p_target);
				append(p_source);
				append(idx);
			} break;
			default: {
				ERR_PRINT("Compiler bug: unresolved assign.");

				// Shouldn't get here, but fail-safe to a regular assignment
				append(GDScriptFunction::OPCODE_ASSIGN, 2);
				append(p_target);
				append(p_source);
			}
		}
	} else {
		if (p_target.type.kind == GDScriptDataType::BUILTIN && p_source.type.kind == GDScriptDataType::BUILTIN && p_target.type.builtin_type != p_source.type.builtin_type) {
			// Need conversion..
			append(GDScriptFunction::OPCODE_ASSIGN_TYPED_BUILTIN, 2);
			append(p_target);
			append(p_source);
			append(p_target.type.builtin_type);
		} else {
			// Either untyped assignment or already type-checked by the parser
			append(GDScriptFunction::OPCODE_ASSIGN, 2);
			append(p_target);
			append(p_source);
		}
	}
}

void GDScriptByteCodeGenerator::write_assign_true(const Address &p_target) {
	append(GDScriptFunction::OPCODE_ASSIGN_TRUE, 1);
	append(p_target);
}

void GDScriptByteCodeGenerator::write_assign_false(const Address &p_target) {
	append(GDScriptFunction::OPCODE_ASSIGN_FALSE, 1);
	append(p_target);
}

void GDScriptByteCodeGenerator::write_cast(const Address &p_target, const Address &p_source, const GDScriptDataType &p_type) {
	int index = 0;

	switch (p_type.kind) {
		case GDScriptDataType::BUILTIN: {
			append(GDScriptFunction::OPCODE_CAST_TO_BUILTIN, 2);
			index = p_type.builtin_type;
		} break;
		case GDScriptDataType::NATIVE: {
			int class_idx = GDScriptLanguage::get_singleton()->get_global_map()[p_type.native_type];
			class_idx |= (GDScriptFunction::ADDR_TYPE_GLOBAL << GDScriptFunction::ADDR_BITS);
			append(GDScriptFunction::OPCODE_CAST_TO_NATIVE, 3);
			index = class_idx;
		} break;
		case GDScriptDataType::SCRIPT:
		case GDScriptDataType::GDSCRIPT: {
			Variant script = p_type.script_type;
			int idx = get_constant_pos(script);
			idx |= (GDScriptFunction::ADDR_TYPE_LOCAL_CONSTANT << GDScriptFunction::ADDR_BITS);

			append(GDScriptFunction::OPCODE_CAST_TO_SCRIPT, 3);
			index = idx;
		} break;
		default: {
			return;
		}
	}

	append(p_source);
	append(p_target);
	append(index);
}

void GDScriptByteCodeGenerator::write_call(const Address &p_target, const Address &p_base, const StringName &p_function_name, const Vector<Address> &p_arguments) {
	append(p_target.mode == Address::NIL ? GDScriptFunction::OPCODE_CALL : GDScriptFunction::OPCODE_CALL_RETURN, 2 + p_arguments.size());
	for (int i = 0; i < p_arguments.size(); i++) {
		append(p_arguments[i]);
	}
	append(p_base);
	append(p_target);
	append(p_arguments.size());
	append(p_function_name);
}

void GDScriptByteCodeGenerator::write_super_call(const Address &p_target, const StringName &p_function_name, const Vector<Address> &p_arguments) {
	append(GDScriptFunction::OPCODE_CALL_SELF_BASE, 1 + p_arguments.size());
	for (int i = 0; i < p_arguments.size(); i++) {
		append(p_arguments[i]);
	}
	append(p_target);
	append(p_arguments.size());
	append(p_function_name);
}

void GDScriptByteCodeGenerator::write_call_async(const Address &p_target, const Address &p_base, const StringName &p_function_name, const Vector<Address> &p_arguments) {
	append(GDScriptFunction::OPCODE_CALL_ASYNC, 2 + p_arguments.size());
	for (int i = 0; i < p_arguments.size(); i++) {
		append(p_arguments[i]);
	}
	append(p_base);
	append(p_target);
	append(p_arguments.size());
	append(p_function_name);
}

void GDScriptByteCodeGenerator::write_call_builtin(const Address &p_target, GDScriptFunctions::Function p_function, const Vector<Address> &p_arguments) {
	append(GDScriptFunction::OPCODE_CALL_BUILT_IN, 1 + p_arguments.size());
	for (int i = 0; i < p_arguments.size(); i++) {
		append(p_arguments[i]);
	}
	append(p_target);
	append(p_arguments.size());
	append(p_function);
}

void GDScriptByteCodeGenerator::write_call_builtin_type(const Address &p_target, const Address &p_base, Variant::Type p_type, const StringName &p_method, const Vector<Address> &p_arguments) {
	bool is_validated = false;

	// Check if all types are correct.
	if (Variant::is_builtin_method_vararg(p_type, p_method)) {
		is_validated = true; // Vararg works fine with any argument, since they can be any type.
	} else if (p_arguments.size() == Variant::get_builtin_method_argument_count(p_type, p_method)) {
		bool all_types_exact = true;
		for (int i = 0; i < p_arguments.size(); i++) {
			if (!IS_BUILTIN_TYPE(p_arguments[i], Variant::get_builtin_method_argument_type(p_type, p_method, i))) {
				all_types_exact = false;
				break;
			}
		}

		is_validated = all_types_exact;
	}

	if (!is_validated) {
		// Perform regular call.
		write_call(p_target, p_base, p_method, p_arguments);
		return;
	}

	append(GDScriptFunction::OPCODE_CALL_BUILTIN_TYPE_VALIDATED, 2 + p_arguments.size());

	for (int i = 0; i < p_arguments.size(); i++) {
		append(p_arguments[i]);
	}
	append(p_base);
	append(p_target);
	append(p_arguments.size());
	append(Variant::get_validated_builtin_method(p_type, p_method));
}

void GDScriptByteCodeGenerator::write_call_method_bind(const Address &p_target, const Address &p_base, MethodBind *p_method, const Vector<Address> &p_arguments) {
	append(p_target.mode == Address::NIL ? GDScriptFunction::OPCODE_CALL_METHOD_BIND : GDScriptFunction::OPCODE_CALL_METHOD_BIND_RET, 2 + p_arguments.size());
	for (int i = 0; i < p_arguments.size(); i++) {
		append(p_arguments[i]);
	}
	append(p_base);
	append(p_target);
	append(p_arguments.size());
	append(p_method);
}

void GDScriptByteCodeGenerator::write_call_ptrcall(const Address &p_target, const Address &p_base, MethodBind *p_method, const Vector<Address> &p_arguments) {
#define CASE_TYPE(m_type)                                                               \
	case Variant::m_type:                                                               \
		append(GDScriptFunction::OPCODE_CALL_PTRCALL_##m_type, 2 + p_arguments.size()); \
		break

	bool is_ptrcall = true;

	if (p_method->has_return()) {
		MethodInfo info;
		ClassDB::get_method_info(p_method->get_instance_class(), p_method->get_name(), &info);
		switch (info.return_val.type) {
			CASE_TYPE(BOOL);
			CASE_TYPE(INT);
			CASE_TYPE(FLOAT);
			CASE_TYPE(STRING);
			CASE_TYPE(VECTOR2);
			CASE_TYPE(VECTOR2I);
			CASE_TYPE(RECT2);
			CASE_TYPE(RECT2I);
			CASE_TYPE(VECTOR3);
			CASE_TYPE(VECTOR3I);
			CASE_TYPE(TRANSFORM2D);
			CASE_TYPE(PLANE);
			CASE_TYPE(AABB);
			CASE_TYPE(BASIS);
			CASE_TYPE(TRANSFORM);
			CASE_TYPE(COLOR);
			CASE_TYPE(STRING_NAME);
			CASE_TYPE(NODE_PATH);
			CASE_TYPE(RID);
			CASE_TYPE(QUAT);
			CASE_TYPE(OBJECT);
			CASE_TYPE(CALLABLE);
			CASE_TYPE(SIGNAL);
			CASE_TYPE(DICTIONARY);
			CASE_TYPE(ARRAY);
			CASE_TYPE(PACKED_BYTE_ARRAY);
			CASE_TYPE(PACKED_INT32_ARRAY);
			CASE_TYPE(PACKED_INT64_ARRAY);
			CASE_TYPE(PACKED_FLOAT32_ARRAY);
			CASE_TYPE(PACKED_FLOAT64_ARRAY);
			CASE_TYPE(PACKED_STRING_ARRAY);
			CASE_TYPE(PACKED_VECTOR2_ARRAY);
			CASE_TYPE(PACKED_VECTOR3_ARRAY);
			CASE_TYPE(PACKED_COLOR_ARRAY);
			default:
				append(p_target.mode == Address::NIL ? GDScriptFunction::OPCODE_CALL_METHOD_BIND : GDScriptFunction::OPCODE_CALL_METHOD_BIND_RET, 2 + p_arguments.size());
				is_ptrcall = false;
				break;
		}
	} else {
		append(GDScriptFunction::OPCODE_CALL_PTRCALL_NO_RETURN, 2 + p_arguments.size());
	}

	for (int i = 0; i < p_arguments.size(); i++) {
		append(p_arguments[i]);
	}
	append(p_base);
	append(p_target);
	append(p_arguments.size());
	append(p_method);
	if (is_ptrcall) {
		alloc_ptrcall(p_arguments.size());
	}

#undef CASE_TYPE
}

void GDScriptByteCodeGenerator::write_call_self(const Address &p_target, const StringName &p_function_name, const Vector<Address> &p_arguments) {
	append(p_target.mode == Address::NIL ? GDScriptFunction::OPCODE_CALL : GDScriptFunction::OPCODE_CALL_RETURN, 2 + p_arguments.size());
	for (int i = 0; i < p_arguments.size(); i++) {
		append(p_arguments[i]);
	}
	append(GDScriptFunction::ADDR_TYPE_SELF << GDScriptFunction::ADDR_BITS);
	append(p_target);
	append(p_arguments.size());
	append(p_function_name);
}

void GDScriptByteCodeGenerator::write_call_script_function(const Address &p_target, const Address &p_base, const StringName &p_function_name, const Vector<Address> &p_arguments) {
	append(p_target.mode == Address::NIL ? GDScriptFunction::OPCODE_CALL : GDScriptFunction::OPCODE_CALL_RETURN, 2 + p_arguments.size());
	for (int i = 0; i < p_arguments.size(); i++) {
		append(p_arguments[i]);
	}
	append(p_base);
	append(p_target);
	append(p_arguments.size());
	append(p_function_name);
}

void GDScriptByteCodeGenerator::write_construct(const Address &p_target, Variant::Type p_type, const Vector<Address> &p_arguments) {
	// Try to find an appropriate constructor.
	bool all_have_type = true;
	Vector<Variant::Type> arg_types;
	for (int i = 0; i < p_arguments.size(); i++) {
		if (!HAS_BUILTIN_TYPE(p_arguments[i])) {
			all_have_type = false;
			break;
		}
		arg_types.push_back(p_arguments[i].type.builtin_type);
	}
	if (all_have_type) {
		int valid_constructor = -1;
		for (int i = 0; i < Variant::get_constructor_count(p_type); i++) {
			if (Variant::get_constructor_argument_count(p_type, i) != p_arguments.size()) {
				continue;
			}
			int types_correct = true;
			for (int j = 0; j < arg_types.size(); j++) {
				if (arg_types[j] != Variant::get_constructor_argument_type(p_type, i, j)) {
					types_correct = false;
					break;
				}
			}
			if (types_correct) {
				valid_constructor = i;
				break;
			}
		}
		if (valid_constructor >= 0) {
			append(GDScriptFunction::OPCODE_CONSTRUCT_VALIDATED, 1 + p_arguments.size());
			for (int i = 0; i < p_arguments.size(); i++) {
				append(p_arguments[i]);
			}
			append(p_target);
			append(p_arguments.size());
			append(Variant::get_validated_constructor(p_type, valid_constructor));
			return;
		}
	}

	append(GDScriptFunction::OPCODE_CONSTRUCT, 1 + p_arguments.size());
	for (int i = 0; i < p_arguments.size(); i++) {
		append(p_arguments[i]);
	}
	append(p_target);
	append(p_arguments.size());
	append(p_type);
}

void GDScriptByteCodeGenerator::write_construct_array(const Address &p_target, const Vector<Address> &p_arguments) {
	append(GDScriptFunction::OPCODE_CONSTRUCT_ARRAY, 1 + p_arguments.size());
	for (int i = 0; i < p_arguments.size(); i++) {
		append(p_arguments[i]);
	}
	append(p_target);
	append(p_arguments.size());
}

void GDScriptByteCodeGenerator::write_construct_dictionary(const Address &p_target, const Vector<Address> &p_arguments) {
	append(GDScriptFunction::OPCODE_CONSTRUCT_DICTIONARY, 1 + p_arguments.size());
	for (int i = 0; i < p_arguments.size(); i++) {
		append(p_arguments[i]);
	}
	append(p_target);
	append(p_arguments.size() / 2); // This is number of key-value pairs, so only half of actual arguments.
}

void GDScriptByteCodeGenerator::write_await(const Address &p_target, const Address &p_operand) {
	append(GDScriptFunction::OPCODE_AWAIT, 1);
	append(p_operand);
	append(GDScriptFunction::OPCODE_AWAIT_RESUME, 1);
	append(p_target);
}

void GDScriptByteCodeGenerator::write_if(const Address &p_condition) {
	append(GDScriptFunction::OPCODE_JUMP_IF_NOT, 1);
	append(p_condition);
	if_jmp_addrs.push_back(opcodes.size());
	append(0); // Jump destination, will be patched.
}

void GDScriptByteCodeGenerator::write_else() {
	append(GDScriptFunction::OPCODE_JUMP, 0); // Jump from true if block;
	int else_jmp_addr = opcodes.size();
	append(0); // Jump destination, will be patched.

	patch_jump(if_jmp_addrs.back()->get());
	if_jmp_addrs.pop_back();
	if_jmp_addrs.push_back(else_jmp_addr);
}

void GDScriptByteCodeGenerator::write_endif() {
	patch_jump(if_jmp_addrs.back()->get());
	if_jmp_addrs.pop_back();
}

void GDScriptByteCodeGenerator::write_for(const Address &p_variable, const Address &p_list) {
	int counter_pos = add_temporary() | (GDScriptFunction::ADDR_TYPE_STACK << GDScriptFunction::ADDR_BITS);
	int container_pos = add_temporary() | (GDScriptFunction::ADDR_TYPE_STACK << GDScriptFunction::ADDR_BITS);

	current_breaks_to_patch.push_back(List<int>());

	// Assign container.
	append(GDScriptFunction::OPCODE_ASSIGN, 2);
	append(container_pos);
	append(p_list);

	GDScriptFunction::Opcode begin_opcode = GDScriptFunction::OPCODE_ITERATE_BEGIN;
	GDScriptFunction::Opcode iterate_opcode = GDScriptFunction::OPCODE_ITERATE;

	if (p_list.type.has_type) {
		if (p_list.type.kind == GDScriptDataType::BUILTIN) {
			switch (p_list.type.builtin_type) {
				case Variant::INT:
					begin_opcode = GDScriptFunction::OPCODE_ITERATE_BEGIN_INT;
					iterate_opcode = GDScriptFunction::OPCODE_ITERATE_INT;
					break;
				case Variant::FLOAT:
					begin_opcode = GDScriptFunction::OPCODE_ITERATE_BEGIN_FLOAT;
					iterate_opcode = GDScriptFunction::OPCODE_ITERATE_FLOAT;
					break;
				case Variant::VECTOR2:
					begin_opcode = GDScriptFunction::OPCODE_ITERATE_BEGIN_VECTOR2;
					iterate_opcode = GDScriptFunction::OPCODE_ITERATE_VECTOR2;
					break;
				case Variant::VECTOR2I:
					begin_opcode = GDScriptFunction::OPCODE_ITERATE_BEGIN_VECTOR2I;
					iterate_opcode = GDScriptFunction::OPCODE_ITERATE_VECTOR2I;
					break;
				case Variant::VECTOR3:
					begin_opcode = GDScriptFunction::OPCODE_ITERATE_BEGIN_VECTOR3;
					iterate_opcode = GDScriptFunction::OPCODE_ITERATE_VECTOR3;
					break;
				case Variant::VECTOR3I:
					begin_opcode = GDScriptFunction::OPCODE_ITERATE_BEGIN_VECTOR3I;
					iterate_opcode = GDScriptFunction::OPCODE_ITERATE_VECTOR3I;
					break;
				case Variant::STRING:
					begin_opcode = GDScriptFunction::OPCODE_ITERATE_BEGIN_STRING;
					iterate_opcode = GDScriptFunction::OPCODE_ITERATE_STRING;
					break;
				case Variant::DICTIONARY:
					begin_opcode = GDScriptFunction::OPCODE_ITERATE_BEGIN_DICTIONARY;
					iterate_opcode = GDScriptFunction::OPCODE_ITERATE_DICTIONARY;
					break;
				case Variant::ARRAY:
					begin_opcode = GDScriptFunction::OPCODE_ITERATE_BEGIN_ARRAY;
					iterate_opcode = GDScriptFunction::OPCODE_ITERATE_ARRAY;
					break;
				case Variant::PACKED_BYTE_ARRAY:
					begin_opcode = GDScriptFunction::OPCODE_ITERATE_BEGIN_PACKED_BYTE_ARRAY;
					iterate_opcode = GDScriptFunction::OPCODE_ITERATE_PACKED_BYTE_ARRAY;
					break;
				case Variant::PACKED_INT32_ARRAY:
					begin_opcode = GDScriptFunction::OPCODE_ITERATE_BEGIN_PACKED_INT32_ARRAY;
					iterate_opcode = GDScriptFunction::OPCODE_ITERATE_PACKED_INT32_ARRAY;
					break;
				case Variant::PACKED_INT64_ARRAY:
					begin_opcode = GDScriptFunction::OPCODE_ITERATE_BEGIN_PACKED_INT64_ARRAY;
					iterate_opcode = GDScriptFunction::OPCODE_ITERATE_PACKED_INT64_ARRAY;
					break;
				case Variant::PACKED_FLOAT32_ARRAY:
					begin_opcode = GDScriptFunction::OPCODE_ITERATE_BEGIN_PACKED_FLOAT32_ARRAY;
					iterate_opcode = GDScriptFunction::OPCODE_ITERATE_PACKED_FLOAT32_ARRAY;
					break;
				case Variant::PACKED_FLOAT64_ARRAY:
					begin_opcode = GDScriptFunction::OPCODE_ITERATE_BEGIN_PACKED_FLOAT64_ARRAY;
					iterate_opcode = GDScriptFunction::OPCODE_ITERATE_PACKED_FLOAT64_ARRAY;
					break;
				case Variant::PACKED_STRING_ARRAY:
					begin_opcode = GDScriptFunction::OPCODE_ITERATE_BEGIN_PACKED_STRING_ARRAY;
					iterate_opcode = GDScriptFunction::OPCODE_ITERATE_PACKED_STRING_ARRAY;
					break;
				case Variant::PACKED_VECTOR2_ARRAY:
					begin_opcode = GDScriptFunction::OPCODE_ITERATE_BEGIN_PACKED_VECTOR2_ARRAY;
					iterate_opcode = GDScriptFunction::OPCODE_ITERATE_PACKED_VECTOR2_ARRAY;
					break;
				case Variant::PACKED_VECTOR3_ARRAY:
					begin_opcode = GDScriptFunction::OPCODE_ITERATE_BEGIN_PACKED_VECTOR3_ARRAY;
					iterate_opcode = GDScriptFunction::OPCODE_ITERATE_PACKED_VECTOR3_ARRAY;
					break;
				case Variant::PACKED_COLOR_ARRAY:
					begin_opcode = GDScriptFunction::OPCODE_ITERATE_BEGIN_PACKED_COLOR_ARRAY;
					iterate_opcode = GDScriptFunction::OPCODE_ITERATE_PACKED_COLOR_ARRAY;
					break;
				default:
					break;
			}
		} else {
			begin_opcode = GDScriptFunction::OPCODE_ITERATE_BEGIN_OBJECT;
			iterate_opcode = GDScriptFunction::OPCODE_ITERATE_OBJECT;
		}
	}

	// Begin loop.
	append(begin_opcode, 3);
	append(counter_pos);
	append(container_pos);
	append(p_variable);
	for_jmp_addrs.push_back(opcodes.size());
	append(0); // End of loop address, will be patched.
	append(GDScriptFunction::OPCODE_JUMP, 0);
	append(opcodes.size() + 6); // Skip over 'continue' code.

	// Next iteration.
	int continue_addr = opcodes.size();
	continue_addrs.push_back(continue_addr);
	append(iterate_opcode, 3);
	append(counter_pos);
	append(container_pos);
	append(p_variable);
	for_jmp_addrs.push_back(opcodes.size());
	append(0); // Jump destination, will be patched.
}

void GDScriptByteCodeGenerator::write_endfor() {
	// Jump back to loop check.
	append(GDScriptFunction::OPCODE_JUMP, 0);
	append(continue_addrs.back()->get());
	continue_addrs.pop_back();

	// Patch end jumps (two of them).
	for (int i = 0; i < 2; i++) {
		patch_jump(for_jmp_addrs.back()->get());
		for_jmp_addrs.pop_back();
	}

	// Patch break statements.
	for (const List<int>::Element *E = current_breaks_to_patch.back()->get().front(); E; E = E->next()) {
		patch_jump(E->get());
	}
	current_breaks_to_patch.pop_back();

	// Remove loop temporaries.
	pop_temporary();
	pop_temporary();
}

void GDScriptByteCodeGenerator::start_while_condition() {
	current_breaks_to_patch.push_back(List<int>());
	continue_addrs.push_back(opcodes.size());
}

void GDScriptByteCodeGenerator::write_while(const Address &p_condition) {
	// Condition check.
	append(GDScriptFunction::OPCODE_JUMP_IF_NOT, 1);
	append(p_condition);
	while_jmp_addrs.push_back(opcodes.size());
	append(0); // End of loop address, will be patched.
}

void GDScriptByteCodeGenerator::write_endwhile() {
	// Jump back to loop check.
	append(GDScriptFunction::OPCODE_JUMP, 0);
	append(continue_addrs.back()->get());
	continue_addrs.pop_back();

	// Patch end jump.
	patch_jump(while_jmp_addrs.back()->get());
	while_jmp_addrs.pop_back();

	// Patch break statements.
	for (const List<int>::Element *E = current_breaks_to_patch.back()->get().front(); E; E = E->next()) {
		patch_jump(E->get());
	}
	current_breaks_to_patch.pop_back();
}

void GDScriptByteCodeGenerator::start_match() {
	match_continues_to_patch.push_back(List<int>());
}

void GDScriptByteCodeGenerator::start_match_branch() {
	// Patch continue statements.
	for (const List<int>::Element *E = match_continues_to_patch.back()->get().front(); E; E = E->next()) {
		patch_jump(E->get());
	}
	match_continues_to_patch.pop_back();
	// Start a new list for next branch.
	match_continues_to_patch.push_back(List<int>());
}

void GDScriptByteCodeGenerator::end_match() {
	// Patch continue statements.
	for (const List<int>::Element *E = match_continues_to_patch.back()->get().front(); E; E = E->next()) {
		patch_jump(E->get());
	}
	match_continues_to_patch.pop_back();
}

void GDScriptByteCodeGenerator::write_break() {
	append(GDScriptFunction::OPCODE_JUMP, 0);
	current_breaks_to_patch.back()->get().push_back(opcodes.size());
	append(0);
}

void GDScriptByteCodeGenerator::write_continue() {
	append(GDScriptFunction::OPCODE_JUMP, 0);
	append(continue_addrs.back()->get());
}

void GDScriptByteCodeGenerator::write_continue_match() {
	append(GDScriptFunction::OPCODE_JUMP, 0);
	match_continues_to_patch.back()->get().push_back(opcodes.size());
	append(0);
}

void GDScriptByteCodeGenerator::write_breakpoint() {
	append(GDScriptFunction::OPCODE_BREAKPOINT, 0);
}

void GDScriptByteCodeGenerator::write_newline(int p_line) {
	append(GDScriptFunction::OPCODE_LINE, 0);
	append(p_line);
	current_line = p_line;
}

void GDScriptByteCodeGenerator::write_return(const Address &p_return_value) {
	append(GDScriptFunction::OPCODE_RETURN, 1);
	append(p_return_value);
}

void GDScriptByteCodeGenerator::write_assert(const Address &p_test, const Address &p_message) {
	append(GDScriptFunction::OPCODE_ASSERT, 2);
	append(p_test);
	append(p_message);
}

void GDScriptByteCodeGenerator::start_block() {
	push_stack_identifiers();
}

void GDScriptByteCodeGenerator::end_block() {
	pop_stack_identifiers();
}

GDScriptByteCodeGenerator::~GDScriptByteCodeGenerator() {
	if (!ended && function != nullptr) {
		memdelete(function);
	}
}
