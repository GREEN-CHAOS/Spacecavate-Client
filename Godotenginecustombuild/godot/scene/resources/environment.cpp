/*************************************************************************/
/*  environment.cpp                                                      */
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

#include "environment.h"

#include "core/config/project_settings.h"
#include "servers/rendering_server.h"
#include "texture.h"

RID Environment::get_rid() const {
	return environment;
}

// Background

void Environment::set_background(BGMode p_bg) {
	bg_mode = p_bg;
	RS::get_singleton()->environment_set_background(environment, RS::EnvironmentBG(p_bg));
	_change_notify();
	if (bg_mode != BG_SKY) {
		set_fog_aerial_perspective(0.0);
	}
}

Environment::BGMode Environment::get_background() const {
	return bg_mode;
}

void Environment::set_sky(const Ref<Sky> &p_sky) {
	bg_sky = p_sky;
	RID sb_rid;
	if (bg_sky.is_valid()) {
		sb_rid = bg_sky->get_rid();
	}
	RS::get_singleton()->environment_set_sky(environment, sb_rid);
}

Ref<Sky> Environment::get_sky() const {
	return bg_sky;
}

void Environment::set_sky_custom_fov(float p_scale) {
	bg_sky_custom_fov = p_scale;
	RS::get_singleton()->environment_set_sky_custom_fov(environment, p_scale);
}

float Environment::get_sky_custom_fov() const {
	return bg_sky_custom_fov;
}

void Environment::set_sky_rotation(const Vector3 &p_rotation) {
	bg_sky_rotation = p_rotation;
	RS::get_singleton()->environment_set_sky_orientation(environment, Basis(p_rotation));
}

Vector3 Environment::get_sky_rotation() const {
	return bg_sky_rotation;
}

void Environment::set_bg_color(const Color &p_color) {
	bg_color = p_color;
	RS::get_singleton()->environment_set_bg_color(environment, p_color);
}

Color Environment::get_bg_color() const {
	return bg_color;
}

void Environment::set_bg_energy(float p_energy) {
	bg_energy = p_energy;
	RS::get_singleton()->environment_set_bg_energy(environment, p_energy);
}

float Environment::get_bg_energy() const {
	return bg_energy;
}

void Environment::set_canvas_max_layer(int p_max_layer) {
	bg_canvas_max_layer = p_max_layer;
	RS::get_singleton()->environment_set_canvas_max_layer(environment, p_max_layer);
}

int Environment::get_canvas_max_layer() const {
	return bg_canvas_max_layer;
}

void Environment::set_camera_feed_id(int p_id) {
	bg_camera_feed_id = p_id;
// FIXME: Disabled during Vulkan refactoring, should be ported.
#if 0
	RS::get_singleton()->environment_set_camera_feed_id(environment, camera_feed_id);
#endif
}

int Environment::get_camera_feed_id() const {
	return bg_camera_feed_id;
}

// Ambient light

void Environment::set_ambient_light_color(const Color &p_color) {
	ambient_color = p_color;
	_update_ambient_light();
}

Color Environment::get_ambient_light_color() const {
	return ambient_color;
}

void Environment::set_ambient_source(AmbientSource p_source) {
	ambient_source = p_source;
	_update_ambient_light();
	_change_notify();
}

Environment::AmbientSource Environment::get_ambient_source() const {
	return ambient_source;
}

void Environment::set_ambient_light_energy(float p_energy) {
	ambient_energy = p_energy;
	_update_ambient_light();
}

float Environment::get_ambient_light_energy() const {
	return ambient_energy;
}

void Environment::set_ambient_light_sky_contribution(float p_ratio) {
	ambient_sky_contribution = p_ratio;
	_update_ambient_light();
}

float Environment::get_ambient_light_sky_contribution() const {
	return ambient_sky_contribution;
}

void Environment::set_reflection_source(ReflectionSource p_source) {
	reflection_source = p_source;
	_update_ambient_light();
	_change_notify();
}

Environment::ReflectionSource Environment::get_reflection_source() const {
	return reflection_source;
}

void Environment::set_ao_color(const Color &p_color) {
	ao_color = p_color;
	_update_ambient_light();
}

Color Environment::get_ao_color() const {
	return ao_color;
}

void Environment::_update_ambient_light() {
	RS::get_singleton()->environment_set_ambient_light(
			environment,
			ambient_color,
			RS::EnvironmentAmbientSource(ambient_source),
			ambient_energy,
			ambient_sky_contribution, RS::EnvironmentReflectionSource(reflection_source),
			ao_color);
}

// Tonemap

void Environment::set_tonemapper(ToneMapper p_tone_mapper) {
	tone_mapper = p_tone_mapper;
	_update_tonemap();
}

Environment::ToneMapper Environment::get_tonemapper() const {
	return tone_mapper;
}

void Environment::set_tonemap_exposure(float p_exposure) {
	tonemap_exposure = p_exposure;
	_update_tonemap();
}

float Environment::get_tonemap_exposure() const {
	return tonemap_exposure;
}

void Environment::set_tonemap_white(float p_white) {
	tonemap_white = p_white;
	_update_tonemap();
}

float Environment::get_tonemap_white() const {
	return tonemap_white;
}

void Environment::set_tonemap_auto_exposure_enabled(bool p_enabled) {
	tonemap_auto_exposure_enabled = p_enabled;
	_update_tonemap();
	_change_notify();
}

bool Environment::is_tonemap_auto_exposure_enabled() const {
	return tonemap_auto_exposure_enabled;
}

void Environment::set_tonemap_auto_exposure_min(float p_auto_exposure_min) {
	tonemap_auto_exposure_min = p_auto_exposure_min;
	_update_tonemap();
}

float Environment::get_tonemap_auto_exposure_min() const {
	return tonemap_auto_exposure_min;
}

void Environment::set_tonemap_auto_exposure_max(float p_auto_exposure_max) {
	tonemap_auto_exposure_max = p_auto_exposure_max;
	_update_tonemap();
}

float Environment::get_tonemap_auto_exposure_max() const {
	return tonemap_auto_exposure_max;
}

void Environment::set_tonemap_auto_exposure_speed(float p_auto_exposure_speed) {
	tonemap_auto_exposure_speed = p_auto_exposure_speed;
	_update_tonemap();
}

float Environment::get_tonemap_auto_exposure_speed() const {
	return tonemap_auto_exposure_speed;
}

void Environment::set_tonemap_auto_exposure_grey(float p_auto_exposure_grey) {
	tonemap_auto_exposure_grey = p_auto_exposure_grey;
	_update_tonemap();
}

float Environment::get_tonemap_auto_exposure_grey() const {
	return tonemap_auto_exposure_grey;
}

void Environment::_update_tonemap() {
	RS::get_singleton()->environment_set_tonemap(
			environment,
			RS::EnvironmentToneMapper(tone_mapper),
			tonemap_exposure,
			tonemap_white,
			tonemap_auto_exposure_enabled,
			tonemap_auto_exposure_min,
			tonemap_auto_exposure_max,
			tonemap_auto_exposure_speed,
			tonemap_auto_exposure_grey);
}

// SSR

void Environment::set_ssr_enabled(bool p_enabled) {
	ssr_enabled = p_enabled;
	_update_ssr();
	_change_notify();
}

bool Environment::is_ssr_enabled() const {
	return ssr_enabled;
}

void Environment::set_ssr_max_steps(int p_steps) {
	ssr_max_steps = p_steps;
	_update_ssr();
}

int Environment::get_ssr_max_steps() const {
	return ssr_max_steps;
}

void Environment::set_ssr_fade_in(float p_fade_in) {
	ssr_fade_in = p_fade_in;
	_update_ssr();
}

float Environment::get_ssr_fade_in() const {
	return ssr_fade_in;
}

void Environment::set_ssr_fade_out(float p_fade_out) {
	ssr_fade_out = p_fade_out;
	_update_ssr();
}

float Environment::get_ssr_fade_out() const {
	return ssr_fade_out;
}

void Environment::set_ssr_depth_tolerance(float p_depth_tolerance) {
	ssr_depth_tolerance = p_depth_tolerance;
	_update_ssr();
}

float Environment::get_ssr_depth_tolerance() const {
	return ssr_depth_tolerance;
}

