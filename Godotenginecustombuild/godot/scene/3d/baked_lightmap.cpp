/*************************************************************************/
/*  baked_lightmap.cpp                                                   */
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

#include "baked_lightmap.h"

#include "core/io/config_file.h"
#include "core/io/resource_saver.h"
#include "core/math/camera_matrix.h"
#include "core/math/delaunay_3d.h"
#include "core/os/dir_access.h"
#include "core/os/file_access.h"
#include "core/os/os.h"
#include "core/templates/sort_array.h"
#include "lightmap_probe.h"

void BakedLightmapData::add_user(const NodePath &p_path, const Rect2 &p_uv_scale, int p_slice_index, int32_t p_sub_instance) {
	User user;
	user.path = p_path;
	user.uv_scale = p_uv_scale;
	user.slice_index = p_slice_index;
	user.sub_instance = p_sub_instance;
	users.push_back(user);
}

int BakedLightmapData::get_user_count() const {
	return users.size();
}

NodePath BakedLightmapData::get_user_path(int p_user) const {
	ERR_FAIL_INDEX_V(p_user, users.size(), NodePath());
	return users[p_user].path;
}

int32_t BakedLightmapData::get_user_sub_instance(int p_user) const {
	ERR_FAIL_INDEX_V(p_user, users.size(), -1);
	return users[p_user].sub_instance;
}

Rect2 BakedLightmapData::get_user_lightmap_uv_scale(int p_user) const {
	ERR_FAIL_INDEX_V(p_user, users.size(), Rect2());
	return users[p_user].uv_scale;
}

int BakedLightmapData::get_user_lightmap_slice_index(int p_user) const {
	ERR_FAIL_INDEX_V(p_user, users.size(), -1);
	return users[p_user].slice_index;
}

void BakedLightmapData::clear_users() {
	users.clear();
}

void BakedLightmapData::_set_user_data(const Array &p_data) {
	ERR_FAIL_COND((p_data.size() % 4) != 0);

	for (int i = 0; i < p_data.size(); i += 4) {
		add_user(p_data[i + 0], p_data[i + 1], p_data[i + 2], p_data[i + 3]);
	}
}

Array BakedLightmapData::_get_user_data() const {
	Array ret;
	for (int i = 0; i < users.size(); i++) {
		ret.push_back(users[i].path);
		ret.push_back(users[i].uv_scale);
		ret.push_back(users[i].slice_index);
		ret.push_back(users[i].sub_instance);
	}
	return ret;
}

RID BakedLightmapData::get_rid() const {
	return lightmap;
}

void BakedLightmapData::clear() {
	users.clear();
}

void BakedLightmapData::set_light_texture(const Ref<TextureLayered> &p_light_texture) {
	light_texture = p_light_texture;
	RS::get_singleton()->lightmap_set_textures(lightmap, light_texture.is_valid() ? light_texture->get_rid() : RID(), uses_spherical_harmonics);
}

Ref<TextureLayered> BakedLightmapData::get_light_texture() const {
	return light_texture;
}

void BakedLightmapData::set_uses_spherical_harmonics(bool p_enable) {
	uses_spherical_harmonics = p_enable;
	RS::get_singleton()->lightmap_set_textures(lightmap, light_texture.is_valid() ? light_texture->get_rid() : RID(), uses_spherical_harmonics);
}

bool BakedLightmapData::is_using_spherical_harmonics() const {
	return uses_spherical_harmonics;
}

void BakedLightmapData::set_capture_data(const AABB &p_bounds, bool p_interior, const PackedVector3Array &p_points, const PackedColorArray &p_point_sh, const PackedInt32Array &p_tetrahedra, const PackedInt32Array &p_bsp_tree) {
	if (p_points.size()) {
		int pc = p_points.size();
		ERR_FAIL_COND(pc * 9 != p_point_sh.size());
		ERR_FAIL_COND((p_tetrahedra.size() % 4) != 0);
		ERR_FAIL_COND((p_bsp_tree.size() % 6) != 0);
		RS::get_singleton()->lightmap_set_probe_capture_data(lightmap, p_points, p_point_sh, p_tetrahedra, p_bsp_tree);
		RS::get_singleton()->lightmap_set_probe_bounds(lightmap, p_bounds);
		RS::get_singleton()->lightmap_set_probe_interior(lightmap, p_interior);
	} else {
		RS::get_singleton()->lightmap_set_probe_capture_data(lightmap, PackedVector3Array(), PackedColorArray(), PackedInt32Array(), PackedInt32Array());
		RS::get_singleton()->lightmap_set_probe_bounds(lightmap, AABB());
		RS::get_singleton()->lightmap_set_probe_interior(lightmap, false);
	}
	interior = p_interior;
	bounds = p_bounds;
}

PackedVector3Array BakedLightmapData::get_capture_points() const {
	return RS::get_singleton()->lightmap_get_probe_capture_points(lightmap);
}

PackedColorArray BakedLightmapData::get_capture_sh() const {
	return RS::get_singleton()->lightmap_get_probe_capture_sh(lightmap);
}

PackedInt32Array BakedLightmapData::get_capture_tetrahedra() const {
	return RS::get_singleton()->lightmap_get_probe_capture_tetrahedra(lightmap);
}

PackedInt32Array BakedLightmapData::get_capture_bsp_tree() const {
	return RS::get_singleton()->lightmap_get_probe_capture_bsp_tree(lightmap);
}

AABB BakedLightmapData::get_capture_bounds() const {
	return bounds;
}

bool BakedLightmapData::is_interior() const {
	return interior;
}

void BakedLightmapData::_set_probe_data(const Dictionary &p_data) {
	ERR_FAIL_COND(!p_data.has("bounds"));
	ERR_FAIL_COND(!p_data.has("points"));
	ERR_FAIL_COND(!p_data.has("tetrahedra"));
	ERR_FAIL_COND(!p_data.has("bsp"));
	ERR_FAIL_COND(!p_data.has("sh"));
	ERR_FAIL_COND(!p_data.has("interior"));
	set_capture_data(p_data["bounds"], p_data["interior"], p_data["points"], p_data["sh"], p_data["tetrahedra"], p_data["bsp"]);
}

Dictionary BakedLightmapData::_get_probe_data() const {
	Dictionary d;
	d["bounds"] = get_capture_bounds();
	d["points"] = get_capture_points();
	d["tetrahedra"] = get_capture_tetrahedra();
	d["bsp"] = get_capture_bsp_tree();
	d["sh"] = get_capture_sh();
	d["interior"] = is_interior();
	return d;
}

void BakedLightmapData::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_set_user_data", "data"), &BakedLightmapData::_set_user_data);
	ClassDB::bind_method(D_METHOD("_get_user_data"), &BakedLightmapData::_get_user_data);

	ClassDB::bind_method(D_METHOD("set_light_texture", "light_texture"), &BakedLightmapData::set_light_texture);
	ClassDB::bind_method(D_METHOD("get_light_texture"), &BakedLightmapData::get_light_texture);

	ClassDB::bind_method(D_METHOD("set_uses_spherical_harmonics", "uses_spherical_harmonics"), &BakedLightmapData::set_uses_spherical_harmonics);
	ClassDB::bind_method(D_METHOD("is_using_spherical_harmonics"), &BakedLightmapData::is_using_spherical_harmonics);

	ClassDB::bind_method(D_METHOD("add_user", "path", "lightmap", "offset"), &BakedLightmapData::add_user);
	ClassDB::bind_method(D_METHOD("get_user_count"), &BakedLightmapData::get_user_count);
	ClassDB::bind_method(D_METHOD("get_user_path", "user_idx"), &BakedLightmapData::get_user_path);
	ClassDB::bind_method(D_METHOD("clear_users"), &BakedLightmapData::clear_users);

	ClassDB::bind_method(D_METHOD("_set_probe_data", "data"), &BakedLightmapData::_set_probe_data);
	ClassDB::bind_method(D_METHOD("_get_probe_data"), &BakedLightmapData::_get_probe_data);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "light_texture", PROPERTY_HINT_RESOURCE_TYPE, "TextureLayered"), "set_light_texture", "get_light_texture");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "uses_spherical_harmonics", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL), "set_uses_spherical_harmonics", "is_using_spherical_harmonics");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "user_data", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL), "_set_user_data", "_get_user_data");
	ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "probe_data", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL), "_set_probe_data", "_get_probe_data");
}

