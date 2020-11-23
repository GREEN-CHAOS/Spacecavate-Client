/*************************************************************************/
/*  gdscript_byte_codegen.h                                              */
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

#ifndef GDSCRIPT_BYTE_CODEGEN
#define GDSCRIPT_BYTE_CODEGEN

#include "gdscript_codegen.h"

#include "gdscript_function.h"

class GDScriptByteCodeGenerator : public GDScriptCodeGenerator {
	bool ended = false;
	GDScriptFunction *function = nullptr;
	bool debug_stack = false;

	Vector<int> opcodes;
	List<Map<StringName, int>> stack_id_stack;
	Map<StringName, int> stack_identifiers;
	Map<StringName, int> local_constants;

	List<GDScriptFunction::StackDebug> stack_debug;
	List<Map<StringName, int>> block_identifier_stack;
	Map<StringName, int> block_identifiers;

	int current_stack_size = 0;
	int current_temporaries = 0;
	int current_line = 0;
	int stack_max = 0;
	int instr_args_max = 0;
	int ptrcall_max = 0;

	HashMap<Variant, int, VariantHasher, VariantComparator> constant_map;
	Map<StringName, int> name_map;
#ifdef TOOLS_ENABLED
	Vector<StringName> named_globals;
#endif
	Map<Variant::ValidatedOperatorEvaluator, int> operator_func_map;
	Map<Variant::ValidatedSetter, int> setters_map;
	Map<Variant::ValidatedGetter, int> getters_map;
	Map<Variant::ValidatedKeyedSetter, int> keyed_setters_map;
	Map<Variant::ValidatedKeyedGetter, int> keyed_getters_map;
	Map<Variant::ValidatedIndexedSetter, int> indexed_setters_map;
	Map<Variant::ValidatedIndexedGetter, int> indexed_getters_map;
	Map<Variant::ValidatedBuiltInMethod, int> builtin_method_map;
	Map<Variant::ValidatedConstructor, int> constructors_map;
	Map<MethodBind *, int> method_bind_map;

	List<int> if_jmp_addrs; // List since this can be nested.
	List<int> for_jmp_addrs;
	List<int> while_jmp_addrs;
	List<int> continue_addrs;

	// Used to patch jumps with `and` and `or` operators with short-circuit.
	List<int> logic_op_jump_pos1;
	List<int> logic_op_jump_pos2;

	List<Address> ternary_result;
	List<int> ternary_jump_fail_pos;
	List<int> ternary_jump_skip_pos;

	List<List<int>> current_breaks_to_patch;
	List<List<int>> match_continues_to_patch;

	void add_stack_identifier(const StringName &p_id, int p_stackpos) {
		stack_identifiers[p_id] = p_stackpos;
		if (debug_stack) {
			block_identifiers[p_id] = p_stackpos;
			GDScriptFunction::StackDebug sd;
			sd.added = true;
			sd.line = current_line;
			sd.identifier = p_id;
			sd.pos = p_stackpos;
			stack_debug.push_back(sd);
		}
	}

	void push_stack_identifiers() {
		stack_id_stack.push_back(stack_identifiers);
		if (debug_stack) {
			block_identifier_stack.push_back(block_identifiers);
			block_identifiers.clear();
		}
	}

	void pop_stack_identifiers() {
		stack_identifiers = stack_id_stack.back()->get();
		current_stack_size = stack_identifiers.size() + current_temporaries;
		stack_id_stack.pop_back();

		if (debug_stack) {
			for (Map<StringName, int>::Element *E = block_identifiers.front(); E; E = E->next()) {
				GDScriptFunction::StackDebug sd;
				sd.added = false;
				sd.identifier = E->key();
				sd.line = current_line;
				sd.pos = E->get();
				stack_debug.push_back(sd);
			}
			block_identifiers = block_identifier_stack.back()->get();
			block_identifier_stack.pop_back();
		}
	}

	int get_name_map_pos(const StringName &p_identifier) {
		int ret;
		if (!name_map.has(p_identifier)) {
			ret = name_map.size();
			name_map[p_identifier] = ret;
		} else {
			ret = name_map[p_identifier];
		}
		return ret;
	}

	int get_constant_pos(const Variant &p_constant) {
		if (constant_map.has(p_constant))
			return constant_map[p_constant];
		int pos = constant_map.size();
		constant_map[p_constant] = pos;
		return pos;
	}

