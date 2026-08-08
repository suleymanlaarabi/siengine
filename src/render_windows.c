#include "render_internal.h"
#include "render_shaders.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

static SDL_GPUColorTargetBlendState blend_state(SIBlendModeValue blend) {
    SDL_GPUColorTargetBlendState state = {
        .src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
        .dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
        .color_blend_op = SDL_GPU_BLENDOP_ADD,
        .src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
        .dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
        .alpha_blend_op = SDL_GPU_BLENDOP_ADD,
        .color_write_mask = SDL_GPU_COLORCOMPONENT_R | SDL_GPU_COLORCOMPONENT_G |
                            SDL_GPU_COLORCOMPONENT_B | SDL_GPU_COLORCOMPONENT_A,
        .enable_blend = true,
        .enable_color_write_mask = true,
    };

    if (blend == SI_BLEND_ADDITIVE) {
        state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    } else if (blend == SI_BLEND_MULTIPLY) {
        state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_DST_COLOR;
        state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ZERO;
    }
    return state;
}

static SDL_GPUGraphicsPipeline *create_pipeline(
    SIEngineCtx *engine,
    SIRenderState *render,
    SDL_GPUTextureFormat format,
    SIBlendModeValue blend,
    SDL_GPUShader *fragment_shader
) {
    static const SDL_GPUVertexBufferDescription vertex_buffers[] = {
        { .slot = 0, .pitch = sizeof(SIRenderVertex), .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX }
    };
    static const SDL_GPUVertexAttribute attributes[] = {
        { .location = 0,
          .buffer_slot = 0,
          .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
          .offset = 0 },
        { .location = 1,
          .buffer_slot = 0,
          .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
          .offset = 8 },
        { .location = 2,
          .buffer_slot = 0,
          .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
          .offset = 16 }
    };
    SDL_GPUColorTargetDescription target = { .format = format, .blend_state = blend_state(blend) };
    SDL_GPUGraphicsPipelineCreateInfo info = {
        .vertex_shader = render->vertex_shader, .fragment_shader = fragment_shader,
        .vertex_input_state = {
            .vertex_buffer_descriptions = vertex_buffers, .num_vertex_buffers = 1,
            .vertex_attributes = attributes, .num_vertex_attributes = 3,
        },
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .rasterizer_state = {
            .fill_mode = SDL_GPU_FILLMODE_FILL, .cull_mode = SDL_GPU_CULLMODE_NONE,
            .front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE, .enable_depth_clip = true,
        },
        .multisample_state = { .sample_count = SDL_GPU_SAMPLECOUNT_1 },
        .target_info = { .color_target_descriptions = &target, .num_color_targets = 1 },
    };
    return SDL_CreateGPUGraphicsPipeline(engine->primary_gpu, &info);
}

