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
    GLuint programs[3];
    GLuint vertex_array;
    GLuint geometry_buffer;
    GLuint instance_buffer;
    uint32_t instance_capacity;
    bool initialized;
    GLint camera_uniforms[3];
    GLint texture_size_uniforms[3];
    GLint sheet_uniforms[3];
    GLint sheet_layout_uniforms[3];
    GLint pivot_uniforms[3];
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

static void backend_init_resources(void) {
    backend.programs[SI_RENDER_SPRITE] = create_program(sprite_fragment_shader_source);
    backend.programs[SI_RENDER_RECTANGLE] = create_program(shape_fragment_shader_source);
    backend.programs[SI_RENDER_CIRCLE] = create_program(circle_fragment_shader_source);
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
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(SIInstance2D), (void *)offsetof(SIInstance2D, x));
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(SIInstance2D), (void *)offsetof(SIInstance2D, rotation));
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(SIInstance2D), (void *)offsetof(SIInstance2D, scale_x));
    glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, sizeof(SIInstance2D), (void *)offsetof(SIInstance2D, width));
    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(SIInstance2D), (void *)offsetof(SIInstance2D, color));
    glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, sizeof(SIInstance2D), (void *)offsetof(SIInstance2D, frame_index));
    glVertexAttribPointer(7, 2, GL_FLOAT, GL_FALSE, sizeof(SIInstance2D), (void *)offsetof(SIInstance2D, flip_x));
    for (uint32_t i = 1; i < 8; i++)
        glVertexAttribDivisor(i, 1);
    for (uint32_t i = 0; i < 3; i++) {
        glUseProgram(backend.programs[i]);
        backend.camera_uniforms[i] = glGetUniformLocation(backend.programs[i], "camera_bounds");
        backend.texture_size_uniforms[i] = glGetUniformLocation(backend.programs[i], "texture_size");
        backend.sheet_uniforms[i] = glGetUniformLocation(backend.programs[i], "sheet");
        backend.sheet_layout_uniforms[i] = glGetUniformLocation(backend.programs[i], "sheet_layout");
        backend.pivot_uniforms[i] = glGetUniformLocation(backend.programs[i], "pivot");
    }
    glUseProgram(backend.programs[SI_RENDER_SPRITE]);
    glUniform1i(glGetUniformLocation(backend.programs[SI_RENDER_SPRITE], "sprite_texture"), 0);
    glBindVertexArray(0);
    backend.initialized = true;
}

void sibackend_init(void) {}

void sibackend_shutdown(void) {
    glDeleteProgram(backend.programs[0]);
    glDeleteProgram(backend.programs[1]);
    glDeleteProgram(backend.programs[2]);
    glDeleteBuffers(1, &backend.geometry_buffer);
    glDeleteBuffers(1, &backend.instance_buffer);
    glDeleteVertexArrays(1, &backend.vertex_array);
    backend = (SIBackendState){};
}

void sibackend_begin_frame(void) {
    SIEngineCtx *engine = ecs_get_resource(SIEngineCtx);
    emscripten_webgl_make_context_current((EMSCRIPTEN_WEBGL_CONTEXT_HANDLE)(uintptr_t)engine->gl_context);
    if (!backend.initialized)
        backend_init_resources();
    glBindVertexArray(backend.vertex_array);
}

void sibackend_upload_instances(const void *data, uint32_t count, uint32_t stride) {
    glBindBuffer(GL_ARRAY_BUFFER, backend.instance_buffer);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)count * stride, data, GL_DYNAMIC_DRAW);
    int width;
    int height;
    SDL_GetWindowSizeInPixels(ecs_get_resource(SIEngineCtx)->window, &width, &height);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glScissor(0, 0, width, height);
    glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

static void set_viewport(const SIRenderView *view, uint32_t pixel_width, uint32_t pixel_height) {
    SIRenderViewport viewport = sirender_viewport_rect(view, pixel_width, pixel_height);
    glViewport(
        (int)viewport.x,
        (int)((float)pixel_height - viewport.y - viewport.height),
        (int)viewport.width,
        (int)viewport.height
    );
    glScissor(
        (int)viewport.x,
        (int)((float)pixel_height - viewport.y - viewport.height),
        (int)viewport.width,
        (int)viewport.height
    );
}

void sibackend_draw_batch(const void *batch_data, const void *view_data) {
    const SIRenderBatch *batch = batch_data;
    const SIRenderView *view = view_data;
    GLuint program = backend.programs[batch->primitive];
    int width;
    int height;
    SDL_GetWindowSizeInPixels(ecs_get_resource(SIEngineCtx)->window, &width, &height);
    set_viewport(view, width, height);
    glUseProgram(program);
    glUniform4f(backend.camera_uniforms[batch->primitive], view->left, view->top, view->right, view->bottom);
    glUniform4f(backend.texture_size_uniforms[batch->primitive], (float)batch->texture_width, (float)batch->texture_height, 0, 0);
    glUniform4f(backend.sheet_uniforms[batch->primitive], batch->has_sheet ? (float)batch->sheet.columns : 0, batch->has_sheet ? (float)batch->sheet.rows : 0, batch->has_sheet ? (float)batch->sheet.frame_width : 0, batch->has_sheet ? (float)batch->sheet.frame_height : 0);
    glUniform4f(backend.sheet_layout_uniforms[batch->primitive], batch->has_sheet ? (float)batch->sheet.margin_x : 0, batch->has_sheet ? (float)batch->sheet.margin_y : 0, batch->has_sheet ? (float)batch->sheet.spacing_x : 0, batch->has_sheet ? (float)batch->sheet.spacing_y : 0);
    glUniform4f(backend.pivot_uniforms[batch->primitive], batch->pivot_x, batch->pivot_y, 0, 0);
    glEnable(GL_BLEND);
    if (batch->blend == SI_BLEND_ADDITIVE)
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    else if (batch->blend == SI_BLEND_MULTIPLY)
        glBlendFunc(GL_DST_COLOR, GL_ZERO);
    else
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    if (batch->primitive == SI_RENDER_SPRITE) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, (GLuint)batch->gpu_texture);
    }
    glDrawArraysInstanced(
        GL_TRIANGLES,
        batch->primitive == SI_RENDER_TRIANGLE ? 6 : 0,
        batch->primitive == SI_RENDER_TRIANGLE ? 3 : 6,
        batch->instance_count
    );
}

void sibackend_end_frame(void) { glBindVertexArray(0); }

void sibackend_texture_create(const char *path, SIFilterMode filter, void *texture_data) {
    if (!backend.initialized)
        backend_init_resources();
    SITexture *texture = texture_data;
    SDL_Surface *loaded = IMG_Load(path);
    SDL_Surface *surface = SDL_ConvertSurface(loaded, SDL_PIXELFORMAT_RGBA32);
    texture->width = (uint32_t)surface->w;
    texture->height = (uint32_t)surface->h;
    GLuint gpu_texture;
    glGenTextures(1, &gpu_texture);
    glBindTexture(GL_TEXTURE_2D, gpu_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter == SI_FILTER_LINEAR ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter == SI_FILTER_LINEAR ? GL_LINEAR : GL_NEAREST);
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