	int get_operation_pos(const Variant::ValidatedOperatorEvaluator p_operation) {
		if (operator_func_map.has(p_operation))
			return operator_func_map[p_operation];
		int pos = operator_func_map.size();
		operator_func_map[p_operation] = pos;
		return pos;
	}

	int get_setter_pos(const Variant::ValidatedSetter p_setter) {
		if (setters_map.has(p_setter))
			return setters_map[p_setter];
		int pos = setters_map.size();
		setters_map[p_setter] = pos;
		return pos;
	}

	int get_getter_pos(const Variant::ValidatedGetter p_getter) {
		if (getters_map.has(p_getter))
			return getters_map[p_getter];
		int pos = getters_map.size();
		getters_map[p_getter] = pos;
		return pos;
	}

	int get_keyed_setter_pos(const Variant::ValidatedKeyedSetter p_keyed_setter) {
		if (keyed_setters_map.has(p_keyed_setter))
			return keyed_setters_map[p_keyed_setter];
		int pos = keyed_setters_map.size();
		keyed_setters_map[p_keyed_setter] = pos;
		return pos;
	}

	int get_keyed_getter_pos(const Variant::ValidatedKeyedGetter p_keyed_getter) {
		if (keyed_getters_map.has(p_keyed_getter))
			return keyed_getters_map[p_keyed_getter];
		int pos = keyed_getters_map.size();
		keyed_getters_map[p_keyed_getter] = pos;
		return pos;
	}

	int get_indexed_setter_pos(const Variant::ValidatedIndexedSetter p_indexed_setter) {
		if (indexed_setters_map.has(p_indexed_setter))
			return indexed_setters_map[p_indexed_setter];
		int pos = indexed_setters_map.size();
		indexed_setters_map[p_indexed_setter] = pos;
		return pos;
	}

	int get_indexed_getter_pos(const Variant::ValidatedIndexedGetter p_indexed_getter) {
		if (indexed_getters_map.has(p_indexed_getter))
			return indexed_getters_map[p_indexed_getter];
		int pos = indexed_getters_map.size();
		indexed_getters_map[p_indexed_getter] = pos;
		return pos;
	}

	int get_builtin_method_pos(const Variant::ValidatedBuiltInMethod p_method) {
		if (builtin_method_map.has(p_method)) {
			return builtin_method_map[p_method];
		}
		int pos = builtin_method_map.size();
		builtin_method_map[p_method] = pos;
		return pos;
	}

	int get_constructor_pos(const Variant::ValidatedConstructor p_constructor) {
		if (constructors_map.has(p_constructor)) {
			return constructors_map[p_constructor];
		}
		int pos = constructors_map.size();
		constructors_map[p_constructor] = pos;
		return pos;
	}

	int get_method_bind_pos(MethodBind *p_method) {
		if (method_bind_map.has(p_method)) {
			return method_bind_map[p_method];
		}
		int pos = method_bind_map.size();
		method_bind_map[p_method] = pos;
		return pos;
	}

	void alloc_stack(int p_level) {
		if (p_level >= stack_max)
			stack_max = p_level + 1;
	}

	int increase_stack() {
		int top = current_stack_size++;
		alloc_stack(current_stack_size);
		return top;
	}

	void alloc_ptrcall(int p_params) {
		if (p_params >= ptrcall_max)
			ptrcall_max = p_params;
	}

