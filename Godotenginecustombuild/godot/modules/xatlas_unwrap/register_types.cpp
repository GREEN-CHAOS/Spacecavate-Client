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

#include "core/error/error_macros.h"

#include "core/crypto/crypto_core.h"

#include "thirdparty/xatlas/xatlas.h"

#include <stdio.h>
#include <stdlib.h>

extern bool (*array_mesh_lightmap_unwrap_callback)(float p_texel_size, const float *p_vertices, const float *p_normals, int p_vertex_count, const int *p_indices, int p_index_count, float **r_uv, int **r_vertex, int *r_vertex_count, int **r_index, int *r_index_count, int *r_size_hint_x, int *r_size_hint_y, int *&r_cache_data, unsigned int &r_cache_size, bool &r_used_cache);

bool xatlas_mesh_lightmap_unwrap_callback(float p_texel_size, const float *p_vertices, const float *p_normals, int p_vertex_count, const int *p_indices, int p_index_count, float **r_uvs, int **r_vertices, int *r_vertex_count, int **r_indices, int *r_index_count, int *r_size_hint_x, int *r_size_hint_y, int *&r_cache_data, unsigned int &r_cache_size, bool &r_used_cache) {
	CryptoCore::MD5Context ctx;
	ctx.start();

	ctx.update((unsigned char *)&p_texel_size, sizeof(float));
	ctx.update((unsigned char *)p_indices, sizeof(int) * p_index_count);
	ctx.update((unsigned char *)p_vertices, sizeof(float) * p_vertex_count);
	ctx.update((unsigned char *)p_normals, sizeof(float) * p_vertex_count);

	unsigned char hash[16];
	ctx.finish(hash);

	bool cached = false;
	unsigned int cache_idx = 0;

	if (r_used_cache && r_cache_size) {
		//Check if hash is in cache data

		int *cache_data = r_cache_data;
		int n_entries = cache_data[0];
		unsigned int r_idx = 1;
		for (int i = 0; i < n_entries; ++i) {
			if (memcmp(&cache_data[r_idx], hash, 16) == 0) {
				cached = true;
				cache_idx = r_idx;
				break;
			}

			r_idx += 4; // hash
			r_idx += 2; // size hint

			int vertex_count = cache_data[r_idx];
			r_idx += 1; // vertex count
			r_idx += vertex_count; // vertex
			r_idx += vertex_count * 2; // uvs

			int index_count = cache_data[r_idx];
			r_idx += 1; // index count
			r_idx += index_count; // indices
		}
	}

	if (r_used_cache && cached) {
		int *cache_data = r_cache_data;

		// Return cache data pointer to the caller
		r_cache_data = &cache_data[cache_idx];

		cache_idx += 4;

		// Load size
		*r_size_hint_x = cache_data[cache_idx];
		*r_size_hint_y = cache_data[cache_idx + 1];
		cache_idx += 2;

		// Load vertices
		*r_vertex_count = cache_data[cache_idx];
		cache_idx++;
		*r_vertices = &cache_data[cache_idx];
		cache_idx += *r_vertex_count;

		// Load UVs
		*r_uvs = (float *)&cache_data[cache_idx];
		cache_idx += *r_vertex_count * 2;

		// Load indices
		*r_index_count = cache_data[cache_idx];
		cache_idx++;
		*r_indices = &cache_data[cache_idx];

		// Return cache data size to the caller
		r_cache_size = sizeof(int) * (4 + 2 + 1 + *r_vertex_count + (*r_vertex_count * 2) + 1 + *r_index_count); // hash + size hint + vertex_count + vertices + uvs + index_count + indices
		r_used_cache = true;
		return true;
	}

	//set up input mesh
	xatlas::MeshDecl input_mesh;
	input_mesh.indexData = p_indices;
	input_mesh.indexCount = p_index_count;
	input_mesh.indexFormat = xatlas::IndexFormat::UInt32;

	input_mesh.vertexCount = p_vertex_count;
	input_mesh.vertexPositionData = p_vertices;
	input_mesh.vertexPositionStride = sizeof(float) * 3;
	input_mesh.vertexNormalData = p_normals;
	input_mesh.vertexNormalStride = sizeof(uint32_t) * 3;
	input_mesh.vertexUvData = nullptr;
	input_mesh.vertexUvStride = 0;

	xatlas::ChartOptions chart_options;
	xatlas::PackOptions pack_options;

	pack_options.maxChartSize = 4096;
	pack_options.blockAlign = true;
	pack_options.padding = 1;
	pack_options.texelsPerUnit = 1.0 / p_texel_size;

	xatlas::Atlas *atlas = xatlas::Create();
	printf("Adding mesh..\n");
	xatlas::AddMeshError::Enum err = xatlas::AddMesh(atlas, input_mesh, 1);
	ERR_FAIL_COND_V_MSG(err != xatlas::AddMeshError::Enum::Success, false, xatlas::StringForEnum(err));

	printf("Generate..\n");
	xatlas::Generate(atlas, chart_options, xatlas::ParameterizeOptions(), pack_options);

	*r_size_hint_x = atlas->width;
	*r_size_hint_y = atlas->height;

	float w = *r_size_hint_x;
	float h = *r_size_hint_y;

	if (w == 0 || h == 0) {
		return false; //could not bake because there is no area
	}

	const xatlas::Mesh &output = atlas->meshes[0];

	*r_vertices = (int *)malloc(sizeof(int) * output.vertexCount);
	*r_uvs = (float *)malloc(sizeof(float) * output.vertexCount * 2);
	*r_indices = (int *)malloc(sizeof(int) * output.indexCount);

	float max_x = 0;
	float max_y = 0;
	for (uint32_t i = 0; i < output.vertexCount; i++) {
		(*r_vertices)[i] = output.vertexArray[i].xref;
		(*r_uvs)[i * 2 + 0] = output.vertexArray[i].uv[0] / w;
		(*r_uvs)[i * 2 + 1] = output.vertexArray[i].uv[1] / h;
		max_x = MAX(max_x, output.vertexArray[i].uv[0]);
		max_y = MAX(max_y, output.vertexArray[i].uv[1]);
	}

	printf("Final texture size: %f,%f - max %f,%f\n", w, h, max_x, max_y);
	*r_vertex_count = output.vertexCount;

	for (uint32_t i = 0; i < output.indexCount; i++) {
		(*r_indices)[i] = output.indexArray[i];
	}

	*r_index_count = output.indexCount;

	xatlas::Destroy(atlas);

	if (r_used_cache) {
		unsigned int new_cache_size = 4 + 2 + 1 + *r_vertex_count + (*r_vertex_count * 2) + 1 + *r_index_count; // hash + size hint + vertex_count + vertices + uvs + index_count + indices
		new_cache_size *= sizeof(int);
		int *new_cache_data = (int *)memalloc(new_cache_size);
		unsigned int new_cache_idx = 0;

		// hash
		memcpy(&new_cache_data[new_cache_idx], hash, 16);
		new_cache_idx += 4;

		// size hint
		new_cache_data[new_cache_idx] = *r_size_hint_x;
		new_cache_data[new_cache_idx + 1] = *r_size_hint_y;
		new_cache_idx += 2;

		// vertex count
		new_cache_data[new_cache_idx] = *r_vertex_count;
		new_cache_idx++;

		// vertices
		memcpy(&new_cache_data[new_cache_idx], *r_vertices, sizeof(int) * *r_vertex_count);
		new_cache_idx += *r_vertex_count;

		// uvs
		memcpy(&new_cache_data[new_cache_idx], *r_uvs, sizeof(float) * *r_vertex_count * 2);
		new_cache_idx += *r_vertex_count * 2;

		// index count
		new_cache_data[new_cache_idx] = *r_index_count;
		new_cache_idx++;

		// indices
		memcpy(&new_cache_data[new_cache_idx], *r_indices, sizeof(int) * *r_index_count);
		new_cache_idx += *r_index_count;

		// Return cache data to the caller
		r_cache_data = new_cache_data;
		r_cache_size = new_cache_size;
		r_used_cache = false;
	}

	return true;
}

void register_xatlas_unwrap_types() {
	array_mesh_lightmap_unwrap_callback = xatlas_mesh_lightmap_unwrap_callback;
}

void unregister_xatlas_unwrap_types() {
}
