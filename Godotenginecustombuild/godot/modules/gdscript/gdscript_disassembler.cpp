/*************************************************************************/
/*  gdscript_disassembler.cpp                                            */
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

#ifdef DEBUG_ENABLED

#include "gdscript_function.h"

#include "core/string/string_builder.h"
#include "gdscript.h"
#include "gdscript_functions.h"

static String _get_variant_string(const Variant &p_variant) {
	String txt;
	if (p_variant.get_type() == Variant::STRING) {
		txt = "\"" + String(p_variant) + "\"";
	} else if (p_variant.get_type() == Variant::STRING_NAME) {
		txt = "&\"" + String(p_variant) + "\"";
	} else if (p_variant.get_type() == Variant::NODE_PATH) {
		txt = "^\"" + String(p_variant) + "\"";
	} else if (p_variant.get_type() == Variant::OBJECT) {
		Object *obj = p_variant;
		if (!obj) {
			txt = "null";
		} else {
			GDScriptNativeClass *cls = Object::cast_to<GDScriptNativeClass>(obj);
			if (cls) {
				txt += cls->get_name();
				txt += " (class)";
			} else {
				txt = obj->get_class();
				if (obj->get_script_instance()) {
					txt += "(" + obj->get_script_instance()->get_script()->get_path() + ")";
				}
			}
		}
	} else {
		txt = p_variant;
	}
	return txt;
}

static String _disassemble_address(const GDScript *p_script, const GDScriptFunction &p_function, int p_address) {
	int addr = p_address & GDScriptFunction::ADDR_MASK;

	switch (p_address >> GDScriptFunction::ADDR_BITS) {
		case GDScriptFunction::ADDR_TYPE_SELF: {
			return "self";
		} break;
		case GDScriptFunction::ADDR_TYPE_CLASS: {
			return "class";
		} break;
		case GDScriptFunction::ADDR_TYPE_MEMBER: {
			return "member(" + p_script->debug_get_member_by_index(addr) + ")";
		} break;
		case GDScriptFunction::ADDR_TYPE_CLASS_CONSTANT: {
			return "class_const(" + p_function.get_global_name(addr) + ")";
		} break;
		case GDScriptFunction::ADDR_TYPE_LOCAL_CONSTANT: {
			return "const(" + _get_variant_string(p_function.get_constant(addr)) + ")";
		} break;
		case GDScriptFunction::ADDR_TYPE_STACK: {
			return "stack(" + itos(addr) + ")";
		} break;
		case GDScriptFunction::ADDR_TYPE_STACK_VARIABLE: {
			return "var_stack(" + itos(addr) + ")";
		} break;
		case GDScriptFunction::ADDR_TYPE_GLOBAL: {
			return "global(" + _get_variant_string(GDScriptLanguage::get_singleton()->get_global_array()[addr]) + ")";
		} break;
		case GDScriptFunction::ADDR_TYPE_NAMED_GLOBAL: {
			return "named_global(" + p_function.get_global_name(addr) + ")";
		} break;
		case GDScriptFunction::ADDR_TYPE_NIL: {
			return "nil";
		} break;
	}

	return "<err>";
}

