/*************************************************************************/
/*  baked_lightmap.h                                                     */
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

#ifndef BAKED_LIGHTMAP_H
#define BAKED_LIGHTMAP_H

#include "core/templates/local_vector.h"
#include "scene/3d/light_3d.h"
#include "scene/3d/lightmapper.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/3d/multimesh_instance_3d.h"
#include "scene/3d/visual_instance_3d.h"
#include "scene/resources/sky.h"

class BakedLightmapData : public Resource {
	GDCLASS(BakedLightmapData, Resource);
	RES_BASE_EXTENSION("lmbake")

	Ref<TextureLayered> light_texture;

	bool uses_spherical_harmonics = false;
	bool interior = false;

	RID lightmap;
	AABB bounds;

	struct User {
		NodePath path;
		int32_t sub_instance;
		Rect2 uv_scale;
		int slice_index;
	};

	Vector<User> users;

	void _set_user_data(const Array &p_data);
	Array _get_user_data() const;
	void _set_probe_data(const Dictionary &p_data);
	Dictionary _get_probe_data() const;

protected:
	static void _bind_methods();

public:
	void add_user(const NodePath &p_path, const Rect2 &p_uv_scale, int p_slice_index, int32_t p_sub_instance = -1);
	int get_user_count() const;
	NodePath get_user_path(int p_user) const;
	int32_t get_user_sub_instance(int p_user) const;
	Rect2 get_user_lightmap_uv_scale(int p_user) const;
	int get_user_lightmap_slice_index(int p_user) const;
	void clear_users();

	void set_light_texture(const Ref<TextureLayered> &p_light_texture);
	Ref<TextureLayered> get_light_texture() const;

	void set_uses_spherical_harmonics(bool p_enable);
	bool is_using_spherical_harmonics() const;

	bool is_interior() const;

	void set_capture_data(const AABB &p_bounds, bool p_interior, const PackedVector3Array &p_points, const PackedColorArray &p_point_sh, const PackedInt32Array &p_tetrahedra, const PackedInt32Array &p_bsp_tree);
	PackedVector3Array get_capture_points() const;
	PackedColorArray get_capture_sh() const;
	PackedInt32Array get_capture_tetrahedra() const;
	PackedInt32Array get_capture_bsp_tree() const;
	AABB get_capture_bounds() const;

	void clear();

	virtual RID get_rid() const override;
	BakedLightmapData();
	~BakedLightmapData();
};

class BakedLightmap : public VisualInstance3D {
	GDCLASS(BakedLightmap, VisualInstance3D);

public:
	enum BakeQuality {
		BAKE_QUALITY_LOW,
		BAKE_QUALITY_MEDIUM,
		BAKE_QUALITY_HIGH,
		BAKE_QUALITY_ULTRA,
	};

	enum GenerateProbes {
		GENERATE_PROBES_DISABLED,
		GENERATE_PROBES_SUBDIV_4,
		GENERATE_PROBES_SUBDIV_8,
		GENERATE_PROBES_SUBDIV_16,
		GENERATE_PROBES_SUBDIV_32,
	};

	enum BakeError {
		BAKE_ERROR_OK,
		BAKE_ERROR_NO_LIGHTMAPPER,
		BAKE_ERROR_NO_SAVE_PATH,
		BAKE_ERROR_NO_MESHES,
		BAKE_ERROR_MESHES_INVALID,
		BAKE_ERROR_CANT_CREATE_IMAGE,
		BAKE_ERROR_USER_ABORTED,
	};

	enum EnvironmentMode {
		ENVIRONMENT_MODE_DISABLED,
		ENVIRONMENT_MODE_SCENE,
		ENVIRONMENT_MODE_CUSTOM_SKY,
		ENVIRONMENT_MODE_CUSTOM_COLOR,
	};

private:
	BakeQuality bake_quality;
	bool use_denoiser;
	int bounces;
	float bias;
	int max_texture_size;
	bool interior;
	EnvironmentMode environment_mode;
	Ref<Sky> environment_custom_sky;
	Color environment_custom_color;
	float environment_custom_energy;
	bool directional;
	GenerateProbes gen_probes;

	Ref<BakedLightmapData> light_data;

	struct LightsFound {
		Transform xform;
		Light3D *light;
	};

	struct MeshesFound {
		Transform xform;
		NodePath node_path;
		int32_t subindex;
		Ref<Mesh> mesh;
		int32_t lightmap_scale;
		Vector<Ref<Material>> overrides;
	};