void Environment::_update_ssr() {
	RS::get_singleton()->environment_set_ssr(
			environment,
			ssr_enabled,
			ssr_max_steps,
			ssr_fade_in,
			ssr_fade_out,
			ssr_depth_tolerance);
}

// SSAO

void Environment::set_ssao_enabled(bool p_enabled) {
	ssao_enabled = p_enabled;
	_update_ssao();
	_change_notify();
}

bool Environment::is_ssao_enabled() const {
	return ssao_enabled;
}

void Environment::set_ssao_radius(float p_radius) {
	ssao_radius = p_radius;
	_update_ssao();
}

float Environment::get_ssao_radius() const {
	return ssao_radius;
}

void Environment::set_ssao_intensity(float p_intensity) {
	ssao_intensity = p_intensity;
	_update_ssao();
}

float Environment::get_ssao_intensity() const {
	return ssao_intensity;
}

void Environment::set_ssao_bias(float p_bias) {
	ssao_bias = p_bias;
	_update_ssao();
}

float Environment::get_ssao_bias() const {
	return ssao_bias;
}

void Environment::set_ssao_direct_light_affect(float p_direct_light_affect) {
	ssao_direct_light_affect = p_direct_light_affect;
	_update_ssao();
}

float Environment::get_ssao_direct_light_affect() const {
	return ssao_direct_light_affect;
}

void Environment::set_ssao_ao_channel_affect(float p_ao_channel_affect) {
	ssao_ao_channel_affect = p_ao_channel_affect;
	_update_ssao();
}

float Environment::get_ssao_ao_channel_affect() const {
	return ssao_ao_channel_affect;
}

void Environment::set_ssao_blur(SSAOBlur p_blur) {
	ssao_blur = p_blur;
	_update_ssao();
}

Environment::SSAOBlur Environment::get_ssao_blur() const {
	return ssao_blur;
}

void Environment::set_ssao_edge_sharpness(float p_edge_sharpness) {
	ssao_edge_sharpness = p_edge_sharpness;
	_update_ssao();
}

float Environment::get_ssao_edge_sharpness() const {
	return ssao_edge_sharpness;
}

void Environment::_update_ssao() {
	RS::get_singleton()->environment_set_ssao(
			environment,
			ssao_enabled,
			ssao_radius,
			ssao_intensity,
			ssao_bias,
			ssao_direct_light_affect,
			ssao_ao_channel_affect,
			RS::EnvironmentSSAOBlur(ssao_blur),
			ssao_edge_sharpness);
}

// SDFGI

void Environment::set_sdfgi_enabled(bool p_enabled) {
	sdfgi_enabled = p_enabled;
	_update_sdfgi();
}

bool Environment::is_sdfgi_enabled() const {
	return sdfgi_enabled;
}

void Environment::set_sdfgi_cascades(SDFGICascades p_cascades) {
	sdfgi_cascades = p_cascades;
	_update_sdfgi();
}

Environment::SDFGICascades Environment::get_sdfgi_cascades() const {
	return sdfgi_cascades;
}

void Environment::set_sdfgi_min_cell_size(float p_size) {
	sdfgi_min_cell_size = p_size;
	_change_notify("sdfgi_max_distance");
	_change_notify("sdfgi_cascade0_distance");
	_update_sdfgi();
}

float Environment::get_sdfgi_min_cell_size() const {
	return sdfgi_min_cell_size;
}

void Environment::set_sdfgi_max_distance(float p_distance) {
	p_distance /= 64.0;
	int cc[3] = { 4, 6, 8 };
	int cascades = cc[sdfgi_cascades];
	for (int i = 0; i < cascades; i++) {
		p_distance *= 0.5; //halve for each cascade
	}
	sdfgi_min_cell_size = p_distance;
	_change_notify("sdfgi_min_cell_size");
	_change_notify("sdfgi_cascade0_distance");
	_update_sdfgi();
}

float Environment::get_sdfgi_max_distance() const {
	float md = sdfgi_min_cell_size;
	md *= 64.0;
	int cc[3] = { 4, 6, 8 };
	int cascades = cc[sdfgi_cascades];
	for (int i = 0; i < cascades; i++) {
		md *= 2.0;
	}
	return md;
}

void Environment::set_sdfgi_cascade0_distance(float p_distance) {
	sdfgi_min_cell_size = p_distance / 64.0;
	_change_notify("sdfgi_min_cell_size");
	_change_notify("sdfgi_max_distance");
	_update_sdfgi();
}

float Environment::get_sdfgi_cascade0_distance() const {
	return sdfgi_min_cell_size * 64.0;
}

void Environment::set_sdfgi_y_scale(SDFGIYScale p_y_scale) {
	sdfgi_y_scale = p_y_scale;
	_update_sdfgi();
}

Environment::SDFGIYScale Environment::get_sdfgi_y_scale() const {
	return sdfgi_y_scale;
}

void Environment::set_sdfgi_use_occlusion(bool p_enabled) {
	sdfgi_use_occlusion = p_enabled;
	_update_sdfgi();
}

bool Environment::is_sdfgi_using_occlusion() const {
	return sdfgi_use_occlusion;
}

void Environment::set_sdfgi_use_multi_bounce(bool p_enabled) {
	sdfgi_use_multibounce = p_enabled;
	_update_sdfgi();
}

bool Environment::is_sdfgi_using_multi_bounce() const {
	return sdfgi_use_multibounce;
}

void Environment::set_sdfgi_read_sky_light(bool p_enabled) {
	sdfgi_read_sky_light = p_enabled;
	_update_sdfgi();
}

bool Environment::is_sdfgi_reading_sky_light() const {
	return sdfgi_read_sky_light;
}

void Environment::set_sdfgi_energy(float p_energy) {
	sdfgi_energy = p_energy;
	_update_sdfgi();
}

float Environment::get_sdfgi_energy() const {
	return sdfgi_energy;
}

void Environment::set_sdfgi_normal_bias(float p_bias) {
	sdfgi_normal_bias = p_bias;
	_update_sdfgi();
}

float Environment::get_sdfgi_normal_bias() const {
	return sdfgi_normal_bias;
}

void Environment::set_sdfgi_probe_bias(float p_bias) {
	sdfgi_probe_bias = p_bias;
	_update_sdfgi();
}

float Environment::get_sdfgi_probe_bias() const {
	return sdfgi_probe_bias;
}

void Environment::_update_sdfgi() {
	RS::get_singleton()->environment_set_sdfgi(
			environment,
			sdfgi_enabled,
			RS::EnvironmentSDFGICascades(sdfgi_cascades),
			sdfgi_min_cell_size,
			RS::EnvironmentSDFGIYScale(sdfgi_y_scale),
			sdfgi_use_occlusion,
			sdfgi_use_multibounce,
			sdfgi_read_sky_light,
			sdfgi_energy,
			sdfgi_normal_bias,
			sdfgi_probe_bias);
}

// Glow

void Environment::set_glow_enabled(bool p_enabled) {
	glow_enabled = p_enabled;
	_update_glow();
	_change_notify();
}

bool Environment::is_glow_enabled() const {
	return glow_enabled;
}

void Environment::set_glow_level(int p_level, float p_intensity) {
	ERR_FAIL_INDEX(p_level, RS::MAX_GLOW_LEVELS);

	glow_levels.write[p_level] = p_intensity;

	_update_glow();
}

float Environment::get_glow_level(int p_level) const {
	ERR_FAIL_INDEX_V(p_level, RS::MAX_GLOW_LEVELS, false);

	return glow_levels[p_level];
}

void Environment::set_glow_normalized(bool p_normalized) {
	glow_normalize_levels = p_normalized;

	_update_glow();
}

bool Environment::is_glow_normalized() const {
	return glow_normalize_levels;
}

void Environment::set_glow_intensity(float p_intensity) {
	glow_intensity = p_intensity;
	_update_glow();
}

float Environment::get_glow_intensity() const {
	return glow_intensity;
}

void Environment::set_glow_strength(float p_strength) {
	glow_strength = p_strength;
	_update_glow();
}

float Environment::get_glow_strength() const {
	return glow_strength;
}

void Environment::set_glow_mix(float p_mix) {
	glow_mix = p_mix;
	_update_glow();
}

