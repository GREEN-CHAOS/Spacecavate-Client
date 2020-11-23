/*************************************************************************/
/*  environment.h                                                        */
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

#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include "core/io/resource.h"
#include "scene/resources/sky.h"
#include "scene/resources/texture.h"
#include "servers/rendering_server.h"

class Environment : public Resource {
	GDCLASS(Environment, Resource);

public:
	enum BGMode {
		BG_CLEAR_COLOR,
		BG_COLOR,
		BG_SKY,
		BG_CANVAS,
		BG_KEEP,
		BG_CAMERA_FEED,
		BG_MAX
	};

	enum AmbientSource {
		AMBIENT_SOURCE_BG,
		AMBIENT_SOURCE_DISABLED,
		AMBIENT_SOURCE_COLOR,
		AMBIENT_SOURCE_SKY,
	};

	enum ReflectionSource {
		REFLECTION_SOURCE_BG,
		REFLECTION_SOURCE_DISABLED,
		REFLECTION_SOURCE_SKY,
	};

	enum ToneMapper {
		TONE_MAPPER_LINEAR,
		TONE_MAPPER_REINHARDT,
		TONE_MAPPER_FILMIC,
		TONE_MAPPER_ACES,
	};

	enum SSAOBlur {
		SSAO_BLUR_DISABLED,
		SSAO_BLUR_1x1,
		SSAO_BLUR_2x2,
		SSAO_BLUR_3x3,
	};

	enum SDFGICascades {
		SDFGI_CASCADES_4,
		SDFGI_CASCADES_6,
		SDFGI_CASCADES_8,
	};

	enum SDFGIYScale {
		SDFGI_Y_SCALE_DISABLED,
		SDFGI_Y_SCALE_75_PERCENT,
		SDFGI_Y_SCALE_50_PERCENT,
	};

	enum GlowBlendMode {
		GLOW_BLEND_MODE_ADDITIVE,
		GLOW_BLEND_MODE_SCREEN,
		GLOW_BLEND_MODE_SOFTLIGHT,
		GLOW_BLEND_MODE_REPLACE,
		GLOW_BLEND_MODE_MIX,
	};

	enum VolumetricFogShadowFilter {
		VOLUMETRIC_FOG_SHADOW_FILTER_DISABLED,
		VOLUMETRIC_FOG_SHADOW_FILTER_LOW,
		VOLUMETRIC_FOG_SHADOW_FILTER_MEDIUM,
		VOLUMETRIC_FOG_SHADOW_FILTER_HIGH,
	};

private:
	RID environment;

	// Background
	BGMode bg_mode = BG_CLEAR_COLOR;
	Ref<Sky> bg_sky;
	float bg_sky_custom_fov = 0;
	Vector3 bg_sky_rotation;
	Color bg_color;
	float bg_energy = 1.0;
	int bg_canvas_max_layer = 0;
	int bg_camera_feed_id = 1;

	// Ambient light
	Color ambient_color;
	AmbientSource ambient_source = AMBIENT_SOURCE_BG;
	float ambient_energy = 1.0;
	float ambient_sky_contribution = 1.0;
	ReflectionSource reflection_source = REFLECTION_SOURCE_BG;
	Color ao_color;
	void _update_ambient_light();

	// Tonemap
	ToneMapper tone_mapper = TONE_MAPPER_LINEAR;
	float tonemap_exposure = 1.0;
	float tonemap_white = 1.0;
	bool tonemap_auto_exposure_enabled = false;
	float tonemap_auto_exposure_min = 0.05;
	float tonemap_auto_exposure_max = 8;
	float tonemap_auto_exposure_speed = 0.5;
	float tonemap_auto_exposure_grey = 0.4;
	void _update_tonemap();

	// SSR
	bool ssr_enabled = false;
	int ssr_max_steps = 64;
	float ssr_fade_in = 0.15;
	float ssr_fade_out = 2.0;
	float ssr_depth_tolerance = 0.2;
	void _update_ssr();

