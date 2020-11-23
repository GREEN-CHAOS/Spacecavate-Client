/*************************************************************************/
/*  variant_setget.cpp                                                   */
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

#include "variant.h"

#include "core/core_string_names.h"
#include "core/debugger/engine_debugger.h"
#include "core/object/class_db.h"
#include "core/templates/local_vector.h"
#include "core/variant/variant_internal.h"

/**** NAMED SETTERS AND GETTERS ****/

#define SETGET_STRUCT(m_base_type, m_member_type, m_member)                                                                          \
	struct VariantSetGet_##m_base_type##_##m_member {                                                                                \
		static void get(const Variant *base, Variant *member) {                                                                      \
			VariantTypeAdjust<m_member_type>::adjust(member);                                                                        \
			*VariantGetInternalPtr<m_member_type>::get_ptr(member) = VariantGetInternalPtr<m_base_type>::get_ptr(base)->m_member;    \
		}                                                                                                                            \
		static void ptr_get(const void *base, void *member) {                                                                        \
			PtrToArg<m_member_type>::encode(PtrToArg<m_base_type>::convert(base).m_member, member);                                  \
		}                                                                                                                            \
		static void set(Variant *base, const Variant *value, bool &valid) {                                                          \
			if (value->get_type() == GetTypeInfo<m_member_type>::VARIANT_TYPE) {                                                     \
				VariantGetInternalPtr<m_base_type>::get_ptr(base)->m_member = *VariantGetInternalPtr<m_member_type>::get_ptr(value); \
				valid = true;                                                                                                        \
			} else {                                                                                                                 \
				valid = false;                                                                                                       \
			}                                                                                                                        \
		}                                                                                                                            \
		static void validated_set(Variant *base, const Variant *value) {                                                             \
			VariantGetInternalPtr<m_base_type>::get_ptr(base)->m_member = *VariantGetInternalPtr<m_member_type>::get_ptr(value);     \
		}                                                                                                                            \
		static void ptr_set(void *base, const void *member) {                                                                        \
			m_base_type b = PtrToArg<m_base_type>::convert(base);                                                                    \
			b.m_member = PtrToArg<m_member_type>::convert(member);                                                                   \
			PtrToArg<m_base_type>::encode(b, base);                                                                                  \
		}                                                                                                                            \
		static Variant::Type get_type() { return GetTypeInfo<m_member_type>::VARIANT_TYPE; }                                         \
	};

#define SETGET_NUMBER_STRUCT(m_base_type, m_member_type, m_member)                                                                \
	struct VariantSetGet_##m_base_type##_##m_member {                                                                             \
		static void get(const Variant *base, Variant *member) {                                                                   \
			VariantTypeAdjust<m_member_type>::adjust(member);                                                                     \
			*VariantGetInternalPtr<m_member_type>::get_ptr(member) = VariantGetInternalPtr<m_base_type>::get_ptr(base)->m_member; \
		}                                                                                                                         \
		static void ptr_get(const void *base, void *member) {                                                                     \
			PtrToArg<m_member_type>::encode(PtrToArg<m_base_type>::convert(base).m_member, member);                               \
		}                                                                                                                         \
		static void set(Variant *base, const Variant *value, bool &valid) {                                                       \
			if (value->get_type() == Variant::FLOAT) {                                                                            \
				VariantGetInternalPtr<m_base_type>::get_ptr(base)->m_member = *VariantGetInternalPtr<double>::get_ptr(value);     \
				valid = true;                                                                                                     \
			} else if (value->get_type() == Variant::INT) {                                                                       \
				VariantGetInternalPtr<m_base_type>::get_ptr(base)->m_member = *VariantGetInternalPtr<int64_t>::get_ptr(value);    \
				valid = true;                                                                                                     \
			} else {                                                                                                              \
				valid = false;                                                                                                    \
			}                                                                                                                     \
		}                                                                                                                         \
		static void validated_set(Variant *base, const Variant *value) {                                                          \
			VariantGetInternalPtr<m_base_type>::get_ptr(base)->m_member = *VariantGetInternalPtr<m_member_type>::get_ptr(value);  \
		}                                                                                                                         \
		static void ptr_set(void *base, const void *member) {                                                                     \
			m_base_type b = PtrToArg<m_base_type>::convert(base);                                                                 \
			b.m_member = PtrToArg<m_member_type>::convert(member);                                                                \
			PtrToArg<m_base_type>::encode(b, base);                                                                               \
		}                                                                                                                         \
		static Variant::Type get_type() { return GetTypeInfo<m_member_type>::VARIANT_TYPE; }                                      \
	};

#define SETGET_STRUCT_CUSTOM(m_base_type, m_member_type, m_member, m_custom)                                                         \
	struct VariantSetGet_##m_base_type##_##m_member {                                                                                \
		static void get(const Variant *base, Variant *member) {                                                                      \
			VariantTypeAdjust<m_member_type>::adjust(member);                                                                        \
			*VariantGetInternalPtr<m_member_type>::get_ptr(member) = VariantGetInternalPtr<m_base_type>::get_ptr(base)->m_custom;    \
		}                                                                                                                            \
		static void ptr_get(const void *base, void *member) {                                                                        \
			PtrToArg<m_member_type>::encode(PtrToArg<m_base_type>::convert(base).m_custom, member);                                  \
		}                                                                                                                            \
		static void set(Variant *base, const Variant *value, bool &valid) {                                                          \
			if (value->get_type() == GetTypeInfo<m_member_type>::VARIANT_TYPE) {                                                     \
				VariantGetInternalPtr<m_base_type>::get_ptr(base)->m_custom = *VariantGetInternalPtr<m_member_type>::get_ptr(value); \
				valid = true;                                                                                                        \
			} else {                                                                                                                 \
				valid = false;                                                                                                       \
			}                                                                                                                        \
		}                                                                                                                            \
		static void validated_set(Variant *base, const Variant *value) {                                                             \
			VariantGetInternalPtr<m_base_type>::get_ptr(base)->m_custom = *VariantGetInternalPtr<m_member_type>::get_ptr(value);     \
		}                                                                                                                            \
		static void ptr_set(void *base, const void *member) {                                                                        \
			m_base_type b = PtrToArg<m_base_type>::convert(base);                                                                    \
			b.m_custom = PtrToArg<m_member_type>::convert(member);                                                                   \
			PtrToArg<m_base_type>::encode(b, base);                                                                                  \
		}                                                                                                                            \
		static Variant::Type get_type() { return GetTypeInfo<m_member_type>::VARIANT_TYPE; }                                         \
	};

#define SETGET_NUMBER_STRUCT_CUSTOM(m_base_type, m_member_type, m_member, m_custom)                                               \
	struct VariantSetGet_##m_base_type##_##m_member {                                                                             \
		static void get(const Variant *base, Variant *member) {                                                                   \
			VariantTypeAdjust<m_member_type>::adjust(member);                                                                     \
			*VariantGetInternalPtr<m_member_type>::get_ptr(member) = VariantGetInternalPtr<m_base_type>::get_ptr(base)->m_custom; \
		}                                                                                                                         \
		static void ptr_get(const void *base, void *member) {                                                                     \
			PtrToArg<m_member_type>::encode(PtrToArg<m_base_type>::convert(base).m_custom, member);                               \
		}                                                                                                                         \
		static void set(Variant *base, const Variant *value, bool &valid) {                                                       \
			if (value->get_type() == Variant::FLOAT) {                                                                            \
				VariantGetInternalPtr<m_base_type>::get_ptr(base)->m_custom = *VariantGetInternalPtr<double>::get_ptr(value);     \
				valid = true;                                                                                                     \
			} else if (value->get_type() == Variant::INT) {                                                                       \
				VariantGetInternalPtr<m_base_type>::get_ptr(base)->m_custom = *VariantGetInternalPtr<int64_t>::get_ptr(value);    \
				valid = true;                                                                                                     \
			} else {                                                                                                              \
				valid = false;                                                                                                    \
			}                                                                                                                     \
		}                                                                                                                         \
		static void validated_set(Variant *base, const Variant *value) {                                                          \
			VariantGetInternalPtr<m_base_type>::get_ptr(base)->m_custom = *VariantGetInternalPtr<m_member_type>::get_ptr(value);  \
		}                                                                                                                         \
		static void ptr_set(void *base, const void *member) {                                                                     \
			m_base_type b = PtrToArg<m_base_type>::convert(base);                                                                 \
			b.m_custom = PtrToArg<m_member_type>::convert(member);                                                                \
			PtrToArg<m_base_type>::encode(b, base);                                                                               \
		}                                                                                                                         \
		static Variant::Type get_type() { return GetTypeInfo<m_member_type>::VARIANT_TYPE; }                                      \
	};

#define SETGET_STRUCT_FUNC(m_base_type, m_member_type, m_member, m_setter, m_getter)                                                \
	struct VariantSetGet_##m_base_type##_##m_member {                                                                               \
		static void get(const Variant *base, Variant *member) {                                                                     \
			VariantTypeAdjust<m_member_type>::adjust(member);                                                                       \
			*VariantGetInternalPtr<m_member_type>::get_ptr(member) = VariantGetInternalPtr<m_base_type>::get_ptr(base)->m_getter(); \
		}                                                                                                                           \
		static void ptr_get(const void *base, void *member) {                                                                       \
			PtrToArg<m_member_type>::encode(PtrToArg<m_base_type>::convert(base).m_getter(), member);                               \
		}                                                                                                                           \
		static void set(Variant *base, const Variant *value, bool &valid) {                                                         \
			if (value->get_type() == GetTypeInfo<m_member_type>::VARIANT_TYPE) {                                                    \
				VariantGetInternalPtr<m_base_type>::get_ptr(base)->m_setter(*VariantGetInternalPtr<m_member_type>::get_ptr(value)); \
				valid = true;                                                                                                       \
			} else {                                                                                                                \
				valid = false;                                                                                                      \
			}                                                                                                                       \
		}                                                                                                                           \
		static void validated_set(Variant *base, const Variant *value) {                                                            \
			VariantGetInternalPtr<m_base_type>::get_ptr(base)->m_setter(*VariantGetInternalPtr<m_member_type>::get_ptr(value));     \
		}                                                                                                                           \
		static void ptr_set(void *base, const void *member) {                                                                       \
			m_base_type b = PtrToArg<m_base_type>::convert(base);                                                                   \
			b.m_setter(PtrToArg<m_member_type>::convert(member));                                                                   \
			PtrToArg<m_base_type>::encode(b, base);                                                                                 \
		}                                                                                                                           \
		static Variant::Type get_type() { return GetTypeInfo<m_member_type>::VARIANT_TYPE; }                                        \
	};

#define SETGET_NUMBER_STRUCT_FUNC(m_base_type, m_member_type, m_member, m_setter, m_getter)                                         \
	struct VariantSetGet_##m_base_type##_##m_member {                                                                               \
		static void get(const Variant *base, Variant *member) {                                                                     \
			VariantTypeAdjust<m_member_type>::adjust(member);                                                                       \
			*VariantGetInternalPtr<m_member_type>::get_ptr(member) = VariantGetInternalPtr<m_base_type>::get_ptr(base)->m_getter(); \
		}                                                                                                                           \
		static void ptr_get(const void *base, void *member) {                                                                       \
			PtrToArg<m_member_type>::encode(PtrToArg<m_base_type>::convert(base).m_getter(), member);                               \
		}                                                                                                                           \
		static void set(Variant *base, const Variant *value, bool &valid) {                                                         \
			if (value->get_type() == Variant::FLOAT) {                                                                              \
				VariantGetInternalPtr<m_base_type>::get_ptr(base)->m_setter(*VariantGetInternalPtr<double>::get_ptr(value));        \
				valid = true;                                                                                                       \
			} else if (value->get_type() == Variant::INT) {                                                                         \
				VariantGetInternalPtr<m_base_type>::get_ptr(base)->m_setter(*VariantGetInternalPtr<int64_t>::get_ptr(value));       \
				valid = true;                                                                                                       \
			} else {                                                                                                                \
				valid = false;                                                                                                      \
			}                                                                                                                       \
		}                                                                                                                           \
		static void validated_set(Variant *base, const Variant *value) {                                                            \
			VariantGetInternalPtr<m_base_type>::get_ptr(base)->m_setter(*VariantGetInternalPtr<m_member_type>::get_ptr(value));     \
		}                                                                                                                           \
		static void ptr_set(void *base, const void *member) {                                                                       \
			m_base_type b = PtrToArg<m_base_type>::convert(base);                                                                   \
			b.m_setter(PtrToArg<m_member_type>::convert(member));                                                                   \
			PtrToArg<m_base_type>::encode(b, base);                                                                                 \
		}                                                                                                                           \
		static Variant::Type get_type() { return GetTypeInfo<m_member_type>::VARIANT_TYPE; }                                        \
	};