float Environment::get_glow_mix() const {
	return glow_mix;
}

void Environment::set_glow_bloom(float p_threshold) {
	glow_bloom = p_threshold;
	_update_glow();
}

float Environment::get_glow_bloom() const {
	return glow_bloom;
}

void Environment::set_glow_blend_mode(GlowBlendMode p_mode) {
	glow_blend_mode = p_mode;
	_update_glow();
	_change_notify();
}

Environment::GlowBlendMode Environment::get_glow_blend_mode() const {
	return glow_blend_mode;
}

void Environment::set_glow_hdr_bleed_threshold(float p_threshold) {
	glow_hdr_bleed_threshold = p_threshold;
	_update_glow();
}

float Environment::get_glow_hdr_bleed_threshold() const {
	return glow_hdr_bleed_threshold;
}

void Environment::set_glow_hdr_bleed_scale(float p_scale) {
	glow_hdr_bleed_scale = p_scale;
	_update_glow();
}

float Environment::get_glow_hdr_bleed_scale() const {
	return glow_hdr_bleed_scale;
}

void Environment::set_glow_hdr_luminance_cap(float p_amount) {
	glow_hdr_luminance_cap = p_amount;
	_update_glow();
}

float Environment::get_glow_hdr_luminance_cap() const {
	return glow_hdr_luminance_cap;
}

void Environment::_update_glow() {
	Vector<float> normalized_levels;
	if (glow_normalize_levels) {
		normalized_levels.resize(7);
		float size = 0.0;
		for (int i = 0; i < glow_levels.size(); i++) {
			size += glow_levels[i];
		}
		for (int i = 0; i < glow_levels.size(); i++) {
			normalized_levels.write[i] = glow_levels[i] / size;
		}
	} else {
		normalized_levels = glow_levels;
	}

	RS::get_singleton()->environment_set_glow(
			environment,
			glow_enabled,
			normalized_levels,
			glow_intensity,
			glow_strength,
			glow_mix,
			glow_bloom,
			RS::EnvironmentGlowBlendMode(glow_blend_mode),
			glow_hdr_bleed_threshold,
			glow_hdr_bleed_scale,
			glow_hdr_luminance_cap);
}

// Fog

void Environment::set_fog_enabled(bool p_enabled) {
	fog_enabled = p_enabled;
	_update_fog();
	_change_notify();
}

bool Environment::is_fog_enabled() const {
	return fog_enabled;
}

void Environment::set_fog_light_color(const Color &p_light_color) {
	fog_light_color = p_light_color;
	_update_fog();
}
Color Environment::get_fog_light_color() const {
	return fog_light_color;
}
void Environment::set_fog_light_energy(float p_amount) {
	fog_light_energy = p_amount;
	_update_fog();
}
float Environment::get_fog_light_energy() const {
	return fog_light_energy;
}
void Environment::set_fog_sun_scatter(float p_amount) {
	fog_sun_scatter = p_amount;
	_update_fog();
}
float Environment::get_fog_sun_scatter() const {
	return fog_sun_scatter;
}
void Environment::set_fog_density(float p_amount) {
	fog_density = p_amount;
	_update_fog();
}
float Environment::get_fog_density() const {
	return fog_density;
}
void Environment::set_fog_height(float p_amount) {
	fog_height = p_amount;
	_update_fog();
}
float Environment::get_fog_height() const {
	return fog_height;
}
void Environment::set_fog_height_density(float p_amount) {
	fog_height_density = p_amount;
	_update_fog();
}
float Environment::get_fog_height_density() const {
	return fog_height_density;
}

void Environment::set_fog_aerial_perspective(float p_aerial_perspective) {
	fog_aerial_perspective = p_aerial_perspective;
	_update_fog();
}
float Environment::get_fog_aerial_perspective() const {
	return fog_aerial_perspective;
}

void Environment::_update_fog() {
	RS::get_singleton()->environment_set_fog(
			environment,
			fog_enabled,
			fog_light_color,
			fog_light_energy,
			fog_sun_scatter,
			fog_density,
			fog_height,
			fog_height_density,
			fog_aerial_perspective);
}

// Volumetric Fog

void Environment::_update_volumetric_fog() {
	RS::get_singleton()->environment_set_volumetric_fog(environment, volumetric_fog_enabled, volumetric_fog_density, volumetric_fog_light, volumetric_fog_light_energy, volumetric_fog_length, volumetric_fog_detail_spread, volumetric_fog_gi_inject, RS::EnvVolumetricFogShadowFilter(volumetric_fog_shadow_filter));
}

void Environment::set_volumetric_fog_enabled(bool p_enable) {
	volumetric_fog_enabled = p_enable;
	_update_volumetric_fog();
	_change_notify();
}

bool Environment::is_volumetric_fog_enabled() const {
	return volumetric_fog_enabled;
}
void Environment::set_volumetric_fog_density(float p_density) {
	p_density = CLAMP(p_density, 0.0000001, 1.0);
	volumetric_fog_density = p_density;
	_update_volumetric_fog();
}
float Environment::get_volumetric_fog_density() const {
	return volumetric_fog_density;
}
void Environment::set_volumetric_fog_light(Color p_color) {
	volumetric_fog_light = p_color;
	_update_volumetric_fog();
}
Color Environment::get_volumetric_fog_light() const {
	return volumetric_fog_light;
}
void Environment::set_volumetric_fog_light_energy(float p_begin) {
	volumetric_fog_light_energy = p_begin;
	_update_volumetric_fog();
}
float Environment::get_volumetric_fog_light_energy() const {
	return volumetric_fog_light_energy;
}
void Environment::set_volumetric_fog_length(float p_length) {
	volumetric_fog_length = p_length;
	_update_volumetric_fog();
}
float Environment::get_volumetric_fog_length() const {
	return volumetric_fog_length;
}
void Environment::set_volumetric_fog_detail_spread(float p_detail_spread) {
	volumetric_fog_detail_spread = p_detail_spread;
	_update_volumetric_fog();
}
float Environment::get_volumetric_fog_detail_spread() const {
	return volumetric_fog_detail_spread;
}

void Environment::set_volumetric_fog_gi_inject(float p_gi_inject) {
	volumetric_fog_gi_inject = p_gi_inject;
	_update_volumetric_fog();
}
float Environment::get_volumetric_fog_gi_inject() const {
	return volumetric_fog_gi_inject;
}

void Environment::set_volumetric_fog_shadow_filter(VolumetricFogShadowFilter p_filter) {
	volumetric_fog_shadow_filter = p_filter;
	_update_volumetric_fog();
}

Environment::VolumetricFogShadowFilter Environment::get_volumetric_fog_shadow_filter() const {
	return volumetric_fog_shadow_filter;
}

// Adjustment

void Environment::set_adjustment_enabled(bool p_enabled) {
	adjustment_enabled = p_enabled;
	_update_adjustment();
	_change_notify();
}

bool Environment::is_adjustment_enabled() const {
	return adjustment_enabled;
}

void Environment::set_adjustment_brightness(float p_brightness) {
	adjustment_brightness = p_brightness;
	_update_adjustment();
}

float Environment::get_adjustment_brightness() const {
	return adjustment_brightness;
}

void Environment::set_adjustment_contrast(float p_contrast) {
	adjustment_contrast = p_contrast;
	_update_adjustment();
}

float Environment::get_adjustment_contrast() const {
	return adjustment_contrast;
}

void Environment::set_adjustment_saturation(float p_saturation) {
	adjustment_saturation = p_saturation;
	_update_adjustment();
}

float Environment::get_adjustment_saturation() const {
	return adjustment_saturation;
}

void Environment::set_adjustment_color_correction(const Ref<Texture2D> &p_ramp) {
	adjustment_color_correction = p_ramp;
	_update_adjustment();
}

Ref<Texture2D> Environment::get_adjustment_color_correction() const {
	return adjustment_color_correction;
}

void Environment::_update_adjustment() {
	RS::get_singleton()->environment_set_adjustment(
			environment,
			adjustment_enabled,
			adjustment_brightness,
			adjustment_contrast,
			adjustment_saturation,
			adjustment_color_correction.is_valid() ? adjustment_color_correction->get_rid() : RID());
}

// Private methods, constructor and destructor