BakedLightmapData::BakedLightmapData() {
	lightmap = RS::get_singleton()->lightmap_create();
}

BakedLightmapData::~BakedLightmapData() {
	RS::get_singleton()->free(lightmap);
}

///////////////////////////

void BakedLightmap::_find_meshes_and_lights(Node *p_at_node, Vector<MeshesFound> &meshes, Vector<LightsFound> &lights, Vector<Vector3> &probes) {
	MeshInstance3D *mi = Object::cast_to<MeshInstance3D>(p_at_node);
	if (mi && mi->get_gi_mode() == GeometryInstance3D::GI_MODE_BAKED && mi->is_visible_in_tree()) {
		Ref<Mesh> mesh = mi->get_mesh();
		if (mesh.is_valid()) {
			bool all_have_uv2_and_normal = true;
			bool surfaces_found = false;
			for (int i = 0; i < mesh->get_surface_count(); i++) {
				if (mesh->surface_get_primitive_type(i) != Mesh::PRIMITIVE_TRIANGLES) {
					continue;
				}
				if (!(mesh->surface_get_format(i) & Mesh::ARRAY_FORMAT_TEX_UV2)) {
					all_have_uv2_and_normal = false;
					break;
				}
				if (!(mesh->surface_get_format(i) & Mesh::ARRAY_FORMAT_NORMAL)) {
					all_have_uv2_and_normal = false;
					break;
				}
				surfaces_found = true;
			}

			if (surfaces_found && all_have_uv2_and_normal) {
				//READY TO BAKE! size hint could be computed if not found, actually..

				MeshesFound mf;
				mf.xform = get_global_transform().affine_inverse() * mi->get_global_transform();
				mf.node_path = get_path_to(mi);
				mf.subindex = -1;
				mf.mesh = mesh;

				static const int lightmap_scale[GeometryInstance3D::LIGHTMAP_SCALE_MAX] = { 1, 2, 4, 8 };
				mf.lightmap_scale = lightmap_scale[mi->get_lightmap_scale()];

				Ref<Material> all_override = mi->get_material_override();
				for (int i = 0; i < mesh->get_surface_count(); i++) {
					if (all_override.is_valid()) {
						mf.overrides.push_back(all_override);
					} else {
						mf.overrides.push_back(mi->get_surface_material(i));
					}
				}

				meshes.push_back(mf);
			}
		}
	}

	Node3D *s = Object::cast_to<Node3D>(p_at_node);

	if (!mi && s) {
		Array bmeshes = p_at_node->call("get_bake_bmeshes");
		if (bmeshes.size() && (bmeshes.size() & 1) == 0) {
			Transform xf = get_global_transform().affine_inverse() * s->get_global_transform();
			for (int i = 0; i < bmeshes.size(); i += 2) {
				Ref<Mesh> mesh = bmeshes[i];
				if (!mesh.is_valid()) {
					continue;
				}

				MeshesFound mf;

				Transform mesh_xf = bmeshes[i + 1];
				mf.xform = xf * mesh_xf;
				mf.node_path = get_path_to(s);
				mf.subindex = i / 2;
				mf.lightmap_scale = 1;
				mf.mesh = mesh;

				meshes.push_back(mf);
			}
		}
	}

	Light3D *light = Object::cast_to<Light3D>(p_at_node);

	if (light && light->get_bake_mode() != Light3D::BAKE_DISABLED) {
		LightsFound lf;
		lf.xform = get_global_transform().affine_inverse() * light->get_global_transform();
		lf.light = light;
		lights.push_back(lf);
	}

	LightmapProbe *probe = Object::cast_to<LightmapProbe>(p_at_node);

	if (probe) {
		Transform xf = get_global_transform().affine_inverse() * probe->get_global_transform();
		probes.push_back(xf.origin);
	}

	for (int i = 0; i < p_at_node->get_child_count(); i++) {
		Node *child = p_at_node->get_child(i);
		if (!child->get_owner()) {
			continue; //maybe a helper
		}

		_find_meshes_and_lights(child, meshes, lights, probes);
	}
}

int BakedLightmap::_bsp_get_simplex_side(const Vector<Vector3> &p_points, const LocalVector<BSPSimplex> &p_simplices, const Plane &p_plane, uint32_t p_simplex) const {
	int over = 0;
	int under = 0;
	int coplanar = 0;
	const BSPSimplex &s = p_simplices[p_simplex];
	for (int i = 0; i < 4; i++) {
		const Vector3 v = p_points[s.vertices[i]];
		if (p_plane.has_point(v)) { //coplanar
			coplanar++;
		} else if (p_plane.is_point_over(v)) {
			over++;
		} else {
			under++;
		}
	}

	ERR_FAIL_COND_V(under == 0 && over == 0, -2); //should never happen, we discarded flat simplices before, but in any case drop it from the bsp tree and throw an error
	if (under == 0) {
		return 1; // all over
	} else if (over == 0) {
		return -1; // all under
	} else {
		return 0; // crossing
	}
}

//#define DEBUG_BSP