#define SETGET_STRUCT_FUNC_INDEX(m_base_type, m_member_type, m_member, m_setter, m_getter, m_index)                                          \
	struct VariantSetGet_##m_base_type##_##m_member {                                                                                        \
		static void get(const Variant *base, Variant *member) {                                                                              \
			VariantTypeAdjust<m_member_type>::adjust(member);                                                                                \
			*VariantGetInternalPtr<m_member_type>::get_ptr(member) = VariantGetInternalPtr<m_base_type>::get_ptr(base)->m_getter(m_index);   \
		}                                                                                                                                    \
		static void ptr_get(const void *base, void *member) {                                                                                \
			PtrToArg<m_member_type>::encode(PtrToArg<m_base_type>::convert(base).m_getter(m_index), member);                                 \
		}                                                                                                                                    \
		static void set(Variant *base, const Variant *value, bool &valid) {                                                                  \
			if (value->get_type() == GetTypeInfo<m_member_type>::VARIANT_TYPE) {                                                             \
				VariantGetInternalPtr<m_base_type>::get_ptr(base)->m_setter(m_index, *VariantGetInternalPtr<m_member_type>::get_ptr(value)); \
				valid = true;                                                                                                                \
			} else {                                                                                                                         \
				valid = false;                                                                                                               \
			}                                                                                                                                \
		}                                                                                                                                    \
		static void validated_set(Variant *base, const Variant *value) {                                                                     \
			VariantGetInternalPtr<m_base_type>::get_ptr(base)->m_setter(m_index, *VariantGetInternalPtr<m_member_type>::get_ptr(value));     \
		}                                                                                                                                    \
		static void ptr_set(void *base, const void *member) {                                                                                \
			m_base_type b = PtrToArg<m_base_type>::convert(base);                                                                            \
			b.m_setter(m_index, PtrToArg<m_member_type>::convert(member));                                                                   \
			PtrToArg<m_base_type>::encode(b, base);                                                                                          \
		}                                                                                                                                    \
		static Variant::Type get_type() { return GetTypeInfo<m_member_type>::VARIANT_TYPE; }                                                 \
	};

SETGET_NUMBER_STRUCT(Vector2, double, x)
SETGET_NUMBER_STRUCT(Vector2, double, y)

SETGET_NUMBER_STRUCT(Vector2i, int64_t, x)
SETGET_NUMBER_STRUCT(Vector2i, int64_t, y)

SETGET_NUMBER_STRUCT(Vector3, double, x)
SETGET_NUMBER_STRUCT(Vector3, double, y)
SETGET_NUMBER_STRUCT(Vector3, double, z)

SETGET_NUMBER_STRUCT(Vector3i, int64_t, x)
SETGET_NUMBER_STRUCT(Vector3i, int64_t, y)
SETGET_NUMBER_STRUCT(Vector3i, int64_t, z)

SETGET_STRUCT(Rect2, Vector2, position)
SETGET_STRUCT(Rect2, Vector2, size)
SETGET_STRUCT_FUNC(Rect2, Vector2, end, set_end, get_end)

SETGET_STRUCT(Rect2i, Vector2i, position)
SETGET_STRUCT(Rect2i, Vector2i, size)
SETGET_STRUCT_FUNC(Rect2i, Vector2i, end, set_end, get_end)

SETGET_STRUCT(AABB, Vector3, position)
SETGET_STRUCT(AABB, Vector3, size)
SETGET_STRUCT_FUNC(AABB, Vector3, end, set_end, get_end)

SETGET_STRUCT_CUSTOM(Transform2D, Vector2, x, elements[0])
SETGET_STRUCT_CUSTOM(Transform2D, Vector2, y, elements[1])
SETGET_STRUCT_CUSTOM(Transform2D, Vector2, origin, elements[2])

SETGET_NUMBER_STRUCT_CUSTOM(Plane, double, x, normal.x)
SETGET_NUMBER_STRUCT_CUSTOM(Plane, double, y, normal.y)
SETGET_NUMBER_STRUCT_CUSTOM(Plane, double, z, normal.z)
SETGET_STRUCT(Plane, Vector3, normal)
SETGET_NUMBER_STRUCT(Plane, double, d)

SETGET_NUMBER_STRUCT(Quat, double, x)
SETGET_NUMBER_STRUCT(Quat, double, y)
SETGET_NUMBER_STRUCT(Quat, double, z)
SETGET_NUMBER_STRUCT(Quat, double, w)

SETGET_STRUCT_FUNC_INDEX(Basis, Vector3, x, set_axis, get_axis, 0)
SETGET_STRUCT_FUNC_INDEX(Basis, Vector3, y, set_axis, get_axis, 1)
SETGET_STRUCT_FUNC_INDEX(Basis, Vector3, z, set_axis, get_axis, 2)

SETGET_STRUCT(Transform, Basis, basis)
SETGET_STRUCT(Transform, Vector3, origin)

SETGET_NUMBER_STRUCT(Color, double, r)
SETGET_NUMBER_STRUCT(Color, double, g)
SETGET_NUMBER_STRUCT(Color, double, b)
SETGET_NUMBER_STRUCT(Color, double, a)

SETGET_NUMBER_STRUCT_FUNC(Color, int64_t, r8, set_r8, get_r8)
SETGET_NUMBER_STRUCT_FUNC(Color, int64_t, g8, set_g8, get_g8)
SETGET_NUMBER_STRUCT_FUNC(Color, int64_t, b8, set_b8, get_b8)
SETGET_NUMBER_STRUCT_FUNC(Color, int64_t, a8, set_a8, get_a8)

SETGET_NUMBER_STRUCT_FUNC(Color, double, h, set_h, get_h)
SETGET_NUMBER_STRUCT_FUNC(Color, double, s, set_s, get_s)
SETGET_NUMBER_STRUCT_FUNC(Color, double, v, set_v, get_v)

struct VariantSetterGetterInfo {
	void (*setter)(Variant *base, const Variant *value, bool &valid);
	void (*getter)(const Variant *base, Variant *value);
	Variant::ValidatedSetter validated_setter;
	Variant::ValidatedGetter validated_getter;
	Variant::PTRSetter ptr_setter;
	Variant::PTRGetter ptr_getter;
	Variant::Type member_type;
};

static LocalVector<VariantSetterGetterInfo> variant_setters_getters[Variant::VARIANT_MAX];
static LocalVector<StringName> variant_setters_getters_names[Variant::VARIANT_MAX]; //one next to another to make it cache friendly

template <class T>
static void register_member(Variant::Type p_type, const StringName &p_member) {
	VariantSetterGetterInfo sgi;
	sgi.setter = T::set;
	sgi.validated_setter = T::validated_set;
	sgi.ptr_setter = T::ptr_set;

	sgi.getter = T::get;
	sgi.validated_getter = T::get;
	sgi.ptr_getter = T::ptr_get;

	sgi.member_type = T::get_type();

	variant_setters_getters[p_type].push_back(sgi);
	variant_setters_getters_names[p_type].push_back(p_member);
}

void register_named_setters_getters() {
#define REGISTER_MEMBER(m_base_type, m_member) register_member<VariantSetGet_##m_base_type##_##m_member>(GetTypeInfo<m_base_type>::VARIANT_TYPE, #m_member)

	REGISTER_MEMBER(Vector2, x);
	REGISTER_MEMBER(Vector2, y);

	REGISTER_MEMBER(Vector2i, x);
	REGISTER_MEMBER(Vector2i, y);

	REGISTER_MEMBER(Vector3, x);
	REGISTER_MEMBER(Vector3, y);
	REGISTER_MEMBER(Vector3, z);

	REGISTER_MEMBER(Vector3i, x);
	REGISTER_MEMBER(Vector3i, y);
	REGISTER_MEMBER(Vector3i, z);

	REGISTER_MEMBER(Rect2, position);
	REGISTER_MEMBER(Rect2, size);
	REGISTER_MEMBER(Rect2, end);

	REGISTER_MEMBER(Rect2i, position);
	REGISTER_MEMBER(Rect2i, size);
	REGISTER_MEMBER(Rect2i, end);

	REGISTER_MEMBER(AABB, position);
	REGISTER_MEMBER(AABB, size);
	REGISTER_MEMBER(AABB, end);

	REGISTER_MEMBER(Transform2D, x);
	REGISTER_MEMBER(Transform2D, y);
	REGISTER_MEMBER(Transform2D, origin);

	REGISTER_MEMBER(Plane, x);
	REGISTER_MEMBER(Plane, y);
	REGISTER_MEMBER(Plane, z);
	REGISTER_MEMBER(Plane, d);
	REGISTER_MEMBER(Plane, normal);

	REGISTER_MEMBER(Quat, x);
	REGISTER_MEMBER(Quat, y);
	REGISTER_MEMBER(Quat, z);
	REGISTER_MEMBER(Quat, w);

	REGISTER_MEMBER(Basis, x);
	REGISTER_MEMBER(Basis, y);
	REGISTER_MEMBER(Basis, z);

	REGISTER_MEMBER(Transform, basis);
	REGISTER_MEMBER(Transform, origin);

	REGISTER_MEMBER(Color, r);
	REGISTER_MEMBER(Color, g);
	REGISTER_MEMBER(Color, b);
	REGISTER_MEMBER(Color, a);

	REGISTER_MEMBER(Color, r8);
	REGISTER_MEMBER(Color, g8);
	REGISTER_MEMBER(Color, b8);
	REGISTER_MEMBER(Color, a8);

	REGISTER_MEMBER(Color, h);
	REGISTER_MEMBER(Color, s);
	REGISTER_MEMBER(Color, v);
}

void unregister_named_setters_getters() {
	for (int i = 0; i < Variant::VARIANT_MAX; i++) {
		variant_setters_getters[i].clear();
		variant_setters_getters_names[i].clear();
	}
}

bool Variant::has_member(Variant::Type p_type, const StringName &p_member) {
	ERR_FAIL_INDEX_V(p_type, Variant::VARIANT_MAX, false);

	for (uint32_t i = 0; i < variant_setters_getters_names[p_type].size(); i++) {
		if (variant_setters_getters_names[p_type][i] == p_member) {
			return true;
		}
	}
	return false;
}

Variant::Type Variant::get_member_type(Variant::Type p_type, const StringName &p_member) {
	ERR_FAIL_INDEX_V(p_type, Variant::VARIANT_MAX, Variant::VARIANT_MAX);

	for (uint32_t i = 0; i < variant_setters_getters_names[p_type].size(); i++) {
		if (variant_setters_getters_names[p_type][i] == p_member) {
			return variant_setters_getters[p_type][i].member_type;
		}
	}

	return Variant::NIL;
}

void Variant::get_member_list(Variant::Type p_type, List<StringName> *r_members) {
	for (uint32_t i = 0; i < variant_setters_getters_names[p_type].size(); i++) {
		r_members->push_back(variant_setters_getters_names[p_type][i]);
	}
}

Variant::ValidatedSetter Variant::get_member_validated_setter(Variant::Type p_type, const StringName &p_member) {
	ERR_FAIL_INDEX_V(p_type, Variant::VARIANT_MAX, nullptr);

	for (uint32_t i = 0; i < variant_setters_getters_names[p_type].size(); i++) {
		if (variant_setters_getters_names[p_type][i] == p_member) {
			return variant_setters_getters[p_type][i].validated_setter;
		}
	}

	return nullptr;
}
Variant::ValidatedGetter Variant::get_member_validated_getter(Variant::Type p_type, const StringName &p_member) {
	ERR_FAIL_INDEX_V(p_type, Variant::VARIANT_MAX, nullptr);

	for (uint32_t i = 0; i < variant_setters_getters_names[p_type].size(); i++) {
		if (variant_setters_getters_names[p_type][i] == p_member) {
			return variant_setters_getters[p_type][i].validated_getter;
		}
	}

	return nullptr;
}

Variant::PTRSetter Variant::get_member_ptr_setter(Variant::Type p_type, const StringName &p_member) {
	ERR_FAIL_INDEX_V(p_type, Variant::VARIANT_MAX, nullptr);

	for (uint32_t i = 0; i < variant_setters_getters_names[p_type].size(); i++) {
		if (variant_setters_getters_names[p_type][i] == p_member) {
			return variant_setters_getters[p_type][i].ptr_setter;
		}
	}

	return nullptr;
}

Variant::PTRGetter Variant::get_member_ptr_getter(Variant::Type p_type, const StringName &p_member) {
	ERR_FAIL_INDEX_V(p_type, Variant::VARIANT_MAX, nullptr);

	for (uint32_t i = 0; i < variant_setters_getters_names[p_type].size(); i++) {
		if (variant_setters_getters_names[p_type][i] == p_member) {
			return variant_setters_getters[p_type][i].ptr_getter;
		}
	}

	return nullptr;
}

