#if !defined(__EMSCRIPTEN__)

#include "assets_internal.h"
#include "backend.h"
#include "engine_internal.h"
#include "render_internal.h"
#include "render_shaders.h"
#include <SDL3/SDL_gpu.h>
#include <SDL3_image/SDL_image.h>
#include <math.h>
#include <stddef.h>
#include <string.h>

typedef struct {
    SDL_GPUCommandBuffer *command;
    SDL_GPURenderPass *pass;
    SDL_GPUTexture *swapchain;
    SDL_GPUShader *vertex_shader;
    SDL_GPUShader *sprite_fragment_shader;
    SDL_GPUShader *shape_fragment_shader;
    SDL_GPUShader *circle_fragment_shader;
    SDL_GPUGraphicsPipeline *pipelines[SI_PIPELINE_COUNT][SI_BLEND_COUNT];
    SDL_GPUSampler *samplers[2];
    SDL_GPUBuffer *geometry_buffer;
    SDL_GPUTransferBuffer *geometry_transfer;
    SDL_GPUBuffer *instance_buffer;
    SDL_GPUTransferBuffer *instance_transfer;
    uint32_t instance_capacity;
    uint32_t pixel_width;
    uint32_t pixel_height;
} SIBackendState;

static SIBackendState backend;

static SDL_GPUColorTargetBlendState blend_state(SIBlendModeValue blend) {
    SDL_GPUColorTargetBlendState state = {
        .src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
        .dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
        .color_blend_op = SDL_GPU_BLENDOP_ADD,
        .src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
        .dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
        .alpha_blend_op = SDL_GPU_BLENDOP_ADD,
        .enable_blend = true,
    };
    if (blend == SI_BLEND_ADDITIVE) {
        state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
        state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    } else if (blend == SI_BLEND_MULTIPLY) {
        state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_DST_COLOR;
        state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ZERO;
    }
    return state;
}

static SDL_GPUGraphicsPipeline *create_pipeline(
    SDL_GPUDevice *gpu,
    SDL_GPUShader *fragment_shader,
    SDL_GPUTextureFormat format,
    SIBlendModeValue blend
) {
    static const SDL_GPUVertexBufferDescription buffers[] = {
        { .slot = 0, .pitch = sizeof(float) * 2, .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX },
        { .slot = 1,
          .pitch = sizeof(SIInstance2D),
          .input_rate = SDL_GPU_VERTEXINPUTRATE_INSTANCE },
    };
    static const SDL_GPUVertexAttribute attributes[] = {
        { .location = 0, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, .offset = 0 },
        { .location = 1, .buffer_slot = 1, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, .offset = offsetof(SIInstance2D, x) },
        { .location = 2, .buffer_slot = 1, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT, .offset = offsetof(SIInstance2D, rotation) },
        { .location = 3, .buffer_slot = 1, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, .offset = offsetof(SIInstance2D, scale_x) },
        { .location = 4, .buffer_slot = 1, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, .offset = offsetof(SIInstance2D, width) },
        { .location = 5, .buffer_slot = 1, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, .offset = offsetof(SIInstance2D, color) },
        { .location = 6, .buffer_slot = 1, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT, .offset = offsetof(SIInstance2D, frame_index) },
        { .location = 7, .buffer_slot = 1, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, .offset = offsetof(SIInstance2D, flip_x) },
    };
    SDL_GPUGraphicsPipelineCreateInfo info = {
        .vertex_shader = backend.vertex_shader,
        .fragment_shader = fragment_shader,
        .vertex_input_state = {
            .vertex_buffer_descriptions = buffers,
            .num_vertex_buffers = 2,
            .vertex_attributes = attributes,
            .num_vertex_attributes = 8,
        },
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .rasterizer_state = {
            .fill_mode = SDL_GPU_FILLMODE_FILL,
            .cull_mode = SDL_GPU_CULLMODE_NONE,
            .front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE,
            .enable_depth_clip = true,
        },
        .multisample_state = {
            .sample_count = SDL_GPU_SAMPLECOUNT_1,
        },
        .target_info = {
            .color_target_descriptions = &(SDL_GPUColorTargetDescription){
                .format = format,
                .blend_state = blend_state(blend),
            },
            .num_color_targets = 1,
        },
    };
    return SDL_CreateGPUGraphicsPipeline(gpu, &info);
}