	// SSAO
	bool ssao_enabled = false;
	float ssao_radius = 1.0;
	float ssao_intensity = 1.0;
	float ssao_bias = 0.01;
	float ssao_direct_light_affect = 0.0;
	float ssao_ao_channel_affect = 0.0;
	SSAOBlur ssao_blur = SSAO_BLUR_3x3;
	float ssao_edge_sharpness = 4.0;
	void _update_ssao();

	// SDFGI
	bool sdfgi_enabled = false;
	SDFGICascades sdfgi_cascades = SDFGI_CASCADES_6;
	float sdfgi_min_cell_size = 0.2;
	SDFGIYScale sdfgi_y_scale = SDFGI_Y_SCALE_DISABLED;
	bool sdfgi_use_occlusion = false;
	bool sdfgi_use_multibounce = false;
	bool sdfgi_read_sky_light = false;
	float sdfgi_energy = 1.0;
	float sdfgi_normal_bias = 1.1;
	float sdfgi_probe_bias = 1.1;
	void _update_sdfgi();

	// Glow
	bool glow_enabled = false;
	Vector<float> glow_levels;
	bool glow_normalize_levels = false;
	float glow_intensity = 0.8;
	float glow_strength = 1.0;
	float glow_mix = 0.05;
	float glow_bloom = 0.0;
	GlowBlendMode glow_blend_mode = GLOW_BLEND_MODE_SOFTLIGHT;
	float glow_hdr_bleed_threshold = 1.0;
	float glow_hdr_bleed_scale = 2.0;
	float glow_hdr_luminance_cap = 12.0;
	void _update_glow();

	// Fog
	bool fog_enabled = false;
	Color fog_light_color = Color(0.5, 0.6, 0.7);
	float fog_light_energy = 1.0;
	float fog_sun_scatter = 0.0;
	float fog_density = 0.001;
	float fog_height = 0.0;
	float fog_height_density = 0.0; //can be negative to invert effect
	float fog_aerial_perspective = 0.0;

	void _update_fog();

	// Volumetric Fog
	bool volumetric_fog_enabled = false;
	float volumetric_fog_density = 0.01;
	Color volumetric_fog_light = Color(0.0, 0.0, 0.0);
	float volumetric_fog_light_energy = 1.0;
	float volumetric_fog_length = 64.0;
	float volumetric_fog_detail_spread = 2.0;
	VolumetricFogShadowFilter volumetric_fog_shadow_filter = VOLUMETRIC_FOG_SHADOW_FILTER_LOW;
	float volumetric_fog_gi_inject = 0.0;
	void _update_volumetric_fog();

	// Adjustment
	bool adjustment_enabled = false;
	float adjustment_brightness = 1.0;
	float adjustment_contrast = 1.0;
	float adjustment_saturation = 1.0;
	Ref<Texture2D> adjustment_color_correction;
	void _update_adjustment();

protected:
	static void _bind_methods();
	virtual void _validate_property(PropertyInfo &property) const override;
#ifndef DISABLE_DEPRECATED
	// Kept for compatibility from 3.x to 4.0.
	bool _set(const StringName &p_name, const Variant &p_value);
#endif

public:
	virtual RID get_rid() const override;

	// Background
	void set_background(BGMode p_bg);
	BGMode get_background() const;
	void set_sky(const Ref<Sky> &p_sky);
	Ref<Sky> get_sky() const;
	void set_sky_custom_fov(float p_scale);
	float get_sky_custom_fov() const;
	void set_sky_rotation(const Vector3 &p_rotation);
	Vector3 get_sky_rotation() const;
	void set_bg_color(const Color &p_color);
	Color get_bg_color() const;
	void set_bg_energy(float p_energy);
	float get_bg_energy() const;
	void set_canvas_max_layer(int p_max_layer);
	int get_canvas_max_layer() const;
	void set_camera_feed_id(int p_id);
	int get_camera_feed_id() const;