void Environment::_validate_property(PropertyInfo &property) const {
	if (property.name == "sky" || property.name == "sky_custom_fov" || property.name == "sky_rotation" || property.name == "ambient_light/sky_contribution") {
		if (bg_mode != BG_SKY && ambient_source != AMBIENT_SOURCE_SKY && reflection_source != REFLECTION_SOURCE_SKY) {
			property.usage = PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL;
		}
	}

	if (property.name == "fog_aerial_perspective") {
		if (bg_mode != BG_SKY) {
			property.usage = PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL;
		}
	}

	if (property.name == "glow_intensity" && glow_blend_mode == GLOW_BLEND_MODE_MIX) {
		property.usage = PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL;
	}

	if (property.name == "glow_mix" && glow_blend_mode != GLOW_BLEND_MODE_MIX) {
		property.usage = PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL;
	}

	if (property.name == "background_color") {
		if (bg_mode != BG_COLOR && ambient_source != AMBIENT_SOURCE_COLOR) {
			property.usage = PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL;
		}
	}

	if (property.name == "background_canvas_max_layer") {
		if (bg_mode != BG_CANVAS) {
			property.usage = PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL;
		}
	}

	if (property.name == "background_camera_feed_id") {
		if (bg_mode != BG_CAMERA_FEED) {
			property.usage = PROPERTY_USAGE_NOEDITOR;
		}
	}

	static const char *hide_prefixes[] = {
		"fog_",
		"volumetric_fog_",
		"auto_exposure_",
		"ss_reflections_",
		"ssao_",
		"glow_",
		"adjustment_",
		nullptr

	};

	static const char *high_end_prefixes[] = {
		"auto_exposure_",
		"tonemap_",
		"ss_reflections_",
		"ssao_",
		nullptr

	};

	const char **prefixes = hide_prefixes;
	while (*prefixes) {
		String prefix = String(*prefixes);

		String enabled = prefix + "enabled";
		if (property.name.begins_with(prefix) && property.name != enabled && !bool(get(enabled))) {
			property.usage = PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL;
			return;
		}

		prefixes++;
	}

	if (RenderingServer::get_singleton()->is_low_end()) {
		prefixes = high_end_prefixes;
		while (*prefixes) {
			String prefix = String(*prefixes);

			if (property.name.begins_with(prefix)) {
				property.usage = PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL;
				return;
			}

			prefixes++;
		}
	}
}

#ifndef DISABLE_DEPRECATED
// Kept for compatibility from 3.x to 4.0.
bool Environment::_set(const StringName &p_name, const Variant &p_value) {
	if (p_name == "background_sky") {
		set_sky(p_value);
		return true;
	} else if (p_name == "background_sky_custom_fov") {
		set_sky_custom_fov(p_value);
		return true;
	} else if (p_name == "background_sky_orientation") {
		Vector3 euler = p_value.operator Basis().get_euler();
		set_sky_rotation(euler);
		return true;
	} else {
		return false;
	}
}
#endif

