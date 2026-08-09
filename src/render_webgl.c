#if defined(__EMSCRIPTEN__)
#include "render_internal.h"
#include <SDL3/SDL_video.h>
#include <math.h>
#include <stddef.h>

static const char *vertex_shader_source =
    "#version 300 es\n"
    "precision highp float;\n"
    "layout(location = 0) in vec2 position;\n"
    "layout(location = 1) in vec2 uv;\n"
    "layout(location = 2) in vec4 color;\n"
    "out vec2 out_uv;\n"
    "out vec4 out_color;\n"
    "void main() {\n"
    "    gl_Position = vec4(position, 0.0, 1.0);\n"
    "    out_uv = uv;\n"
    "    out_color = color;\n"
    "}\n";

static const char *sprite_fragment_shader_source =
    "#version 300 es\n"
    "precision highp float;\n"
    "uniform sampler2D sprite_texture;\n"
    "in vec2 out_uv;\n"
    "in vec4 out_color;\n"
    "out vec4 color;\n"
    "void main() { color = texture(sprite_texture, out_uv) * out_color; }\n";

static const char *shape_fragment_shader_source =
    "#version 300 es\n"
    "precision highp float;\n"
    "in vec4 out_color;\n"
    "out vec4 color;\n"
    "void main() { color = out_color; }\n";

static const char *circle_fragment_shader_source =
    "#version 300 es\n"
    "precision highp float;\n"
    "in vec2 out_uv;\n"
    "in vec4 out_color;\n"
    "out vec4 color;\n"
    "void main() {\n"
    "    float distance = length(out_uv * 2.0 - 1.0);\n"
    "    float edge = fwidth(distance);\n"
    "    float alpha = 1.0 - smoothstep(1.0 - edge, 1.0 + edge, distance);\n"
    "    if (alpha <= 0.0) discard;\n"
    "    color = vec4(out_color.rgb, out_color.a * alpha);\n"
    "}\n";

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

static void ensure_webgl_resources(SIRenderState *render) {
    if (render->programs[0])
        return;

    render->programs[SI_RENDER_PIPELINE_SPRITE] =
        create_program(sprite_fragment_shader_source);
    render->programs[SI_RENDER_PIPELINE_SHAPE] =
        create_program(shape_fragment_shader_source);
    render->programs[SI_RENDER_PIPELINE_CIRCLE] =
        create_program(circle_fragment_shader_source);

    glGenVertexArrays(1, &render->vertex_array);
    glBindVertexArray(render->vertex_array);
    glGenBuffers(1, &render->vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, render->vertex_buffer);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(SIRenderVertex), (void *)offsetof(SIRenderVertex, x));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(SIRenderVertex), (void *)offsetof(SIRenderVertex, u));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(SIRenderVertex), (void *)offsetof(SIRenderVertex, r));

    glUseProgram(render->programs[SI_RENDER_PIPELINE_SPRITE]);
    glUniform1i(glGetUniformLocation(render->programs[SI_RENDER_PIPELINE_SPRITE], "sprite_texture"), 0);
    glBindVertexArray(0);
}

static void set_blend_mode(SIBlendModeValue blend) {
    glEnable(GL_BLEND);
    if (blend == SI_BLEND_ADDITIVE)
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    else if (blend == SI_BLEND_MULTIPLY)
        glBlendFunc(GL_DST_COLOR, GL_ZERO);
    else
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

static void set_viewport(const SIRenderView *view, uint32_t pixel_width, uint32_t pixel_height) {
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

    int x = (int)viewport_x;
    int y = (int)((float)pixel_height - viewport_y - viewport_height);
    int width = (int)viewport_width;
    int height = (int)viewport_height;
    glViewport(x, y, width, height);
    glScissor(x, y, width, height);
}

void sirender_draw_window(ecs_iter_t *it) {
    (void)it;
    SIEngineCtx *engine = ecs_resource(SIEngineCtx);
    SIRenderState *render = ecs_resource(SIRenderState);
    int pixel_width;
    int pixel_height;

    if (!engine->window || !render->frame_started)
        return;

    SDL_GetWindowSizeInPixels(engine->window, &pixel_width, &pixel_height);
    ensure_webgl_resources(render);
    glBindVertexArray(render->vertex_array);
    glBindBuffer(GL_ARRAY_BUFFER, render->vertex_buffer);
    uint32_t vertex_count = sirender_build_vertices(render);
    if (vertex_count) {
        glBufferData(
            GL_ARRAY_BUFFER,
            (GLsizeiptr)(vertex_count * sizeof(*render->vertices)),
            render->vertices,
            GL_DYNAMIC_DRAW
        );
    }

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glScissor(0, 0, (int)pixel_width, (int)pixel_height);
    glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    for (uint32_t view_index = 0; view_index < render->view_count; view_index++) {
        SIRenderView *view = &render->views[view_index];
        set_viewport(view, pixel_width, pixel_height);

        for (uint32_t first = 0; first < view->queue.count;) {
            SIRenderCommand *command = &view->queue.commands[first];
            uint32_t last = first + 1;
            while (last < view->queue.count &&
                   view->queue.commands[last].pipeline == command->pipeline &&
                   view->queue.commands[last].texture == command->texture &&
                   view->queue.commands[last].filter == command->filter &&
                   view->queue.commands[last].blend == command->blend) {
                last++;
            }

            glUseProgram(render->programs[command->pipeline]);
            set_blend_mode(command->blend);
            if (command->pipeline == SI_RENDER_PIPELINE_SPRITE) {
                SITextureSlot *slot = siengine_texture_slot(command->texture);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, slot->webgl);
            }

            uint32_t primitive_count = 0;
            for (uint32_t i = first; i < last; i++)
                primitive_count += view->queue.commands[i].primitive == SI_RENDER_TRIANGLE ? 1 : 2;
            glDrawArrays(GL_TRIANGLES, (GLint)command->vertex_offset, (GLsizei)(primitive_count * 3));
            first = last;
        }
    }

    glBindVertexArray(0);
}

void sirender_webgl_shutdown(SIRenderState *render) {
    for (uint32_t i = 0; i < 3; i++) {
        if (render->programs[i])
            glDeleteProgram(render->programs[i]);
    }
    if (render->vertex_buffer)
        glDeleteBuffers(1, &render->vertex_buffer);
    if (render->vertex_array)
        glDeleteVertexArrays(1, &render->vertex_array);
}
#endif
