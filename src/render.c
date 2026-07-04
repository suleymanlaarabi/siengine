#include "engine_internal.h"
#include "siengine.h"
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_surface.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    float x, y, z;
} SICubeVertex;

typedef struct {
    SIMat4 mvp;
} SIVertexUniforms;

typedef struct {
    float color[4];
} SIFragmentUniforms;

typedef struct {
    SDL_GPUShader *vertex;
    SDL_GPUShader *fragment;
} SIShaderPair;

static const SICubeVertex CUBE_VERTICES[] = {
    { -0.5f, -0.5f, -0.5f }, { 0.5f, -0.5f, -0.5f }, { 0.5f, 0.5f, -0.5f }, { -0.5f, 0.5f, -0.5f },
    { -0.5f, -0.5f, 0.5f },  { 0.5f, -0.5f, 0.5f },  { 0.5f, 0.5f, 0.5f },  { -0.5f, 0.5f, 0.5f },
};

static const uint16_t CUBE_INDICES[] = {
    0, 2, 1, 0, 3, 2, 4, 5, 6, 4, 6, 7, 0, 1, 5, 0, 5, 4,
    3, 6, 2, 3, 7, 6, 1, 2, 6, 1, 6, 5, 0, 4, 7, 0, 7, 3,
};

static void begin_drawing(ecs_iter_t *it) {
    SIEngineCtx *ctx = ecs_resource(it->world, SIEngineCtx);
    if (ctx->primary_gpu == NULL) {
        ctx->cmd = NULL;
        return;
    }

    ctx->cmd = SDL_AcquireGPUCommandBuffer(ctx->primary_gpu);
    if (ctx->cmd == NULL) {
        fprintf(stderr, "siengine: SDL_AcquireGPUCommandBuffer failed: %s\n", SDL_GetError());
    }
}

static void *load_file_from_project(const char *path, size_t *size) {
    void *data = SDL_LoadFile(path, size);
    if (data != NULL) {
        return data;
    }

    char parent_path[256];
    snprintf(parent_path, sizeof(parent_path), "../%s", path);
    return SDL_LoadFile(parent_path, size);
}

static SDL_GPUShader *load_shader(
    SDL_GPUDevice *gpu,
    const char *path,
    SDL_GPUShaderStage stage,
    uint32_t uniform_buffers
) {
    size_t code_size = 0;
    void *code = load_file_from_project(path, &code_size);
    if (code == NULL) {
        fprintf(stderr, "siengine: failed to load shader %s: %s\n", path, SDL_GetError());
        return NULL;
    }

    SDL_GPUShader *shader = SDL_CreateGPUShader(
        gpu,
        &(SDL_GPUShaderCreateInfo){
            .code_size = code_size,
            .code = code,
            .entrypoint = "main",
            .format = SDL_GPU_SHADERFORMAT_SPIRV,
            .stage = stage,
            .num_uniform_buffers = uniform_buffers,
        }
    );
    SDL_free(code);

    if (shader == NULL) {
        fprintf(stderr, "siengine: SDL_CreateGPUShader failed for %s: %s\n", path, SDL_GetError());
    }

    return shader;
}

static SIShaderPair load_cube_shaders(SDL_GPUDevice *gpu) {
    return (SIShaderPair){
        .vertex = load_shader(gpu, "shaders/cube.vert.spv", SDL_GPU_SHADERSTAGE_VERTEX, 1),
        .fragment = load_shader(gpu, "shaders/cube.frag.spv", SDL_GPU_SHADERSTAGE_FRAGMENT, 1),
    };
}