static void create_geometry(SDL_GPUDevice *gpu) {
    static const float geometry[] = {
        0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1,
        -0.5f, 0.5f, 0.5f, 0.5f, 0, -0.5f,
    };
    backend.geometry_buffer = SDL_CreateGPUBuffer(
        gpu,
        &(SDL_GPUBufferCreateInfo){
            .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
            .size = sizeof(geometry),
        }
    );
    backend.geometry_transfer = SDL_CreateGPUTransferBuffer(
        gpu,
        &(SDL_GPUTransferBufferCreateInfo){
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = sizeof(geometry),
        }
    );
    memcpy(SDL_MapGPUTransferBuffer(gpu, backend.geometry_transfer, false), geometry, sizeof(geometry));
    SDL_UnmapGPUTransferBuffer(gpu, backend.geometry_transfer);
    SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(backend.command);
    SDL_UploadToGPUBuffer(
        copy,
        &(SDL_GPUTransferBufferLocation){ .transfer_buffer = backend.geometry_transfer },
        &(SDL_GPUBufferRegion){ .buffer = backend.geometry_buffer, .size = sizeof(geometry) },
        false
    );
    SDL_EndGPUCopyPass(copy);
}

static void ensure_resources(SDL_GPUTextureFormat format) {
    if (backend.vertex_shader)
        return;

    SDL_GPUDevice *gpu = ecs_get_resource(SIEngineCtx)->primary_gpu;
    backend.vertex_shader = SDL_CreateGPUShader(
        gpu,
        &(SDL_GPUShaderCreateInfo){
            .code_size = si_sprite_vertex_shader_size,
            .code = si_sprite_vertex_shader,
            .entrypoint = "main",
            .format = SDL_GPU_SHADERFORMAT_SPIRV,
            .stage = SDL_GPU_SHADERSTAGE_VERTEX,
            .num_uniform_buffers = 1,
        }
    );
    backend.sprite_fragment_shader = SDL_CreateGPUShader(
        gpu,
        &(SDL_GPUShaderCreateInfo){
            .code_size = si_sprite_fragment_shader_size,
            .code = si_sprite_fragment_shader,
            .entrypoint = "main",
            .format = SDL_GPU_SHADERFORMAT_SPIRV,
            .stage = SDL_GPU_SHADERSTAGE_FRAGMENT,
            .num_samplers = 1,
        }
    );
    backend.shape_fragment_shader = SDL_CreateGPUShader(
        gpu,
        &(SDL_GPUShaderCreateInfo){
            .code_size = si_shape_fragment_shader_size,
            .code = si_shape_fragment_shader,
            .entrypoint = "main",
            .format = SDL_GPU_SHADERFORMAT_SPIRV,
            .stage = SDL_GPU_SHADERSTAGE_FRAGMENT,
        }
    );
    backend.circle_fragment_shader = SDL_CreateGPUShader(
        gpu,
        &(SDL_GPUShaderCreateInfo){
            .code_size = si_circle_fragment_shader_size,
            .code = si_circle_fragment_shader,
            .entrypoint = "main",
            .format = SDL_GPU_SHADERFORMAT_SPIRV,
            .stage = SDL_GPU_SHADERSTAGE_FRAGMENT,
        }
    );

    for (uint32_t pipeline = 0; pipeline < SI_PIPELINE_COUNT; pipeline++) {
        SDL_GPUShader *fragment = pipeline == SI_PIPELINE_SPRITE
                                      ? backend.sprite_fragment_shader
                                      : pipeline == SI_PIPELINE_SHAPE ? backend.shape_fragment_shader
                                                                      : backend.circle_fragment_shader;
        for (uint32_t blend = 0; blend < SI_BLEND_COUNT; blend++)
            backend.pipelines[pipeline][blend] = create_pipeline(gpu, fragment, format, blend);
    }

    SDL_GPUSamplerCreateInfo sampler = {
        .min_filter = SDL_GPU_FILTER_NEAREST,
        .mag_filter = SDL_GPU_FILTER_NEAREST,
        .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
        .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
        .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
        .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
    };
    backend.samplers[SI_FILTER_NEAREST] = SDL_CreateGPUSampler(gpu, &sampler);
    sampler.min_filter = SDL_GPU_FILTER_LINEAR;
    sampler.mag_filter = SDL_GPU_FILTER_LINEAR;
    sampler.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
    backend.samplers[SI_FILTER_LINEAR] = SDL_CreateGPUSampler(gpu, &sampler);
    create_geometry(gpu);
}