void Variant::set_named(const StringName &p_member, const Variant &p_value, bool &r_valid) {
	uint32_t s = variant_setters_getters[type].size();
	if (s) {
		for (uint32_t i = 0; i < s; i++) {
			if (variant_setters_getters_names[type][i] == p_member) {
				variant_setters_getters[type][i].setter(this, &p_value, r_valid);
				return;
			}
		}
		r_valid = false;

	} else if (type == Variant::OBJECT) {
		Object *obj = get_validated_object();
		if (!obj) {
			r_valid = false;
		} else {
			obj->set(p_member, p_value, &r_valid);
			return;
		}
	} else if (type == Variant::DICTIONARY) {
		Variant *v = VariantGetInternalPtr<Dictionary>::get_ptr(this)->getptr(p_member);
		if (v) {
			*v = p_value;
			r_valid = true;
		} else {
			r_valid = false;
		}

	} else {
		r_valid = false;
	}
}

Variant Variant::get_named(const StringName &p_member, bool &r_valid) const {
	Variant ret;
	uint32_t s = variant_setters_getters[type].size();
	if (s) {
		for (uint32_t i = 0; i < s; i++) {
			if (variant_setters_getters_names[type][i] == p_member) {
				variant_setters_getters[type][i].getter(this, &ret);
				r_valid = true;
				return ret;
			}
		}

		r_valid = false;

	} else if (type == Variant::OBJECT) {
		Object *obj = get_validated_object();
		if (!obj) {
			r_valid = false;
			return "Instance base is null.";
		} else {
			return obj->get(p_member, &r_valid);
		}
	} else if (type == Variant::DICTIONARY) {
		const Variant *v = VariantGetInternalPtr<Dictionary>::get_ptr(this)->getptr(p_member);
		if (v) {
			r_valid = true;

			return *v;
		} else {
			r_valid = false;
		}

	} else {
		r_valid = false;
	}

	return ret;
}

/**** INDEXED SETTERS AND GETTERS ****/

#ifdef DEBUG_ENABLED

#define OOB_TEST(m_idx, m_v) \
	ERR_FAIL_INDEX(m_idx, m_v)

#else

#define OOB_TEST(m_idx, m_v)

#endif

#ifdef DEBUG_ENABLED

#define NULL_TEST(m_key) \
	ERR_FAIL_COND(!m_key)

#else

#define NULL_TEST(m_key)

#endif

#define INDEXED_SETGET_STRUCT_TYPED(m_base_type, m_elem_type)                                                                        \
	struct VariantIndexedSetGet_##m_base_type {                                                                                      \
		static void get(const Variant *base, int64_t index, Variant *value, bool &oob) {                                             \
			int64_t size = VariantGetInternalPtr<m_base_type>::get_ptr(base)->size();                                                \
			if (index < 0) {                                                                                                         \
				index += size;                                                                                                       \
			}                                                                                                                        \
			if (index < 0 || index >= size) {                                                                                        \
				oob = true;                                                                                                          \
				return;                                                                                                              \
			}                                                                                                                        \
			VariantTypeAdjust<m_elem_type>::adjust(value);                                                                           \
			*VariantGetInternalPtr<m_elem_type>::get_ptr(value) = (*VariantGetInternalPtr<m_base_type>::get_ptr(base))[index];       \
			oob = false;                                                                                                             \
		}                                                                                                                            \
		static void ptr_get(const void *base, int64_t index, void *member) {                                                         \
			/* avoid ptrconvert for performance*/                                                                                    \
			const m_base_type &v = *reinterpret_cast<const m_base_type *>(base);                                                     \
			if (index < 0)                                                                                                           \
				index += v.size();                                                                                                   \
			OOB_TEST(index, v.size());                                                                                               \
			PtrToArg<m_elem_type>::encode(v[index], member);                                                                         \
		}                                                                                                                            \
		static void set(Variant *base, int64_t index, const Variant *value, bool &valid, bool &oob) {                                \
			if (value->get_type() != GetTypeInfo<m_elem_type>::VARIANT_TYPE) {                                                       \
				oob = false;                                                                                                         \
				valid = false;                                                                                                       \
				return;                                                                                                              \
			}                                                                                                                        \
			int64_t size = VariantGetInternalPtr<m_base_type>::get_ptr(base)->size();                                                \
			if (index < 0) {                                                                                                         \
				index += size;                                                                                                       \
			}                                                                                                                        \
			if (index < 0 || index >= size) {                                                                                        \
				oob = true;                                                                                                          \
				valid = false;                                                                                                       \
				return;                                                                                                              \
			}                                                                                                                        \
			(*VariantGetInternalPtr<m_base_type>::get_ptr(base)).write[index] = *VariantGetInternalPtr<m_elem_type>::get_ptr(value); \
			oob = false;                                                                                                             \
			valid = true;                                                                                                            \
		}                                                                                                                            \
		static void validated_set(Variant *base, int64_t index, const Variant *value, bool &oob) {                                   \
			int64_t size = VariantGetInternalPtr<m_base_type>::get_ptr(base)->size();                                                \
			if (index < 0) {                                                                                                         \
				index += size;                                                                                                       \
			}                                                                                                                        \
			if (index < 0 || index >= size) {                                                                                        \
				oob = true;                                                                                                          \
				return;                                                                                                              \
			}                                                                                                                        \
			(*VariantGetInternalPtr<m_base_type>::get_ptr(base)).write[index] = *VariantGetInternalPtr<m_elem_type>::get_ptr(value); \
			oob = false;                                                                                                             \
		}                                                                                                                            \
		static void ptr_set(void *base, int64_t index, const void *member) {                                                         \
			/* avoid ptrconvert for performance*/                                                                                    \
			m_base_type &v = *reinterpret_cast<m_base_type *>(base);                                                                 \
			if (index < 0)                                                                                                           \
				index += v.size();                                                                                                   \
			OOB_TEST(index, v.size());                                                                                               \
			v.write[index] = PtrToArg<m_elem_type>::convert(member);                                                                 \
		}                                                                                                                            \
		static Variant::Type get_index_type() { return GetTypeInfo<m_elem_type>::VARIANT_TYPE; }                                     \
		static uint64_t get_indexed_size(const Variant *base) { return VariantGetInternalPtr<m_base_type>::get_ptr(base)->size(); }  \
	};

#define INDEXED_SETGET_STRUCT_TYPED_NUMERIC(m_base_type, m_elem_type, m_assign_type)                                                 \
	struct VariantIndexedSetGet_##m_base_type {                                                                                      \
		static void get(const Variant *base, int64_t index, Variant *value, bool &oob) {                                             \
			int64_t size = VariantGetInternalPtr<m_base_type>::get_ptr(base)->size();                                                \
			if (index < 0) {                                                                                                         \
				index += size;                                                                                                       \
			}                                                                                                                        \
			if (index < 0 || index >= size) {                                                                                        \
				oob = true;                                                                                                          \
				return;                                                                                                              \
			}                                                                                                                        \
			VariantTypeAdjust<m_elem_type>::adjust(value);                                                                           \
			*VariantGetInternalPtr<m_elem_type>::get_ptr(value) = (*VariantGetInternalPtr<m_base_type>::get_ptr(base))[index];       \
			oob = false;                                                                                                             \
		}                                                                                                                            \
		static void ptr_get(const void *base, int64_t index, void *member) {                                                         \
			/* avoid ptrconvert for performance*/                                                                                    \
			const m_base_type &v = *reinterpret_cast<const m_base_type *>(base);                                                     \
			if (index < 0)                                                                                                           \
				index += v.size();                                                                                                   \
			OOB_TEST(index, v.size());                                                                                               \
			PtrToArg<m_elem_type>::encode(v[index], member);                                                                         \
		}                                                                                                                            \
		static void set(Variant *base, int64_t index, const Variant *value, bool &valid, bool &oob) {                                \
			int64_t size = VariantGetInternalPtr<m_base_type>::get_ptr(base)->size();                                                \
			if (index < 0) {                                                                                                         \
				index += size;                                                                                                       \
			}                                                                                                                        \
			if (index < 0 || index >= size) {                                                                                        \
				oob = true;                                                                                                          \
				valid = false;                                                                                                       \
				return;                                                                                                              \
			}                                                                                                                        \
			m_assign_type num;                                                                                                       \
			if (value->get_type() == Variant::INT) {                                                                                 \
				num = (m_assign_type)*VariantGetInternalPtr<int64_t>::get_ptr(value);                                                \
			} else if (value->get_type() == Variant::FLOAT) {                                                                        \
				num = (m_assign_type)*VariantGetInternalPtr<double>::get_ptr(value);                                                 \
			} else {                                                                                                                 \
				oob = false;                                                                                                         \
				valid = false;                                                                                                       \
				return;                                                                                                              \
			}                                                                                                                        \
			(*VariantGetInternalPtr<m_base_type>::get_ptr(base)).write[index] = num;                                                 \
			oob = false;                                                                                                             \
			valid = true;                                                                                                            \
		}                                                                                                                            \
		static void validated_set(Variant *base, int64_t index, const Variant *value, bool &oob) {                                   \
			int64_t size = VariantGetInternalPtr<m_base_type>::get_ptr(base)->size();                                                \
			if (index < 0) {                                                                                                         \
				index += size;                                                                                                       \
			}                                                                                                                        \
			if (index < 0 || index >= size) {                                                                                        \
				oob = true;                                                                                                          \
				return;                                                                                                              \
			}                                                                                                                        \
			(*VariantGetInternalPtr<m_base_type>::get_ptr(base)).write[index] = *VariantGetInternalPtr<m_elem_type>::get_ptr(value); \
			oob = false;                                                                                                             \
		}                                                                                                                            \
		static void ptr_set(void *base, int64_t index, const void *member) {                                                         \
			/* avoid ptrconvert for performance*/                                                                                    \
			m_base_type &v = *reinterpret_cast<m_base_type *>(base);                                                                 \
			if (index < 0)                                                                                                           \
				index += v.size();                                                                                                   \
			OOB_TEST(index, v.size());                                                                                               \
			v.write[index] = PtrToArg<m_elem_type>::convert(member);                                                                 \
		}                                                                                                                            \
		static Variant::Type get_index_type() { return GetTypeInfo<m_elem_type>::VARIANT_TYPE; }                                     \
		static uint64_t get_indexed_size(const Variant *base) { return VariantGetInternalPtr<m_base_type>::get_ptr(base)->size(); }  \
	};

#define INDEXED_SETGET_STRUCT_BULTIN_NUMERIC(m_base_type, m_elem_type, m_assign_type, m_max)                                   \
	struct VariantIndexedSetGet_##m_base_type {                                                                                \
		static void get(const Variant *base, int64_t index, Variant *value, bool &oob) {                                       \
			if (index < 0 || index >= m_max) {                                                                                 \
				oob = true;                                                                                                    \
				return;                                                                                                        \
			}                                                                                                                  \
			VariantTypeAdjust<m_elem_type>::adjust(value);                                                                     \
			*VariantGetInternalPtr<m_elem_type>::get_ptr(value) = (*VariantGetInternalPtr<m_base_type>::get_ptr(base))[index]; \
			oob = false;                                                                                                       \
		}                                                                                                                      \
		static void ptr_get(const void *base, int64_t index, void *member) {                                                   \
			/* avoid ptrconvert for performance*/                                                                              \
			const m_base_type &v = *reinterpret_cast<const m_base_type *>(base);                                               \
			OOB_TEST(index, m_max);                                                                                            \
			PtrToArg<m_elem_type>::encode(v[index], member);                                                                   \
		}                                                                                                                      \
		static void set(Variant *base, int64_t index, const Variant *value, bool &valid, bool &oob) {                          \
			if (index < 0 || index >= m_max) {                                                                                 \
				oob = true;                                                                                                    \
				valid = false;                                                                                                 \
				return;                                                                                                        \
			}                                                                                                                  \
			m_assign_type num;                                                                                                 \
			if (value->get_type() == Variant::INT) {                                                                           \
				num = (m_assign_type)*VariantGetInternalPtr<int64_t>::get_ptr(value);                                          \
			} else if (value->get_type() == Variant::FLOAT) {                                                                  \
				num = (m_assign_type)*VariantGetInternalPtr<double>::get_ptr(value);                                           \
			} else {                                                                                                           \
				oob = false;                                                                                                   \
				valid = false;                                                                                                 \
				return;                                                                                                        \
			}                                                                                                                  \
			(*VariantGetInternalPtr<m_base_type>::get_ptr(base))[index] = num;                                                 \
			oob = false;                                                                                                       \
			valid = true;                                                                                                      \
		}                                                                                                                      \
		static void validated_set(Variant *base, int64_t index, const Variant *value, bool &oob) {                             \
			if (index < 0 || index >= m_max) {                                                                                 \
				oob = true;                                                                                                    \
				return;                                                                                                        \
			}                                                                                                                  \
			(*VariantGetInternalPtr<m_base_type>::get_ptr(base))[index] = *VariantGetInternalPtr<m_elem_type>::get_ptr(value); \
			oob = false;                                                                                                       \
		}                                                                                                                      \
		static void ptr_set(void *base, int64_t index, const void *member) {                                                   \
			/* avoid ptrconvert for performance*/                                                                              \
			m_base_type &v = *reinterpret_cast<m_base_type *>(base);                                                           \
			OOB_TEST(index, m_max);                                                                                            \
			v[index] = PtrToArg<m_elem_type>::convert(member);                                                                 \
		}                                                                                                                      \
		static Variant::Type get_index_type() { return GetTypeInfo<m_elem_type>::VARIANT_TYPE; }                               \
		static uint64_t get_indexed_size(const Variant *base) { return m_max; }                                                \
	};