	void _find_meshes_and_lights(Node *p_at_node, Vector<MeshesFound> &meshes, Vector<LightsFound> &lights, Vector<Vector3> &probes);

	void _assign_lightmaps();
	void _clear_lightmaps();

	struct BakeTimeData {
		String text;
		int pass;
		uint64_t last_step;
	};

	struct BSPSimplex {
		int vertices[4];
		int planes[4];
	};

	struct BSPNode {
		static const int32_t EMPTY_LEAF = INT32_MIN;
		Plane plane;
		int32_t over = EMPTY_LEAF, under = EMPTY_LEAF;
	};

	int _bsp_get_simplex_side(const Vector<Vector3> &p_points, const LocalVector<BSPSimplex> &p_simplices, const Plane &p_plane, uint32_t p_simplex) const;
	int32_t _compute_bsp_tree(const Vector<Vector3> &p_points, const LocalVector<Plane> &p_planes, LocalVector<int32_t> &planes_tested, const LocalVector<BSPSimplex> &p_simplices, const LocalVector<int32_t> &p_simplex_indices, LocalVector<BSPNode> &bsp_nodes);

	struct BakeStepUD {
		Lightmapper::BakeStepFunc func;
		void *ud;
		float from_percent;
		float to_percent;
	};

	static bool _lightmap_bake_step_function(float p_completion, const String &p_text, void *ud, bool p_refresh);

	struct GenProbesOctree {
		Vector3i offset;
		uint32_t size;
		GenProbesOctree *children[8] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
		~GenProbesOctree() {
			for (int i = 0; i < 8; i++) {
				if (children[i] != nullptr) {
					memdelete(children[i]);
				}
			}
		}
	};

	struct Vector3iHash {
		_FORCE_INLINE_ static uint32_t hash(const Vector3i &p_vtx) {
			uint32_t h = hash_djb2_one_32(p_vtx.x);
			h = hash_djb2_one_32(p_vtx.y, h);
			return hash_djb2_one_32(p_vtx.z, h);
		}
	};

	void _plot_triangle_into_octree(GenProbesOctree *p_cell, float p_cell_size, const Vector3 *p_triangle);
	void _gen_new_positions_from_octree(const GenProbesOctree *p_cell, float p_cell_size, const Vector<Vector3> &probe_positions, LocalVector<Vector3> &new_probe_positions, HashMap<Vector3i, bool, Vector3iHash> &positions_used, const AABB &p_bounds);

protected:
	void _validate_property(PropertyInfo &property) const override;
	static void _bind_methods();
	void _notification(int p_what);

public:
	void set_light_data(const Ref<BakedLightmapData> &p_data);
	Ref<BakedLightmapData> get_light_data() const;

	void set_bake_quality(BakeQuality p_quality);
	BakeQuality get_bake_quality() const;

	void set_use_denoiser(bool p_enable);
	bool is_using_denoiser() const;

	void set_directional(bool p_enable);
	bool is_directional() const;

	void set_interior(bool p_interior);
	bool is_interior() const;

	void set_environment_mode(EnvironmentMode p_mode);
	EnvironmentMode get_environment_mode() const;

	void set_environment_custom_sky(const Ref<Sky> &p_sky);
	Ref<Sky> get_environment_custom_sky() const;

	void set_environment_custom_color(const Color &p_color);
	Color get_environment_custom_color() const;

	void set_environment_custom_energy(float p_energy);
	float get_environment_custom_energy() const;

	void set_bounces(int p_bounces);
	int get_bounces() const;

	void set_bias(float p_bias);
	float get_bias() const;

	void set_max_texture_size(int p_size);
	int get_max_texture_size() const;

	void set_generate_probes(GenerateProbes p_generate_probes);
	GenerateProbes get_generate_probes() const;

	AABB get_aabb() const override;
	Vector<Face3> get_faces(uint32_t p_usage_flags) const override;

	BakeError bake(Node *p_from_node, String p_image_data_path = "", Lightmapper::BakeStepFunc p_bake_step = nullptr, void *p_bake_userdata = nullptr);
	BakedLightmap();
};

VARIANT_ENUM_CAST(BakedLightmap::BakeQuality);
VARIANT_ENUM_CAST(BakedLightmap::GenerateProbes);
VARIANT_ENUM_CAST(BakedLightmap::BakeError);
VARIANT_ENUM_CAST(BakedLightmap::EnvironmentMode);

#endif // BAKED_LIGHTMAP_H