int32_t BakedLightmap::_compute_bsp_tree(const Vector<Vector3> &p_points, const LocalVector<Plane> &p_planes, LocalVector<int32_t> &planes_tested, const LocalVector<BSPSimplex> &p_simplices, const LocalVector<int32_t> &p_simplex_indices, LocalVector<BSPNode> &bsp_nodes) {
	//if we reach here, it means there is more than one simplex
	int32_t node_index = (int32_t)bsp_nodes.size();
	bsp_nodes.push_back(BSPNode());

	//test with all the simplex planes
	Plane best_plane;
	float best_plane_score = -1.0;

	for (uint32_t i = 0; i < p_simplex_indices.size(); i++) {
		const BSPSimplex &s = p_simplices[p_simplex_indices[i]];
		for (int j = 0; j < 4; j++) {
			uint32_t plane_index = s.planes[j];
			if (planes_tested[plane_index] == node_index) {
				continue; //tested this plane already
			}

			planes_tested[plane_index] = node_index;

			static const int face_order[4][3] = {
				{ 0, 1, 2 },
				{ 0, 2, 3 },
				{ 0, 1, 3 },
				{ 1, 2, 3 }
			};

			// despite getting rid of plane duplicates, we should still use here the actual plane to avoid numerical error
			// from thinking this same simplex is intersecting rather than on a side
			Vector3 v0 = p_points[s.vertices[face_order[j][0]]];
			Vector3 v1 = p_points[s.vertices[face_order[j][1]]];
			Vector3 v2 = p_points[s.vertices[face_order[j][2]]];

			Plane plane(v0, v1, v2);

			//test with all the simplices
			int over_count = 0;
			int under_count = 0;

			for (uint32_t k = 0; k < p_simplex_indices.size(); k++) {
				int side = _bsp_get_simplex_side(p_points, p_simplices, plane, p_simplex_indices[k]);
				if (side == -2) {
					continue; //this simplex is invalid, skip for now
				} else if (side < 0) {
					under_count++;
				} else if (side > 0) {
					over_count++;
				}
			}

			if (under_count == 0 && over_count == 0) {
				continue; //most likely precision issue with a flat simplex, do not try this plane
			}

			if (under_count > over_count) { //make sure under is always less than over, so we can compute the same ratio
				SWAP(under_count, over_count);
			}

			float score = 0; //by default, score is 0 (worst)
			if (over_count > 0) {
				//give score mainly based on ratio (under / over), this means that this plane is splitting simplices a lot, but its balanced
				score = float(under_count) / over_count;
			}

			//adjusting priority over least splits, probably not a great idea
			//score *= Math::sqrt(float(over_count + under_count) / p_simplex_indices.size()); //also multiply score

			if (score > best_plane_score) {
				best_plane = plane;
				best_plane_score = score;
			}
		}
	}

	LocalVector<int32_t> indices_over;
	LocalVector<int32_t> indices_under;

	//split again, but add to list
	for (uint32_t i = 0; i < p_simplex_indices.size(); i++) {
		uint32_t index = p_simplex_indices[i];
		int side = _bsp_get_simplex_side(p_points, p_simplices, best_plane, index);

		if (side == -2) {
			continue; //simplex sits on the plane, does not make sense to use it
		}
		if (side <= 0) {
			indices_under.push_back(index);
		}

		if (side >= 0) {
			indices_over.push_back(index);
		}
	}

#ifdef DEBUG_BSP
	print_line("node " + itos(node_index) + " found plane: " + best_plane + " score:" + rtos(best_plane_score) + " - over " + itos(indices_over.size()) + " under " + itos(indices_under.size()) + " intersecting " + itos(intersecting));
#endif

	if (best_plane_score < 0.0 || indices_over.size() == p_simplex_indices.size() || indices_under.size() == p_simplex_indices.size()) {
		ERR_FAIL_COND_V(p_simplex_indices.size() <= 1, 0); //should not happen, this is a bug

		// Failed to separate the tetrahedrons using planes
		// this means Delaunay borked at some point.
		// Luckily, because we are using tetrahedrons, we can resort to
		// less precise but still working ways to generate the separating plane
		// this will most likely look bad when interpolating, but at least it will not crash.
		// and the arctifact will most likely also be very small, so too difficult to notice.

		//find the longest axis

		WARN_PRINT("Inconsistency found in triangulation while building BSP, probe interpolation quality may degrade a bit.");

		LocalVector<Vector3> centers;
		AABB bounds_all;
		for (uint32_t i = 0; i < p_simplex_indices.size(); i++) {
			AABB bounds;
			for (uint32_t j = 0; j < 4; j++) {
				Vector3 p = p_points[p_simplices[p_simplex_indices[i]].vertices[j]];
				if (j == 0) {
					bounds.position = p;
				} else {
					bounds.expand_to(p);
				}
			}
			if (i == 0) {
				centers.push_back(bounds.position + bounds.size * 0.5);
			} else {
				bounds_all.merge_with(bounds);
			}
		}
		Vector3::Axis longest_axis = Vector3::Axis(bounds_all.get_longest_axis_index());

		//find the simplex that will go under
		uint32_t min_d_idx = 0xFFFFFFFF;
		float min_d_dist = 1e20;

		for (uint32_t i = 0; i < centers.size(); i++) {
			if (centers[i][longest_axis] < min_d_dist) {
				min_d_idx = i;
				min_d_dist = centers[i][longest_axis];
			}
		}
		//rebuild best_plane and over/under arrays
		best_plane = Plane();
		best_plane.normal[longest_axis] = 1.0;
		best_plane.d = min_d_dist;

		indices_under.clear();
		indices_under.push_back(min_d_idx);

		indices_over.clear();

		for (uint32_t i = 0; i < p_simplex_indices.size(); i++) {
			if (i == min_d_idx) {
				continue;
			}
			indices_over.push_back(p_simplex_indices[i]);
		}
	}

	BSPNode node;
	node.plane = best_plane;

	if (indices_under.size() == 0) {
		//noting to do here
		node.under = BSPNode::EMPTY_LEAF;
	} else if (indices_under.size() == 1) {
		node.under = -(indices_under[0] + 1);
	} else {
		node.under = _compute_bsp_tree(p_points, p_planes, planes_tested, p_simplices, indices_under, bsp_nodes);
	}

	if (indices_over.size() == 0) {
		//noting to do here
		node.over = BSPNode::EMPTY_LEAF;
	} else if (indices_over.size() == 1) {
		node.over = -(indices_over[0] + 1);
	} else {
		node.over = _compute_bsp_tree(p_points, p_planes, planes_tested, p_simplices, indices_over, bsp_nodes);
	}

	bsp_nodes[node_index] = node;

	return node_index;
}

bool BakedLightmap::_lightmap_bake_step_function(float p_completion, const String &p_text, void *ud, bool p_refresh) {
	BakeStepUD *bsud = (BakeStepUD *)ud;
	bool ret = false;
	if (bsud->func) {
		ret = bsud->func(bsud->from_percent + p_completion * (bsud->to_percent - bsud->from_percent), p_text, bsud->ud, p_refresh);
	}
	return ret;
}

void BakedLightmap::_plot_triangle_into_octree(GenProbesOctree *p_cell, float p_cell_size, const Vector3 *p_triangle) {
	for (int i = 0; i < 8; i++) {
		Vector3i pos = p_cell->offset;
		uint32_t half_size = p_cell->size / 2;
		if (i & 1) {
			pos.x += half_size;
		}
		if (i & 2) {
			pos.y += half_size;
		}
		if (i & 4) {
			pos.z += half_size;
		}

		AABB subcell;
		subcell.position = Vector3(pos) * p_cell_size;
		subcell.size = Vector3(half_size, half_size, half_size) * p_cell_size;

		if (!Geometry3D::triangle_box_overlap(subcell.position + subcell.size * 0.5, subcell.size * 0.5, p_triangle)) {
			continue;
		}

		if (p_cell->children[i] == nullptr) {
			GenProbesOctree *child = memnew(GenProbesOctree);
			child->offset = pos;
			child->size = half_size;
			p_cell->children[i] = child;
		}

		if (half_size > 1) {
			//still levels missing
			_plot_triangle_into_octree(p_cell->children[i], p_cell_size, p_triangle);
		}
	}
}

void BakedLightmap::_gen_new_positions_from_octree(const GenProbesOctree *p_cell, float p_cell_size, const Vector<Vector3> &probe_positions, LocalVector<Vector3> &new_probe_positions, HashMap<Vector3i, bool, Vector3iHash> &positions_used, const AABB &p_bounds) {
	for (int i = 0; i < 8; i++) {
		Vector3i pos = p_cell->offset;
		if (i & 1) {
			pos.x += p_cell->size;
		}
		if (i & 2) {
			pos.y += p_cell->size;
		}
		if (i & 4) {
			pos.z += p_cell->size;
		}

		if (p_cell->size == 1 && !positions_used.has(pos)) {
			//new position to insert!
			Vector3 real_pos = p_bounds.position + Vector3(pos) * p_cell_size;
			//see if a user submitted probe is too close
			int ppcount = probe_positions.size();
			const Vector3 *pp = probe_positions.ptr();
			bool exists = false;
			for (int j = 0; j < ppcount; j++) {
				if (pp[j].distance_to(real_pos) < CMP_EPSILON) {
					exists = true;
					break;
				}
			}

			if (!exists) {
				new_probe_positions.push_back(real_pos);
			}

			positions_used[pos] = true;
		}

		if (p_cell->children[i] != nullptr) {
			_gen_new_positions_from_octree(p_cell->children[i], p_cell_size, probe_positions, new_probe_positions, positions_used, p_bounds);
		}
	}
}