#define INDEXED_SETGET_STRUCT_BULTIN_ACCESSOR(m_base_type, m_elem_type, m_accessor, m_max)                                                \
	struct VariantIndexedSetGet_##m_base_type {                                                                                           \
		static void get(const Variant *base, int64_t index, Variant *value, bool &oob) {                                                  \
			if (index < 0 || index >= m_max) {                                                                                            \
				oob = true;                                                                                                               \
				return;                                                                                                                   \
			}                                                                                                                             \
			VariantTypeAdjust<m_elem_type>::adjust(value);                                                                                \
			*VariantGetInternalPtr<m_elem_type>::get_ptr(value) = (*VariantGetInternalPtr<m_base_type>::get_ptr(base))m_accessor[index];  \
			oob = false;                                                                                                                  \
		}                                                                                                                                 \
		static void ptr_get(const void *base, int64_t index, void *member) {                                                              \
			/* avoid ptrconvert for performance*/                                                                                         \
			const m_base_type &v = *reinterpret_cast<const m_base_type *>(base);                                                          \
			OOB_TEST(index, m_max);                                                                                                       \
			PtrToArg<m_elem_type>::encode(v m_accessor[index], member);                                                                   \
		}                                                                                                                                 \
		static void set(Variant *base, int64_t index, const Variant *value, bool &valid, bool &oob) {                                     \
			if (value->get_type() != GetTypeInfo<m_elem_type>::VARIANT_TYPE) {                                                            \
				oob = false;                                                                                                              \
				valid = false;                                                                                                            \
			}                                                                                                                             \
			if (index < 0 || index >= m_max) {                                                                                            \
				oob = true;                                                                                                               \
				valid = false;                                                                                                            \
				return;                                                                                                                   \
			}                                                                                                                             \
			(*VariantGetInternalPtr<m_base_type>::get_ptr(base)) m_accessor[index] = *VariantGetInternalPtr<m_elem_type>::get_ptr(value); \
			oob = false;                                                                                                                  \
			valid = true;                                                                                                                 \
		}                                                                                                                                 \
		static void validated_set(Variant *base, int64_t index, const Variant *value, bool &oob) {                                        \
			if (index < 0 || index >= m_max) {                                                                                            \
				oob = true;                                                                                                               \
				return;                                                                                                                   \
			}                                                                                                                             \
			(*VariantGetInternalPtr<m_base_type>::get_ptr(base)) m_accessor[index] = *VariantGetInternalPtr<m_elem_type>::get_ptr(value); \
			oob = false;                                                                                                                  \
		}                                                                                                                                 \
		static void ptr_set(void *base, int64_t index, const void *member) {                                                              \
			/* avoid ptrconvert for performance*/                                                                                         \
			m_base_type &v = *reinterpret_cast<m_base_type *>(base);                                                                      \
			OOB_TEST(index, m_max);                                                                                                       \
			v m_accessor[index] = PtrToArg<m_elem_type>::convert(member);                                                                 \
		}                                                                                                                                 \
		static Variant::Type get_index_type() { return GetTypeInfo<m_elem_type>::VARIANT_TYPE; }                                          \
		static uint64_t get_indexed_size(const Variant *base) { return m_max; }                                                           \
	};

#define INDEXED_SETGET_STRUCT_BULTIN_FUNC(m_base_type, m_elem_type, m_set, m_get, m_max)                                           \
	struct VariantIndexedSetGet_##m_base_type {                                                                                    \
		static void get(const Variant *base, int64_t index, Variant *value, bool &oob) {                                           \
			if (index < 0 || index >= m_max) {                                                                                     \
				oob = true;                                                                                                        \
				return;                                                                                                            \
			}                                                                                                                      \
			VariantTypeAdjust<m_elem_type>::adjust(value);                                                                         \
			*VariantGetInternalPtr<m_elem_type>::get_ptr(value) = VariantGetInternalPtr<m_base_type>::get_ptr(base)->m_get(index); \
			oob = false;                                                                                                           \
		}                                                                                                                          \
		static void ptr_get(const void *base, int64_t index, void *member) {                                                       \
			/* avoid ptrconvert for performance*/                                                                                  \
			const m_base_type &v = *reinterpret_cast<const m_base_type *>(base);                                                   \
			OOB_TEST(index, m_max);                                                                                                \
			PtrToArg<m_elem_type>::encode(v.m_get(index), member);                                                                 \
		}                                                                                                                          \
		static void set(Variant *base, int64_t index, const Variant *value, bool &valid, bool &oob) {                              \
			if (value->get_type() != GetTypeInfo<m_elem_type>::VARIANT_TYPE) {                                                     \
				oob = false;                                                                                                       \
				valid = false;                                                                                                     \
			}                                                                                                                      \
			if (index < 0 || index >= m_max) {                                                                                     \
				oob = true;                                                                                                        \
				valid = false;                                                                                                     \
				return;                                                                                                            \
			}                                                                                                                      \
			VariantGetInternalPtr<m_base_type>::get_ptr(base)->m_set(index, *VariantGetInternalPtr<m_elem_type>::get_ptr(value));  \
			oob = false;                                                                                                           \
			valid = true;                                                                                                          \
		}                                                                                                                          \
		static void validated_set(Variant *base, int64_t index, const Variant *value, bool &oob) {                                 \
			if (index < 0 || index >= m_max) {                                                                                     \
				oob = true;                                                                                                        \
				return;                                                                                                            \
			}                                                                                                                      \
			VariantGetInternalPtr<m_base_type>::get_ptr(base)->m_set(index, *VariantGetInternalPtr<m_elem_type>::get_ptr(value));  \
			oob = false;                                                                                                           \
		}                                                                                                                          \
		static void ptr_set(void *base, int64_t index, const void *member) {                                                       \
			/* avoid ptrconvert for performance*/                                                                                  \
			m_base_type &v = *reinterpret_cast<m_base_type *>(base);                                                               \
			OOB_TEST(index, m_max);                                                                                                \
			v.m_set(index, PtrToArg<m_elem_type>::convert(member));                                                                \
		}                                                                                                                          \
		static Variant::Type get_index_type() { return GetTypeInfo<m_elem_type>::VARIANT_TYPE; }                                   \
		static uint64_t get_indexed_size(const Variant *base) { return m_max; }                                                    \
	};

#define INDEXED_SETGET_STRUCT_VARIANT(m_base_type)                                                    \
	struct VariantIndexedSetGet_##m_base_type {                                                       \
		static void get(const Variant *base, int64_t index, Variant *value, bool &oob) {              \
			int64_t size = VariantGetInternalPtr<m_base_type>::get_ptr(base)->size();                 \
			if (index < 0) {                                                                          \
				index += size;                                                                        \
			}                                                                                         \
			if (index < 0 || index >= size) {                                                         \
				oob = true;                                                                           \
				return;                                                                               \
			}                                                                                         \
			*value = (*VariantGetInternalPtr<m_base_type>::get_ptr(base))[index];                     \
			oob = false;                                                                              \
		}                                                                                             \
		static void ptr_get(const void *base, int64_t index, void *member) {                          \
			/* avoid ptrconvert for performance*/                                                     \
			const m_base_type &v = *reinterpret_cast<const m_base_type *>(base);                      \
			if (index < 0)                                                                            \
				index += v.size();                                                                    \
			OOB_TEST(index, v.size());                                                                \
			PtrToArg<Variant>::encode(v[index], member);                                              \
		}                                                                                             \
		static void set(Variant *base, int64_t index, const Variant *value, bool &valid, bool &oob) { \
			int64_t size = VariantGetInternalPtr<m_base_type>::get_ptr(base)->size();                 \
			if (index < 0) {                                                                          \
				index += size;                                                                        \
			}                                                                                         \
			if (index < 0 || index >= size) {                                                         \
				oob = true;                                                                           \
				valid = false;                                                                        \
				return;                                                                               \
			}                                                                                         \
			(*VariantGetInternalPtr<m_base_type>::get_ptr(base))[index] = *value;                     \
			oob = false;                                                                              \
			valid = true;                                                                             \
		}                                                                                             \
		static void validated_set(Variant *base, int64_t index, const Variant *value, bool &oob) {    \
			int64_t size = VariantGetInternalPtr<m_base_type>::get_ptr(base)->size();                 \
			if (index < 0) {                                                                          \
				index += size;                                                                        \
			}                                                                                         \
			if (index < 0 || index >= size) {                                                         \
				oob = true;                                                                           \
				return;                                                                               \
			}                                                                                         \
			(*VariantGetInternalPtr<m_base_type>::get_ptr(base))[index] = *value;                     \
			oob = false;                                                                              \
		}                                                                                             \
		static void ptr_set(void *base, int64_t index, const void *member) {                          \
			/* avoid ptrconvert for performance*/                                                     \
			m_base_type &v = *reinterpret_cast<m_base_type *>(base);                                  \
			if (index < 0)                                                                            \
				index += v.size();                                                                    \
			OOB_TEST(index, v.size());                                                                \
			v[index] = PtrToArg<Variant>::convert(member);                                            \
		}                                                                                             \
		static Variant::Type get_index_type() { return Variant::NIL; }                                \
		static uint64_t get_indexed_size(const Variant *base) { return 0; }                           \
	};

#define INDEXED_SETGET_STRUCT_DICT(m_base_type)                                                                                     \
	struct VariantIndexedSetGet_##m_base_type {                                                                                     \
		static void get(const Variant *base, int64_t index, Variant *value, bool &oob) {                                            \
			const Variant *ptr = VariantGetInternalPtr<m_base_type>::get_ptr(base)->getptr(index);                                  \
			if (!ptr) {                                                                                                             \
				oob = true;                                                                                                         \
				return;                                                                                                             \
			}                                                                                                                       \
			*value = *ptr;                                                                                                          \
			oob = false;                                                                                                            \
		}                                                                                                                           \
		static void ptr_get(const void *base, int64_t index, void *member) {                                                        \
			/* avoid ptrconvert for performance*/                                                                                   \
			const m_base_type &v = *reinterpret_cast<const m_base_type *>(base);                                                    \
			const Variant *ptr = v.getptr(index);                                                                                   \
			NULL_TEST(ptr);                                                                                                         \
			PtrToArg<Variant>::encode(*ptr, member);                                                                                \
		}                                                                                                                           \
		static void set(Variant *base, int64_t index, const Variant *value, bool &valid, bool &oob) {                               \
			(*VariantGetInternalPtr<m_base_type>::get_ptr(base))[index] = *value;                                                   \
			oob = false;                                                                                                            \
			valid = true;                                                                                                           \
		}                                                                                                                           \
		static void validated_set(Variant *base, int64_t index, const Variant *value, bool &oob) {                                  \
			(*VariantGetInternalPtr<m_base_type>::get_ptr(base))[index] = *value;                                                   \
			oob = false;                                                                                                            \
		}                                                                                                                           \
		static void ptr_set(void *base, int64_t index, const void *member) {                                                        \
			m_base_type &v = *reinterpret_cast<m_base_type *>(base);                                                                \
			v[index] = PtrToArg<Variant>::convert(member);                                                                          \
		}                                                                                                                           \
		static Variant::Type get_index_type() { return Variant::NIL; }                                                              \
		static uint64_t get_indexed_size(const Variant *base) { return VariantGetInternalPtr<m_base_type>::get_ptr(base)->size(); } \
	};

INDEXED_SETGET_STRUCT_BULTIN_NUMERIC(Vector2, double, real_t, 2)
INDEXED_SETGET_STRUCT_BULTIN_NUMERIC(Vector2i, int64_t, int32_t, 2)
INDEXED_SETGET_STRUCT_BULTIN_NUMERIC(Vector3, double, real_t, 3)
INDEXED_SETGET_STRUCT_BULTIN_NUMERIC(Vector3i, int64_t, int32_t, 3)
INDEXED_SETGET_STRUCT_BULTIN_NUMERIC(Quat, double, real_t, 4)
INDEXED_SETGET_STRUCT_BULTIN_NUMERIC(Color, double, float, 4)

INDEXED_SETGET_STRUCT_BULTIN_ACCESSOR(Transform2D, Vector2, .elements, 3)
INDEXED_SETGET_STRUCT_BULTIN_FUNC(Basis, Vector3, set_axis, get_axis, 3)