void GDScriptFunction::disassemble(const Vector<String> &p_code_lines) const {
#define DADDR(m_ip) (_disassemble_address(_script, *this, _code_ptr[ip + m_ip]))

	for (int ip = 0; ip < _code_size;) {
		StringBuilder text;
		int incr = 0;

		text += " ";
		text += itos(ip);
		text += ": ";

		// This makes the compiler complain if some opcode is unchecked in the switch.
		Opcode code = Opcode(_code_ptr[ip] & INSTR_MASK);
		int instr_var_args = (_code_ptr[ip] & INSTR_ARGS_MASK) >> INSTR_BITS;

		switch (code) {
			case OPCODE_OPERATOR: {
				int operation = _code_ptr[ip + 4];

				text += "operator ";

				text += DADDR(3);
				text += " = ";
				text += DADDR(1);
				text += " ";
				text += Variant::get_operator_name(Variant::Operator(operation));
				text += " ";
				text += DADDR(2);

				incr += 5;
			} break;
			case OPCODE_OPERATOR_VALIDATED: {
				text += "validated operator ";

				text += DADDR(3);
				text += " = ";
				text += DADDR(1);
				text += " <operator function> ";
				text += DADDR(2);

				incr += 5;
			} break;
			case OPCODE_EXTENDS_TEST: {
				text += "is object ";
				text += DADDR(3);
				text += " = ";
				text += DADDR(1);
				text += " is ";
				text += DADDR(2);

				incr += 4;
			} break;
			case OPCODE_IS_BUILTIN: {
				text += "is builtin ";
				text += DADDR(2);
				text += " = ";
				text += DADDR(1);
				text += " is ";
				text += Variant::get_type_name(Variant::Type(_code_ptr[ip + 3]));

				incr += 4;
			} break;
			case OPCODE_SET_KEYED: {
				text += "set keyed ";
				text += DADDR(1);
				text += "[";
				text += DADDR(2);
				text += "] = ";
				text += DADDR(3);

				incr += 4;
			} break;
			case OPCODE_SET_KEYED_VALIDATED: {
				text += "set keyed validated ";
				text += DADDR(1);
				text += "[";
				text += DADDR(2);
				text += "] = ";
				text += DADDR(3);

				incr += 5;
			} break;
			case OPCODE_SET_INDEXED_VALIDATED: {
				text += "set indexed validated ";
				text += DADDR(1);
				text += "[";
				text += DADDR(2);
				text += "] = ";
				text += DADDR(3);

				incr += 5;
			} break;
			case OPCODE_GET_KEYED: {
				text += "get keyed ";
				text += DADDR(3);
				text += " = ";
				text += DADDR(1);
				text += "[";
				text += DADDR(2);
				text += "]";

				incr += 4;
			} break;
			case OPCODE_GET_KEYED_VALIDATED: {
				text += "get keyed validated ";
				text += DADDR(3);
				text += " = ";
				text += DADDR(1);
				text += "[";
				text += DADDR(2);
				text += "]";

				incr += 5;
			} break;
			case OPCODE_GET_INDEXED_VALIDATED: {
				text += "get indexed validated ";
				text += DADDR(3);
				text += " = ";
				text += DADDR(1);
				text += "[";
				text += DADDR(2);
				text += "]";

				incr += 5;
			} break;
			case OPCODE_SET_NAMED: {
				text += "set_named ";
				text += DADDR(1);
				text += "[\"";
				text += _global_names_ptr[_code_ptr[ip + 3]];
				text += "\"] = ";
				text += DADDR(2);

				incr += 4;
			} break;
			case OPCODE_SET_NAMED_VALIDATED: {
				text += "set_named validated ";
				text += DADDR(1);
				text += "[\"";
				text += "<unknown name>";
				text += "\"] = ";
				text += DADDR(2);

				incr += 4;
			} break;
			case OPCODE_GET_NAMED: {
				text += "get_named ";
				text += DADDR(2);
				text += " = ";
				text += DADDR(1);
				text += "[\"";
				text += _global_names_ptr[_code_ptr[ip + 3]];
				text += "\"]";

				incr += 4;
			} break;
			case OPCODE_GET_NAMED_VALIDATED: {
				text += "get_named validated ";
				text += DADDR(2);
				text += " = ";
				text += DADDR(1);
				text += "[\"";
				text += "<unknown name>";
				text += "\"]";

				incr += 4;
			} break;
			case OPCODE_SET_MEMBER: {
				text += "set_member ";
				text += "[\"";
				text += _global_names_ptr[_code_ptr[ip + 2]];
				text += "\"] = ";
				text += DADDR(1);

				incr += 3;
			} break;
			case OPCODE_GET_MEMBER: {
				text += "get_member ";
				text += DADDR(1);
				text += " = ";
				text += "[\"";
				text += _global_names_ptr[_code_ptr[ip + 2]];
				text += "\"]";

				incr += 3;
			} break;
			case OPCODE_ASSIGN: {
				text += "assign ";
				text += DADDR(1);
				text += " = ";
				text += DADDR(2);

				incr += 3;
			} break;
			case OPCODE_ASSIGN_TRUE: {
				text += "assign ";
				text += DADDR(1);
				text += " = true";

				incr += 2;
			} break;
			case OPCODE_ASSIGN_FALSE: {
				text += "assign ";
				text += DADDR(1);
				text += " = false";

				incr += 2;
			} break;
			case OPCODE_ASSIGN_TYPED_BUILTIN: {
				text += "assign typed builtin (";
				text += Variant::get_type_name((Variant::Type)_code_ptr[ip + 3]);
				text += ") ";
				text += DADDR(1);
				text += " = ";
				text += DADDR(2);

				incr += 4;
			} break;
			case OPCODE_ASSIGN_TYPED_NATIVE: {
				Variant class_name = _constants_ptr[_code_ptr[ip + 3]];
				GDScriptNativeClass *nc = Object::cast_to<GDScriptNativeClass>(class_name.operator Object *());

				text += "assign typed native (";
				text += nc->get_name().operator String();
				text += ") ";
				text += DADDR(1);
				text += " = ";
				text += DADDR(2);

				incr += 4;
			} break;
			case OPCODE_ASSIGN_TYPED_SCRIPT: {
				Variant script = _constants_ptr[_code_ptr[ip + 3]];
				Script *sc = Object::cast_to<Script>(script.operator Object *());

				text += "assign typed script (";
				text += sc->get_path();
				text += ") ";
				text += DADDR(1);
				text += " = ";
				text += DADDR(2);

				incr += 4;
			} break;
			case OPCODE_CAST_TO_BUILTIN: {
				text += "cast builtin ";
				text += DADDR(2);
				text += " = ";
				text += DADDR(1);
				text += " as ";
				text += Variant::get_type_name(Variant::Type(_code_ptr[ip + 1]));

				incr += 4;
			} break;
			case OPCODE_CAST_TO_NATIVE: {
				Variant class_name = _constants_ptr[_code_ptr[ip + 1]];
				GDScriptNativeClass *nc = Object::cast_to<GDScriptNativeClass>(class_name.operator Object *());

				text += "cast native ";
				text += DADDR(2);
				text += " = ";
				text += DADDR(1);
				text += " as ";
				text += nc->get_name();

				incr += 4;
			} break;
			case OPCODE_CAST_TO_SCRIPT: {
				text += "cast ";
				text += DADDR(2);
				text += " = ";
				text += DADDR(1);
				text += " as ";
				text += DADDR(3);

				incr += 4;
			} break;
			case OPCODE_CONSTRUCT: {
				Variant::Type t = Variant::Type(_code_ptr[ip + 3 + instr_var_args]);
				int argc = _code_ptr[ip + 1 + instr_var_args];

				text += "construct ";
				text += DADDR(1 + argc);
				text += " = ";

				text += Variant::get_type_name(t) + "(";
				for (int i = 0; i < argc; i++) {
					if (i > 0)
						text += ", ";
					text += DADDR(i + 1);
				}
				text += ")";

				incr = 3 + instr_var_args;
			} break;
			case OPCODE_CONSTRUCT_VALIDATED: {
				int argc = _code_ptr[ip + 1 + instr_var_args];

				text += "construct validated ";
				text += DADDR(1 + argc);
				text += " = ";

				text += "<unkown type>(";
				for (int i = 0; i < argc; i++) {
					if (i > 0)
						text += ", ";
					text += DADDR(i + 1);
				}
				text += ")";

				incr = 3 + instr_var_args;
			} break;
			case OPCODE_CONSTRUCT_ARRAY: {
				int argc = _code_ptr[ip + 1 + instr_var_args];
				text += " make_array ";
				text += DADDR(1 + argc);
				text += " = [";

				for (int i = 0; i < argc; i++) {
					if (i > 0)
						text += ", ";
					text += DADDR(1 + i);
				}

				text += "]";

				incr += 3 + argc;
			} break;
			case OPCODE_CONSTRUCT_DICTIONARY: {
				int argc = _code_ptr[ip + 1 + instr_var_args];
				text += "make_dict ";
				text += DADDR(1 + argc * 2);
				text += " = {";

				for (int i = 0; i < argc; i++) {
					if (i > 0)
						text += ", ";
					text += DADDR(1 + i * 2 + 0);
					text += ": ";
					text += DADDR(1 + i * 2 + 1);
				}

				text += "}";

				incr += 3 + argc * 2;
			} break;
			case OPCODE_CALL:
			case OPCODE_CALL_RETURN:
			case OPCODE_CALL_ASYNC: {
				bool ret = (_code_ptr[ip] & INSTR_MASK) == OPCODE_CALL_RETURN;
				bool async = (_code_ptr[ip] & INSTR_MASK) == OPCODE_CALL_ASYNC;

				if (ret) {
					text += "call-ret ";
				} else if (async) {
					text += "call-async ";
				} else {
					text += "call ";
				}

				int argc = _code_ptr[ip + 1 + instr_var_args];
				if (ret || async) {
					text += DADDR(2 + argc) + " = ";
				}

				text += DADDR(1 + argc) + ".";
				text += String(_global_names_ptr[_code_ptr[ip + 2 + instr_var_args]]);
				text += "(";

				for (int i = 0; i < argc; i++) {
					if (i > 0)
						text += ", ";
					text += DADDR(1 + i);
				}
				text += ")";

				incr = 5 + argc;
			} break;
			case OPCODE_CALL_METHOD_BIND:
			case OPCODE_CALL_METHOD_BIND_RET: {
				bool ret = (_code_ptr[ip] & INSTR_MASK) == OPCODE_CALL_METHOD_BIND_RET;

				if (ret) {
					text += "call-method_bind-ret ";
				} else {
					text += "call-method_bind ";
				}

				MethodBind *method = _methods_ptr[_code_ptr[ip + 2 + instr_var_args]];

				int argc = _code_ptr[ip + 1 + instr_var_args];
				if (ret) {
					text += DADDR(2 + argc) + " = ";
				}

				text += DADDR(1 + argc) + ".";
				text += method->get_name();
				text += "(";

				for (int i = 0; i < argc; i++) {
					if (i > 0)
						text += ", ";
					text += DADDR(1 + i);
				}
				text += ")";

				incr = 5 + argc;
			} break;
			case OPCODE_CALL_PTRCALL_NO_RETURN: {
				text += "call-ptrcall (no return) ";

				MethodBind *method = _methods_ptr[_code_ptr[ip + 2 + instr_var_args]];

				int argc = _code_ptr[ip + 1 + instr_var_args];

				text += DADDR(1 + argc) + ".";
				text += method->get_name();
				text += "(";

				for (int i = 0; i < argc; i++) {
					if (i > 0)
						text += ", ";
					text += DADDR(1 + i);
				}
				text += ")";

				incr = 5 + argc;
			} break;

#define DISASSEMBLE_PTRCALL(m_type)                                            \
	case OPCODE_CALL_PTRCALL_##m_type: {                                       \
		text += "call-ptrcall (return ";                                       \
		text += #m_type;                                                       \
		text += ") ";                                                          \
		MethodBind *method = _methods_ptr[_code_ptr[ip + 2 + instr_var_args]]; \
		int argc = _code_ptr[ip + 1 + instr_var_args];                         \
		text += DADDR(2 + argc) + " = ";                                       \
		text += DADDR(1 + argc) + ".";                                         \
		text += method->get_name();                                            \
		text += "(";                                                           \
		for (int i = 0; i < argc; i++) {                                       \
			if (i > 0)                                                         \
				text += ", ";                                                  \
			text += DADDR(1 + i);                                              \
		}                                                                      \
		text += ")";                                                           \
		incr = 5 + argc;                                                       \
	} break

				DISASSEMBLE_PTRCALL(BOOL);
				DISASSEMBLE_PTRCALL(INT);
				DISASSEMBLE_PTRCALL(FLOAT);
				DISASSEMBLE_PTRCALL(STRING);
				DISASSEMBLE_PTRCALL(VECTOR2);
				DISASSEMBLE_PTRCALL(VECTOR2I);
				DISASSEMBLE_PTRCALL(RECT2);
				DISASSEMBLE_PTRCALL(RECT2I);
				DISASSEMBLE_PTRCALL(VECTOR3);
				DISASSEMBLE_PTRCALL(VECTOR3I);
				DISASSEMBLE_PTRCALL(TRANSFORM2D);
				DISASSEMBLE_PTRCALL(PLANE);
				DISASSEMBLE_PTRCALL(AABB);
				DISASSEMBLE_PTRCALL(BASIS);
				DISASSEMBLE_PTRCALL(TRANSFORM);
				DISASSEMBLE_PTRCALL(COLOR);
				DISASSEMBLE_PTRCALL(STRING_NAME);
				DISASSEMBLE_PTRCALL(NODE_PATH);
				DISASSEMBLE_PTRCALL(RID);
				DISASSEMBLE_PTRCALL(QUAT);
				DISASSEMBLE_PTRCALL(OBJECT);
				DISASSEMBLE_PTRCALL(CALLABLE);
				DISASSEMBLE_PTRCALL(SIGNAL);
				DISASSEMBLE_PTRCALL(DICTIONARY);
				DISASSEMBLE_PTRCALL(ARRAY);
				DISASSEMBLE_PTRCALL(PACKED_BYTE_ARRAY);
				DISASSEMBLE_PTRCALL(PACKED_INT32_ARRAY);
				DISASSEMBLE_PTRCALL(PACKED_INT64_ARRAY);
				DISASSEMBLE_PTRCALL(PACKED_FLOAT32_ARRAY);
				DISASSEMBLE_PTRCALL(PACKED_FLOAT64_ARRAY);
				DISASSEMBLE_PTRCALL(PACKED_STRING_ARRAY);
				DISASSEMBLE_PTRCALL(PACKED_VECTOR2_ARRAY);
				DISASSEMBLE_PTRCALL(PACKED_VECTOR3_ARRAY);
				DISASSEMBLE_PTRCALL(PACKED_COLOR_ARRAY);

			case OPCODE_CALL_BUILTIN_TYPE_VALIDATED: {
				int argc = _code_ptr[ip + 1 + instr_var_args];

				text += "call-builtin-method validated ";

				text += DADDR(2 + argc) + " = ";

				text += DADDR(1) + ".";
				text += "<unknown method>";

				text += "(";

				for (int i = 0; i < argc; i++) {
					if (i > 0)
						text += ", ";
					text += DADDR(1 + i);
				}
				text += ")";

				incr = 5 + argc;
			} break;
			case OPCODE_CALL_BUILT_IN: {
				text += "call-built-in ";

				int argc = _code_ptr[ip + 1 + instr_var_args];
				text += DADDR(1 + argc) + " = ";

				text += GDScriptFunctions::get_func_name(GDScriptFunctions::Function(_code_ptr[ip + 2 + instr_var_args]));
				text += "(";

				for (int i = 0; i < argc; i++) {
					if (i > 0)
						text += ", ";
					text += DADDR(1 + i);
				}
				text += ")";

				incr = 4 + argc;
			} break;
			case OPCODE_CALL_SELF_BASE: {
				text += "call-self-base ";

				int argc = _code_ptr[ip + 1 + instr_var_args];
				text += DADDR(2 + argc) + " = ";

				text += _global_names_ptr[_code_ptr[ip + 2 + instr_var_args]];
				text += "(";

				for (int i = 0; i < argc; i++) {
					if (i > 0)
						text += ", ";
					text += DADDR(1 + i);
				}
				text += ")";

				incr = 4 + argc;
			} break;
			case OPCODE_AWAIT: {
				text += "await ";
				text += DADDR(1);

				incr += 2;
			} break;
			case OPCODE_AWAIT_RESUME: {
				text += "await resume ";
				text += DADDR(1);

				incr = 2;
			} break;
			case OPCODE_JUMP: {
				text += "jump ";
				text += itos(_code_ptr[ip + 1]);

				incr = 2;
			} break;
			case OPCODE_JUMP_IF: {
				text += "jump-if ";
				text += DADDR(1);
				text += " to ";
				text += itos(_code_ptr[ip + 2]);

				incr = 3;
			} break;
			case OPCODE_JUMP_IF_NOT: {
				text += "jump-if-not ";
				text += DADDR(1);
				text += " to ";
				text += itos(_code_ptr[ip + 2]);

				incr = 3;
			} break;
			case OPCODE_JUMP_TO_DEF_ARGUMENT: {
				text += "jump-to-default-argument ";

				incr = 1;
			} break;
			case OPCODE_RETURN: {
				text += "return ";
				text += DADDR(1);

				incr = 2;
			} break;

#define DISASSEMBLE_ITERATE(m_type)      \
	case OPCODE_ITERATE_##m_type: {      \
		text += "for-loop (typed ";      \
		text += #m_type;                 \
		text += ") ";                    \
		text += DADDR(3);                \
		text += " in ";                  \
		text += DADDR(2);                \
		text += " counter ";             \
		text += DADDR(1);                \
		text += " end ";                 \
		text += itos(_code_ptr[ip + 4]); \
		incr += 5;                       \
	} break

#define DISASSEMBLE_ITERATE_BEGIN(m_type) \
	case OPCODE_ITERATE_BEGIN_##m_type: { \
		text += "for-init (typed ";       \
		text += #m_type;                  \
		text += ") ";                     \
		text += DADDR(3);                 \
		text += " in ";                   \
		text += DADDR(2);                 \
		text += " counter ";              \
		text += DADDR(1);                 \
		text += " end ";                  \
		text += itos(_code_ptr[ip + 4]);  \
		incr += 5;                        \
	} break