BakedLightmap::BakeError BakedLightmap::bake(Node *p_from_node, String p_image_data_path, Lightmapper::BakeStepFunc p_bake_step, void *p_bake_userdata) {
	if (p_image_data_path == "" && (get_light_data().is_null() || !get_light_data()->get_path().is_resource_file())) {
		return BAKE_ERROR_NO_SAVE_PATH;
	}

	if (p_image_data_path == "") {
		if (get_light_data().is_null()) {
			return BAKE_ERROR_NO_SAVE_PATH;
		}

		p_image_data_path = get_light_data()->get_path();
		if (!p_image_data_path.is_resource_file()) {
			return BAKE_ERROR_NO_SAVE_PATH;
		}
	}

	Ref<Lightmapper> lightmapper = Lightmapper::create();
	ERR_FAIL_COND_V(lightmapper.is_null(), BAKE_ERROR_NO_LIGHTMAPPER);

	BakeStepUD bsud;
	bsud.func = p_bake_step;
	bsud.ud = p_bake_userdata;
	bsud.from_percent = 0.2;
	bsud.to_percent = 0.8;

	if (p_bake_step) {
		p_bake_step(0.0, TTR("Finding meshes, lights and probes"), p_bake_userdata, true);
	}
	/* STEP 1, FIND MESHES, LIGHTS AND PROBES */
	Vector<Lightmapper::MeshData> mesh_data;
	Vector<LightsFound> lights_found;
	Vector<Vector3> probes_found;
	AABB bounds;
	{
		Vector<MeshesFound> meshes_found;
		_find_meshes_and_lights(p_from_node ? p_from_node : get_parent(), meshes_found, lights_found, probes_found);

		if (meshes_found.size() == 0) {
			return BAKE_ERROR_NO_MESHES;
		}
		// create mesh data for insert

		//get the base material textures, help compute altlas size and bounds
		for (int m_i = 0; m_i < meshes_found.size(); m_i++) {
			if (p_bake_step) {
				float p = (float)(m_i) / meshes_found.size();
				p_bake_step(p * 0.1, vformat(TTR("Preparing geometry %d/%d"), m_i, meshes_found.size()), p_bake_userdata, false);
			}

			MeshesFound &mf = meshes_found.write[m_i];

			Size2i lightmap_size = mf.mesh->get_lightmap_size_hint() * mf.lightmap_scale;
			Vector<RID> overrides;
			overrides.resize(mf.overrides.size());
			for (int i = 0; i < mf.overrides.size(); i++) {
				if (mf.overrides[i].is_valid()) {
					overrides.write[i] = mf.overrides[i]->get_rid();
				}
			}
			TypedArray<Image> images = RS::get_singleton()->bake_render_uv2(mf.mesh->get_rid(), overrides, lightmap_size);

			ERR_FAIL_COND_V(images.empty(), BAKE_ERROR_CANT_CREATE_IMAGE);

			Ref<Image> albedo = images[RS::BAKE_CHANNEL_ALBEDO_ALPHA];
			Ref<Image> orm = images[RS::BAKE_CHANNEL_ORM];

			//multiply albedo by metal

			Lightmapper::MeshData md;

			{
				Dictionary d;
				d["path"] = mf.node_path;
				if (mf.subindex >= 0) {
					d["subindex"] = mf.subindex;
				}
				md.userdata = d;
			}

			{
				if (albedo->get_format() != Image::FORMAT_RGBA8) {
					albedo->convert(Image::FORMAT_RGBA8);
				}
				if (orm->get_format() != Image::FORMAT_RGBA8) {
					orm->convert(Image::FORMAT_RGBA8);
				}
				Vector<uint8_t> albedo_alpha = albedo->get_data();
				Vector<uint8_t> orm_data = orm->get_data();

				Vector<uint8_t> albedom;
				uint32_t len = albedo_alpha.size();
				albedom.resize(len);
				const uint8_t *r_aa = albedo_alpha.ptr();
				const uint8_t *r_orm = orm_data.ptr();
				uint8_t *w_albedo = albedom.ptrw();

				for (uint32_t i = 0; i < len; i += 4) {
					w_albedo[i + 0] = uint8_t(CLAMP(float(r_aa[i + 0]) * (1.0 - float(r_orm[i + 2] / 255.0)), 0, 255));
					w_albedo[i + 1] = uint8_t(CLAMP(float(r_aa[i + 1]) * (1.0 - float(r_orm[i + 2] / 255.0)), 0, 255));
					w_albedo[i + 2] = uint8_t(CLAMP(float(r_aa[i + 2]) * (1.0 - float(r_orm[i + 2] / 255.0)), 0, 255));
					w_albedo[i + 3] = 255;
				}

				md.albedo_on_uv2.instance();
				md.albedo_on_uv2->create(lightmap_size.width, lightmap_size.height, false, Image::FORMAT_RGBA8, albedom);
			}

			md.emission_on_uv2 = images[RS::BAKE_CHANNEL_EMISSION];
			if (md.emission_on_uv2->get_format() != Image::FORMAT_RGBAH) {
				md.emission_on_uv2->convert(Image::FORMAT_RGBAH);
			}

			//get geometry

			Basis normal_xform = mf.xform.basis.inverse().transposed();

			for (int i = 0; i < mf.mesh->get_surface_count(); i++) {
				if (mf.mesh->surface_get_primitive_type(i) != Mesh::PRIMITIVE_TRIANGLES) {
					continue;
				}
				Array a = mf.mesh->surface_get_arrays(i);

				Vector<Vector3> vertices = a[Mesh::ARRAY_VERTEX];
				const Vector3 *vr = vertices.ptr();
				Vector<Vector2> uv = a[Mesh::ARRAY_TEX_UV2];
				const Vector2 *uvr = nullptr;
				Vector<Vector3> normals = a[Mesh::ARRAY_NORMAL];
				const Vector3 *nr = nullptr;
				Vector<int> index = a[Mesh::ARRAY_INDEX];

				ERR_CONTINUE(uv.size() == 0);
				ERR_CONTINUE(normals.size() == 0);

				uvr = uv.ptr();
				nr = normals.ptr();

				int facecount;
				const int *ir = nullptr;

				if (index.size()) {
					facecount = index.size() / 3;
					ir = index.ptr();
				} else {
					facecount = vertices.size() / 3;
				}

				for (int j = 0; j < facecount; j++) {
					uint32_t vidx[3];

					if (ir) {
						for (int k = 0; k < 3; k++) {
							vidx[k] = ir[j * 3 + k];
						}
					} else {
						for (int k = 0; k < 3; k++) {
							vidx[k] = j * 3 + k;
						}
					}

					for (int k = 0; k < 3; k++) {
						Vector3 v = mf.xform.xform(vr[vidx[k]]);
						if (bounds == AABB()) {
							bounds.position = v;
						} else {
							bounds.expand_to(v);
						}
						md.points.push_back(v);

						md.uv2.push_back(uvr[vidx[k]]);
						md.normal.push_back(normal_xform.xform(nr[vidx[k]]).normalized());
					}
				}
			}

			mesh_data.push_back(md);
		}
	}

	/* STEP 2, CREATE PROBES */

	if (p_bake_step) {
		p_bake_step(0.3, TTR("Creating probes"), p_bake_userdata, true);
	}

	//bounds need to include the user probes
	for (int i = 0; i < probes_found.size(); i++) {
		bounds.expand_to(probes_found[i]);
	}

	bounds.grow_by(bounds.size.length() * 0.001);

	if (gen_probes == GENERATE_PROBES_DISABLED) {
		// generate 8 probes on bound endpoints
		for (int i = 0; i < 8; i++) {
			probes_found.push_back(bounds.get_endpoint(i));
		}
	} else {
		// detect probes from geometry
		static const int subdiv_values[6] = { 0, 4, 8, 16, 32 };
		int subdiv = subdiv_values[gen_probes];

		float subdiv_cell_size;
		Vector3i bound_limit;
		{
			int longest_axis = bounds.get_longest_axis_index();
			subdiv_cell_size = bounds.size[longest_axis] / subdiv;
			int axis_n1 = (longest_axis + 1) % 3;
			int axis_n2 = (longest_axis + 2) % 3;

			bound_limit[longest_axis] = subdiv;
			bound_limit[axis_n1] = int(Math::ceil(bounds.size[axis_n1] / subdiv_cell_size));
			bound_limit[axis_n2] = int(Math::ceil(bounds.size[axis_n2] / subdiv_cell_size));
			//compensate bounds
			bounds.size[axis_n1] = bound_limit[axis_n1] * subdiv_cell_size;
			bounds.size[axis_n2] = bound_limit[axis_n2] * subdiv_cell_size;
		}

		GenProbesOctree octree;
		octree.size = subdiv;

		for (int i = 0; i < mesh_data.size(); i++) {
			if (p_bake_step) {
				float p = (float)(i) / mesh_data.size();
				p_bake_step(0.3 + p * 0.1, vformat(TTR("Creating probes from mesh %d/%d"), i, mesh_data.size()), p_bake_userdata, false);
			}

			for (int j = 0; j < mesh_data[i].points.size(); j += 3) {
				Vector3 points[3] = { mesh_data[i].points[j + 0] - bounds.position, mesh_data[i].points[j + 1] - bounds.position, mesh_data[i].points[j + 2] - bounds.position };
				_plot_triangle_into_octree(&octree, subdiv_cell_size, points);
			}
		}

		LocalVector<Vector3> new_probe_positions;
		HashMap<Vector3i, bool, Vector3iHash> positions_used;
		for (uint32_t i = 0; i < 8; i++) { //insert bounding endpoints
			Vector3i pos;
			if (i & 1) {
				pos.x += bound_limit.x;
			}
			if (i & 2) {
				pos.y += bound_limit.y;
			}
			if (i & 4) {
				pos.z += bound_limit.z;
			}

			positions_used[pos] = true;
			Vector3 real_pos = bounds.position + Vector3(pos) * subdiv_cell_size; //use same formula for numerical stability
			new_probe_positions.push_back(real_pos);
		}
		//skip first level, since probes are always added at bounds endpoints anyway (code above this)
		for (int i = 0; i < 8; i++) {
			if (octree.children[i]) {
				_gen_new_positions_from_octree(octree.children[i], subdiv_cell_size, probes_found, new_probe_positions, positions_used, bounds);
			}
		}

		for (uint32_t i = 0; i < new_probe_positions.size(); i++) {
			probes_found.push_back(new_probe_positions[i]);
		}
	}

	// Add everything to lightmapper
	if (p_bake_step) {
		p_bake_step(0.4, TTR("Preparing Lightmapper"), p_bake_userdata, true);
	}

	{
		for (int i = 0; i < mesh_data.size(); i++) {
			lightmapper->add_mesh(mesh_data[i]);
		}
		for (int i = 0; i < lights_found.size(); i++) {
			Light3D *light = lights_found[i].light;
			Transform xf = lights_found[i].xform;

			if (Object::cast_to<DirectionalLight3D>(light)) {
				DirectionalLight3D *l = Object::cast_to<DirectionalLight3D>(light);
				lightmapper->add_directional_light(light->get_bake_mode() == Light3D::BAKE_STATIC, -xf.basis.get_axis(Vector3::AXIS_Z).normalized(), l->get_color(), l->get_param(Light3D::PARAM_ENERGY), l->get_param(Light3D::PARAM_SIZE));
			} else if (Object::cast_to<OmniLight3D>(light)) {
				OmniLight3D *l = Object::cast_to<OmniLight3D>(light);
				lightmapper->add_omni_light(light->get_bake_mode() == Light3D::BAKE_STATIC, xf.origin, l->get_color(), l->get_param(Light3D::PARAM_ENERGY), l->get_param(Light3D::PARAM_RANGE), l->get_param(Light3D::PARAM_ATTENUATION), l->get_param(Light3D::PARAM_SIZE));
			} else if (Object::cast_to<SpotLight3D>(light)) {
				SpotLight3D *l = Object::cast_to<SpotLight3D>(light);
				lightmapper->add_spot_light(light->get_bake_mode() == Light3D::BAKE_STATIC, xf.origin, -xf.basis.get_axis(Vector3::AXIS_Z).normalized(), l->get_color(), l->get_param(Light3D::PARAM_ENERGY), l->get_param(Light3D::PARAM_RANGE), l->get_param(Light3D::PARAM_ATTENUATION), l->get_param(Light3D::PARAM_SPOT_ANGLE), l->get_param(Light3D::PARAM_SPOT_ATTENUATION), l->get_param(Light3D::PARAM_SIZE));
			}
		}
		for (int i = 0; i < probes_found.size(); i++) {
			lightmapper->add_probe(probes_found[i]);
		}
	}

	Ref<Image> environment_image;
	Basis environment_transform;

	// Add everything to lightmapper
	if (environment_mode != ENVIRONMENT_MODE_DISABLED) {
		if (p_bake_step) {
			p_bake_step(4.1, TTR("Preparing Environment"), p_bake_userdata, true);
		}

		environment_transform = get_global_transform().basis;

		switch (environment_mode) {
			case ENVIRONMENT_MODE_DISABLED: {
				//nothing
			} break;
			case ENVIRONMENT_MODE_SCENE: {
				Ref<World3D> world = get_world_3d();
				if (world.is_valid()) {
					Ref<Environment> env = world->get_environment();
					if (env.is_null()) {
						env = world->get_fallback_environment();
					}

					if (env.is_valid()) {
						environment_image = RS::get_singleton()->environment_bake_panorama(env->get_rid(), true, Size2i(128, 64));
					}
				}
			} break;
			case ENVIRONMENT_MODE_CUSTOM_SKY: {
				if (environment_custom_sky.is_valid()) {
					environment_image = RS::get_singleton()->sky_bake_panorama(environment_custom_sky->get_rid(), environment_custom_energy, true, Size2i(128, 64));
				}

			} break;
			case ENVIRONMENT_MODE_CUSTOM_COLOR: {
				environment_image.instance();
				environment_image->create(128, 64, false, Image::FORMAT_RGBAF);
				Color c = environment_custom_color;
				c.r *= environment_custom_energy;
				c.g *= environment_custom_energy;
				c.b *= environment_custom_energy;
				for (int i = 0; i < 128; i++) {
					for (int j = 0; j < 64; j++) {
						environment_image->set_pixel(i, j, c);
					}
				}

			} break;
		}
	}

	Lightmapper::BakeError bake_err = lightmapper->bake(Lightmapper::BakeQuality(bake_quality), use_denoiser, bounces, bias, max_texture_size, directional, Lightmapper::GenerateProbes(gen_probes), environment_image, environment_transform, _lightmap_bake_step_function, &bsud);

	if (bake_err == Lightmapper::BAKE_ERROR_LIGHTMAP_CANT_PRE_BAKE_MESHES) {
		return BAKE_ERROR_MESHES_INVALID;
	}

	/* POSTBAKE: Save Textures */

	Ref<TextureLayered> texture;
	{
		Vector<Ref<Image>> images;
		for (int i = 0; i < lightmapper->get_bake_texture_count(); i++) {
			images.push_back(lightmapper->get_bake_texture(i));
		}
		//we assume they are all the same, so lets create a large one for saving
		Ref<Image> large_image;
		large_image.instance();

		large_image->create(images[0]->get_width(), images[0]->get_height() * images.size(), false, images[0]->get_format());

		for (int i = 0; i < lightmapper->get_bake_texture_count(); i++) {
			large_image->blit_rect(images[i], Rect2(0, 0, images[i]->get_width(), images[i]->get_height()), Point2(0, images[i]->get_height() * i));
		}

		String base_path = p_image_data_path.get_basename() + ".exr";

		Ref<ConfigFile> config;

		config.instance();
		if (FileAccess::exists(base_path + ".import")) {
			config->load(base_path + ".import");
		}

		config->set_value("remap", "importer", "2d_array_texture");
		config->set_value("remap", "type", "StreamTexture2DArray");
		if (!config->has_section_key("params", "compress/mode")) {
			config->set_value("params", "compress/mode", 2); //user may want another compression, so leave it be
		}
		config->set_value("params", "compress/channel_pack", 1);
		config->set_value("params", "mipmaps/generate", false);
		config->set_value("params", "slices/horizontal", 1);
		config->set_value("params", "slices/vertical", images.size());

		config->save(base_path + ".import");

		Error err = large_image->save_exr(base_path, false);
		ERR_FAIL_COND_V(err, BAKE_ERROR_CANT_CREATE_IMAGE);
		ResourceLoader::import(base_path);
		Ref<Texture> t = ResourceLoader::load(base_path); //if already loaded, it will be updated on refocus?
		ERR_FAIL_COND_V(t.is_null(), BAKE_ERROR_CANT_CREATE_IMAGE);
		texture = t;
	}

	/* POSTBAKE: Save Light Data */

	Ref<BakedLightmapData> data;
	if (get_light_data().is_valid()) {
		data = get_light_data();
		set_light_data(Ref<BakedLightmapData>()); //clear
		data->clear();
	} else {
		data.instance();
	}

	data->set_light_texture(texture);
	data->set_uses_spherical_harmonics(directional);

	for (int i = 0; i < lightmapper->get_bake_mesh_count(); i++) {
		Dictionary d = lightmapper->get_bake_mesh_userdata(i);
		NodePath np = d["path"];
		int32_t subindex = -1;
		if (d.has("subindex")) {
			subindex = d["subindex"];
		}

		Rect2 uv_scale = lightmapper->get_bake_mesh_uv_scale(i);
		int slice_index = lightmapper->get_bake_mesh_texture_slice(i);
		data->add_user(np, uv_scale, slice_index, subindex);
	}

	{
		// create tetrahedrons
		Vector<Vector3> points;
		Vector<Color> sh;
		points.resize(lightmapper->get_bake_probe_count());
		sh.resize(lightmapper->get_bake_probe_count() * 9);
		for (int i = 0; i < lightmapper->get_bake_probe_count(); i++) {
			points.write[i] = lightmapper->get_bake_probe_point(i);
			Vector<Color> colors = lightmapper->get_bake_probe_sh(i);
			ERR_CONTINUE(colors.size() != 9);
			for (int j = 0; j < 9; j++) {
				sh.write[i * 9 + j] = colors[j];
			}
		}

		//Obtain solved simplices

		if (p_bake_step) {
			p_bake_step(0.8, TTR("Generating Probe Volumes"), p_bake_userdata, true);
		}
		Vector<Delaunay3D::OutputSimplex> solved_simplices = Delaunay3D::tetrahedralize(points);

		LocalVector<BSPSimplex> bsp_simplices;
		LocalVector<Plane> bsp_planes;
		LocalVector<int32_t> bsp_simplex_indices;
		PackedInt32Array tetrahedrons;

		for (int i = 0; i < solved_simplices.size(); i++) {
			//Prepare a special representation of the simplex, which uses a BSP Tree
			BSPSimplex bsp_simplex;
			for (int j = 0; j < 4; j++) {
				bsp_simplex.vertices[j] = solved_simplices[i].points[j];
			}
			for (int j = 0; j < 4; j++) {
				static const int face_order[4][3] = {
					{ 0, 1, 2 },
					{ 0, 2, 3 },
					{ 0, 1, 3 },
					{ 1, 2, 3 }
				};
				Vector3 a = points[solved_simplices[i].points[face_order[j][0]]];
				Vector3 b = points[solved_simplices[i].points[face_order[j][1]]];
				Vector3 c = points[solved_simplices[i].points[face_order[j][2]]];

				//store planes in an array, but ensure they are reused, to speed up processing

				Plane p(a, b, c);
				int plane_index = -1;
				for (uint32_t k = 0; k < bsp_planes.size(); k++) {
					if (bsp_planes[k].is_equal_approx_any_side(p)) {
						plane_index = k;
						break;
					}
				}

				if (plane_index == -1) {
					plane_index = bsp_planes.size();
					bsp_planes.push_back(p);
				}

				bsp_simplex.planes[j] = plane_index;

				//also fill simplex array
				tetrahedrons.push_back(solved_simplices[i].points[j]);
			}

			bsp_simplex_indices.push_back(bsp_simplices.size());
			bsp_simplices.push_back(bsp_simplex);
		}

//#define DEBUG_SIMPLICES_AS_OBJ_FILE
#ifdef DEBUG_SIMPLICES_AS_OBJ_FILE
		{
			FileAccessRef f = FileAccess::open("res://bsp.obj", FileAccess::WRITE);
			for (uint32_t i = 0; i < bsp_simplices.size(); i++) {
				f->store_line("o Simplex" + itos(i));
				for (int j = 0; j < 4; j++) {
					f->store_line(vformat("v %f %f %f", points[bsp_simplices[i].vertices[j]].x, points[bsp_simplices[i].vertices[j]].y, points[bsp_simplices[i].vertices[j]].z));
				}
				static const int face_order[4][3] = {
					{ 1, 2, 3 },
					{ 1, 3, 4 },
					{ 1, 2, 4 },
					{ 2, 3, 4 }
				};

				for (int j = 0; j < 4; j++) {
					f->store_line(vformat("f %d %d %d", 4 * i + face_order[j][0], 4 * i + face_order[j][1], 4 * i + face_order[j][2]));
				}
			}
			f->close();
		}
#endif

		LocalVector<BSPNode> bsp_nodes;
		LocalVector<int32_t> planes_tested;
		planes_tested.resize(bsp_planes.size());
		for (uint32_t i = 0; i < planes_tested.size(); i++) {
			planes_tested[i] = 0x7FFFFFFF;
		}

		if (p_bake_step) {
			p_bake_step(0.9, TTR("Generating Probe Acceleration Structures"), p_bake_userdata, true);
		}

		_compute_bsp_tree(points, bsp_planes, planes_tested, bsp_simplices, bsp_simplex_indices, bsp_nodes);

		PackedInt32Array bsp_array;
		bsp_array.resize(bsp_nodes.size() * 6); // six 32 bits values used for each BSP node
		{
			float *fptr = (float *)bsp_array.ptrw();
			int32_t *iptr = (int32_t *)bsp_array.ptrw();
			for (uint32_t i = 0; i < bsp_nodes.size(); i++) {
				fptr[i * 6 + 0] = bsp_nodes[i].plane.normal.x;
				fptr[i * 6 + 1] = bsp_nodes[i].plane.normal.y;
				fptr[i * 6 + 2] = bsp_nodes[i].plane.normal.z;
				fptr[i * 6 + 3] = bsp_nodes[i].plane.d;
				iptr[i * 6 + 4] = bsp_nodes[i].over;
				iptr[i * 6 + 5] = bsp_nodes[i].under;
			}
//#define DEBUG_BSP_TREE
#ifdef DEBUG_BSP_TREE
			FileAccessRef f = FileAccess::open("res://bsp.txt", FileAccess::WRITE);
			for (uint32_t i = 0; i < bsp_nodes.size(); i++) {
				f->store_line(itos(i) + " - plane: " + bsp_nodes[i].plane + " over: " + itos(bsp_nodes[i].over) + " under: " + itos(bsp_nodes[i].under));
			}
#endif
		}

		/* Obtain the colors from the images, they will be re-created as cubemaps on the server, depending on the driver */

		data->set_capture_data(bounds, interior, points, sh, tetrahedrons, bsp_array);
		/* Compute a BSP tree of the simplices, so it's easy to find the exact one */
	}

	Error err = ResourceSaver::save(p_image_data_path, data);
	data->set_path(p_image_data_path);

	if (err != OK) {
		return BAKE_ERROR_CANT_CREATE_IMAGE;
	}

	set_light_data(data);

	return BAKE_ERROR_OK;
}