static bool upload_cube_mesh(SIEngineCtx *ctx) {
    uint32_t vertex_size = sizeof(CUBE_VERTICES);
    uint32_t index_size = sizeof(CUBE_INDICES);
    uint32_t transfer_size = vertex_size + index_size;

    ctx->cube_vertex_buffer = SDL_CreateGPUBuffer(
        ctx->primary_gpu,
        &(SDL_GPUBufferCreateInfo){ .usage = SDL_GPU_BUFFERUSAGE_VERTEX, .size = vertex_size }
    );
    ctx->cube_index_buffer = SDL_CreateGPUBuffer(
        ctx->primary_gpu,
        &(SDL_GPUBufferCreateInfo){ .usage = SDL_GPU_BUFFERUSAGE_INDEX, .size = index_size }
    );
    SDL_GPUTransferBuffer *transfer = SDL_CreateGPUTransferBuffer(
        ctx->primary_gpu,
        &(SDL_GPUTransferBufferCreateInfo){
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = transfer_size,
        }
    );

    if (ctx->cube_vertex_buffer == NULL || ctx->cube_index_buffer == NULL || transfer == NULL) {
        fprintf(stderr, "siengine: failed to create cube buffers: %s\n", SDL_GetError());
        if (transfer != NULL) {
            SDL_ReleaseGPUTransferBuffer(ctx->primary_gpu, transfer);
        }
        return false;
    }

    uint8_t *mapped = SDL_MapGPUTransferBuffer(ctx->primary_gpu, transfer, false);
    if (mapped == NULL) {
        fprintf(stderr, "siengine: SDL_MapGPUTransferBuffer failed: %s\n", SDL_GetError());
        SDL_ReleaseGPUTransferBuffer(ctx->primary_gpu, transfer);
        return false;
    }

    memcpy(mapped, CUBE_VERTICES, vertex_size);
    memcpy(mapped + vertex_size, CUBE_INDICES, index_size);
    SDL_UnmapGPUTransferBuffer(ctx->primary_gpu, transfer);

    SDL_GPUCommandBuffer *cmd = SDL_AcquireGPUCommandBuffer(ctx->primary_gpu);
    SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(cmd);
    SDL_UploadToGPUBuffer(
        copy,
        &(SDL_GPUTransferBufferLocation){ .transfer_buffer = transfer, .offset = 0 },
        &(SDL_GPUBufferRegion){ .buffer = ctx->cube_vertex_buffer,
                                .offset = 0,
                                .size = vertex_size },
        false
    );
    SDL_UploadToGPUBuffer(
        copy,
        &(SDL_GPUTransferBufferLocation){ .transfer_buffer = transfer, .offset = vertex_size },
        &(SDL_GPUBufferRegion){ .buffer = ctx->cube_index_buffer, .offset = 0, .size = index_size },
        false
    );
    SDL_EndGPUCopyPass(copy);
    SDL_SubmitGPUCommandBuffer(cmd);
    SDL_ReleaseGPUTransferBuffer(ctx->primary_gpu, transfer);

    return true;
}