void Environment::_bind_methods() {
	// Background

	ClassDB::bind_method(D_METHOD("set_background", "mode"), &Environment::set_background);
	ClassDB::bind_method(D_METHOD("get_background"), &Environment::get_background);
	ClassDB::bind_method(D_METHOD("set_sky", "sky"), &Environment::set_sky);
	ClassDB::bind_method(D_METHOD("get_sky"), &Environment::get_sky);
	ClassDB::bind_method(D_METHOD("set_sky_custom_fov", "scale"), &Environment::set_sky_custom_fov);
	ClassDB::bind_method(D_METHOD("get_sky_custom_fov"), &Environment::get_sky_custom_fov);
	ClassDB::bind_method(D_METHOD("set_sky_rotation", "euler_radians"), &Environment::set_sky_rotation);
	ClassDB::bind_method(D_METHOD("get_sky_rotation"), &Environment::get_sky_rotation);
	ClassDB::bind_method(D_METHOD("set_bg_color", "color"), &Environment::set_bg_color);
	ClassDB::bind_method(D_METHOD("get_bg_color"), &Environment::get_bg_color);
	ClassDB::bind_method(D_METHOD("set_bg_energy", "energy"), &Environment::set_bg_energy);
	ClassDB::bind_method(D_METHOD("get_bg_energy"), &Environment::get_bg_energy);
	ClassDB::bind_method(D_METHOD("set_canvas_max_layer", "layer"), &Environment::set_canvas_max_layer);
	ClassDB::bind_method(D_METHOD("get_canvas_max_layer"), &Environment::get_canvas_max_layer);
	ClassDB::bind_method(D_METHOD("set_camera_feed_id", "id"), &Environment::set_camera_feed_id);
	ClassDB::bind_method(D_METHOD("get_camera_feed_id"), &Environment::get_camera_feed_id);

	ADD_GROUP("Background", "background_");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "background_mode", PROPERTY_HINT_ENUM, "Clear Color,Custom Color,Sky,Canvas,Keep,Camera Feed"), "set_background", "get_background");
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "background_color"), "set_bg_color", "get_bg_color");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "background_energy", PROPERTY_HINT_RANGE, "0,16,0.01"), "set_bg_energy", "get_bg_energy");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "background_canvas_max_layer", PROPERTY_HINT_RANGE, "-1000,1000,1"), "set_canvas_max_layer", "get_canvas_max_layer");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "background_camera_feed_id", PROPERTY_HINT_RANGE, "1,10,1"), "set_camera_feed_id", "get_camera_feed_id");

	ADD_GROUP("Sky", "sky_");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "sky", PROPERTY_HINT_RESOURCE_TYPE, "Sky"), "set_sky", "get_sky");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "sky_custom_fov", PROPERTY_HINT_RANGE, "0,180,0.1"), "set_sky_custom_fov", "get_sky_custom_fov");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "sky_rotation"), "set_sky_rotation", "get_sky_rotation");

	// Ambient light

	ClassDB::bind_method(D_METHOD("set_ambient_light_color", "color"), &Environment::set_ambient_light_color);
	ClassDB::bind_method(D_METHOD("get_ambient_light_color"), &Environment::get_ambient_light_color);
	ClassDB::bind_method(D_METHOD("set_ambient_source", "source"), &Environment::set_ambient_source);
	ClassDB::bind_method(D_METHOD("get_ambient_source"), &Environment::get_ambient_source);
	ClassDB::bind_method(D_METHOD("set_ambient_light_energy", "energy"), &Environment::set_ambient_light_energy);
	ClassDB::bind_method(D_METHOD("get_ambient_light_energy"), &Environment::get_ambient_light_energy);
	ClassDB::bind_method(D_METHOD("set_ambient_light_sky_contribution", "ratio"), &Environment::set_ambient_light_sky_contribution);
	ClassDB::bind_method(D_METHOD("get_ambient_light_sky_contribution"), &Environment::get_ambient_light_sky_contribution);
	ClassDB::bind_method(D_METHOD("set_reflection_source", "source"), &Environment::set_reflection_source);
	ClassDB::bind_method(D_METHOD("get_reflection_source"), &Environment::get_reflection_source);
	ClassDB::bind_method(D_METHOD("set_ao_color", "color"), &Environment::set_ao_color);
	ClassDB::bind_method(D_METHOD("get_ao_color"), &Environment::get_ao_color);

	ADD_GROUP("Ambient Light", "ambient_light_");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "ambient_light_source", PROPERTY_HINT_ENUM, "Background,Disabled,Color,Sky"), "set_ambient_source", "get_ambient_source");
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "ambient_light_color"), "set_ambient_light_color", "get_ambient_light_color");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "ambient_light_sky_contribution", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_ambient_light_sky_contribution", "get_ambient_light_sky_contribution");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "ambient_light_energy", PROPERTY_HINT_RANGE, "0,16,0.01"), "set_ambient_light_energy", "get_ambient_light_energy");
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "ambient_light_occlusion_color", PROPERTY_HINT_COLOR_NO_ALPHA), "set_ao_color", "get_ao_color");

	ADD_GROUP("Reflected Light", "reflected_light_");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "reflected_light_source", PROPERTY_HINT_ENUM, "Background,Disabled,Sky"), "set_reflection_source", "get_reflection_source");

	// Tonemap

	ClassDB::bind_method(D_METHOD("set_tonemapper", "mode"), &Environment::set_tonemapper);
	ClassDB::bind_method(D_METHOD("get_tonemapper"), &Environment::get_tonemapper);
	ClassDB::bind_method(D_METHOD("set_tonemap_exposure", "exposure"), &Environment::set_tonemap_exposure);
	ClassDB::bind_method(D_METHOD("get_tonemap_exposure"), &Environment::get_tonemap_exposure);
	ClassDB::bind_method(D_METHOD("set_tonemap_white", "white"), &Environment::set_tonemap_white);
	ClassDB::bind_method(D_METHOD("get_tonemap_white"), &Environment::get_tonemap_white);
	ClassDB::bind_method(D_METHOD("set_tonemap_auto_exposure_enabled", "enabled"), &Environment::set_tonemap_auto_exposure_enabled);
	ClassDB::bind_method(D_METHOD("is_tonemap_auto_exposure_enabled"), &Environment::is_tonemap_auto_exposure_enabled);
	ClassDB::bind_method(D_METHOD("set_tonemap_auto_exposure_max", "exposure_max"), &Environment::set_tonemap_auto_exposure_max);
	ClassDB::bind_method(D_METHOD("get_tonemap_auto_exposure_max"), &Environment::get_tonemap_auto_exposure_max);
	ClassDB::bind_method(D_METHOD("set_tonemap_auto_exposure_min", "exposure_min"), &Environment::set_tonemap_auto_exposure_min);
	ClassDB::bind_method(D_METHOD("get_tonemap_auto_exposure_min"), &Environment::get_tonemap_auto_exposure_min);
	ClassDB::bind_method(D_METHOD("set_tonemap_auto_exposure_speed", "exposure_speed"), &Environment::set_tonemap_auto_exposure_speed);
	ClassDB::bind_method(D_METHOD("get_tonemap_auto_exposure_speed"), &Environment::get_tonemap_auto_exposure_speed);
	ClassDB::bind_method(D_METHOD("set_tonemap_auto_exposure_grey", "exposure_grey"), &Environment::set_tonemap_auto_exposure_grey);
	ClassDB::bind_method(D_METHOD("get_tonemap_auto_exposure_grey"), &Environment::get_tonemap_auto_exposure_grey);

	ADD_GROUP("Tonemap", "tonemap_");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "tonemap_mode", PROPERTY_HINT_ENUM, "Linear,Reinhard,Filmic,ACES"), "set_tonemapper", "get_tonemapper");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "tonemap_exposure", PROPERTY_HINT_RANGE, "0,16,0.01"), "set_tonemap_exposure", "get_tonemap_exposure");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "tonemap_white", PROPERTY_HINT_RANGE, "0,16,0.01"), "set_tonemap_white", "get_tonemap_white");
	ADD_GROUP("Auto Exposure", "auto_exposure_");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_exposure_enabled"), "set_tonemap_auto_exposure_enabled", "is_tonemap_auto_exposure_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "auto_exposure_scale", PROPERTY_HINT_RANGE, "0.01,64,0.01"), "set_tonemap_auto_exposure_grey", "get_tonemap_auto_exposure_grey");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "auto_exposure_min_luma", PROPERTY_HINT_RANGE, "0,16,0.01"), "set_tonemap_auto_exposure_min", "get_tonemap_auto_exposure_min");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "auto_exposure_max_luma", PROPERTY_HINT_RANGE, "0,16,0.01"), "set_tonemap_auto_exposure_max", "get_tonemap_auto_exposure_max");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "auto_exposure_speed", PROPERTY_HINT_RANGE, "0.01,64,0.01"), "set_tonemap_auto_exposure_speed", "get_tonemap_auto_exposure_speed");

	// SSR

	ClassDB::bind_method(D_METHOD("set_ssr_enabled", "enabled"), &Environment::set_ssr_enabled);
	ClassDB::bind_method(D_METHOD("is_ssr_enabled"), &Environment::is_ssr_enabled);
	ClassDB::bind_method(D_METHOD("set_ssr_max_steps", "max_steps"), &Environment::set_ssr_max_steps);
	ClassDB::bind_method(D_METHOD("get_ssr_max_steps"), &Environment::get_ssr_max_steps);
	ClassDB::bind_method(D_METHOD("set_ssr_fade_in", "fade_in"), &Environment::set_ssr_fade_in);
	ClassDB::bind_method(D_METHOD("get_ssr_fade_in"), &Environment::get_ssr_fade_in);
	ClassDB::bind_method(D_METHOD("set_ssr_fade_out", "fade_out"), &Environment::set_ssr_fade_out);
	ClassDB::bind_method(D_METHOD("get_ssr_fade_out"), &Environment::get_ssr_fade_out);
	ClassDB::bind_method(D_METHOD("set_ssr_depth_tolerance", "depth_tolerance"), &Environment::set_ssr_depth_tolerance);
	ClassDB::bind_method(D_METHOD("get_ssr_depth_tolerance"), &Environment::get_ssr_depth_tolerance);

	ADD_GROUP("SS Reflections", "ss_reflections_");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "ss_reflections_enabled"), "set_ssr_enabled", "is_ssr_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "ss_reflections_max_steps", PROPERTY_HINT_RANGE, "1,512,1"), "set_ssr_max_steps", "get_ssr_max_steps");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "ss_reflections_fade_in", PROPERTY_HINT_EXP_EASING), "set_ssr_fade_in", "get_ssr_fade_in");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "ss_reflections_fade_out", PROPERTY_HINT_EXP_EASING), "set_ssr_fade_out", "get_ssr_fade_out");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "ss_reflections_depth_tolerance", PROPERTY_HINT_RANGE, "0.01,128,0.1"), "set_ssr_depth_tolerance", "get_ssr_depth_tolerance");

	// SSAO

	ClassDB::bind_method(D_METHOD("set_ssao_enabled", "enabled"), &Environment::set_ssao_enabled);
	ClassDB::bind_method(D_METHOD("is_ssao_enabled"), &Environment::is_ssao_enabled);
	ClassDB::bind_method(D_METHOD("set_ssao_radius", "radius"), &Environment::set_ssao_radius);
	ClassDB::bind_method(D_METHOD("get_ssao_radius"), &Environment::get_ssao_radius);
	ClassDB::bind_method(D_METHOD("set_ssao_intensity", "intensity"), &Environment::set_ssao_intensity);
	ClassDB::bind_method(D_METHOD("get_ssao_intensity"), &Environment::get_ssao_intensity);
	ClassDB::bind_method(D_METHOD("set_ssao_bias", "bias"), &Environment::set_ssao_bias);
	ClassDB::bind_method(D_METHOD("get_ssao_bias"), &Environment::get_ssao_bias);
	ClassDB::bind_method(D_METHOD("set_ssao_direct_light_affect", "amount"), &Environment::set_ssao_direct_light_affect);
	ClassDB::bind_method(D_METHOD("get_ssao_direct_light_affect"), &Environment::get_ssao_direct_light_affect);
	ClassDB::bind_method(D_METHOD("set_ssao_ao_channel_affect", "amount"), &Environment::set_ssao_ao_channel_affect);
	ClassDB::bind_method(D_METHOD("get_ssao_ao_channel_affect"), &Environment::get_ssao_ao_channel_affect);
	ClassDB::bind_method(D_METHOD("set_ssao_blur", "mode"), &Environment::set_ssao_blur);
	ClassDB::bind_method(D_METHOD("get_ssao_blur"), &Environment::get_ssao_blur);
	ClassDB::bind_method(D_METHOD("set_ssao_edge_sharpness", "edge_sharpness"), &Environment::set_ssao_edge_sharpness);
	ClassDB::bind_method(D_METHOD("get_ssao_edge_sharpness"), &Environment::get_ssao_edge_sharpness);

	ADD_GROUP("SSAO", "ssao_");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "ssao_enabled"), "set_ssao_enabled", "is_ssao_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "ssao_radius", PROPERTY_HINT_RANGE, "0.1,128,0.01"), "set_ssao_radius", "get_ssao_radius");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "ssao_intensity", PROPERTY_HINT_RANGE, "0.0,128,0.01"), "set_ssao_intensity", "get_ssao_intensity");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "ssao_bias", PROPERTY_HINT_RANGE, "0.001,8,0.001"), "set_ssao_bias", "get_ssao_bias");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "ssao_light_affect", PROPERTY_HINT_RANGE, "0.00,1,0.01"), "set_ssao_direct_light_affect", "get_ssao_direct_light_affect");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "ssao_ao_channel_affect", PROPERTY_HINT_RANGE, "0.00,1,0.01"), "set_ssao_ao_channel_affect", "get_ssao_ao_channel_affect");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "ssao_blur", PROPERTY_HINT_ENUM, "Disabled,1x1,2x2,3x3"), "set_ssao_blur", "get_ssao_blur");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "ssao_edge_sharpness", PROPERTY_HINT_RANGE, "0,32,0.01"), "set_ssao_edge_sharpness", "get_ssao_edge_sharpness");

	// SDFGI

	ClassDB::bind_method(D_METHOD("set_sdfgi_enabled", "enabled"), &Environment::set_sdfgi_enabled);
	ClassDB::bind_method(D_METHOD("is_sdfgi_enabled"), &Environment::is_sdfgi_enabled);
	ClassDB::bind_method(D_METHOD("set_sdfgi_cascades", "amount"), &Environment::set_sdfgi_cascades);
	ClassDB::bind_method(D_METHOD("get_sdfgi_cascades"), &Environment::get_sdfgi_cascades);
	ClassDB::bind_method(D_METHOD("set_sdfgi_min_cell_size", "size"), &Environment::set_sdfgi_min_cell_size);
	ClassDB::bind_method(D_METHOD("get_sdfgi_min_cell_size"), &Environment::get_sdfgi_min_cell_size);
	ClassDB::bind_method(D_METHOD("set_sdfgi_max_distance", "distance"), &Environment::set_sdfgi_max_distance);
	ClassDB::bind_method(D_METHOD("get_sdfgi_max_distance"), &Environment::get_sdfgi_max_distance);
	ClassDB::bind_method(D_METHOD("set_sdfgi_cascade0_distance", "distance"), &Environment::set_sdfgi_cascade0_distance);
	ClassDB::bind_method(D_METHOD("get_sdfgi_cascade0_distance"), &Environment::get_sdfgi_cascade0_distance);
	ClassDB::bind_method(D_METHOD("set_sdfgi_y_scale", "scale"), &Environment::set_sdfgi_y_scale);
	ClassDB::bind_method(D_METHOD("get_sdfgi_y_scale"), &Environment::get_sdfgi_y_scale);
	ClassDB::bind_method(D_METHOD("set_sdfgi_use_occlusion", "enable"), &Environment::set_sdfgi_use_occlusion);
	ClassDB::bind_method(D_METHOD("is_sdfgi_using_occlusion"), &Environment::is_sdfgi_using_occlusion);
	ClassDB::bind_method(D_METHOD("set_sdfgi_use_multi_bounce", "enable"), &Environment::set_sdfgi_use_multi_bounce);
	ClassDB::bind_method(D_METHOD("is_sdfgi_using_multi_bounce"), &Environment::is_sdfgi_using_multi_bounce);
	ClassDB::bind_method(D_METHOD("set_sdfgi_read_sky_light", "enable"), &Environment::set_sdfgi_read_sky_light);
	ClassDB::bind_method(D_METHOD("is_sdfgi_reading_sky_light"), &Environment::is_sdfgi_reading_sky_light);
	ClassDB::bind_method(D_METHOD("set_sdfgi_energy", "amount"), &Environment::set_sdfgi_energy);
	ClassDB::bind_method(D_METHOD("get_sdfgi_energy"), &Environment::get_sdfgi_energy);
	ClassDB::bind_method(D_METHOD("set_sdfgi_normal_bias", "bias"), &Environment::set_sdfgi_normal_bias);
	ClassDB::bind_method(D_METHOD("get_sdfgi_normal_bias"), &Environment::get_sdfgi_normal_bias);
	ClassDB::bind_method(D_METHOD("set_sdfgi_probe_bias", "bias"), &Environment::set_sdfgi_probe_bias);
	ClassDB::bind_method(D_METHOD("get_sdfgi_probe_bias"), &Environment::get_sdfgi_probe_bias);

	ADD_GROUP("SDFGI", "sdfgi_");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "sdfgi_enabled"), "set_sdfgi_enabled", "is_sdfgi_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "sdfgi_use_multi_bounce"), "set_sdfgi_use_multi_bounce", "is_sdfgi_using_multi_bounce");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "sdfgi_use_occlusion"), "set_sdfgi_use_occlusion", "is_sdfgi_using_occlusion");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "sdfgi_read_sky_light"), "set_sdfgi_read_sky_light", "is_sdfgi_reading_sky_light");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "sdfgi_cascades", PROPERTY_HINT_ENUM, "4 Cascades,6 Cascades,8 Cascades"), "set_sdfgi_cascades", "get_sdfgi_cascades");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "sdfgi_min_cell_size", PROPERTY_HINT_RANGE, "0.01,64,0.01"), "set_sdfgi_min_cell_size", "get_sdfgi_min_cell_size");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "sdfgi_cascade0_distance", PROPERTY_HINT_RANGE, "0.1,16384,0.1,or_greater"), "set_sdfgi_cascade0_distance", "get_sdfgi_cascade0_distance");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "sdfgi_max_distance", PROPERTY_HINT_RANGE, "0.1,16384,0.1,or_greater"), "set_sdfgi_max_distance", "get_sdfgi_max_distance");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "sdfgi_y_scale", PROPERTY_HINT_ENUM, "Disable,75%,50%"), "set_sdfgi_y_scale", "get_sdfgi_y_scale");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "sdfgi_energy"), "set_sdfgi_energy", "get_sdfgi_energy");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "sdfgi_normal_bias"), "set_sdfgi_normal_bias", "get_sdfgi_normal_bias");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "sdfgi_probe_bias"), "set_sdfgi_probe_bias", "get_sdfgi_probe_bias");

	// Glow

	ClassDB::bind_method(D_METHOD("set_glow_enabled", "enabled"), &Environment::set_glow_enabled);
	ClassDB::bind_method(D_METHOD("is_glow_enabled"), &Environment::is_glow_enabled);
	ClassDB::bind_method(D_METHOD("set_glow_level", "idx", "intensity"), &Environment::set_glow_level);
	ClassDB::bind_method(D_METHOD("get_glow_level", "idx"), &Environment::get_glow_level);
	ClassDB::bind_method(D_METHOD("set_glow_normalized", "normalize"), &Environment::set_glow_normalized);
	ClassDB::bind_method(D_METHOD("is_glow_normalized"), &Environment::is_glow_normalized);
	ClassDB::bind_method(D_METHOD("set_glow_intensity", "intensity"), &Environment::set_glow_intensity);
	ClassDB::bind_method(D_METHOD("get_glow_intensity"), &Environment::get_glow_intensity);
	ClassDB::bind_method(D_METHOD("set_glow_strength", "strength"), &Environment::set_glow_strength);
	ClassDB::bind_method(D_METHOD("get_glow_strength"), &Environment::get_glow_strength);
	ClassDB::bind_method(D_METHOD("set_glow_mix", "mix"), &Environment::set_glow_mix);
	ClassDB::bind_method(D_METHOD("get_glow_mix"), &Environment::get_glow_mix);
	ClassDB::bind_method(D_METHOD("set_glow_bloom", "amount"), &Environment::set_glow_bloom);
	ClassDB::bind_method(D_METHOD("get_glow_bloom"), &Environment::get_glow_bloom);
	ClassDB::bind_method(D_METHOD("set_glow_blend_mode", "mode"), &Environment::set_glow_blend_mode);
	ClassDB::bind_method(D_METHOD("get_glow_blend_mode"), &Environment::get_glow_blend_mode);
	ClassDB::bind_method(D_METHOD("set_glow_hdr_bleed_threshold", "threshold"), &Environment::set_glow_hdr_bleed_threshold);
	ClassDB::bind_method(D_METHOD("get_glow_hdr_bleed_threshold"), &Environment::get_glow_hdr_bleed_threshold);
	ClassDB::bind_method(D_METHOD("set_glow_hdr_bleed_scale", "scale"), &Environment::set_glow_hdr_bleed_scale);
	ClassDB::bind_method(D_METHOD("get_glow_hdr_bleed_scale"), &Environment::get_glow_hdr_bleed_scale);
	ClassDB::bind_method(D_METHOD("set_glow_hdr_luminance_cap", "amount"), &Environment::set_glow_hdr_luminance_cap);
	ClassDB::bind_method(D_METHOD("get_glow_hdr_luminance_cap"), &Environment::get_glow_hdr_luminance_cap);

	ADD_GROUP("Glow", "glow_");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "glow_enabled"), "set_glow_enabled", "is_glow_enabled");
	ADD_PROPERTYI(PropertyInfo(Variant::FLOAT, "glow_levels/1", PROPERTY_HINT_RANGE, "0,16,0.01,or_greater"), "set_glow_level", "get_glow_level", 0);
	ADD_PROPERTYI(PropertyInfo(Variant::FLOAT, "glow_levels/2", PROPERTY_HINT_RANGE, "0,16,0.01,or_greater"), "set_glow_level", "get_glow_level", 1);
	ADD_PROPERTYI(PropertyInfo(Variant::FLOAT, "glow_levels/3", PROPERTY_HINT_RANGE, "0,16,0.01,or_greater"), "set_glow_level", "get_glow_level", 2);
	ADD_PROPERTYI(PropertyInfo(Variant::FLOAT, "glow_levels/4", PROPERTY_HINT_RANGE, "0,16,0.01,or_greater"), "set_glow_level", "get_glow_level", 3);
	ADD_PROPERTYI(PropertyInfo(Variant::FLOAT, "glow_levels/5", PROPERTY_HINT_RANGE, "0,16,0.01,or_greater"), "set_glow_level", "get_glow_level", 4);
	ADD_PROPERTYI(PropertyInfo(Variant::FLOAT, "glow_levels/6", PROPERTY_HINT_RANGE, "0,16,0.01,or_greater"), "set_glow_level", "get_glow_level", 5);
	ADD_PROPERTYI(PropertyInfo(Variant::FLOAT, "glow_levels/7", PROPERTY_HINT_RANGE, "0,16,0.01,or_greater"), "set_glow_level", "get_glow_level", 6);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "glow_normalized"), "set_glow_normalized", "is_glow_normalized");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "glow_intensity", PROPERTY_HINT_RANGE, "0.0,8.0,0.01"), "set_glow_intensity", "get_glow_intensity");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "glow_strength", PROPERTY_HINT_RANGE, "0.0,2.0,0.01"), "set_glow_strength", "get_glow_strength");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "glow_mix", PROPERTY_HINT_RANGE, "0.0,1.0,0.001"), "set_glow_mix", "get_glow_mix");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "glow_bloom", PROPERTY_HINT_RANGE, "0.0,1.0,0.01"), "set_glow_bloom", "get_glow_bloom");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "glow_blend_mode", PROPERTY_HINT_ENUM, "Additive,Screen,Softlight,Replace,Mix"), "set_glow_blend_mode", "get_glow_blend_mode");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "glow_hdr_threshold", PROPERTY_HINT_RANGE, "0.0,4.0,0.01"), "set_glow_hdr_bleed_threshold", "get_glow_hdr_bleed_threshold");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "glow_hdr_scale", PROPERTY_HINT_RANGE, "0.0,4.0,0.01"), "set_glow_hdr_bleed_scale", "get_glow_hdr_bleed_scale");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "glow_hdr_luminance_cap", PROPERTY_HINT_RANGE, "0.0,256.0,0.01"), "set_glow_hdr_luminance_cap", "get_glow_hdr_luminance_cap");

	// Fog

	ClassDB::bind_method(D_METHOD("set_fog_enabled", "enabled"), &Environment::set_fog_enabled);
	ClassDB::bind_method(D_METHOD("is_fog_enabled"), &Environment::is_fog_enabled);
	ClassDB::bind_method(D_METHOD("set_fog_light_color", "light_color"), &Environment::set_fog_light_color);
	ClassDB::bind_method(D_METHOD("get_fog_light_color"), &Environment::get_fog_light_color);
	ClassDB::bind_method(D_METHOD("set_fog_light_energy", "light_energy"), &Environment::set_fog_light_energy);
	ClassDB::bind_method(D_METHOD("get_fog_light_energy"), &Environment::get_fog_light_energy);
	ClassDB::bind_method(D_METHOD("set_fog_sun_scatter", "sun_scatter"), &Environment::set_fog_sun_scatter);
	ClassDB::bind_method(D_METHOD("get_fog_sun_scatter"), &Environment::get_fog_sun_scatter);

	ClassDB::bind_method(D_METHOD("set_fog_density", "density"), &Environment::set_fog_density);
	ClassDB::bind_method(D_METHOD("get_fog_density"), &Environment::get_fog_density);

	ClassDB::bind_method(D_METHOD("set_fog_height", "height"), &Environment::set_fog_height);
	ClassDB::bind_method(D_METHOD("get_fog_height"), &Environment::get_fog_height);

	ClassDB::bind_method(D_METHOD("set_fog_height_density", "height_density"), &Environment::set_fog_height_density);
	ClassDB::bind_method(D_METHOD("get_fog_height_density"), &Environment::get_fog_height_density);

	ClassDB::bind_method(D_METHOD("set_fog_aerial_perspective", "aerial_perspective"), &Environment::set_fog_aerial_perspective);
	ClassDB::bind_method(D_METHOD("get_fog_aerial_perspective"), &Environment::get_fog_aerial_perspective);

	ADD_GROUP("Fog", "fog_");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "fog_enabled"), "set_fog_enabled", "is_fog_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "fog_light_color", PROPERTY_HINT_COLOR_NO_ALPHA), "set_fog_light_color", "get_fog_light_color");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "fog_light_energy", PROPERTY_HINT_RANGE, "0,16,0.01,or_greater"), "set_fog_light_energy", "get_fog_light_energy");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "fog_sun_scatter", PROPERTY_HINT_RANGE, "0,1,0.01,or_greater"), "set_fog_sun_scatter", "get_fog_sun_scatter");

	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "fog_density", PROPERTY_HINT_RANGE, "0,16,0.0001"), "set_fog_density", "get_fog_density");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "fog_aerial_perspective", PROPERTY_HINT_RANGE, "0,1,0.001"), "set_fog_aerial_perspective", "get_fog_aerial_perspective");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "fog_height", PROPERTY_HINT_RANGE, "-1024,1024,0.01,or_lesser,or_greater"), "set_fog_height", "get_fog_height");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "fog_height_density", PROPERTY_HINT_RANGE, "0,128,0.001,or_greater"), "set_fog_height_density", "get_fog_height_density");

	ClassDB::bind_method(D_METHOD("set_volumetric_fog_enabled", "enabled"), &Environment::set_volumetric_fog_enabled);
	ClassDB::bind_method(D_METHOD("is_volumetric_fog_enabled"), &Environment::is_volumetric_fog_enabled);
	ClassDB::bind_method(D_METHOD("set_volumetric_fog_light", "color"), &Environment::set_volumetric_fog_light);
	ClassDB::bind_method(D_METHOD("get_volumetric_fog_light"), &Environment::get_volumetric_fog_light);
	ClassDB::bind_method(D_METHOD("set_volumetric_fog_density", "density"), &Environment::set_volumetric_fog_density);
	ClassDB::bind_method(D_METHOD("get_volumetric_fog_density"), &Environment::get_volumetric_fog_density);
	ClassDB::bind_method(D_METHOD("set_volumetric_fog_light_energy", "begin"), &Environment::set_volumetric_fog_light_energy);
	ClassDB::bind_method(D_METHOD("get_volumetric_fog_light_energy"), &Environment::get_volumetric_fog_light_energy);
	ClassDB::bind_method(D_METHOD("set_volumetric_fog_length", "length"), &Environment::set_volumetric_fog_length);
	ClassDB::bind_method(D_METHOD("get_volumetric_fog_length"), &Environment::get_volumetric_fog_length);
	ClassDB::bind_method(D_METHOD("set_volumetric_fog_detail_spread", "detail_spread"), &Environment::set_volumetric_fog_detail_spread);
	ClassDB::bind_method(D_METHOD("get_volumetric_fog_detail_spread"), &Environment::get_volumetric_fog_detail_spread);
	ClassDB::bind_method(D_METHOD("set_volumetric_fog_gi_inject", "gi_inject"), &Environment::set_volumetric_fog_gi_inject);
	ClassDB::bind_method(D_METHOD("get_volumetric_fog_gi_inject"), &Environment::get_volumetric_fog_gi_inject);
	ClassDB::bind_method(D_METHOD("set_volumetric_fog_shadow_filter", "shadow_filter"), &Environment::set_volumetric_fog_shadow_filter);
	ClassDB::bind_method(D_METHOD("get_volumetric_fog_shadow_filter"), &Environment::get_volumetric_fog_shadow_filter);

	ADD_GROUP("Volumetric Fog", "volumetric_fog_");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "volumetric_fog_enabled"), "set_volumetric_fog_enabled", "is_volumetric_fog_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "volumetric_fog_density", PROPERTY_HINT_RANGE, "0,1,0.0001,or_greater"), "set_volumetric_fog_density", "get_volumetric_fog_density");
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "volumetric_fog_light", PROPERTY_HINT_COLOR_NO_ALPHA), "set_volumetric_fog_light", "get_volumetric_fog_light");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "volumetric_fog_light_energy", PROPERTY_HINT_RANGE, "0,1024,0.01,or_greater"), "set_volumetric_fog_light_energy", "get_volumetric_fog_light_energy");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "volumetric_fog_gi_inject", PROPERTY_HINT_EXP_RANGE, "0.00,16,0.01"), "set_volumetric_fog_gi_inject", "get_volumetric_fog_gi_inject");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "volumetric_fog_length", PROPERTY_HINT_RANGE, "0,1024,0.01,or_greater"), "set_volumetric_fog_length", "get_volumetric_fog_length");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "volumetric_fog_detail_spread", PROPERTY_HINT_EXP_EASING, "0.01,16,0.01"), "set_volumetric_fog_detail_spread", "get_volumetric_fog_detail_spread");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "volumetric_fog_shadow_filter", PROPERTY_HINT_ENUM, "Disabled,Low,Medium,High"), "set_volumetric_fog_shadow_filter", "get_volumetric_fog_shadow_filter");

	// Adjustment

	ClassDB::bind_method(D_METHOD("set_adjustment_enabled", "enabled"), &Environment::set_adjustment_enabled);
	ClassDB::bind_method(D_METHOD("is_adjustment_enabled"), &Environment::is_adjustment_enabled);
	ClassDB::bind_method(D_METHOD("set_adjustment_brightness", "brightness"), &Environment::set_adjustment_brightness);
	ClassDB::bind_method(D_METHOD("get_adjustment_brightness"), &Environment::get_adjustment_brightness);
	ClassDB::bind_method(D_METHOD("set_adjustment_contrast", "contrast"), &Environment::set_adjustment_contrast);
	ClassDB::bind_method(D_METHOD("get_adjustment_contrast"), &Environment::get_adjustment_contrast);
	ClassDB::bind_method(D_METHOD("set_adjustment_saturation", "saturation"), &Environment::set_adjustment_saturation);
	ClassDB::bind_method(D_METHOD("get_adjustment_saturation"), &Environment::get_adjustment_saturation);
	ClassDB::bind_method(D_METHOD("set_adjustment_color_correction", "color_correction"), &Environment::set_adjustment_color_correction);
	ClassDB::bind_method(D_METHOD("get_adjustment_color_correction"), &Environment::get_adjustment_color_correction);

	ADD_GROUP("Adjustments", "adjustment_");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "adjustment_enabled"), "set_adjustment_enabled", "is_adjustment_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "adjustment_brightness", PROPERTY_HINT_RANGE, "0.01,8,0.01"), "set_adjustment_brightness", "get_adjustment_brightness");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "adjustment_contrast", PROPERTY_HINT_RANGE, "0.01,8,0.01"), "set_adjustment_contrast", "get_adjustment_contrast");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "adjustment_saturation", PROPERTY_HINT_RANGE, "0.01,8,0.01"), "set_adjustment_saturation", "get_adjustment_saturation");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "adjustment_color_correction", PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"), "set_adjustment_color_correction", "get_adjustment_color_correction");

	// Constants

	BIND_ENUM_CONSTANT(BG_CLEAR_COLOR);
	BIND_ENUM_CONSTANT(BG_COLOR);
	BIND_ENUM_CONSTANT(BG_SKY);
	BIND_ENUM_CONSTANT(BG_CANVAS);
	BIND_ENUM_CONSTANT(BG_KEEP);
	BIND_ENUM_CONSTANT(BG_CAMERA_FEED);
	BIND_ENUM_CONSTANT(BG_MAX);

	BIND_ENUM_CONSTANT(AMBIENT_SOURCE_BG);
	BIND_ENUM_CONSTANT(AMBIENT_SOURCE_DISABLED);
	BIND_ENUM_CONSTANT(AMBIENT_SOURCE_COLOR);
	BIND_ENUM_CONSTANT(AMBIENT_SOURCE_SKY);

	BIND_ENUM_CONSTANT(REFLECTION_SOURCE_BG);
	BIND_ENUM_CONSTANT(REFLECTION_SOURCE_DISABLED);
	BIND_ENUM_CONSTANT(REFLECTION_SOURCE_SKY);

	BIND_ENUM_CONSTANT(TONE_MAPPER_LINEAR);
	BIND_ENUM_CONSTANT(TONE_MAPPER_REINHARDT);
	BIND_ENUM_CONSTANT(TONE_MAPPER_FILMIC);
	BIND_ENUM_CONSTANT(TONE_MAPPER_ACES);

	BIND_ENUM_CONSTANT(GLOW_BLEND_MODE_ADDITIVE);
	BIND_ENUM_CONSTANT(GLOW_BLEND_MODE_SCREEN);
	BIND_ENUM_CONSTANT(GLOW_BLEND_MODE_SOFTLIGHT);
	BIND_ENUM_CONSTANT(GLOW_BLEND_MODE_REPLACE);
	BIND_ENUM_CONSTANT(GLOW_BLEND_MODE_MIX);

	BIND_ENUM_CONSTANT(SSAO_BLUR_DISABLED);
	BIND_ENUM_CONSTANT(SSAO_BLUR_1x1);
	BIND_ENUM_CONSTANT(SSAO_BLUR_2x2);
	BIND_ENUM_CONSTANT(SSAO_BLUR_3x3);

	BIND_ENUM_CONSTANT(SDFGI_CASCADES_4);
	BIND_ENUM_CONSTANT(SDFGI_CASCADES_6);
	BIND_ENUM_CONSTANT(SDFGI_CASCADES_8);

	BIND_ENUM_CONSTANT(SDFGI_Y_SCALE_DISABLED);
	BIND_ENUM_CONSTANT(SDFGI_Y_SCALE_75_PERCENT);
	BIND_ENUM_CONSTANT(SDFGI_Y_SCALE_50_PERCENT);

	BIND_ENUM_CONSTANT(VOLUMETRIC_FOG_SHADOW_FILTER_DISABLED);
	BIND_ENUM_CONSTANT(VOLUMETRIC_FOG_SHADOW_FILTER_LOW);
	BIND_ENUM_CONSTANT(VOLUMETRIC_FOG_SHADOW_FILTER_MEDIUM);
	BIND_ENUM_CONSTANT(VOLUMETRIC_FOG_SHADOW_FILTER_HIGH);
}

Environment::Environment() {
	environment = RS::get_singleton()->environment_create();

	set_camera_feed_id(bg_camera_feed_id);

	glow_levels.resize(7);
	glow_levels.write[0] = 0.0;
	glow_levels.write[1] = 0.0;
	glow_levels.write[2] = 1.0;
	glow_levels.write[3] = 0.0;
	glow_levels.write[4] = 1.0;
	glow_levels.write[5] = 0.0;
	glow_levels.write[6] = 0.0;

	_update_ambient_light();
	_update_tonemap();
	_update_ssr();
	_update_ssao();
	_update_sdfgi();
	_update_glow();
	_update_fog();
	_update_adjustment();
	_update_volumetric_fog();
	_change_notify();
}

Environment::~Environment() {
	RS::get_singleton()->free(environment);
}