	// Ambient light
	void set_ambient_light_color(const Color &p_color);
	Color get_ambient_light_color() const;
	void set_ambient_source(AmbientSource p_source);
	AmbientSource get_ambient_source() const;
	void set_ambient_light_energy(float p_energy);
	float get_ambient_light_energy() const;
	void set_ambient_light_sky_contribution(float p_ratio);
	float get_ambient_light_sky_contribution() const;
	void set_reflection_source(ReflectionSource p_source);
	ReflectionSource get_reflection_source() const;
	void set_ao_color(const Color &p_color);
	Color get_ao_color() const;

	// Tonemap
	void set_tonemapper(ToneMapper p_tone_mapper);
	ToneMapper get_tonemapper() const;
	void set_tonemap_exposure(float p_exposure);
	float get_tonemap_exposure() const;
	void set_tonemap_white(float p_white);
	float get_tonemap_white() const;
	void set_tonemap_auto_exposure_enabled(bool p_enabled);
	bool is_tonemap_auto_exposure_enabled() const;
	void set_tonemap_auto_exposure_min(float p_auto_exposure_min);
	float get_tonemap_auto_exposure_min() const;
	void set_tonemap_auto_exposure_max(float p_auto_exposure_max);
	float get_tonemap_auto_exposure_max() const;
	void set_tonemap_auto_exposure_speed(float p_auto_exposure_speed);
	float get_tonemap_auto_exposure_speed() const;
	void set_tonemap_auto_exposure_grey(float p_auto_exposure_grey);
	float get_tonemap_auto_exposure_grey() const;

	// SSR
	void set_ssr_enabled(bool p_enabled);
	bool is_ssr_enabled() const;
	void set_ssr_max_steps(int p_steps);
	int get_ssr_max_steps() const;
	void set_ssr_fade_in(float p_fade_in);
	float get_ssr_fade_in() const;
	void set_ssr_fade_out(float p_fade_out);
	float get_ssr_fade_out() const;
	void set_ssr_depth_tolerance(float p_depth_tolerance);
	float get_ssr_depth_tolerance() const;

	// SSAO
	void set_ssao_enabled(bool p_enabled);
	bool is_ssao_enabled() const;
	void set_ssao_radius(float p_radius);
	float get_ssao_radius() const;
	void set_ssao_intensity(float p_intensity);
	float get_ssao_intensity() const;
	void set_ssao_bias(float p_bias);
	float get_ssao_bias() const;
	void set_ssao_direct_light_affect(float p_direct_light_affect);
	float get_ssao_direct_light_affect() const;
	void set_ssao_ao_channel_affect(float p_ao_channel_affect);
	float get_ssao_ao_channel_affect() const;
	void set_ssao_blur(SSAOBlur p_blur);
	SSAOBlur get_ssao_blur() const;
	void set_ssao_edge_sharpness(float p_edge_sharpness);
	float get_ssao_edge_sharpness() const;

	// SDFGI
	void set_sdfgi_enabled(bool p_enabled);
	bool is_sdfgi_enabled() const;
	void set_sdfgi_cascades(SDFGICascades p_cascades);
	SDFGICascades get_sdfgi_cascades() const;
	void set_sdfgi_min_cell_size(float p_size);
	float get_sdfgi_min_cell_size() const;
	void set_sdfgi_max_distance(float p_distance);
	float get_sdfgi_max_distance() const;
	void set_sdfgi_cascade0_distance(float p_distance);
	float get_sdfgi_cascade0_distance() const;
	void set_sdfgi_y_scale(SDFGIYScale p_y_scale);
	SDFGIYScale get_sdfgi_y_scale() const;
	void set_sdfgi_use_occlusion(bool p_enabled);
	bool is_sdfgi_using_occlusion() const;
	void set_sdfgi_use_multi_bounce(bool p_enabled);
	bool is_sdfgi_using_multi_bounce() const;
	void set_sdfgi_read_sky_light(bool p_enabled);
	bool is_sdfgi_reading_sky_light() const;
	void set_sdfgi_energy(float p_energy);
	float get_sdfgi_energy() const;
	void set_sdfgi_normal_bias(float p_bias);
	float get_sdfgi_normal_bias() const;
	void set_sdfgi_probe_bias(float p_bias);
	float get_sdfgi_probe_bias() const;