static bool ensure_cube_pipeline(SIEngineCtx *ctx, SDL_GPUTextureFormat color_format) {
    if (ctx->cube_pipeline != NULL) {
        return true;
    }

    if (!upload_cube_mesh(ctx)) {
        return false;
    }

    SIShaderPair shaders = load_cube_shaders(ctx->primary_gpu);
    if (shaders.vertex == NULL || shaders.fragment == NULL) {
        if (shaders.vertex != NULL) {
            SDL_ReleaseGPUShader(ctx->primary_gpu, shaders.vertex);
        }
        if (shaders.fragment != NULL) {
            SDL_ReleaseGPUShader(ctx->primary_gpu, shaders.fragment);
        }
        return false;
    }

    SDL_GPUVertexBufferDescription vertex_buffers[] = {
        {
            .slot = 0,
            .pitch = sizeof(SICubeVertex),
            .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
        },
    };
    SDL_GPUVertexAttribute vertex_attributes[] = {
        {
            .location = 0,
            .buffer_slot = 0,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
            .offset = offsetof(SICubeVertex, x),
        },
    };
    SDL_GPUColorTargetDescription color_targets[] = {
        { .format = color_format },
    };

    ctx->depth_format = SDL_GPU_TEXTUREFORMAT_D16_UNORM;
    ctx->cube_pipeline = SDL_CreateGPUGraphicsPipeline(
        ctx->primary_gpu,
        &(SDL_GPUGraphicsPipelineCreateInfo){
            .vertex_shader = shaders.vertex,
            .fragment_shader = shaders.fragment,
            .vertex_input_state =
                {
                    .vertex_buffer_descriptions = vertex_buffers,
                    .num_vertex_buffers = 1,
                    .vertex_attributes = vertex_attributes,
                    .num_vertex_attributes = 1,
                },
            .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
            .rasterizer_state =
                {
                    .fill_mode = SDL_GPU_FILLMODE_FILL,
                    .cull_mode = SDL_GPU_CULLMODE_BACK,
                    .front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE,
                    .enable_depth_clip = true,
                },
            .multisample_state = { .sample_count = SDL_GPU_SAMPLECOUNT_1 },
            .depth_stencil_state =
                {
                    .compare_op = SDL_GPU_COMPAREOP_LESS,
                    .enable_depth_test = true,
                    .enable_depth_write = true,
                },
            .target_info =
                {
                    .color_target_descriptions = color_targets,
                    .num_color_targets = 1,
                    .depth_stencil_format = ctx->depth_format,
                    .has_depth_stencil_target = true,
                },
        }
    );

    SDL_ReleaseGPUShader(ctx->primary_gpu, shaders.vertex);
    SDL_ReleaseGPUShader(ctx->primary_gpu, shaders.fragment);

    if (ctx->cube_pipeline == NULL) {
        fprintf(stderr, "siengine: SDL_CreateGPUGraphicsPipeline failed: %s\n", SDL_GetError());
        return false;
    }

    return true;
}

static SDL_GPUTexture *
ensure_depth_target(SIEngineCtx *ctx, SDL_Window *window, uint32_t width, uint32_t height) {
    for (uint32_t i = 0; i < ctx->depth_target_count; i++) {
        SIDepthTarget *target = &ctx->depth_targets[i];
        if (target->window != window) {
            continue;
        }

        if (target->width == width && target->height == height && target->texture != NULL) {
            return target->texture;
        }

        if (target->texture != NULL) {
            SDL_ReleaseGPUTexture(ctx->primary_gpu, target->texture);
        }
        target->texture = NULL;
        target->width = width;
        target->height = height;

        target->texture = SDL_CreateGPUTexture(
            ctx->primary_gpu,
            &(SDL_GPUTextureCreateInfo){
                .type = SDL_GPU_TEXTURETYPE_2D,
                .format = ctx->depth_format,
                .usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
                .width = width,
                .height = height,
                .layer_count_or_depth = 1,
                .num_levels = 1,
                .sample_count = SDL_GPU_SAMPLECOUNT_1,
            }
        );
        return target->texture;
    }

    if (ctx->depth_target_count >= 8) {
        fprintf(stderr, "siengine: maximum number of windows with depth targets reached\n");
        return NULL;
    }

    SIDepthTarget *target = &ctx->depth_targets[ctx->depth_target_count++];
    target->window = window;
    target->width = width;
    target->height = height;
    target->texture = SDL_CreateGPUTexture(
        ctx->primary_gpu,
        &(SDL_GPUTextureCreateInfo){
            .type = SDL_GPU_TEXTURETYPE_2D,
            .format = ctx->depth_format,
            .usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
            .width = width,
            .height = height,
            .layer_count_or_depth = 1,
            .num_levels = 1,
            .sample_count = SDL_GPU_SAMPLECOUNT_1,
        }
    );

    if (target->texture == NULL) {
        fprintf(stderr, "siengine: SDL_CreateGPUTexture(depth) failed: %s\n", SDL_GetError());
    }

    return target->texture;
}

