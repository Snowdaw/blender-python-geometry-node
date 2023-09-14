/* SPDX-FileCopyrightText: 2023 Blender Authors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

#include "eevee_defines.hh"
#include "gpu_shader_create_info.hh"

GPU_SHADER_CREATE_INFO(eevee_subsurface_eval)
    .do_static_compilation(true)
    .additional_info("eevee_shared", "eevee_global_ubo", "eevee_render_pass_out")
    .sampler(0, ImageType::FLOAT_2D_ARRAY, "gbuffer_closure_tx")
    .sampler(1, ImageType::FLOAT_2D_ARRAY, "gbuffer_color_tx")
    .sampler(2, ImageType::FLOAT_2D, "radiance_tx")
    .early_fragment_test(true)
    .fragment_out(0, Type::VEC4, "out_combined")
    .fragment_source("eevee_subsurface_eval_frag.glsl")
    /* TODO(fclem) Output to diffuse pass without feedback loop. */
    .additional_info("draw_fullscreen", "draw_view", "eevee_hiz_data");