static void
ensure_gpu_resources(SIEngineCtx *engine, SIRenderState *render, SDL_GPUTextureFormat format) {
    if (render->vertex_shader)
        return;

    render->vertex_shader = SDL_CreateGPUShader(
        engine->primary_gpu,
        &(SDL_GPUShaderCreateInfo){
            .code_size = si_sprite_vertex_shader_size,
            .code = si_sprite_vertex_shader,
            .entrypoint = "main",
            .format = SDL_GPU_SHADERFORMAT_SPIRV,
            .stage = SDL_GPU_SHADERSTAGE_VERTEX,
        }
    );
    render->sprite_fragment_shader = SDL_CreateGPUShader(
        engine->primary_gpu,
        &(SDL_GPUShaderCreateInfo){
            .code_size = si_sprite_fragment_shader_size,
            .code = si_sprite_fragment_shader,
            .entrypoint = "main",
            .format = SDL_GPU_SHADERFORMAT_SPIRV,
            .stage = SDL_GPU_SHADERSTAGE_FRAGMENT,
            .num_samplers = 1,
        }
    );
    render->shape_fragment_shader = SDL_CreateGPUShader(
        engine->primary_gpu,
        &(SDL_GPUShaderCreateInfo){
            .code_size = si_shape_fragment_shader_size,
            .code = si_shape_fragment_shader,
            .entrypoint = "main",
            .format = SDL_GPU_SHADERFORMAT_SPIRV,
            .stage = SDL_GPU_SHADERSTAGE_FRAGMENT,
        }
    );
    render->circle_fragment_shader = SDL_CreateGPUShader(
        engine->primary_gpu,
        &(SDL_GPUShaderCreateInfo){
            .code_size = si_circle_fragment_shader_size,
            .code = si_circle_fragment_shader,
            .entrypoint = "main",
            .format = SDL_GPU_SHADERFORMAT_SPIRV,
            .stage = SDL_GPU_SHADERSTAGE_FRAGMENT,
        }
    );
    for (uint32_t i = 0; i < 3; i++) {
        render->sprite_pipelines[i] =
            create_pipeline(engine, render, format, i, render->sprite_fragment_shader);
        render->shape_pipelines[i] =
            create_pipeline(engine, render, format, i, render->shape_fragment_shader);
        render->circle_pipelines[i] =
            create_pipeline(engine, render, format, i, render->circle_fragment_shader);
    }

    SDL_GPUSamplerCreateInfo info = {
        .min_filter = SDL_GPU_FILTER_NEAREST,
        .mag_filter = SDL_GPU_FILTER_NEAREST,
        .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
        .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
        .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
        .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
    };
    render->samplers[SI_FILTER_NEAREST] = SDL_CreateGPUSampler(engine->primary_gpu, &info);
    info.min_filter = SDL_GPU_FILTER_LINEAR;
    info.mag_filter = SDL_GPU_FILTER_LINEAR;
    info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
    render->samplers[SI_FILTER_LINEAR] = SDL_CreateGPUSampler(engine->primary_gpu, &info);
}

static void
ensure_vertex_buffers(SIEngineCtx *engine, SIRenderState *render, uint32_t vertex_count) {
    if (vertex_count <= render->gpu_vertex_capacity)
        return;

    uint32_t capacity = render->gpu_vertex_capacity ? render->gpu_vertex_capacity * 2 : 1024;
    while (capacity < vertex_count)
        capacity *= 2;

    if (render->vertex_buffer)
        SDL_ReleaseGPUBuffer(engine->primary_gpu, render->vertex_buffer);
    if (render->transfer_buffer)
        SDL_ReleaseGPUTransferBuffer(engine->primary_gpu, render->transfer_buffer);

    render->vertex_buffer = SDL_CreateGPUBuffer(
        engine->primary_gpu,
        &(SDL_GPUBufferCreateInfo){ .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
                                    .size = capacity * sizeof(SIRenderVertex) }
    );
    render->transfer_buffer = SDL_CreateGPUTransferBuffer(
        engine->primary_gpu,
        &(SDL_GPUTransferBufferCreateInfo){ .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
                                            .size = capacity * sizeof(SIRenderVertex) }
    );
    render->gpu_vertex_capacity = capacity;
}