static bool find_active_camera(ecs_world_t *world, SIMat4 *view, const SICamera3d **camera) {
    bool found = false;
    ecs_query_id_t q = ecs_query(
        world,
        { .terms = {
              ecs_in(SICamera3d),
              ecs_in(SIPosition3d),
              ecs_in(SIRotation3d),
              ecs_filter(SIActiveCamera),
          } }
    );
    ecs_iter_t it = ecs_query_iter(world, q);

    while (!found && ecs_iter_next(&it)) {
        SICamera3d *cameras = ecs_field(&it, 0);
        SIPosition3d *positions = ecs_field(&it, 1);
        SIRotation3d *rotations = ecs_field(&it, 2);

        if (it.count > 0) {
            *camera = &cameras[0];
            *view = si_mat4_view(positions[0], rotations[0]);
            found = true;
        }
    }

    ecs_query_fini(world, q);
    return found;
}

static void draw_cubes(ecs_world_t *world, SDL_GPURenderPass *pass, SIMat4 view_projection) {
    SDL_GPUBufferBinding vertex_binding = {
        .buffer = ecs_resource(world, SIEngineCtx)->cube_vertex_buffer
    };
    SDL_GPUBufferBinding index_binding = {
        .buffer = ecs_resource(world, SIEngineCtx)->cube_index_buffer
    };
    SDL_BindGPUVertexBuffers(pass, 0, &vertex_binding, 1);
    SDL_BindGPUIndexBuffer(pass, &index_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);

    ecs_query_id_t q = ecs_query(
        world,
        { .terms = {
              ecs_in(SIPosition3d),
              ecs_in(SIRotation3d),
              ecs_in(SIScale3d),
              ecs_in(SIColor),
              ecs_filter(SICube),
          } }
    );
    ecs_iter_t cube_it = ecs_query_iter(world, q);

    while (ecs_iter_next(&cube_it)) {
        SIPosition3d *positions = ecs_field(&cube_it, 0);
        SIRotation3d *rotations = ecs_field(&cube_it, 1);
        SIScale3d *scales = ecs_field(&cube_it, 2);
        SIColor *colors = ecs_field(&cube_it, 3);

        for (uint32_t i = 0; i < cube_it.count; i++) {
            SIMat4 model = si_mat4_model(positions[i], rotations[i], scales[i]);
            SIVertexUniforms vertex_uniforms = {
                .mvp = si_mat4_mul(view_projection, model),
            };
            SIFragmentUniforms fragment_uniforms = {
                .color = { colors[i].r, colors[i].g, colors[i].b, colors[i].a },
            };

            SDL_PushGPUVertexUniformData(
                ecs_resource(world, SIEngineCtx)->cmd,
                0,
                &vertex_uniforms,
                sizeof(vertex_uniforms)
            );
            SDL_PushGPUFragmentUniformData(
                ecs_resource(world, SIEngineCtx)->cmd,
                0,
                &fragment_uniforms,
                sizeof(fragment_uniforms)
            );
            SDL_DrawGPUIndexedPrimitives(pass, 36, 1, 0, 0, 0);
        }
    }

    ecs_query_fini(world, q);
}