INDEXED_SETGET_STRUCT_TYPED_NUMERIC(PackedByteArray, int64_t, uint8_t)
INDEXED_SETGET_STRUCT_TYPED_NUMERIC(PackedInt32Array, int64_t, int32_t)
INDEXED_SETGET_STRUCT_TYPED_NUMERIC(PackedInt64Array, int64_t, int64_t)
INDEXED_SETGET_STRUCT_TYPED_NUMERIC(PackedFloat32Array, double, float)
INDEXED_SETGET_STRUCT_TYPED_NUMERIC(PackedFloat64Array, double, double)
INDEXED_SETGET_STRUCT_TYPED(PackedVector2Array, Vector2)
INDEXED_SETGET_STRUCT_TYPED(PackedVector3Array, Vector3)
INDEXED_SETGET_STRUCT_TYPED(PackedStringArray, String)
INDEXED_SETGET_STRUCT_TYPED(PackedColorArray, Color)

INDEXED_SETGET_STRUCT_VARIANT(Array)
INDEXED_SETGET_STRUCT_DICT(Dictionary)

struct VariantIndexedSetterGetterInfo {
	void (*setter)(Variant *base, int64_t index, const Variant *value, bool &valid, bool &oob);
	void (*getter)(const Variant *base, int64_t index, Variant *value, bool &oob);

	Variant::ValidatedIndexedSetter validated_setter;
	Variant::ValidatedIndexedGetter validated_getter;

	Variant::PTRIndexedSetter ptr_setter;
	Variant::PTRIndexedGetter ptr_getter;

	uint64_t (*get_indexed_size)(const Variant *base);

	Variant::Type index_type;

	bool valid = false;
};

static VariantIndexedSetterGetterInfo variant_indexed_setters_getters[Variant::VARIANT_MAX];

template <class T>
static void register_indexed_member(Variant::Type p_type) {
	VariantIndexedSetterGetterInfo &sgi = variant_indexed_setters_getters[p_type];

	sgi.setter = T::set;
	sgi.validated_setter = T::validated_set;
	sgi.ptr_setter = T::ptr_set;

	sgi.getter = T::get;
	sgi.validated_getter = T::get;
	sgi.ptr_getter = T::ptr_get;

	sgi.index_type = T::get_index_type();
	sgi.get_indexed_size = T::get_indexed_size;

	sgi.valid = true;
}

void register_indexed_setters_getters() {
#define REGISTER_INDEXED_MEMBER(m_base_type) register_indexed_member<VariantIndexedSetGet_##m_base_type>(GetTypeInfo<m_base_type>::VARIANT_TYPE)

	REGISTER_INDEXED_MEMBER(Vector2);
	REGISTER_INDEXED_MEMBER(Vector2i);
	REGISTER_INDEXED_MEMBER(Vector3);
	REGISTER_INDEXED_MEMBER(Vector3i);
	REGISTER_INDEXED_MEMBER(Quat);
	REGISTER_INDEXED_MEMBER(Color);
	REGISTER_INDEXED_MEMBER(Transform2D);
	REGISTER_INDEXED_MEMBER(Basis);

	REGISTER_INDEXED_MEMBER(PackedByteArray);
	REGISTER_INDEXED_MEMBER(PackedInt32Array);
	REGISTER_INDEXED_MEMBER(PackedInt64Array);
	REGISTER_INDEXED_MEMBER(PackedFloat64Array);
	REGISTER_INDEXED_MEMBER(PackedVector2Array);
	REGISTER_INDEXED_MEMBER(PackedVector3Array);
	REGISTER_INDEXED_MEMBER(PackedStringArray);
	REGISTER_INDEXED_MEMBER(PackedColorArray);

	REGISTER_INDEXED_MEMBER(Array);
	REGISTER_INDEXED_MEMBER(Dictionary);
}

static void unregister_indexed_setters_getters() {
}

bool Variant::has_indexing(Variant::Type p_type) {
	ERR_FAIL_INDEX_V(p_type, Variant::VARIANT_MAX, false);
	return variant_indexed_setters_getters[p_type].valid;
}

Variant::Type Variant::get_indexed_element_type(Variant::Type p_type) {
	ERR_FAIL_INDEX_V(p_type, Variant::VARIANT_MAX, Variant::VARIANT_MAX);
	return variant_indexed_setters_getters[p_type].index_type;
}

Variant::ValidatedIndexedSetter Variant::get_member_validated_indexed_setter(Variant::Type p_type) {
	ERR_FAIL_INDEX_V(p_type, Variant::VARIANT_MAX, nullptr);
	return variant_indexed_setters_getters[p_type].validated_setter;
}
Variant::ValidatedIndexedGetter Variant::get_member_validated_indexed_getter(Variant::Type p_type) {
	ERR_FAIL_INDEX_V(p_type, Variant::VARIANT_MAX, nullptr);
	return variant_indexed_setters_getters[p_type].validated_getter;
}

Variant::PTRIndexedSetter Variant::get_member_ptr_indexed_setter(Variant::Type p_type) {
	ERR_FAIL_INDEX_V(p_type, Variant::VARIANT_MAX, nullptr);
	return variant_indexed_setters_getters[p_type].ptr_setter;
}
Variant::PTRIndexedGetter Variant::get_member_ptr_indexed_getter(Variant::Type p_type) {
	ERR_FAIL_INDEX_V(p_type, Variant::VARIANT_MAX, nullptr);
	return variant_indexed_setters_getters[p_type].ptr_getter;
}

void Variant::set_indexed(int64_t p_index, const Variant &p_value, bool &r_valid, bool &r_oob) {
	if (likely(variant_indexed_setters_getters[type].valid)) {
		variant_indexed_setters_getters[type].setter(this, p_index, &p_value, r_valid, r_oob);
	} else {
		r_valid = false;
		r_oob = false;
	}
}
Variant Variant::get_indexed(int64_t p_index, bool &r_valid, bool &r_oob) const {
	if (likely(variant_indexed_setters_getters[type].valid)) {
		Variant ret;
		variant_indexed_setters_getters[type].getter(this, p_index, &ret, r_oob);
		r_valid = !r_oob;
		return ret;
	} else {
		r_valid = false;
		r_oob = false;
		return Variant();
	}
}

uint64_t Variant::get_indexed_size() const {
	if (likely(variant_indexed_setters_getters[type].valid && variant_indexed_setters_getters[type].get_indexed_size)) {
		return variant_indexed_setters_getters[type].get_indexed_size(this);
	} else {
		return 0;
	}
}

struct VariantKeyedSetGetDictionary {
	static void get(const Variant *base, const Variant *key, Variant *value, bool &r_valid) {
		const Variant *ptr = VariantGetInternalPtr<Dictionary>::get_ptr(base)->getptr(*key);
		if (!ptr) {
			r_valid = false;
			return;
		}
		*value = *ptr;
		r_valid = true;
	}
	static void ptr_get(const void *base, const void *key, void *value) {
		/* avoid ptrconvert for performance*/
		const Dictionary &v = *reinterpret_cast<const Dictionary *>(base);
		const Variant *ptr = v.getptr(PtrToArg<Variant>::convert(key));
		NULL_TEST(ptr);
		PtrToArg<Variant>::encode(*ptr, value);
	}
	static void set(Variant *base, const Variant *key, const Variant *value, bool &r_valid) {
		(*VariantGetInternalPtr<Dictionary>::get_ptr(base))[*key] = *value;
		r_valid = true;
	}
	static void ptr_set(void *base, const void *key, const void *value) {
		Dictionary &v = *reinterpret_cast<Dictionary *>(base);
		v[PtrToArg<Variant>::convert(key)] = PtrToArg<Variant>::convert(value);
	}

	static bool has(const Variant *base, const Variant *key, bool &r_valid) {
		r_valid = true;
		return VariantGetInternalPtr<Dictionary>::get_ptr(base)->has(*key);
	}
	static bool ptr_has(const void *base, const void *key) {
		/* avoid ptrconvert for performance*/
		const Dictionary &v = *reinterpret_cast<const Dictionary *>(base);
		return v.has(PtrToArg<Variant>::convert(key));
	}
};

struct VariantKeyedSetGetObject {
	static void get(const Variant *base, const Variant *key, Variant *value, bool &r_valid) {
		Object *obj = base->get_validated_object();

		if (!obj) {
			r_valid = false;
			*value = Variant();
			return;
		}
		*value = obj->getvar(*key, &r_valid);
	}
	static void ptr_get(const void *base, const void *key, void *value) {
		const Object *obj = PtrToArg<Object *>::convert(base);
		NULL_TEST(obj);
		Variant v = obj->getvar(PtrToArg<Variant>::convert(key));
		PtrToArg<Variant>::encode(v, value);
	}
	static void set(Variant *base, const Variant *key, const Variant *value, bool &r_valid) {
		Object *obj = base->get_validated_object();

		if (!obj) {
			r_valid = false;
			return;
		}
		obj->setvar(*key, *value, &r_valid);
	}
	static void ptr_set(void *base, const void *key, const void *value) {
		Object *obj = PtrToArg<Object *>::convert(base);
		NULL_TEST(obj);
		obj->setvar(PtrToArg<Variant>::convert(key), PtrToArg<Variant>::convert(value));
	}

	static bool has(const Variant *base, const Variant *key, bool &r_valid) {
		Object *obj = base->get_validated_object();
		if (obj != nullptr) {
			r_valid = false;
			return false;
		}
		r_valid = true;
		bool exists;
		obj->getvar(*key, &exists);
		return exists;
	}
	static bool ptr_has(const void *base, const void *key) {
		const Object *obj = PtrToArg<Object *>::convert(base);
		ERR_FAIL_COND_V(!obj, false);
		bool valid;
		obj->getvar(PtrToArg<Variant>::convert(key), &valid);
		return valid;
	}
};

/*typedef void (*ValidatedKeyedSetter)(Variant *base, const Variant *key, const Variant *value);
typedef void (*ValidatedKeyedGetter)(const Variant *base, const Variant *key, Variant *value, bool &valid);
typedef bool (*ValidatedKeyedChecker)(const Variant *base, const Variant *key);

typedef void (*PTRKeyedSetter)(void *base, const void *key, const void *value);
typedef void (*PTRKeyedGetter)(const void *base, const void *key, void *value);
typedef bool (*PTRKeyedChecker)(const void *base, const void *key);*/

struct VariantKeyedSetterGetterInfo {
	Variant::ValidatedKeyedSetter validated_setter;
	Variant::ValidatedKeyedGetter validated_getter;
	Variant::ValidatedKeyedChecker validated_checker;

	Variant::PTRKeyedSetter ptr_setter;
	Variant::PTRKeyedGetter ptr_getter;
	Variant::PTRKeyedChecker ptr_checker;

	bool valid = false;
};

static VariantKeyedSetterGetterInfo variant_keyed_setters_getters[Variant::VARIANT_MAX];

template <class T>
static void register_keyed_member(Variant::Type p_type) {
	VariantKeyedSetterGetterInfo &sgi = variant_keyed_setters_getters[p_type];

	sgi.validated_setter = T::set;
	sgi.ptr_setter = T::ptr_set;

	sgi.validated_getter = T::get;
	sgi.ptr_getter = T::ptr_get;

	sgi.validated_checker = T::has;
	sgi.ptr_checker = T::ptr_has;

	sgi.valid = true;
}

static void register_keyed_setters_getters() {
	register_keyed_member<VariantKeyedSetGetDictionary>(Variant::DICTIONARY);
	register_keyed_member<VariantKeyedSetGetObject>(Variant::OBJECT);
}
bool Variant::is_keyed(Variant::Type p_type) {
	ERR_FAIL_INDEX_V(p_type, VARIANT_MAX, false);
	return variant_keyed_setters_getters[p_type].valid;
}

Variant::ValidatedKeyedSetter Variant::get_member_validated_keyed_setter(Variant::Type p_type) {
	ERR_FAIL_INDEX_V(p_type, VARIANT_MAX, nullptr);
	return variant_keyed_setters_getters[p_type].validated_setter;
}
Variant::ValidatedKeyedGetter Variant::get_member_validated_keyed_getter(Variant::Type p_type) {
	ERR_FAIL_INDEX_V(p_type, VARIANT_MAX, nullptr);
	return variant_keyed_setters_getters[p_type].validated_getter;
}
Variant::ValidatedKeyedChecker Variant::get_member_validated_keyed_checker(Variant::Type p_type) {
	ERR_FAIL_INDEX_V(p_type, VARIANT_MAX, nullptr);
	return variant_keyed_setters_getters[p_type].validated_checker;
}

Variant::PTRKeyedSetter Variant::get_member_ptr_keyed_setter(Variant::Type p_type) {
	ERR_FAIL_INDEX_V(p_type, VARIANT_MAX, nullptr);
	return variant_keyed_setters_getters[p_type].ptr_setter;
}
Variant::PTRKeyedGetter Variant::get_member_ptr_keyed_getter(Variant::Type p_type) {
	ERR_FAIL_INDEX_V(p_type, VARIANT_MAX, nullptr);
	return variant_keyed_setters_getters[p_type].ptr_getter;
}
Variant::PTRKeyedChecker Variant::get_member_ptr_keyed_checker(Variant::Type p_type) {
	ERR_FAIL_INDEX_V(p_type, VARIANT_MAX, nullptr);
	return variant_keyed_setters_getters[p_type].ptr_checker;
}