static uint32_t build_vertices(SIRenderState *render) {
    static const uint8_t corners[6][2] = { { 0, 0 }, { 1, 0 }, { 1, 1 },
                                           { 0, 0 }, { 1, 1 }, { 0, 1 } };
    uint32_t required = 0;
    for (uint32_t view_index = 0; view_index < render->view_count; view_index++) {
        SIRenderQueue *queue = &render->views[view_index].queue;
        for (uint32_t i = 0; i < queue->count; i++)
            required += queue->commands[i].primitive == SI_RENDER_TRIANGLE ? 3 : 6;
    }

    if (required > render->vertex_capacity) {
        render->vertex_capacity = render->vertex_capacity ? render->vertex_capacity * 2 : 1024;
        while (render->vertex_capacity < required)
            render->vertex_capacity *= 2;
        render->vertices =
            realloc(render->vertices, render->vertex_capacity * sizeof(*render->vertices));
    }

    uint32_t vertex_offset = 0;
    for (uint32_t view_index = 0; view_index < render->view_count; view_index++) {
        SIRenderView *view = &render->views[view_index];
        SIRenderQueue *queue = &view->queue;
        view->vertex_offset = vertex_offset;

        for (uint32_t i = 0; i < queue->count; i++) {
            SIRenderCommand *command = &queue->commands[i];
            uint32_t vertex_count = command->primitive == SI_RENDER_TRIANGLE ? 3 : 6;
            command->vertex_offset = vertex_offset;
            float u0 = command->flip_x ? command->u1 : command->u0;
            float u1 = command->flip_x ? command->u0 : command->u1;
            float v0 = command->flip_y ? command->v1 : command->v0;
            float v1 = command->flip_y ? command->v0 : command->v1;
            float c = cosf(command->rotation);
            float s = sinf(command->rotation);

            for (uint32_t vertex = 0; vertex < vertex_count; vertex++) {
                float local_x;
                float local_y;
                float local_u;
                float local_v;
                if (command->primitive == SI_RENDER_TRIANGLE) {
                    static const float points[3][2] = {
                        { -0.5f, 0.5f },
                        { 0.5f, 0.5f },
                        { 0.0f, -0.5f },
                    };
                    local_x = points[vertex][0] * command->shape_a * command->scale_x;
                    local_y = points[vertex][1] * command->shape_b * command->scale_y;
                    local_u = 0.5f;
                    local_v = 0.5f;
                } else {
                    local_x = (corners[vertex][0] -
                               (command->primitive == SI_RENDER_SPRITE ? command->pivot_x : 0.5f)) *
                              command->width * command->scale_x;
                    local_y = (corners[vertex][1] -
                               (command->primitive == SI_RENDER_SPRITE ? command->pivot_y : 0.5f)) *
                              command->height * command->scale_y;
                    local_u = corners[vertex][0] ? u1 : u0;
                    local_v = corners[vertex][1] ? v1 : v0;
                }
                float world_x = command->x + local_x * c - local_y * s;
                float world_y = command->y + local_x * s + local_y * c;
                float clip_x = (world_x - view->left) / (view->right - view->left) * 2.0f - 1.0f;
                float clip_y = 1.0f - (world_y - view->top) / (view->bottom - view->top) * 2.0f;

                render->vertices[vertex_offset + vertex] = (SIRenderVertex){
                    .x = clip_x,
                    .y = clip_y,
                    .u = local_u,
                    .v = local_v,
                    .r = command->color.r,
                    .g = command->color.g,
                    .b = command->color.b,
                    .a = command->color.a,
                };
            }
            vertex_offset += vertex_count;
        }
    }
    return required;
}