static void drawing(ecs_iter_t *it) {
    SIEngineCtx *ctx = ecs_resource(it->world, SIEngineCtx);
    SIWindowHandle *windows = ecs_field(it, 0);

    if (ctx->primary_gpu == NULL || ctx->cmd == NULL) {
        return;
    }

    SIMat4 view;
    const SICamera3d *camera = NULL;
    if (!find_active_camera(it->world, &view, &camera)) {
        return;
    }

    for (uint32_t i = 0; i < it->count; i++) {
        SDL_GPUTexture *swapchain_texture;
        uint32_t window_width, window_height;
        if (!SDL_WaitAndAcquireGPUSwapchainTexture(
                ctx->cmd,
                windows[i].handle,
                &swapchain_texture,
                &window_width,
                &window_height
            )) {
            fprintf(
                stderr,
                "siengine: SDL_WaitAndAcquireGPUSwapchainTexture failed: %s\n",
                SDL_GetError()
            );
            continue;
        }
        if (swapchain_texture == NULL || window_width == 0 || window_height == 0) {
            continue;
        }

        SDL_GPUTextureFormat color_format =
            SDL_GetGPUSwapchainTextureFormat(ctx->primary_gpu, windows[i].handle);
        if (!ensure_cube_pipeline(ctx, color_format)) {
            continue;
        }

        SDL_GPUTexture *depth =
            ensure_depth_target(ctx, windows[i].handle, window_width, window_height);
        if (depth == NULL) {
            continue;
        }

        float aspect = (float)window_width / (float)window_height;
        float fov_y = camera->fov_y > 0.0f ? camera->fov_y : 1.0471976f;
        float near_clip = camera->near_clip > 0.0f ? camera->near_clip : 0.1f;
        float far_clip = camera->far_clip > near_clip ? camera->far_clip : 1000.0f;
        SIMat4 projection = si_mat4_perspective(fov_y, aspect, near_clip, far_clip);
        SIMat4 view_projection = si_mat4_mul(projection, view);

        SDL_GPUColorTargetInfo color_target = {
            .texture = swapchain_texture,
            .clear_color = { 0.05f, 0.05f, 0.08f, 1.0f },
            .load_op = SDL_GPU_LOADOP_CLEAR,
            .store_op = SDL_GPU_STOREOP_STORE,
        };
        SDL_GPUDepthStencilTargetInfo depth_target = {
            .texture = depth,
            .clear_depth = 1.0f,
            .load_op = SDL_GPU_LOADOP_CLEAR,
            .store_op = SDL_GPU_STOREOP_DONT_CARE,
        };

        SDL_GPURenderPass *pass = SDL_BeginGPURenderPass(ctx->cmd, &color_target, 1, &depth_target);
        SDL_BindGPUGraphicsPipeline(pass, ctx->cube_pipeline);
        draw_cubes(it->world, pass, view_projection);
        SDL_EndGPURenderPass(pass);
    }
}

static void end_drawing(ecs_iter_t *it) {
    SIEngineCtx *ctx = ecs_resource(it->world, SIEngineCtx);
    if (ctx->cmd == NULL) {
        return;
    }

    SDL_SubmitGPUCommandBuffer(ctx->cmd);
    ctx->cmd = NULL;
}

void sirender_shutdown(SIEngineCtx *ctx) {
    if (ctx == NULL || ctx->primary_gpu == NULL) {
        return;
    }

    for (uint32_t i = 0; i < ctx->depth_target_count; i++) {
        if (ctx->depth_targets[i].texture != NULL) {
            SDL_ReleaseGPUTexture(ctx->primary_gpu, ctx->depth_targets[i].texture);
        }
    }
    ctx->depth_target_count = 0;

    if (ctx->cube_pipeline != NULL) {
        SDL_ReleaseGPUGraphicsPipeline(ctx->primary_gpu, ctx->cube_pipeline);
        ctx->cube_pipeline = NULL;
    }
    if (ctx->cube_vertex_buffer != NULL) {
        SDL_ReleaseGPUBuffer(ctx->primary_gpu, ctx->cube_vertex_buffer);
        ctx->cube_vertex_buffer = NULL;
    }
    if (ctx->cube_index_buffer != NULL) {
        SDL_ReleaseGPUBuffer(ctx->primary_gpu, ctx->cube_index_buffer);
        ctx->cube_index_buffer = NULL;
    }
}

void sirender_register(ecs_world_t *world) {
    ecs_system(
        world,
        {
            .name = "BeginDrawing",
            .phase = EcsPreRender,
            .callback = begin_drawing,
        }
    );
    ecs_system(
        world,
        {
            .name = "Drawing",
            .query.terms = { ecs_inout(SIWindowHandle) },
            .phase = EcsOnRender,
            .callback = drawing,
        }
    );
    ecs_system(
        world,
        {
            .name = "EndDrawing",
            .query.terms = {},
            .phase = EcsPostRender,
            .callback = end_drawing,
        }
    );
}