void BakedLightmap::_notification(int p_what) {
	if (p_what == NOTIFICATION_POST_ENTER_TREE) {
		if (light_data.is_valid()) {
			_assign_lightmaps();
		}
	}

	if (p_what == NOTIFICATION_EXIT_TREE) {
		if (light_data.is_valid()) {
			_clear_lightmaps();
		}
	}
}

void BakedLightmap::_assign_lightmaps() {
	ERR_FAIL_COND(!light_data.is_valid());

	for (int i = 0; i < light_data->get_user_count(); i++) {
		Node *node = get_node(light_data->get_user_path(i));
		int instance_idx = light_data->get_user_sub_instance(i);
		if (instance_idx >= 0) {
			RID instance = node->call("get_bake_mesh_instance", instance_idx);
			if (instance.is_valid()) {
				RS::get_singleton()->instance_geometry_set_lightmap(instance, get_instance(), light_data->get_user_lightmap_uv_scale(i), light_data->get_user_lightmap_slice_index(i));
			}
		} else {
			VisualInstance3D *vi = Object::cast_to<VisualInstance3D>(node);
			ERR_CONTINUE(!vi);
			RS::get_singleton()->instance_geometry_set_lightmap(vi->get_instance(), get_instance(), light_data->get_user_lightmap_uv_scale(i), light_data->get_user_lightmap_slice_index(i));
		}
	}
}