#define DISASSEMBLE_ITERATE_TYPES(m_macro) \
	m_macro(INT);                          \
	m_macro(FLOAT);                        \
	m_macro(VECTOR2);                      \
	m_macro(VECTOR2I);                     \
	m_macro(VECTOR3);                      \
	m_macro(VECTOR3I);                     \
	m_macro(STRING);                       \
	m_macro(DICTIONARY);                   \
	m_macro(ARRAY);                        \
	m_macro(PACKED_BYTE_ARRAY);            \
	m_macro(PACKED_INT32_ARRAY);           \
	m_macro(PACKED_INT64_ARRAY);           \
	m_macro(PACKED_FLOAT32_ARRAY);         \
	m_macro(PACKED_FLOAT64_ARRAY);         \
	m_macro(PACKED_STRING_ARRAY);          \
	m_macro(PACKED_VECTOR2_ARRAY);         \
	m_macro(PACKED_VECTOR3_ARRAY);         \
	m_macro(PACKED_COLOR_ARRAY);           \
	m_macro(OBJECT)

			case OPCODE_ITERATE_BEGIN: {
				text += "for-init ";
				text += DADDR(3);
				text += " in ";
				text += DADDR(2);
				text += " counter ";
				text += DADDR(1);
				text += " end ";
				text += itos(_code_ptr[ip + 4]);

				incr += 5;
			} break;
				DISASSEMBLE_ITERATE_TYPES(DISASSEMBLE_ITERATE_BEGIN);
			case OPCODE_ITERATE: {
				text += "for-loop ";
				text += DADDR(2);
				text += " in ";
				text += DADDR(2);
				text += " counter ";
				text += DADDR(1);
				text += " end ";
				text += itos(_code_ptr[ip + 4]);

				incr += 5;
			} break;
				DISASSEMBLE_ITERATE_TYPES(DISASSEMBLE_ITERATE);
			case OPCODE_LINE: {
				int line = _code_ptr[ip + 1] - 1;
				if (line >= 0 && line < p_code_lines.size()) {
					text += "line ";
					text += itos(line + 1);
					text += ": ";
					text += p_code_lines[line];
				} else {
					text += "";
				}

				incr += 2;
			} break;
			case OPCODE_ASSERT: {
				text += "assert (";
				text += DADDR(1);
				text += ", ";
				text += DADDR(2);
				text += ")";

				incr += 3;
			} break;
			case OPCODE_BREAKPOINT: {
				text += "breakpoint";

				incr += 1;
			} break;
			case OPCODE_END: {
				text += "== END ==";

				incr += 1;
			} break;
		}

		ip += incr;
		if (text.get_string_length() > 0) {
			print_line(text.as_string());
		}
	}
}

#endif