void Variant::set_keyed(const Variant &p_key, const Variant &p_value, bool &r_valid) {
	if (likely(variant_keyed_setters_getters[type].valid)) {
		variant_keyed_setters_getters[type].validated_setter(this, &p_key, &p_value, r_valid);
	} else {
		r_valid = false;
	}
}
Variant Variant::get_keyed(const Variant &p_key, bool &r_valid) const {
	if (likely(variant_keyed_setters_getters[type].valid)) {
		Variant ret;
		variant_keyed_setters_getters[type].validated_getter(this, &p_key, &ret, r_valid);
		return ret;
	} else {
		r_valid = false;
		return Variant();
	}
}
bool Variant::has_key(const Variant &p_key, bool &r_valid) const {
	if (likely(variant_keyed_setters_getters[type].valid)) {
		return variant_keyed_setters_getters[type].validated_checker(this, &p_key, r_valid);
	} else {
		r_valid = false;
		return false;
	}
}

void Variant::set(const Variant &p_index, const Variant &p_value, bool *r_valid) {
	if (type == DICTIONARY || type == OBJECT) {
		bool valid;
		set_keyed(p_index, p_value, valid);
		if (r_valid) {
			*r_valid = valid;
		}
	} else {
		bool valid = false;
		if (p_index.get_type() == STRING_NAME) {
			set_named(*VariantGetInternalPtr<StringName>::get_ptr(&p_index), p_value, valid);
		} else if (p_index.get_type() == INT) {
			bool obb;
			set_indexed(*VariantGetInternalPtr<int64_t>::get_ptr(&p_index), p_value, valid, obb);
			if (obb) {
				valid = false;
			}
		} else if (p_index.get_type() == STRING) { // less efficient version of named
			set_named(*VariantGetInternalPtr<String>::get_ptr(&p_index), p_value, valid);
		} else if (p_index.get_type() == FLOAT) { // less efficient version of indexed
			bool obb;
			set_indexed(*VariantGetInternalPtr<double>::get_ptr(&p_index), p_value, valid, obb);
			if (obb) {
				valid = false;
			}
		}
		if (r_valid) {
			*r_valid = valid;
		}
	}
}

Variant Variant::get(const Variant &p_index, bool *r_valid) const {
	Variant ret;
	if (type == DICTIONARY || type == OBJECT) {
		bool valid;
		ret = get_keyed(p_index, valid);
		if (r_valid) {
			*r_valid = valid;
		}
	} else {
		bool valid = false;
		if (p_index.get_type() == STRING_NAME) {
			ret = get_named(*VariantGetInternalPtr<StringName>::get_ptr(&p_index), valid);
		} else if (p_index.get_type() == INT) {
			bool obb;
			ret = get_indexed(*VariantGetInternalPtr<int64_t>::get_ptr(&p_index), valid, obb);
			if (obb) {
				valid = false;
			}
		} else if (p_index.get_type() == STRING) { // less efficient version of named
			ret = get_named(*VariantGetInternalPtr<String>::get_ptr(&p_index), valid);
		} else if (p_index.get_type() == FLOAT) { // less efficient version of indexed
			bool obb;
			ret = get_indexed(*VariantGetInternalPtr<double>::get_ptr(&p_index), valid, obb);
			if (obb) {
				valid = false;
			}
		}
		if (r_valid) {
			*r_valid = valid;
		}
	}

	return ret;
}

void Variant::get_property_list(List<PropertyInfo> *p_list) const {
	if (type == DICTIONARY) {
		const Dictionary *dic = reinterpret_cast<const Dictionary *>(_data._mem);
		List<Variant> keys;
		dic->get_key_list(&keys);
		for (List<Variant>::Element *E = keys.front(); E; E = E->next()) {
			if (E->get().get_type() == Variant::STRING) {
				p_list->push_back(PropertyInfo(Variant::STRING, E->get()));
			}
		}
	} else if (type == OBJECT) {
		Object *obj = get_validated_object();
		ERR_FAIL_COND(!obj);
		obj->get_property_list(p_list);

	} else {
		List<StringName> members;
		get_member_list(type, &members);
		for (List<StringName>::Element *E = members.front(); E; E = E->next()) {
			PropertyInfo pi;
			pi.name = E->get();
			pi.type = get_member_type(type, E->get());
			p_list->push_back(pi);
		}
	}
}

bool Variant::iter_init(Variant &r_iter, bool &valid) const {
	valid = true;
	switch (type) {
		case INT: {
			r_iter = 0;
			return _data._int > 0;
		} break;
		case FLOAT: {
			r_iter = 0;
			return _data._float > 0.0;
		} break;
		case VECTOR2: {
			double from = reinterpret_cast<const Vector2 *>(_data._mem)->x;
			double to = reinterpret_cast<const Vector2 *>(_data._mem)->y;

			r_iter = from;

			return from < to;
		} break;
		case VECTOR2I: {
			int64_t from = reinterpret_cast<const Vector2i *>(_data._mem)->x;
			int64_t to = reinterpret_cast<const Vector2i *>(_data._mem)->y;

			r_iter = from;

			return from < to;
		} break;
		case VECTOR3: {
			double from = reinterpret_cast<const Vector3 *>(_data._mem)->x;
			double to = reinterpret_cast<const Vector3 *>(_data._mem)->y;
			double step = reinterpret_cast<const Vector3 *>(_data._mem)->z;

			r_iter = from;

			if (from == to) {
				return false;
			} else if (from < to) {
				return step > 0;
			}
			return step < 0;
		} break;
		case VECTOR3I: {
			int64_t from = reinterpret_cast<const Vector3i *>(_data._mem)->x;
			int64_t to = reinterpret_cast<const Vector3i *>(_data._mem)->y;
			int64_t step = reinterpret_cast<const Vector3i *>(_data._mem)->z;

			r_iter = from;

			if (from == to) {
				return false;
			} else if (from < to) {
				return step > 0;
			}
			return step < 0;
		} break;
		case OBJECT: {
			if (!_get_obj().obj) {
				valid = false;
				return false;
			}

#ifdef DEBUG_ENABLED

			if (EngineDebugger::is_active() && !_get_obj().id.is_reference() && ObjectDB::get_instance(_get_obj().id) == nullptr) {
				valid = false;
				return false;
			}

#endif
			Callable::CallError ce;
			ce.error = Callable::CallError::CALL_OK;
			Array ref;
			ref.push_back(r_iter);
			Variant vref = ref;
			const Variant *refp[] = { &vref };
			Variant ret = _get_obj().obj->call(CoreStringNames::get_singleton()->_iter_init, refp, 1, ce);

			if (ref.size() != 1 || ce.error != Callable::CallError::CALL_OK) {
				valid = false;
				return false;
			}

			r_iter = ref[0];
			return ret;
		} break;

		case STRING: {
			const String *str = reinterpret_cast<const String *>(_data._mem);
			if (str->empty()) {
				return false;
			}
			r_iter = 0;
			return true;
		} break;
		case DICTIONARY: {
			const Dictionary *dic = reinterpret_cast<const Dictionary *>(_data._mem);
			if (dic->empty()) {
				return false;
			}

			const Variant *next = dic->next(nullptr);
			r_iter = *next;
			return true;

		} break;
		case ARRAY: {
			const Array *arr = reinterpret_cast<const Array *>(_data._mem);
			if (arr->empty()) {
				return false;
			}
			r_iter = 0;
			return true;
		} break;
		case PACKED_BYTE_ARRAY: {
			const Vector<uint8_t> *arr = &PackedArrayRef<uint8_t>::get_array(_data.packed_array);
			if (arr->size() == 0) {
				return false;
			}
			r_iter = 0;
			return true;

		} break;
		case PACKED_INT32_ARRAY: {
			const Vector<int32_t> *arr = &PackedArrayRef<int32_t>::get_array(_data.packed_array);
			if (arr->size() == 0) {
				return false;
			}
			r_iter = 0;
			return true;

		} break;
		case PACKED_INT64_ARRAY: {
			const Vector<int64_t> *arr = &PackedArrayRef<int64_t>::get_array(_data.packed_array);
			if (arr->size() == 0) {
				return false;
			}
			r_iter = 0;
			return true;

		} break;
		case PACKED_FLOAT32_ARRAY: {
			const Vector<float> *arr = &PackedArrayRef<float>::get_array(_data.packed_array);
			if (arr->size() == 0) {
				return false;
			}
			r_iter = 0;
			return true;

		} break;
		case PACKED_FLOAT64_ARRAY: {
			const Vector<double> *arr = &PackedArrayRef<double>::get_array(_data.packed_array);
			if (arr->size() == 0) {
				return false;
			}
			r_iter = 0;
			return true;

		} break;
		case PACKED_STRING_ARRAY: {
			const Vector<String> *arr = &PackedArrayRef<String>::get_array(_data.packed_array);
			if (arr->size() == 0) {
				return false;
			}
			r_iter = 0;
			return true;
		} break;
		case PACKED_VECTOR2_ARRAY: {
			const Vector<Vector2> *arr = &PackedArrayRef<Vector2>::get_array(_data.packed_array);
			if (arr->size() == 0) {
				return false;
			}
			r_iter = 0;
			return true;
		} break;
		case PACKED_VECTOR3_ARRAY: {
			const Vector<Vector3> *arr = &PackedArrayRef<Vector3>::get_array(_data.packed_array);
			if (arr->size() == 0) {
				return false;
			}
			r_iter = 0;
			return true;
		} break;
		case PACKED_COLOR_ARRAY: {
			const Vector<Color> *arr = &PackedArrayRef<Color>::get_array(_data.packed_array);
			if (arr->size() == 0) {
				return false;
			}
			r_iter = 0;
			return true;

		} break;
		default: {
		}
	}

	valid = false;
	return false;
}