static void ensure_instance_buffer(uint32_t count) {
    if (count <= backend.instance_capacity)
        return;

    SDL_GPUDevice *gpu = ecs_get_resource(SIEngineCtx)->primary_gpu;
    uint32_t capacity = backend.instance_capacity ? backend.instance_capacity * 2 : 256;
    while (capacity < count)
        capacity *= 2;
    if (backend.instance_capacity) {
        SDL_ReleaseGPUBuffer(gpu, backend.instance_buffer);
        SDL_ReleaseGPUTransferBuffer(gpu, backend.instance_transfer);
    }
    backend.instance_capacity = capacity;
    backend.instance_buffer = SDL_CreateGPUBuffer(
        gpu,
        &(SDL_GPUBufferCreateInfo){
            .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
            .size = capacity * sizeof(SIInstance2D),
        }
    );
    backend.instance_transfer = SDL_CreateGPUTransferBuffer(
        gpu,
        &(SDL_GPUTransferBufferCreateInfo){
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = capacity * sizeof(SIInstance2D),
        }
    );
}

void sibackend_init(void) {}

void sibackend_shutdown(void) {
    SDL_GPUDevice *gpu = ecs_get_resource(SIEngineCtx)->primary_gpu;
    for (uint32_t pipeline = 0; pipeline < SI_PIPELINE_COUNT; pipeline++)
        for (uint32_t blend = 0; blend < SI_BLEND_COUNT; blend++)
            SDL_ReleaseGPUGraphicsPipeline(gpu, backend.pipelines[pipeline][blend]);
    SDL_ReleaseGPUShader(gpu, backend.vertex_shader);
    SDL_ReleaseGPUShader(gpu, backend.sprite_fragment_shader);
    SDL_ReleaseGPUShader(gpu, backend.shape_fragment_shader);
    SDL_ReleaseGPUShader(gpu, backend.circle_fragment_shader);
    SDL_ReleaseGPUSampler(gpu, backend.samplers[0]);
    SDL_ReleaseGPUSampler(gpu, backend.samplers[1]);
    SDL_ReleaseGPUBuffer(gpu, backend.geometry_buffer);
    SDL_ReleaseGPUTransferBuffer(gpu, backend.geometry_transfer);
    SDL_ReleaseGPUBuffer(gpu, backend.instance_buffer);
    SDL_ReleaseGPUTransferBuffer(gpu, backend.instance_transfer);
    backend = (SIBackendState){};
}

void sibackend_begin_frame(void) {
    backend.command = SDL_AcquireGPUCommandBuffer(ecs_get_resource(SIEngineCtx)->primary_gpu);
}

void sibackend_upload_instances(const void *data, uint32_t count, uint32_t stride) {
    SIEngineCtx *engine = ecs_get_resource(SIEngineCtx);
    ensure_instance_buffer(count ? count : 1);
    ensure_resources(SDL_GetGPUSwapchainTextureFormat(engine->primary_gpu, engine->window));
    if (count) {
        void *mapped = SDL_MapGPUTransferBuffer(engine->primary_gpu, backend.instance_transfer, true);
        memcpy(mapped, data, (size_t)count * stride);
        SDL_UnmapGPUTransferBuffer(engine->primary_gpu, backend.instance_transfer);
        SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(backend.command);
        SDL_UploadToGPUBuffer(
            copy,
            &(SDL_GPUTransferBufferLocation){ .transfer_buffer = backend.instance_transfer },
            &(SDL_GPUBufferRegion){ .buffer = backend.instance_buffer, .size = count * stride },
            true
        );
        SDL_EndGPUCopyPass(copy);
    }
    SDL_WaitAndAcquireGPUSwapchainTexture(
        backend.command,
        engine->window,
        &backend.swapchain,
        &backend.pixel_width,
        &backend.pixel_height
    );
    SDL_GPUColorTargetInfo target = {
        .texture = backend.swapchain,
        .clear_color = { 0.05f, 0.05f, 0.08f, 1.0f },
        .load_op = SDL_GPU_LOADOP_CLEAR,
        .store_op = SDL_GPU_STOREOP_STORE,
    };
    backend.pass = SDL_BeginGPURenderPass(backend.command, &target, 1, NULL);
    SDL_BindGPUVertexBuffers(
        backend.pass,
        0,
        (SDL_GPUBufferBinding[]){
            { .buffer = backend.geometry_buffer },
            { .buffer = backend.instance_buffer },
        },
        2
    );
}

typedef struct {
    float bounds[4];
    float texture_size[4];
    float sheet[4];
    float sheet_layout[4];
    float pivot[4];
} SISceneUniform;

