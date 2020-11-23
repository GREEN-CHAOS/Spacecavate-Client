/*************************************************************************/
/*  voxelizer.h                                                          */
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

#ifndef VOXEL_LIGHT_BAKER_H
#define VOXEL_LIGHT_BAKER_H

#include "core/math/vector3i.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/resources/multimesh.h"

class Voxelizer {
private:
	enum {
		CHILD_EMPTY = 0xFFFFFFFF

	};

	struct Cell {
		uint32_t children[8];
		float albedo[3]; //albedo in RGB24
		float emission[3]; //accumulated light in 16:16 fixed point (needs to be integer for moving lights fast)
		float normal[3];
		uint32_t used_sides;
		float alpha; //used for upsampling
		uint16_t x;
		uint16_t y;
		uint16_t z;
		uint16_t level;

		Cell() {
			for (int i = 0; i < 8; i++) {
				children[i] = CHILD_EMPTY;
			}

			for (int i = 0; i < 3; i++) {
				emission[i] = 0;
				albedo[i] = 0;
				normal[i] = 0;
			}
			alpha = 0;
			used_sides = 0;
			x = y = z = 0;
			level = 0;
		}
	};

	Vector<Cell> bake_cells;
	int cell_subdiv;

	struct CellSort {
		union {
			struct {
				uint64_t z : 16;
				uint64_t y : 16;
				uint64_t x : 16;
				uint64_t level : 16;
			};
			uint64_t key;
		};

		int32_t index;

		_FORCE_INLINE_ bool operator<(const CellSort &p_cell_sort) const {
			return key < p_cell_sort.key;
		}
	};

	struct MaterialCache {
		//128x128 textures
		Vector<Color> albedo;
		Vector<Color> emission;
	};

	Map<Ref<Material>, MaterialCache> material_cache;
	AABB original_bounds;
	AABB po2_bounds;
	int axis_cell_size[3];

	Transform to_cell_space;

	int color_scan_cell_width;
	int bake_texture_size;
	float cell_size;

	int max_original_cells;
	int leaf_voxel_count;

	Vector<Color> _get_bake_texture(Ref<Image> p_image, const Color &p_color_mul, const Color &p_color_add);
	MaterialCache _get_material_cache(Ref<Material> p_material);

	void _plot_face(int p_idx, int p_level, int p_x, int p_y, int p_z, const Vector3 *p_vtx, const Vector3 *p_normal, const Vector2 *p_uv, const MaterialCache &p_material, const AABB &p_aabb);
	void _fixup_plot(int p_idx, int p_level);
	void _debug_mesh(int p_idx, int p_level, const AABB &p_aabb, Ref<MultiMesh> &p_multimesh, int &idx);

	bool sorted;
	void _sort();

public:
	void begin_bake(int p_subdiv, const AABB &p_bounds);
	void plot_mesh(const Transform &p_xform, Ref<Mesh> &p_mesh, const Vector<Ref<Material>> &p_materials, const Ref<Material> &p_override_material);
	void end_bake();

	int get_gi_probe_octree_depth() const;
	Vector3i get_giprobe_octree_size() const;
	int get_giprobe_cell_count() const;
	Vector<uint8_t> get_giprobe_octree_cells() const;
	Vector<uint8_t> get_giprobe_data_cells() const;
	Vector<int> get_giprobe_level_cell_count() const;
	Vector<uint8_t> get_sdf_3d_image() const;

	Ref<MultiMesh> create_debug_multimesh();

	Transform get_to_cell_space_xform() const;
	Voxelizer();
};

#endif // VOXEL_LIGHT_BAKER_H