bool Variant::iter_next(Variant &r_iter, bool &valid) const {
	valid = true;
	switch (type) {
		case INT: {
			int64_t idx = r_iter;
			idx++;
			if (idx >= _data._int) {
				return false;
			}
			r_iter = idx;
			return true;
		} break;
		case FLOAT: {
			int64_t idx = r_iter;
			idx++;
			if (idx >= _data._float) {
				return false;
			}
			r_iter = idx;
			return true;
		} break;
		case VECTOR2: {
			double to = reinterpret_cast<const Vector2 *>(_data._mem)->y;

			double idx = r_iter;
			idx++;

			if (idx >= to) {
				return false;
			}

			r_iter = idx;
			return true;
		} break;
		case VECTOR2I: {
			int64_t to = reinterpret_cast<const Vector2i *>(_data._mem)->y;

			int64_t idx = r_iter;
			idx++;

			if (idx >= to) {
				return false;
			}

			r_iter = idx;
			return true;
		} break;
		case VECTOR3: {
			double to = reinterpret_cast<const Vector3 *>(_data._mem)->y;
			double step = reinterpret_cast<const Vector3 *>(_data._mem)->z;

			double idx = r_iter;
			idx += step;

			if (step < 0 && idx <= to) {
				return false;
			}

			if (step > 0 && idx >= to) {
				return false;
			}

			r_iter = idx;
			return true;
		} break;
		case VECTOR3I: {
			int64_t to = reinterpret_cast<const Vector3i *>(_data._mem)->y;
			int64_t step = reinterpret_cast<const Vector3i *>(_data._mem)->z;

			int64_t idx = r_iter;
			idx += step;

			if (step < 0 && idx <= to) {
				return false;
			}

			if (step > 0 && idx >= to) {
				return false;
			}

			r_iter = idx;
			return true;
		} break;
		case OBJECT: {
			if (!_get_obj().obj) {
				valid = false;
				return false;
			}

#ifdef DEBUG_ENABLED

			if (EngineDebugger::is_active() && !_get_obj().id.is_reference() && ObjectDB::get_instance(_get_obj().id) == nullptr) {
				valid = false;
				return false;
			}

#endif
			Callable::CallError ce;
			ce.error = Callable::CallError::CALL_OK;
			Array ref;
			ref.push_back(r_iter);
			Variant vref = ref;
			const Variant *refp[] = { &vref };
			Variant ret = _get_obj().obj->call(CoreStringNames::get_singleton()->_iter_next, refp, 1, ce);

			if (ref.size() != 1 || ce.error != Callable::CallError::CALL_OK) {
				valid = false;
				return false;
			}

			r_iter = ref[0];

			return ret;
		} break;

		case STRING: {
			const String *str = reinterpret_cast<const String *>(_data._mem);
			int idx = r_iter;
			idx++;
			if (idx >= str->length()) {
				return false;
			}
			r_iter = idx;
			return true;
		} break;
		case DICTIONARY: {
			const Dictionary *dic = reinterpret_cast<const Dictionary *>(_data._mem);
			const Variant *next = dic->next(&r_iter);
			if (!next) {
				return false;
			}

			r_iter = *next;
			return true;

		} break;
		case ARRAY: {
			const Array *arr = reinterpret_cast<const Array *>(_data._mem);
			int idx = r_iter;
			idx++;
			if (idx >= arr->size()) {
				return false;
			}
			r_iter = idx;
			return true;
		} break;
		case PACKED_BYTE_ARRAY: {
			const Vector<uint8_t> *arr = &PackedArrayRef<uint8_t>::get_array(_data.packed_array);
			int idx = r_iter;
			idx++;
			if (idx >= arr->size()) {
				return false;
			}
			r_iter = idx;
			return true;

		} break;
		case PACKED_INT32_ARRAY: {
			const Vector<int32_t> *arr = &PackedArrayRef<int32_t>::get_array(_data.packed_array);
			int32_t idx = r_iter;
			idx++;
			if (idx >= arr->size()) {
				return false;
			}
			r_iter = idx;
			return true;

		} break;
		case PACKED_INT64_ARRAY: {
			const Vector<int64_t> *arr = &PackedArrayRef<int64_t>::get_array(_data.packed_array);
			int64_t idx = r_iter;
			idx++;
			if (idx >= arr->size()) {
				return false;
			}
			r_iter = idx;
			return true;

		} break;
		case PACKED_FLOAT32_ARRAY: {
			const Vector<float> *arr = &PackedArrayRef<float>::get_array(_data.packed_array);
			int idx = r_iter;
			idx++;
			if (idx >= arr->size()) {
				return false;
			}
			r_iter = idx;
			return true;

		} break;
		case PACKED_FLOAT64_ARRAY: {
			const Vector<double> *arr = &PackedArrayRef<double>::get_array(_data.packed_array);
			int idx = r_iter;
			idx++;
			if (idx >= arr->size()) {
				return false;
			}
			r_iter = idx;
			return true;

		} break;
		case PACKED_STRING_ARRAY: {
			const Vector<String> *arr = &PackedArrayRef<String>::get_array(_data.packed_array);
			int idx = r_iter;
			idx++;
			if (idx >= arr->size()) {
				return false;
			}
			r_iter = idx;
			return true;
		} break;
		case PACKED_VECTOR2_ARRAY: {
			const Vector<Vector2> *arr = &PackedArrayRef<Vector2>::get_array(_data.packed_array);
			int idx = r_iter;
			idx++;
			if (idx >= arr->size()) {
				return false;
			}
			r_iter = idx;
			return true;
		} break;
		case PACKED_VECTOR3_ARRAY: {
			const Vector<Vector3> *arr = &PackedArrayRef<Vector3>::get_array(_data.packed_array);
			int idx = r_iter;
			idx++;
			if (idx >= arr->size()) {
				return false;
			}
			r_iter = idx;
			return true;
		} break;
		case PACKED_COLOR_ARRAY: {
			const Vector<Color> *arr = &PackedArrayRef<Color>::get_array(_data.packed_array);
			int idx = r_iter;
			idx++;
			if (idx >= arr->size()) {
				return false;
			}
			r_iter = idx;
			return true;
		} break;
		default: {
		}
	}

	valid = false;
	return false;
}

Variant Variant::iter_get(const Variant &r_iter, bool &r_valid) const {
	r_valid = true;
	switch (type) {
		case INT: {
			return r_iter;
		} break;
		case FLOAT: {
			return r_iter;
		} break;
		case VECTOR2: {
			return r_iter;
		} break;
		case VECTOR2I: {
			return r_iter;
		} break;
		case VECTOR3: {
			return r_iter;
		} break;
		case VECTOR3I: {
			return r_iter;
		} break;
		case OBJECT: {
			if (!_get_obj().obj) {
				r_valid = false;
				return Variant();
			}
#ifdef DEBUG_ENABLED
			if (EngineDebugger::is_active() && !_get_obj().id.is_reference() && ObjectDB::get_instance(_get_obj().id) == nullptr) {
				r_valid = false;
				return Variant();
			}

#endif
			Callable::CallError ce;
			ce.error = Callable::CallError::CALL_OK;
			const Variant *refp[] = { &r_iter };
			Variant ret = _get_obj().obj->call(CoreStringNames::get_singleton()->_iter_get, refp, 1, ce);

			if (ce.error != Callable::CallError::CALL_OK) {
				r_valid = false;
				return Variant();
			}

			//r_iter=ref[0];

			return ret;
		} break;

		case STRING: {
			const String *str = reinterpret_cast<const String *>(_data._mem);
			return str->substr(r_iter, 1);
		} break;
		case DICTIONARY: {
			return r_iter; //iterator is the same as the key

		} break;
		case ARRAY: {
			const Array *arr = reinterpret_cast<const Array *>(_data._mem);
			int idx = r_iter;
#ifdef DEBUG_ENABLED
			if (idx < 0 || idx >= arr->size()) {
				r_valid = false;
				return Variant();
			}
#endif
			return arr->get(idx);
		} break;
		case PACKED_BYTE_ARRAY: {
			const Vector<uint8_t> *arr = &PackedArrayRef<uint8_t>::get_array(_data.packed_array);
			int idx = r_iter;
#ifdef DEBUG_ENABLED
			if (idx < 0 || idx >= arr->size()) {
				r_valid = false;
				return Variant();
			}
#endif
			return arr->get(idx);
		} break;
		case PACKED_INT32_ARRAY: {
			const Vector<int32_t> *arr = &PackedArrayRef<int32_t>::get_array(_data.packed_array);
			int32_t idx = r_iter;
#ifdef DEBUG_ENABLED
			if (idx < 0 || idx >= arr->size()) {
				r_valid = false;
				return Variant();
			}
#endif
			return arr->get(idx);
		} break;
		case PACKED_INT64_ARRAY: {
			const Vector<int64_t> *arr = &PackedArrayRef<int64_t>::get_array(_data.packed_array);
			int64_t idx = r_iter;
#ifdef DEBUG_ENABLED
			if (idx < 0 || idx >= arr->size()) {
				r_valid = false;
				return Variant();
			}
#endif
			return arr->get(idx);
		} break;
		case PACKED_FLOAT32_ARRAY: {
			const Vector<float> *arr = &PackedArrayRef<float>::get_array(_data.packed_array);
			int idx = r_iter;
#ifdef DEBUG_ENABLED
			if (idx < 0 || idx >= arr->size()) {
				r_valid = false;
				return Variant();
			}
#endif
			return arr->get(idx);
		} break;
		case PACKED_FLOAT64_ARRAY: {
			const Vector<double> *arr = &PackedArrayRef<double>::get_array(_data.packed_array);
			int idx = r_iter;
#ifdef DEBUG_ENABLED
			if (idx < 0 || idx >= arr->size()) {
				r_valid = false;
				return Variant();
			}
#endif
			return arr->get(idx);
		} break;
		case PACKED_STRING_ARRAY: {
			const Vector<String> *arr = &PackedArrayRef<String>::get_array(_data.packed_array);
			int idx = r_iter;
#ifdef DEBUG_ENABLED
			if (idx < 0 || idx >= arr->size()) {
				r_valid = false;
				return Variant();
			}
#endif
			return arr->get(idx);
		} break;
		case PACKED_VECTOR2_ARRAY: {
			const Vector<Vector2> *arr = &PackedArrayRef<Vector2>::get_array(_data.packed_array);
			int idx = r_iter;
#ifdef DEBUG_ENABLED
			if (idx < 0 || idx >= arr->size()) {
				r_valid = false;
				return Variant();
			}
#endif
			return arr->get(idx);
		} break;
		case PACKED_VECTOR3_ARRAY: {
			const Vector<Vector3> *arr = &PackedArrayRef<Vector3>::get_array(_data.packed_array);
			int idx = r_iter;
#ifdef DEBUG_ENABLED
			if (idx < 0 || idx >= arr->size()) {
				r_valid = false;
				return Variant();
			}
#endif
			return arr->get(idx);
		} break;
		case PACKED_COLOR_ARRAY: {
			const Vector<Color> *arr = &PackedArrayRef<Color>::get_array(_data.packed_array);
			int idx = r_iter;
#ifdef DEBUG_ENABLED
			if (idx < 0 || idx >= arr->size()) {
				r_valid = false;
				return Variant();
			}
#endif
			return arr->get(idx);
		} break;
		default: {
		}
	}

	r_valid = false;
	return Variant();
}

Variant Variant::duplicate(bool deep) const {
	switch (type) {
		case OBJECT: {
			/*  breaks stuff :(
			if (deep && !_get_obj().ref.is_null()) {
				Ref<Resource> resource = _get_obj().ref;
				if (resource.is_valid()) {
					return resource->duplicate(true);
				}
			}
			*/
			return *this;
		} break;
		case DICTIONARY:
			return operator Dictionary().duplicate(deep);
		case ARRAY:
			return operator Array().duplicate(deep);
		default:
			return *this;
	}
}

void Variant::blend(const Variant &a, const Variant &b, float c, Variant &r_dst) {
	if (a.type != b.type) {
		if (a.is_num() && b.is_num()) {
			real_t va = a;
			real_t vb = b;
			r_dst = va + vb * c;
		} else {
			r_dst = a;
		}
		return;
	}

	switch (a.type) {
		case NIL: {
			r_dst = Variant();
		}
			return;
		case INT: {
			int64_t va = a._data._int;
			int64_t vb = b._data._int;
			r_dst = int(va + vb * c + 0.5);
		}
			return;
		case FLOAT: {
			double ra = a._data._float;
			double rb = b._data._float;
			r_dst = ra + rb * c;
		}
			return;
		case VECTOR2: {
			r_dst = *reinterpret_cast<const Vector2 *>(a._data._mem) + *reinterpret_cast<const Vector2 *>(b._data._mem) * c;
		}
			return;
		case VECTOR2I: {
			int32_t vax = reinterpret_cast<const Vector2i *>(a._data._mem)->x;
			int32_t vbx = reinterpret_cast<const Vector2i *>(b._data._mem)->x;
			int32_t vay = reinterpret_cast<const Vector2i *>(a._data._mem)->y;
			int32_t vby = reinterpret_cast<const Vector2i *>(b._data._mem)->y;
			r_dst = Vector2i(int32_t(vax + vbx * c + 0.5), int32_t(vay + vby * c + 0.5));
		}
			return;
		case RECT2: {
			const Rect2 *ra = reinterpret_cast<const Rect2 *>(a._data._mem);
			const Rect2 *rb = reinterpret_cast<const Rect2 *>(b._data._mem);
			r_dst = Rect2(ra->position + rb->position * c, ra->size + rb->size * c);
		}
			return;
		case RECT2I: {
			const Rect2i *ra = reinterpret_cast<const Rect2i *>(a._data._mem);
			const Rect2i *rb = reinterpret_cast<const Rect2i *>(b._data._mem);

			int32_t vax = ra->position.x;
			int32_t vay = ra->position.y;
			int32_t vbx = ra->size.x;
			int32_t vby = ra->size.y;
			int32_t vcx = rb->position.x;
			int32_t vcy = rb->position.y;
			int32_t vdx = rb->size.x;
			int32_t vdy = rb->size.y;

			r_dst = Rect2i(int32_t(vax + vbx * c + 0.5), int32_t(vay + vby * c + 0.5), int32_t(vcx + vdx * c + 0.5), int32_t(vcy + vdy * c + 0.5));
		}
			return;
		case VECTOR3: {
			r_dst = *reinterpret_cast<const Vector3 *>(a._data._mem) + *reinterpret_cast<const Vector3 *>(b._data._mem) * c;
		}
			return;
		case VECTOR3I: {
			int32_t vax = reinterpret_cast<const Vector3i *>(a._data._mem)->x;
			int32_t vbx = reinterpret_cast<const Vector3i *>(b._data._mem)->x;
			int32_t vay = reinterpret_cast<const Vector3i *>(a._data._mem)->y;
			int32_t vby = reinterpret_cast<const Vector3i *>(b._data._mem)->y;
			int32_t vaz = reinterpret_cast<const Vector3i *>(a._data._mem)->z;
			int32_t vbz = reinterpret_cast<const Vector3i *>(b._data._mem)->z;
			r_dst = Vector3i(int32_t(vax + vbx * c + 0.5), int32_t(vay + vby * c + 0.5), int32_t(vaz + vbz * c + 0.5));
		}
			return;
		case AABB: {
			const ::AABB *ra = reinterpret_cast<const ::AABB *>(a._data._mem);
			const ::AABB *rb = reinterpret_cast<const ::AABB *>(b._data._mem);
			r_dst = ::AABB(ra->position + rb->position * c, ra->size + rb->size * c);
		}
			return;
		case QUAT: {
			Quat empty_rot;
			const Quat *qa = reinterpret_cast<const Quat *>(a._data._mem);
			const Quat *qb = reinterpret_cast<const Quat *>(b._data._mem);
			r_dst = *qa * empty_rot.slerp(*qb, c);
		}
			return;
		case COLOR: {
			const Color *ca = reinterpret_cast<const Color *>(a._data._mem);
			const Color *cb = reinterpret_cast<const Color *>(b._data._mem);
			float new_r = ca->r + cb->r * c;
			float new_g = ca->g + cb->g * c;
			float new_b = ca->b + cb->b * c;
			float new_a = ca->a + cb->a * c;
			new_r = new_r > 1.0 ? 1.0 : new_r;
			new_g = new_g > 1.0 ? 1.0 : new_g;
			new_b = new_b > 1.0 ? 1.0 : new_b;
			new_a = new_a > 1.0 ? 1.0 : new_a;
			r_dst = Color(new_r, new_g, new_b, new_a);
		}
			return;
		default: {
			r_dst = c < 0.5 ? a : b;
		}
			return;
	}
}