void sirender_draw_window(ecs_iter_t *it) {
    SIEngineCtx *engine = ecs_resource(SIEngineCtx);
    SIRenderState *render = ecs_resource(SIRenderState);
    SDL_GPUTexture *swapchain_texture = NULL;
    uint32_t pixel_width = 0;
    uint32_t pixel_height = 0;

    if (!engine->window || !render->cmd)
        return;

    if (!SDL_WaitAndAcquireGPUSwapchainTexture(
            render->cmd,
            engine->window,
            &swapchain_texture,
            &pixel_width,
            &pixel_height
        )) {
        SDL_CancelGPUCommandBuffer(render->cmd);
        render->cmd = NULL;
        return;
    }

    if (!swapchain_texture || pixel_width == 0 || pixel_height == 0) {
        SDL_CancelGPUCommandBuffer(render->cmd);
        render->cmd = NULL;
        return;
    }

    uint32_t vertex_count = build_vertices(render);
    ensure_gpu_resources(
        engine,
        render,
        SDL_GetGPUSwapchainTextureFormat(engine->primary_gpu, engine->window)
    );
    ensure_vertex_buffers(engine, render, vertex_count);

    if (vertex_count) {
        void *mapped = SDL_MapGPUTransferBuffer(engine->primary_gpu, render->transfer_buffer, true);
        memcpy(mapped, render->vertices, vertex_count * sizeof(*render->vertices));
        SDL_UnmapGPUTransferBuffer(engine->primary_gpu, render->transfer_buffer);
        SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(render->cmd);
        SDL_UploadToGPUBuffer(
            copy,
            &(SDL_GPUTransferBufferLocation){ .transfer_buffer = render->transfer_buffer },
            &(SDL_GPUBufferRegion){ .buffer = render->vertex_buffer,
                                    .size = vertex_count * sizeof(*render->vertices) },
            true
        );
        SDL_EndGPUCopyPass(copy);
    }

    SDL_GPUColorTargetInfo color_target = {
        .texture = swapchain_texture,
        .clear_color = { 0.05f, 0.05f, 0.08f, 1.0f },
        .load_op = SDL_GPU_LOADOP_CLEAR,
        .store_op = SDL_GPU_STOREOP_STORE,
    };

    SDL_GPURenderPass *pass = SDL_BeginGPURenderPass(render->cmd, &color_target, 1, NULL);
    if (vertex_count) {
        SDL_GPUBufferBinding vertex_binding = {
            .buffer = render->vertex_buffer,
        };
        SDL_BindGPUVertexBuffers(pass, 0, &vertex_binding, 1);
    }

    for (uint32_t view_index = 0; view_index < render->view_count; view_index++) {
        SIRenderView *view = &render->views[view_index];
        float viewport_x = view->viewport_x * pixel_width;
        float viewport_y = view->viewport_y * pixel_height;
        float viewport_width = view->viewport_width * pixel_width;
        float viewport_height = view->viewport_height * pixel_height;
        if (view->virtual_enabled) {
            float scale_x = viewport_width / (float)view->virtual_width;
            float scale_y = viewport_height / (float)view->virtual_height;
            float scale = scale_x < scale_y ? scale_x : scale_y;
            if (view->pixel_perfect)
                scale = floorf(scale);
            float output_width = view->virtual_width * scale;
            float output_height = view->virtual_height * scale;
            viewport_x += (viewport_width - output_width) * 0.5f;
            viewport_y += (viewport_height - output_height) * 0.5f;
            viewport_width = output_width;
            viewport_height = output_height;
        }
        SDL_GPUViewport viewport = { .x = viewport_x,
                                     .y = viewport_y,
                                     .w = viewport_width,
                                     .h = viewport_height,
                                     .min_depth = 0.0f,
                                     .max_depth = 1.0f };
        SDL_Rect scissor = { .x = (int)viewport.x,
                             .y = (int)viewport.y,
                             .w = (int)viewport.w,
                             .h = (int)viewport.h };
        SDL_SetGPUViewport(pass, &viewport);
        SDL_SetGPUScissor(pass, &scissor);

        for (uint32_t first = 0; first < view->queue.count;) {
            SIRenderCommand *command = &view->queue.commands[first];
            uint32_t last = first + 1;
            while (last < view->queue.count &&
                   view->queue.commands[last].pipeline == command->pipeline &&
                   view->queue.commands[last].gpu_texture == command->gpu_texture &&
                   view->queue.commands[last].filter == command->filter &&
                   view->queue.commands[last].blend == command->blend) {
                last++;
            }

            SDL_GPUGraphicsPipeline **pipelines = render->sprite_pipelines;
            if (command->pipeline == SI_RENDER_PIPELINE_SHAPE)
                pipelines = render->shape_pipelines;
            else if (command->pipeline == SI_RENDER_PIPELINE_CIRCLE)
                pipelines = render->circle_pipelines;
            SDL_BindGPUGraphicsPipeline(pass, pipelines[command->blend]);
            if (command->pipeline == SI_RENDER_PIPELINE_SPRITE) {
                SDL_BindGPUFragmentSamplers(
                    pass,
                    0,
                    &(SDL_GPUTextureSamplerBinding){ .texture = command->gpu_texture,
                                                     .sampler = render->samplers[command->filter] },
                    1
                );
            }
            uint32_t primitive_count = 0;
            for (uint32_t i = first; i < last; i++)
                primitive_count += view->queue.commands[i].primitive == SI_RENDER_TRIANGLE ? 1 : 2;
            SDL_DrawGPUPrimitives(pass, primitive_count * 3, 1, command->vertex_offset, 0);
            first = last;
        }
    }
    SDL_EndGPURenderPass(pass);
}
