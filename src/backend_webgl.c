#if defined(__EMSCRIPTEN__)

#include "assets_internal.h"
#include "backend.h"
#include "engine_internal.h"
#include "render_internal.h"
#include "backend_webgl_shaders.h"
#include <GLES3/gl3.h>
#include <SDL3_image/SDL_image.h>
#include <emscripten/html5_webgl.h>
#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    GLuint programs[SI_PIPELINE_COUNT];
    GLuint vertex_array;
    GLuint geometry_buffer;
    GLuint instance_buffer;
    uint32_t instance_capacity;
    uint32_t pixel_width;
    uint32_t pixel_height;
    GLuint samplers[SI_FILTER_COUNT];
    bool initialized;
    GLint camera_uniforms[SI_PIPELINE_COUNT];
    GLint texture_size_uniforms[SI_PIPELINE_COUNT];
    GLint sheet_uniforms[SI_PIPELINE_COUNT];
    GLint sheet_layout_uniforms[SI_PIPELINE_COUNT];
    GLint pivot_uniforms[SI_PIPELINE_COUNT];
} SIBackendState;

static SIBackendState backend;

static GLuint compile_shader(GLenum type, const char *source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    return shader;
}

static GLuint create_program(const char *fragment_source) {
    GLuint vertex = compile_shader(GL_VERTEX_SHADER, vertex_shader_source);
    GLuint fragment = compile_shader(GL_FRAGMENT_SHADER, fragment_source);
    GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    return program;
}

static void bind_instance_offset(uint32_t first_instance) {
    uintptr_t base = (uintptr_t)first_instance * sizeof(SIInstance2D);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(SIInstance2D), (void *)(base + offsetof(SIInstance2D, x)));
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(SIInstance2D), (void *)(base + offsetof(SIInstance2D, rotation)));
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(SIInstance2D), (void *)(base + offsetof(SIInstance2D, scale_x)));
    glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, sizeof(SIInstance2D), (void *)(base + offsetof(SIInstance2D, width)));
    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(SIInstance2D), (void *)(base + offsetof(SIInstance2D, color)));
    glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, sizeof(SIInstance2D), (void *)(base + offsetof(SIInstance2D, frame_index)));
    glVertexAttribPointer(7, 2, GL_FLOAT, GL_FALSE, sizeof(SIInstance2D), (void *)(base + offsetof(SIInstance2D, flip_x)));
}