	// Glow
	void set_glow_enabled(bool p_enabled);
	bool is_glow_enabled() const;
	void set_glow_level(int p_level, float p_intensity);
	float get_glow_level(int p_level) const;
	void set_glow_normalized(bool p_normalized);
	bool is_glow_normalized() const;
	void set_glow_intensity(float p_intensity);
	float get_glow_intensity() const;
	void set_glow_strength(float p_strength);
	float get_glow_strength() const;
	void set_glow_mix(float p_mix);
	float get_glow_mix() const;
	void set_glow_bloom(float p_threshold);
	float get_glow_bloom() const;
	void set_glow_blend_mode(GlowBlendMode p_mode);
	GlowBlendMode get_glow_blend_mode() const;
	void set_glow_hdr_bleed_threshold(float p_threshold);
	float get_glow_hdr_bleed_threshold() const;
	void set_glow_hdr_bleed_scale(float p_scale);
	float get_glow_hdr_bleed_scale() const;
	void set_glow_hdr_luminance_cap(float p_amount);
	float get_glow_hdr_luminance_cap() const;

	// Fog

	void set_fog_enabled(bool p_enabled);
	bool is_fog_enabled() const;
	void set_fog_light_color(const Color &p_light_color);
	Color get_fog_light_color() const;
	void set_fog_light_energy(float p_amount);
	float get_fog_light_energy() const;
	void set_fog_sun_scatter(float p_amount);
	float get_fog_sun_scatter() const;

	void set_fog_density(float p_amount);
	float get_fog_density() const;
	void set_fog_height(float p_amount);
	float get_fog_height() const;
	void set_fog_height_density(float p_amount);
	float get_fog_height_density() const;
	void set_fog_aerial_perspective(float p_aerial_perspective);
	float get_fog_aerial_perspective() const;

	// Volumetric Fog
	void set_volumetric_fog_enabled(bool p_enable);
	bool is_volumetric_fog_enabled() const;
	void set_volumetric_fog_density(float p_density);
	float get_volumetric_fog_density() const;
	void set_volumetric_fog_light(Color p_color);
	Color get_volumetric_fog_light() const;
	void set_volumetric_fog_light_energy(float p_begin);
	float get_volumetric_fog_light_energy() const;
	void set_volumetric_fog_length(float p_length);
	float get_volumetric_fog_length() const;
	void set_volumetric_fog_detail_spread(float p_detail_spread);
	float get_volumetric_fog_detail_spread() const;
	void set_volumetric_fog_shadow_filter(VolumetricFogShadowFilter p_filter);
	VolumetricFogShadowFilter get_volumetric_fog_shadow_filter() const;
	void set_volumetric_fog_gi_inject(float p_gi_inject);
	float get_volumetric_fog_gi_inject() const;

	// Adjustment
	void set_adjustment_enabled(bool p_enabled);
	bool is_adjustment_enabled() const;
	void set_adjustment_brightness(float p_brightness);
	float get_adjustment_brightness() const;
	void set_adjustment_contrast(float p_contrast);
	float get_adjustment_contrast() const;
	void set_adjustment_saturation(float p_saturation);
	float get_adjustment_saturation() const;
	void set_adjustment_color_correction(const Ref<Texture2D> &p_ramp);
	Ref<Texture2D> get_adjustment_color_correction() const;

	Environment();
	~Environment();
};

VARIANT_ENUM_CAST(Environment::BGMode)
VARIANT_ENUM_CAST(Environment::AmbientSource)
VARIANT_ENUM_CAST(Environment::ReflectionSource)
VARIANT_ENUM_CAST(Environment::ToneMapper)
VARIANT_ENUM_CAST(Environment::SSAOBlur)
VARIANT_ENUM_CAST(Environment::SDFGICascades)
VARIANT_ENUM_CAST(Environment::SDFGIYScale)
VARIANT_ENUM_CAST(Environment::GlowBlendMode)
VARIANT_ENUM_CAST(Environment::VolumetricFogShadowFilter)

#endif // ENVIRONMENT_H