void BakedLightmap::_clear_lightmaps() {
	ERR_FAIL_COND(!light_data.is_valid());
	for (int i = 0; i < light_data->get_user_count(); i++) {
		Node *node = get_node(light_data->get_user_path(i));
		int instance_idx = light_data->get_user_sub_instance(i);
		if (instance_idx >= 0) {
			RID instance = node->call("get_bake_mesh_instance", instance_idx);
			if (instance.is_valid()) {
				RS::get_singleton()->instance_geometry_set_lightmap(instance, RID(), Rect2(), 0);
			}
		} else {
			VisualInstance3D *vi = Object::cast_to<VisualInstance3D>(node);
			ERR_CONTINUE(!vi);
			RS::get_singleton()->instance_geometry_set_lightmap(vi->get_instance(), RID(), Rect2(), 0);
		}
	}
}

void BakedLightmap::set_light_data(const Ref<BakedLightmapData> &p_data) {
	if (light_data.is_valid()) {
		if (is_inside_tree()) {
			_clear_lightmaps();
		}
		set_base(RID());
	}
	light_data = p_data;

	if (light_data.is_valid()) {
		set_base(light_data->get_rid());
		if (is_inside_tree()) {
			_assign_lightmaps();
		}
	}

	update_gizmo();
}

Ref<BakedLightmapData> BakedLightmap::get_light_data() const {
	return light_data;
}