	int address_of(const Address &p_address) {
		switch (p_address.mode) {
			case Address::SELF:
				return GDScriptFunction::ADDR_TYPE_SELF << GDScriptFunction::ADDR_BITS;
			case Address::CLASS:
				return GDScriptFunction::ADDR_TYPE_CLASS << GDScriptFunction::ADDR_BITS;
			case Address::MEMBER:
				return p_address.address | (GDScriptFunction::ADDR_TYPE_MEMBER << GDScriptFunction::ADDR_BITS);
			case Address::CLASS_CONSTANT:
				return p_address.address | (GDScriptFunction::ADDR_TYPE_CLASS_CONSTANT << GDScriptFunction::ADDR_BITS);
			case Address::LOCAL_CONSTANT:
			case Address::CONSTANT:
				return p_address.address | (GDScriptFunction::ADDR_TYPE_LOCAL_CONSTANT << GDScriptFunction::ADDR_BITS);
			case Address::LOCAL_VARIABLE:
			case Address::TEMPORARY:
			case Address::FUNCTION_PARAMETER:
				return p_address.address | (GDScriptFunction::ADDR_TYPE_STACK << GDScriptFunction::ADDR_BITS);
			case Address::GLOBAL:
				return p_address.address | (GDScriptFunction::ADDR_TYPE_GLOBAL << GDScriptFunction::ADDR_BITS);
			case Address::NAMED_GLOBAL:
				return p_address.address | (GDScriptFunction::ADDR_TYPE_NAMED_GLOBAL << GDScriptFunction::ADDR_BITS);
			case Address::NIL:
				return GDScriptFunction::ADDR_TYPE_NIL << GDScriptFunction::ADDR_BITS;
		}
		return -1; // Unreachable.
	}

	void append(GDScriptFunction::Opcode p_code, int p_argument_count) {
		opcodes.push_back((p_code & GDScriptFunction::INSTR_MASK) | (p_argument_count << GDScriptFunction::INSTR_BITS));
		instr_args_max = MAX(instr_args_max, p_argument_count);
	}

	void append(int p_code) {
		opcodes.push_back(p_code);
	}

	void append(const Address &p_address) {
		opcodes.push_back(address_of(p_address));
	}

	void append(const StringName &p_name) {
		opcodes.push_back(get_name_map_pos(p_name));
	}

	void append(const Variant::ValidatedOperatorEvaluator p_operation) {
		opcodes.push_back(get_operation_pos(p_operation));
	}

	void append(const Variant::ValidatedSetter p_setter) {
		opcodes.push_back(get_setter_pos(p_setter));
	}

	void append(const Variant::ValidatedGetter p_getter) {
		opcodes.push_back(get_getter_pos(p_getter));
	}

	void append(const Variant::ValidatedKeyedSetter p_keyed_setter) {
		opcodes.push_back(get_keyed_setter_pos(p_keyed_setter));
	}

	void append(const Variant::ValidatedKeyedGetter p_keyed_getter) {
		opcodes.push_back(get_keyed_getter_pos(p_keyed_getter));
	}

	void append(const Variant::ValidatedIndexedSetter p_indexed_setter) {
		opcodes.push_back(get_indexed_setter_pos(p_indexed_setter));
	}

	void append(const Variant::ValidatedIndexedGetter p_indexed_getter) {
		opcodes.push_back(get_indexed_getter_pos(p_indexed_getter));
	}

	void append(const Variant::ValidatedBuiltInMethod p_method) {
		opcodes.push_back(get_builtin_method_pos(p_method));
	}

	void append(const Variant::ValidatedConstructor p_constructor) {
		opcodes.push_back(get_constructor_pos(p_constructor));
	}

	void append(MethodBind *p_method) {
		opcodes.push_back(get_method_bind_pos(p_method));
	}

	void patch_jump(int p_address) {
		opcodes.write[p_address] = opcodes.size();
	}

public:
	virtual uint32_t add_parameter(const StringName &p_name, bool p_is_optional, const GDScriptDataType &p_type) override;
	virtual uint32_t add_local(const StringName &p_name, const GDScriptDataType &p_type) override;
	virtual uint32_t add_local_constant(const StringName &p_name, const Variant &p_constant) override;
	virtual uint32_t add_or_get_constant(const Variant &p_constant) override;
	virtual uint32_t add_or_get_name(const StringName &p_name) override;
	virtual uint32_t add_temporary() override;
	virtual void pop_temporary() override;

	virtual void start_parameters() override;
	virtual void end_parameters() override;

	virtual void start_block() override;
	virtual void end_block() override;

	virtual void write_start(GDScript *p_script, const StringName &p_function_name, bool p_static, MultiplayerAPI::RPCMode p_rpc_mode, const GDScriptDataType &p_return_type) override;
	virtual GDScriptFunction *write_end() override;

#ifdef DEBUG_ENABLED
	virtual void set_signature(const String &p_signature) override;
#endif
	virtual void set_initial_line(int p_line) override;