static void set_viewport(const SIRenderView *view) {
    SIRenderViewport viewport =
        sirender_viewport_rect(view, backend.pixel_width, backend.pixel_height);
    SDL_SetGPUViewport(
        backend.pass,
        &(SDL_GPUViewport){
            .x = viewport.x,
            .y = viewport.y,
            .w = viewport.width,
            .h = viewport.height,
            .max_depth = 1.0f,
        }
    );
    SDL_SetGPUScissor(
        backend.pass,
        &(SDL_Rect){
            .x = (int)viewport.x,
            .y = (int)viewport.y,
            .w = (int)viewport.width,
            .h = (int)viewport.height,
        }
    );
}

void sibackend_draw_batch(const void *batch_data, const void *view_data) {
    static const uint32_t first_vertex[SI_GEOMETRY_COUNT] = { 0, 6 };
    static const uint32_t vertex_count[SI_GEOMETRY_COUNT] = { 6, 3 };
    const SIRenderBatch *batch = batch_data;
    const SIRenderView *view = view_data;
    SISceneUniform scene = {
        .bounds = { view->left, view->top, view->right, view->bottom },
        .texture_size = { (float)batch->texture_width, (float)batch->texture_height, 0, 0 },
        .sheet = { batch->has_sheet ? (float)batch->sheet.columns : 0.0f,
                   batch->has_sheet ? (float)batch->sheet.rows : 0.0f,
                   batch->has_sheet ? (float)batch->sheet.frame_width : 0.0f,
                   batch->has_sheet ? (float)batch->sheet.frame_height : 0.0f },
        .sheet_layout = { batch->has_sheet ? (float)batch->sheet.margin_x : 0.0f,
                          batch->has_sheet ? (float)batch->sheet.margin_y : 0.0f,
                          batch->has_sheet ? (float)batch->sheet.spacing_x : 0.0f,
                          batch->has_sheet ? (float)batch->sheet.spacing_y : 0.0f },
        .pivot = { batch->pivot_x, batch->pivot_y, 0, 0 },
    };
    set_viewport(view);
    SDL_PushGPUVertexUniformData(backend.command, 0, &scene, sizeof(scene));
    SDL_BindGPUGraphicsPipeline(backend.pass, backend.pipelines[batch->pipeline][batch->blend]);
    if (batch->pipeline == SI_PIPELINE_SPRITE) {
        SDL_BindGPUFragmentSamplers(
            backend.pass,
            0,
            &(SDL_GPUTextureSamplerBinding){
                .texture = (SDL_GPUTexture *)(uintptr_t)batch->gpu_texture,
                .sampler = backend.samplers[batch->filter],
            },
            1
        );
    }
    SDL_DrawGPUPrimitives(
        backend.pass,
        vertex_count[batch->geometry],
        batch->instance_count,
        first_vertex[batch->geometry],
        batch->instance_offset
    );
}

void sibackend_end_frame(void) {
    SDL_EndGPURenderPass(backend.pass);
    SDL_SubmitGPUCommandBuffer(backend.command);
    backend.command = NULL;
    backend.pass = NULL;
}

void sibackend_texture_create(const char *path, SIFilterMode filter, void *texture_data) {
    SITexture *texture = texture_data;
    (void)filter;
    SIEngineCtx *engine = ecs_get_resource(SIEngineCtx);
    SDL_GPUCommandBuffer *command = SDL_AcquireGPUCommandBuffer(engine->primary_gpu);
    SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(command);
    int width;
    int height;
    SDL_GPUTexture *gpu_texture = IMG_LoadGPUTexture(engine->primary_gpu, copy, path, &width, &height);
    SDL_EndGPUCopyPass(copy);
    SDL_GPUFence *fence = SDL_SubmitGPUCommandBufferAndAcquireFence(command);
    SDL_WaitForGPUFences(engine->primary_gpu, true, &fence, 1);
    SDL_ReleaseGPUFence(engine->primary_gpu, fence);
    texture->gpu_handle = (uint64_t)(uintptr_t)gpu_texture;
    texture->width = (uint32_t)width;
    texture->height = (uint32_t)height;
}

void sibackend_texture_destroy(void *texture_data) {
    SITexture *texture = texture_data;
    SDL_ReleaseGPUTexture(
        ecs_get_resource(SIEngineCtx)->primary_gpu,
        (SDL_GPUTexture *)(uintptr_t)texture->gpu_handle
    );
}

#endif