void BakedLightmap::set_bake_quality(BakeQuality p_quality) {
	bake_quality = p_quality;
}

BakedLightmap::BakeQuality BakedLightmap::get_bake_quality() const {
	return bake_quality;
}

AABB BakedLightmap::get_aabb() const {
	return AABB();
}

Vector<Face3> BakedLightmap::get_faces(uint32_t p_usage_flags) const {
	return Vector<Face3>();
}

void BakedLightmap::set_use_denoiser(bool p_enable) {
	use_denoiser = p_enable;
}

bool BakedLightmap::is_using_denoiser() const {
	return use_denoiser;
}

void BakedLightmap::set_directional(bool p_enable) {
	directional = p_enable;
}

bool BakedLightmap::is_directional() const {
	return directional;
}

void BakedLightmap::set_interior(bool p_enable) {
	interior = p_enable;
}

bool BakedLightmap::is_interior() const {
	return interior;
}

void BakedLightmap::set_environment_mode(EnvironmentMode p_mode) {
	environment_mode = p_mode;
	_change_notify();
}

BakedLightmap::EnvironmentMode BakedLightmap::get_environment_mode() const {
	return environment_mode;
}

void BakedLightmap::set_environment_custom_sky(const Ref<Sky> &p_sky) {
	environment_custom_sky = p_sky;
}

Ref<Sky> BakedLightmap::get_environment_custom_sky() const {
	return environment_custom_sky;
}

void BakedLightmap::set_environment_custom_color(const Color &p_color) {
	environment_custom_color = p_color;
}

Color BakedLightmap::get_environment_custom_color() const {
	return environment_custom_color;
}

void BakedLightmap::set_environment_custom_energy(float p_energy) {
	environment_custom_energy = p_energy;
}

float BakedLightmap::get_environment_custom_energy() const {
	return environment_custom_energy;
}

void BakedLightmap::set_bounces(int p_bounces) {
	ERR_FAIL_COND(p_bounces < 0 || p_bounces > 16);
	bounces = p_bounces;
}

int BakedLightmap::get_bounces() const {
	return bounces;
}

void BakedLightmap::set_bias(float p_bias) {
	ERR_FAIL_COND(p_bias < 0.00001);
	bias = p_bias;
}

float BakedLightmap::get_bias() const {
	return bias;
}

void BakedLightmap::set_max_texture_size(int p_size) {
	ERR_FAIL_COND(p_size < 2048);
	max_texture_size = p_size;
}

int BakedLightmap::get_max_texture_size() const {
	return max_texture_size;
}

void BakedLightmap::set_generate_probes(GenerateProbes p_generate_probes) {
	gen_probes = p_generate_probes;
}

BakedLightmap::GenerateProbes BakedLightmap::get_generate_probes() const {
	return gen_probes;
}

void BakedLightmap::_validate_property(PropertyInfo &property) const {
	if (property.name == "environment_custom_sky" && environment_mode != ENVIRONMENT_MODE_CUSTOM_SKY) {
		property.usage = 0;
	}
	if (property.name == "environment_custom_color" && environment_mode != ENVIRONMENT_MODE_CUSTOM_COLOR) {
		property.usage = 0;
	}
	if (property.name == "environment_custom_energy" && environment_mode != ENVIRONMENT_MODE_CUSTOM_COLOR && environment_mode != ENVIRONMENT_MODE_CUSTOM_SKY) {
		property.usage = 0;
	}
}