void Variant::interpolate(const Variant &a, const Variant &b, float c, Variant &r_dst) {
	if (a.type != b.type) {
		if (a.is_num() && b.is_num()) {
			//not as efficient but..
			real_t va = a;
			real_t vb = b;
			r_dst = va + (vb - va) * c;

		} else {
			r_dst = a;
		}
		return;
	}

	switch (a.type) {
		case NIL: {
			r_dst = Variant();
		}
			return;
		case BOOL: {
			r_dst = a;
		}
			return;
		case INT: {
			int64_t va = a._data._int;
			int64_t vb = b._data._int;
			r_dst = int(va + (vb - va) * c);
		}
			return;
		case FLOAT: {
			real_t va = a._data._float;
			real_t vb = b._data._float;
			r_dst = va + (vb - va) * c;
		}
			return;
		case STRING: {
			//this is pretty funny and bizarre, but artists like to use it for typewritter effects
			String sa = *reinterpret_cast<const String *>(a._data._mem);
			String sb = *reinterpret_cast<const String *>(b._data._mem);
			String dst;
			int sa_len = sa.length();
			int sb_len = sb.length();
			int csize = sa_len + (sb_len - sa_len) * c;
			if (csize == 0) {
				r_dst = "";
				return;
			}
			dst.resize(csize + 1);
			dst[csize] = 0;
			int split = csize / 2;

			for (int i = 0; i < csize; i++) {
				char32_t chr = ' ';

				if (i < split) {
					if (i < sa.length()) {
						chr = sa[i];
					} else if (i < sb.length()) {
						chr = sb[i];
					}

				} else {
					if (i < sb.length()) {
						chr = sb[i];
					} else if (i < sa.length()) {
						chr = sa[i];
					}
				}

				dst[i] = chr;
			}

			r_dst = dst;
		}
			return;
		case VECTOR2: {
			r_dst = reinterpret_cast<const Vector2 *>(a._data._mem)->lerp(*reinterpret_cast<const Vector2 *>(b._data._mem), c);
		}
			return;
		case VECTOR2I: {
			int32_t vax = reinterpret_cast<const Vector2i *>(a._data._mem)->x;
			int32_t vbx = reinterpret_cast<const Vector2i *>(b._data._mem)->x;
			int32_t vay = reinterpret_cast<const Vector2i *>(a._data._mem)->y;
			int32_t vby = reinterpret_cast<const Vector2i *>(b._data._mem)->y;
			r_dst = Vector2i(int32_t(vax + vbx * c + 0.5), int32_t(vay + vby * c + 0.5));
		}
			return;

		case RECT2: {
			r_dst = Rect2(reinterpret_cast<const Rect2 *>(a._data._mem)->position.lerp(reinterpret_cast<const Rect2 *>(b._data._mem)->position, c), reinterpret_cast<const Rect2 *>(a._data._mem)->size.lerp(reinterpret_cast<const Rect2 *>(b._data._mem)->size, c));
		}
			return;
		case RECT2I: {
			const Rect2i *ra = reinterpret_cast<const Rect2i *>(a._data._mem);
			const Rect2i *rb = reinterpret_cast<const Rect2i *>(b._data._mem);

			int32_t vax = ra->position.x;
			int32_t vay = ra->position.y;
			int32_t vbx = ra->size.x;
			int32_t vby = ra->size.y;
			int32_t vcx = rb->position.x;
			int32_t vcy = rb->position.y;
			int32_t vdx = rb->size.x;
			int32_t vdy = rb->size.y;

			r_dst = Rect2i(int32_t(vax + vbx * c + 0.5), int32_t(vay + vby * c + 0.5), int32_t(vcx + vdx * c + 0.5), int32_t(vcy + vdy * c + 0.5));
		}
			return;

		case VECTOR3: {
			r_dst = reinterpret_cast<const Vector3 *>(a._data._mem)->lerp(*reinterpret_cast<const Vector3 *>(b._data._mem), c);
		}
			return;
		case VECTOR3I: {
			int32_t vax = reinterpret_cast<const Vector3i *>(a._data._mem)->x;
			int32_t vbx = reinterpret_cast<const Vector3i *>(b._data._mem)->x;
			int32_t vay = reinterpret_cast<const Vector3i *>(a._data._mem)->y;
			int32_t vby = reinterpret_cast<const Vector3i *>(b._data._mem)->y;
			int32_t vaz = reinterpret_cast<const Vector3i *>(a._data._mem)->z;
			int32_t vbz = reinterpret_cast<const Vector3i *>(b._data._mem)->z;
			r_dst = Vector3i(int32_t(vax + vbx * c + 0.5), int32_t(vay + vby * c + 0.5), int32_t(vaz + vbz * c + 0.5));
		}
			return;

		case TRANSFORM2D: {
			r_dst = a._data._transform2d->interpolate_with(*b._data._transform2d, c);
		}
			return;
		case PLANE: {
			r_dst = a;
		}
			return;
		case QUAT: {
			r_dst = reinterpret_cast<const Quat *>(a._data._mem)->slerp(*reinterpret_cast<const Quat *>(b._data._mem), c);
		}
			return;
		case AABB: {
			r_dst = ::AABB(a._data._aabb->position.lerp(b._data._aabb->position, c), a._data._aabb->size.lerp(b._data._aabb->size, c));
		}
			return;
		case BASIS: {
			r_dst = Transform(*a._data._basis).interpolate_with(Transform(*b._data._basis), c).basis;
		}
			return;
		case TRANSFORM: {
			r_dst = a._data._transform->interpolate_with(*b._data._transform, c);
		}
			return;
		case COLOR: {
			r_dst = reinterpret_cast<const Color *>(a._data._mem)->lerp(*reinterpret_cast<const Color *>(b._data._mem), c);
		}
			return;
		case STRING_NAME: {
			r_dst = a;
		}
			return;
		case NODE_PATH: {
			r_dst = a;
		}
			return;
		case RID: {
			r_dst = a;
		}
			return;
		case OBJECT: {
			r_dst = a;
		}
			return;
		case DICTIONARY: {
		}
			return;
		case ARRAY: {
			r_dst = a;
		}
			return;
		case PACKED_BYTE_ARRAY: {
			r_dst = a;
		}
			return;
		case PACKED_INT32_ARRAY: {
			const Vector<int32_t> *arr_a = &PackedArrayRef<int32_t>::get_array(a._data.packed_array);
			const Vector<int32_t> *arr_b = &PackedArrayRef<int32_t>::get_array(b._data.packed_array);
			int32_t sz = arr_a->size();
			if (sz == 0 || arr_b->size() != sz) {
				r_dst = a;
			} else {
				Vector<int32_t> v;
				v.resize(sz);
				{
					int32_t *vw = v.ptrw();
					const int32_t *ar = arr_a->ptr();
					const int32_t *br = arr_b->ptr();

					Variant va;
					for (int32_t i = 0; i < sz; i++) {
						Variant::interpolate(ar[i], br[i], c, va);
						vw[i] = va;
					}
				}
				r_dst = v;
			}
		}
			return;
		case PACKED_INT64_ARRAY: {
			const Vector<int64_t> *arr_a = &PackedArrayRef<int64_t>::get_array(a._data.packed_array);
			const Vector<int64_t> *arr_b = &PackedArrayRef<int64_t>::get_array(b._data.packed_array);
			int64_t sz = arr_a->size();
			if (sz == 0 || arr_b->size() != sz) {
				r_dst = a;
			} else {
				Vector<int64_t> v;
				v.resize(sz);
				{
					int64_t *vw = v.ptrw();
					const int64_t *ar = arr_a->ptr();
					const int64_t *br = arr_b->ptr();

					Variant va;
					for (int64_t i = 0; i < sz; i++) {
						Variant::interpolate(ar[i], br[i], c, va);
						vw[i] = va;
					}
				}
				r_dst = v;
			}
		}
			return;
		case PACKED_FLOAT32_ARRAY: {
			const Vector<float> *arr_a = &PackedArrayRef<float>::get_array(a._data.packed_array);
			const Vector<float> *arr_b = &PackedArrayRef<float>::get_array(b._data.packed_array);
			int sz = arr_a->size();
			if (sz == 0 || arr_b->size() != sz) {
				r_dst = a;
			} else {
				Vector<float> v;
				v.resize(sz);
				{
					float *vw = v.ptrw();
					const float *ar = arr_a->ptr();
					const float *br = arr_b->ptr();

					Variant va;
					for (int i = 0; i < sz; i++) {
						Variant::interpolate(ar[i], br[i], c, va);
						vw[i] = va;
					}
				}
				r_dst = v;
			}
		}
			return;
		case PACKED_FLOAT64_ARRAY: {
			const Vector<double> *arr_a = &PackedArrayRef<double>::get_array(a._data.packed_array);
			const Vector<double> *arr_b = &PackedArrayRef<double>::get_array(b._data.packed_array);
			int sz = arr_a->size();
			if (sz == 0 || arr_b->size() != sz) {
				r_dst = a;
			} else {
				Vector<double> v;
				v.resize(sz);
				{
					double *vw = v.ptrw();
					const double *ar = arr_a->ptr();
					const double *br = arr_b->ptr();

					Variant va;
					for (int i = 0; i < sz; i++) {
						Variant::interpolate(ar[i], br[i], c, va);
						vw[i] = va;
					}
				}
				r_dst = v;
			}
		}
			return;
		case PACKED_STRING_ARRAY: {
			r_dst = a;
		}
			return;
		case PACKED_VECTOR2_ARRAY: {
			const Vector<Vector2> *arr_a = &PackedArrayRef<Vector2>::get_array(a._data.packed_array);
			const Vector<Vector2> *arr_b = &PackedArrayRef<Vector2>::get_array(b._data.packed_array);
			int sz = arr_a->size();
			if (sz == 0 || arr_b->size() != sz) {
				r_dst = a;
			} else {
				Vector<Vector2> v;
				v.resize(sz);
				{
					Vector2 *vw = v.ptrw();
					const Vector2 *ar = arr_a->ptr();
					const Vector2 *br = arr_b->ptr();

					for (int i = 0; i < sz; i++) {
						vw[i] = ar[i].lerp(br[i], c);
					}
				}
				r_dst = v;
			}
		}
			return;
		case PACKED_VECTOR3_ARRAY: {
			const Vector<Vector3> *arr_a = &PackedArrayRef<Vector3>::get_array(a._data.packed_array);
			const Vector<Vector3> *arr_b = &PackedArrayRef<Vector3>::get_array(b._data.packed_array);
			int sz = arr_a->size();
			if (sz == 0 || arr_b->size() != sz) {
				r_dst = a;
			} else {
				Vector<Vector3> v;
				v.resize(sz);
				{
					Vector3 *vw = v.ptrw();
					const Vector3 *ar = arr_a->ptr();
					const Vector3 *br = arr_b->ptr();

					for (int i = 0; i < sz; i++) {
						vw[i] = ar[i].lerp(br[i], c);
					}
				}
				r_dst = v;
			}
		}
			return;
		case PACKED_COLOR_ARRAY: {
			const Vector<Color> *arr_a = &PackedArrayRef<Color>::get_array(a._data.packed_array);
			const Vector<Color> *arr_b = &PackedArrayRef<Color>::get_array(b._data.packed_array);
			int sz = arr_a->size();
			if (sz == 0 || arr_b->size() != sz) {
				r_dst = a;
			} else {
				Vector<Color> v;
				v.resize(sz);
				{
					Color *vw = v.ptrw();
					const Color *ar = arr_a->ptr();
					const Color *br = arr_b->ptr();

					for (int i = 0; i < sz; i++) {
						vw[i] = ar[i].lerp(br[i], c);
					}
				}
				r_dst = v;
			}
		}
			return;
		default: {
			r_dst = a;
		}
	}
}

void Variant::_register_variant_setters_getters() {
	register_named_setters_getters();
	register_indexed_setters_getters();
	register_keyed_setters_getters();
}
void Variant::_unregister_variant_setters_getters() {
	unregister_named_setters_getters();
	unregister_indexed_setters_getters();
}