static void backend_init_resources(void) {
    backend.programs[SI_PIPELINE_SPRITE] = create_program(sprite_fragment_shader_source);
    backend.programs[SI_PIPELINE_SHAPE] = create_program(shape_fragment_shader_source);
    backend.programs[SI_PIPELINE_CIRCLE] = create_program(circle_fragment_shader_source);
    glGenVertexArrays(1, &backend.vertex_array);
    glBindVertexArray(backend.vertex_array);
    static const float geometry[] = {
        0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1,
        -0.5f, 0.5f, 0.5f, 0.5f, 0, -0.5f,
    };
    glGenBuffers(1, &backend.geometry_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, backend.geometry_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(geometry), geometry, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, (void *)0);
    glGenBuffers(1, &backend.instance_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, backend.instance_buffer);
    for (uint32_t i = 1; i < 8; i++)
        glEnableVertexAttribArray(i);
    bind_instance_offset(0);
    for (uint32_t i = 1; i < 8; i++)
        glVertexAttribDivisor(i, 1);
    for (uint32_t i = 0; i < SI_PIPELINE_COUNT; i++) {
        glUseProgram(backend.programs[i]);
        backend.camera_uniforms[i] = glGetUniformLocation(backend.programs[i], "camera_bounds");
        backend.texture_size_uniforms[i] = glGetUniformLocation(backend.programs[i], "texture_size");
        backend.sheet_uniforms[i] = glGetUniformLocation(backend.programs[i], "sheet");
        backend.sheet_layout_uniforms[i] = glGetUniformLocation(backend.programs[i], "sheet_layout");
        backend.pivot_uniforms[i] = glGetUniformLocation(backend.programs[i], "pivot");
    }
    glUseProgram(backend.programs[SI_PIPELINE_SPRITE]);
    glUniform1i(glGetUniformLocation(backend.programs[SI_PIPELINE_SPRITE], "sprite_texture"), 0);
    glGenSamplers(SI_FILTER_COUNT, backend.samplers);
    for (uint32_t filter = 0; filter < SI_FILTER_COUNT; filter++) {
        GLint filter_mode = filter == SI_FILTER_LINEAR ? GL_LINEAR : GL_NEAREST;
        glSamplerParameteri(backend.samplers[filter], GL_TEXTURE_MIN_FILTER, filter_mode);
        glSamplerParameteri(backend.samplers[filter], GL_TEXTURE_MAG_FILTER, filter_mode);
        glSamplerParameteri(backend.samplers[filter], GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glSamplerParameteri(backend.samplers[filter], GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    glBindVertexArray(0);
    backend.initialized = true;
}

void sibackend_init(void) {}

void sibackend_shutdown(void) {
    for (uint32_t pipeline = 0; pipeline < SI_PIPELINE_COUNT; pipeline++)
        glDeleteProgram(backend.programs[pipeline]);
    glDeleteBuffers(1, &backend.geometry_buffer);
    glDeleteBuffers(1, &backend.instance_buffer);
    glDeleteVertexArrays(1, &backend.vertex_array);
    glDeleteSamplers(SI_FILTER_COUNT, backend.samplers);
    backend = (SIBackendState){};
}

void sibackend_begin_frame(void) {
    SIEngineCtx *engine = ecs_get_resource(SIEngineCtx);
    emscripten_webgl_make_context_current((EMSCRIPTEN_WEBGL_CONTEXT_HANDLE)(uintptr_t)engine->gl_context);
    if (!backend.initialized)
        backend_init_resources();
    glBindVertexArray(backend.vertex_array);
}

static void ensure_instance_capacity(uint32_t required_count) {
    if (required_count <= backend.instance_capacity)
        return;

    uint32_t capacity = backend.instance_capacity ? backend.instance_capacity : 256;
    while (capacity < required_count)
        capacity *= 2;
    glBindBuffer(GL_ARRAY_BUFFER, backend.instance_buffer);
    glBufferData(
        GL_ARRAY_BUFFER,
        (GLsizeiptr)capacity * sizeof(SIInstance2D),
        NULL,
        GL_DYNAMIC_DRAW
    );
    backend.instance_capacity = capacity;
}

void sibackend_upload_instances(const void *data, uint32_t count, uint32_t stride) {
    int width;
    int height;
    SDL_GetWindowSizeInPixels(ecs_get_resource(SIEngineCtx)->window, &width, &height);
    backend.pixel_width = (uint32_t)width;
    backend.pixel_height = (uint32_t)height;
    ensure_instance_capacity(count);
    glBindBuffer(GL_ARRAY_BUFFER, backend.instance_buffer);
    glBufferSubData(GL_ARRAY_BUFFER, 0, (GLsizeiptr)count * stride, data);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glScissor(0, 0, backend.pixel_width, backend.pixel_height);
    glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

static void set_viewport(const SIRenderView *view) {
    SIRenderViewport viewport = sirender_viewport_rect(view, backend.pixel_width, backend.pixel_height);
    glViewport(
        (int)viewport.x,
        (int)((float)backend.pixel_height - viewport.y - viewport.height),
        (int)viewport.width,
        (int)viewport.height
    );
    glScissor(
        (int)viewport.x,
        (int)((float)backend.pixel_height - viewport.y - viewport.height),
        (int)viewport.width,
        (int)viewport.height
    );
}

void sibackend_draw_batch(const void *batch_data, const void *view_data) {
    static const uint32_t first_vertex[SI_GEOMETRY_COUNT] = { 0, 6 };
    static const uint32_t vertex_count[SI_GEOMETRY_COUNT] = { 6, 3 };
    const SIRenderBatch *batch = batch_data;
    const SIRenderView *view = view_data;
    GLuint program = backend.programs[batch->pipeline];
    set_viewport(view);
    glUseProgram(program);
    glUniform4f(backend.camera_uniforms[batch->pipeline], view->left, view->top, view->right, view->bottom);
    glUniform4f(backend.texture_size_uniforms[batch->pipeline], (float)batch->texture_width, (float)batch->texture_height, 0, 0);
    glUniform4f(backend.sheet_uniforms[batch->pipeline], batch->has_sheet ? (float)batch->sheet.columns : 0, batch->has_sheet ? (float)batch->sheet.rows : 0, batch->has_sheet ? (float)batch->sheet.frame_width : 0, batch->has_sheet ? (float)batch->sheet.frame_height : 0);
    glUniform4f(backend.sheet_layout_uniforms[batch->pipeline], batch->has_sheet ? (float)batch->sheet.margin_x : 0, batch->has_sheet ? (float)batch->sheet.margin_y : 0, batch->has_sheet ? (float)batch->sheet.spacing_x : 0, batch->has_sheet ? (float)batch->sheet.spacing_y : 0);
    glUniform4f(backend.pivot_uniforms[batch->pipeline], batch->pivot_x, batch->pivot_y, 0, 0);
    glEnable(GL_BLEND);
    if (batch->blend == SI_BLEND_ADDITIVE)
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    else if (batch->blend == SI_BLEND_MULTIPLY)
        glBlendFunc(GL_DST_COLOR, GL_ZERO);
    else
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    if (batch->pipeline == SI_PIPELINE_SPRITE) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, (GLuint)batch->gpu_texture);
        glBindSampler(0, backend.samplers[batch->filter]);
    }
    bind_instance_offset(batch->instance_offset);
    glDrawArraysInstanced(
        GL_TRIANGLES,
        first_vertex[batch->geometry],
        vertex_count[batch->geometry],
        batch->instance_count
    );
}

void sibackend_end_frame(void) { glBindVertexArray(0); }

void sibackend_texture_create(const char *path, SIFilterMode filter, void *texture_data) {
    if (!backend.initialized)
        backend_init_resources();
    (void)filter;
    SITexture *texture = texture_data;
    SDL_Surface *loaded = IMG_Load(path);
    SDL_Surface *surface = SDL_ConvertSurface(loaded, SDL_PIXELFORMAT_RGBA32);
    texture->width = (uint32_t)surface->w;
    texture->height = (uint32_t)surface->h;
    GLuint gpu_texture;
    glGenTextures(1, &gpu_texture);
    glBindTexture(GL_TEXTURE_2D, gpu_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w, surface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels);
    SDL_DestroySurface(loaded);
    SDL_DestroySurface(surface);
    texture->gpu_handle = gpu_texture;
}

void sibackend_texture_destroy(void *texture_data) {
    SITexture *texture = texture_data;
    GLuint gpu_texture = (GLuint)texture->gpu_handle;
    glDeleteTextures(1, &gpu_texture);
}

#endif