void BakedLightmap::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_light_data", "data"), &BakedLightmap::set_light_data);
	ClassDB::bind_method(D_METHOD("get_light_data"), &BakedLightmap::get_light_data);

	ClassDB::bind_method(D_METHOD("set_bake_quality", "bake_quality"), &BakedLightmap::set_bake_quality);
	ClassDB::bind_method(D_METHOD("get_bake_quality"), &BakedLightmap::get_bake_quality);

	ClassDB::bind_method(D_METHOD("set_bounces", "bounces"), &BakedLightmap::set_bounces);
	ClassDB::bind_method(D_METHOD("get_bounces"), &BakedLightmap::get_bounces);

	ClassDB::bind_method(D_METHOD("set_generate_probes", "subdivision"), &BakedLightmap::set_generate_probes);
	ClassDB::bind_method(D_METHOD("get_generate_probes"), &BakedLightmap::get_generate_probes);

	ClassDB::bind_method(D_METHOD("set_bias", "bias"), &BakedLightmap::set_bias);
	ClassDB::bind_method(D_METHOD("get_bias"), &BakedLightmap::get_bias);

	ClassDB::bind_method(D_METHOD("set_environment_mode", "mode"), &BakedLightmap::set_environment_mode);
	ClassDB::bind_method(D_METHOD("get_environment_mode"), &BakedLightmap::get_environment_mode);

	ClassDB::bind_method(D_METHOD("set_environment_custom_sky", "sky"), &BakedLightmap::set_environment_custom_sky);
	ClassDB::bind_method(D_METHOD("get_environment_custom_sky"), &BakedLightmap::get_environment_custom_sky);

	ClassDB::bind_method(D_METHOD("set_environment_custom_color", "color"), &BakedLightmap::set_environment_custom_color);
	ClassDB::bind_method(D_METHOD("get_environment_custom_color"), &BakedLightmap::get_environment_custom_color);

	ClassDB::bind_method(D_METHOD("set_environment_custom_energy", "energy"), &BakedLightmap::set_environment_custom_energy);
	ClassDB::bind_method(D_METHOD("get_environment_custom_energy"), &BakedLightmap::get_environment_custom_energy);

	ClassDB::bind_method(D_METHOD("set_max_texture_size", "max_texture_size"), &BakedLightmap::set_max_texture_size);
	ClassDB::bind_method(D_METHOD("get_max_texture_size"), &BakedLightmap::get_max_texture_size);

	ClassDB::bind_method(D_METHOD("set_use_denoiser", "use_denoiser"), &BakedLightmap::set_use_denoiser);
	ClassDB::bind_method(D_METHOD("is_using_denoiser"), &BakedLightmap::is_using_denoiser);

	ClassDB::bind_method(D_METHOD("set_interior", "enable"), &BakedLightmap::set_interior);
	ClassDB::bind_method(D_METHOD("is_interior"), &BakedLightmap::is_interior);

	ClassDB::bind_method(D_METHOD("set_directional", "directional"), &BakedLightmap::set_directional);
	ClassDB::bind_method(D_METHOD("is_directional"), &BakedLightmap::is_directional);

	//	ClassDB::bind_method(D_METHOD("bake", "from_node"), &BakedLightmap::bake, DEFVAL(Variant()));

	ADD_GROUP("Tweaks", "");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "quality", PROPERTY_HINT_ENUM, "Low,Medium,High,Ultra"), "set_bake_quality", "get_bake_quality");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "bounces", PROPERTY_HINT_RANGE, "0,16,1"), "set_bounces", "get_bounces");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "directional"), "set_directional", "is_directional");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "interior"), "set_interior", "is_interior");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "use_denoiser"), "set_use_denoiser", "is_using_denoiser");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "bias", PROPERTY_HINT_RANGE, "0.00001,0.1,0.00001,or_greater"), "set_bias", "get_bias");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "max_texture_size"), "set_max_texture_size", "get_max_texture_size");
	ADD_GROUP("Environment", "environment_");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "environment_mode", PROPERTY_HINT_ENUM, "Disabled,Scene,Custom Sky,Custom Color"), "set_environment_mode", "get_environment_mode");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "environment_custom_sky", PROPERTY_HINT_RESOURCE_TYPE, "Sky"), "set_environment_custom_sky", "get_environment_custom_sky");
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "environment_custom_color", PROPERTY_HINT_COLOR_NO_ALPHA), "set_environment_custom_color", "get_environment_custom_color");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "environment_custom_energy", PROPERTY_HINT_RANGE, "0,64,0.01"), "set_environment_custom_energy", "get_environment_custom_energy");
	ADD_GROUP("Gen Probes", "generate_probes_");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "generate_probes_subdiv", PROPERTY_HINT_ENUM, "Disabled,4,8,16,32"), "set_generate_probes", "get_generate_probes");
	ADD_GROUP("Data", "");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "light_data", PROPERTY_HINT_RESOURCE_TYPE, "BakedLightmapData"), "set_light_data", "get_light_data");

	BIND_ENUM_CONSTANT(BAKE_QUALITY_LOW);
	BIND_ENUM_CONSTANT(BAKE_QUALITY_MEDIUM);
	BIND_ENUM_CONSTANT(BAKE_QUALITY_HIGH);
	BIND_ENUM_CONSTANT(BAKE_QUALITY_ULTRA);

	BIND_ENUM_CONSTANT(GENERATE_PROBES_DISABLED);
	BIND_ENUM_CONSTANT(GENERATE_PROBES_SUBDIV_4);
	BIND_ENUM_CONSTANT(GENERATE_PROBES_SUBDIV_8);
	BIND_ENUM_CONSTANT(GENERATE_PROBES_SUBDIV_16);
	BIND_ENUM_CONSTANT(GENERATE_PROBES_SUBDIV_32);

	BIND_ENUM_CONSTANT(BAKE_ERROR_OK);
	BIND_ENUM_CONSTANT(BAKE_ERROR_NO_LIGHTMAPPER);
	BIND_ENUM_CONSTANT(BAKE_ERROR_NO_SAVE_PATH);
	BIND_ENUM_CONSTANT(BAKE_ERROR_NO_MESHES);
	BIND_ENUM_CONSTANT(BAKE_ERROR_MESHES_INVALID);
	BIND_ENUM_CONSTANT(BAKE_ERROR_CANT_CREATE_IMAGE);
	BIND_ENUM_CONSTANT(BAKE_ERROR_USER_ABORTED);

	BIND_ENUM_CONSTANT(ENVIRONMENT_MODE_DISABLED);
	BIND_ENUM_CONSTANT(ENVIRONMENT_MODE_SCENE);
	BIND_ENUM_CONSTANT(ENVIRONMENT_MODE_CUSTOM_SKY);
	BIND_ENUM_CONSTANT(ENVIRONMENT_MODE_CUSTOM_COLOR);
}

BakedLightmap::BakedLightmap() {
	environment_mode = ENVIRONMENT_MODE_DISABLED;
	environment_custom_color = Color(0.2, 0.7, 1.0);
	environment_custom_energy = 1.0;

	bake_quality = BAKE_QUALITY_MEDIUM;
	interior = false;
	directional = false;

	gen_probes = GENERATE_PROBES_DISABLED;
	use_denoiser = true;
	bounces = 1;
	bias = 0.0005;
	max_texture_size = 16384;
}
