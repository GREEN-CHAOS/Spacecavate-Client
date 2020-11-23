/*************************************************************************/
/*  doc_data.h                                                           */
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

#ifndef DOC_DATA_H
#define DOC_DATA_H

#include "core/io/xml_parser.h"
#include "core/templates/map.h"
#include "core/variant/variant.h"

class DocData {
public:
	struct ArgumentDoc {
		String name;
		String type;
		String enumeration;
		String default_value;
		bool operator<(const ArgumentDoc &p_arg) const {
			if (name == p_arg.name) {
				return type < p_arg.type;
			}
			return name < p_arg.name;
		}
	};

	struct MethodDoc {
		String name;
		String return_type;
		String return_enum;
		String qualifiers;
		String description;
		Vector<ArgumentDoc> arguments;
		bool operator<(const MethodDoc &p_method) const {
			if (name == p_method.name) {
				// Must be a constructor since there is no overloading.
				// We want this arbitrary order for a class "Foo":
				// - 1. Default constructor: Foo()
				// - 2. Copy constructor: Foo(Foo)
				// - 3+. Other constructors Foo(Bar, ...) based on first argument's name
				if (arguments.size() == 0 || p_method.arguments.size() == 0) { // 1.
					return arguments.size() < p_method.arguments.size();
				}
				if (arguments[0].type == return_type || p_method.arguments[0].type == p_method.return_type) { // 2.
					return (arguments[0].type == return_type) || (p_method.arguments[0].type != p_method.return_type);
				}
				return arguments[0] < p_method.arguments[0];
			}
			return name < p_method.name;
		}
	};

	struct ConstantDoc {
		String name;
		String value;
		bool is_value_valid;
		String enumeration;
		String description;
		bool operator<(const ConstantDoc &p_const) const {
			return name < p_const.name;
		}
	};

	struct PropertyDoc {
		String name;
		String type;
		String enumeration;
		String description;
		String setter, getter;
		String default_value;
		bool overridden = false;
		bool operator<(const PropertyDoc &p_prop) const {
			return name < p_prop.name;
		}
	};

	struct TutorialDoc {
		String link;
		String title;
	};

	struct ClassDoc {
		String name;
		String inherits;
		String category;
		String brief_description;
		String description;
		Vector<TutorialDoc> tutorials;
		Vector<MethodDoc> methods;
		Vector<MethodDoc> signals;
		Vector<ConstantDoc> constants;
		Vector<PropertyDoc> properties;
		Vector<PropertyDoc> theme_properties;
		bool operator<(const ClassDoc &p_class) const {
			return name < p_class.name;
		}
	};

	String version;

	Map<String, ClassDoc> class_list;
	Error _load(Ref<XMLParser> parser);

public:
	void merge_from(const DocData &p_data);
	void remove_from(const DocData &p_data);
	void generate(bool p_basic_types = false);
	Error load_classes(const String &p_dir);
	static Error erase_classes(const String &p_dir);
	Error save_classes(const String &p_default_path, const Map<String, String> &p_class_path);

	Error load_compressed(const uint8_t *p_data, int p_compressed_size, int p_uncompressed_size);
};

#endif // DOC_DATA_H