	virtual void write_operator(const Address &p_target, Variant::Operator p_operator, const Address &p_left_operand, const Address &p_right_operand) override;
	virtual void write_type_test(const Address &p_target, const Address &p_source, const Address &p_type) override;
	virtual void write_type_test_builtin(const Address &p_target, const Address &p_source, Variant::Type p_type) override;
	virtual void write_and_left_operand(const Address &p_left_operand) override;
	virtual void write_and_right_operand(const Address &p_right_operand) override;
	virtual void write_end_and(const Address &p_target) override;
	virtual void write_or_left_operand(const Address &p_left_operand) override;
	virtual void write_or_right_operand(const Address &p_right_operand) override;
	virtual void write_end_or(const Address &p_target) override;
	virtual void write_start_ternary(const Address &p_target) override;
	virtual void write_ternary_condition(const Address &p_condition) override;
	virtual void write_ternary_true_expr(const Address &p_expr) override;
	virtual void write_ternary_false_expr(const Address &p_expr) override;
	virtual void write_end_ternary() override;
	virtual void write_set(const Address &p_target, const Address &p_index, const Address &p_source) override;
	virtual void write_get(const Address &p_target, const Address &p_index, const Address &p_source) override;
	virtual void write_set_named(const Address &p_target, const StringName &p_name, const Address &p_source) override;
	virtual void write_get_named(const Address &p_target, const StringName &p_name, const Address &p_source) override;
	virtual void write_set_member(const Address &p_value, const StringName &p_name) override;
	virtual void write_get_member(const Address &p_target, const StringName &p_name) override;
	virtual void write_assign(const Address &p_target, const Address &p_source) override;
	virtual void write_assign_true(const Address &p_target) override;
	virtual void write_assign_false(const Address &p_target) override;
	virtual void write_cast(const Address &p_target, const Address &p_source, const GDScriptDataType &p_type) override;
	virtual void write_call(const Address &p_target, const Address &p_base, const StringName &p_function_name, const Vector<Address> &p_arguments) override;
	virtual void write_super_call(const Address &p_target, const StringName &p_function_name, const Vector<Address> &p_arguments) override;
	virtual void write_call_async(const Address &p_target, const Address &p_base, const StringName &p_function_name, const Vector<Address> &p_arguments) override;
	virtual void write_call_builtin(const Address &p_target, GDScriptFunctions::Function p_function, const Vector<Address> &p_arguments) override;
	virtual void write_call_builtin_type(const Address &p_target, const Address &p_base, Variant::Type p_type, const StringName &p_method, const Vector<Address> &p_arguments) override;
	virtual void write_call_method_bind(const Address &p_target, const Address &p_base, MethodBind *p_method, const Vector<Address> &p_arguments) override;
	virtual void write_call_ptrcall(const Address &p_target, const Address &p_base, MethodBind *p_method, const Vector<Address> &p_arguments) override;
	virtual void write_call_self(const Address &p_target, const StringName &p_function_name, const Vector<Address> &p_arguments) override;
	virtual void write_call_script_function(const Address &p_target, const Address &p_base, const StringName &p_function_name, const Vector<Address> &p_arguments) override;
	virtual void write_construct(const Address &p_target, Variant::Type p_type, const Vector<Address> &p_arguments) override;
	virtual void write_construct_array(const Address &p_target, const Vector<Address> &p_arguments) override;
	virtual void write_construct_dictionary(const Address &p_target, const Vector<Address> &p_arguments) override;
	virtual void write_await(const Address &p_target, const Address &p_operand) override;
	virtual void write_if(const Address &p_condition) override;
	virtual void write_else() override;
	virtual void write_endif() override;
	virtual void write_for(const Address &p_variable, const Address &p_list) override;
	virtual void write_endfor() override;
	virtual void start_while_condition() override;
	virtual void write_while(const Address &p_condition) override;
	virtual void write_endwhile() override;
	virtual void start_match() override;
	virtual void start_match_branch() override;
	virtual void end_match() override;
	virtual void write_break() override;
	virtual void write_continue() override;
	virtual void write_continue_match() override;
	virtual void write_breakpoint() override;
	virtual void write_newline(int p_line) override;
	virtual void write_return(const Address &p_return_value) override;
	virtual void write_assert(const Address &p_test, const Address &p_message) override;

	virtual ~GDScriptByteCodeGenerator();
};

#endif // GDSCRIPT_BYTE_CODEGEN
