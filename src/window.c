#include "engine_internal.h"
#include "siecs.h"
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_properties.h>
#include <SDL3/SDL_video.h>
#include <siengine.h>
#include <stdint.h>
#if defined(__EMSCRIPTEN__)
#include <emscripten/html5_webgl.h>
#endif

#if !defined(__EMSCRIPTEN__)
static void configure_swapchain(SIEngineCtx *ctx, SDL_Window *window, const SIWindow *window_desc) {
    SDL_GPUPresentMode present_mode = SDL_GPU_PRESENTMODE_VSYNC;
    if (window_desc->vsync &&
        SDL_WindowSupportsGPUPresentMode(ctx->primary_gpu, window, SDL_GPU_PRESENTMODE_MAILBOX)) {
        present_mode = SDL_GPU_PRESENTMODE_MAILBOX;
    } else if (!window_desc->vsync) {
        if (SDL_WindowSupportsGPUPresentMode(
                ctx->primary_gpu,
                window,
                SDL_GPU_PRESENTMODE_IMMEDIATE
            )) {
            present_mode = SDL_GPU_PRESENTMODE_IMMEDIATE;
        } else if (
            SDL_WindowSupportsGPUPresentMode(ctx->primary_gpu, window, SDL_GPU_PRESENTMODE_MAILBOX)
        ) {
            present_mode = SDL_GPU_PRESENTMODE_MAILBOX;
        }
    }

    SDL_SetGPUSwapchainParameters(
        ctx->primary_gpu,
        window,
        SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
        present_mode
    );
}
#endif

static void create_window(const SIWindow *window_desc);

static void on_window_set(const void *new_value) {
    SIEngineCtx *ctx = ecs_get_resource(SIEngineCtx);
    const SIWindow *window_desc = new_value;
    uint32_t width = window_desc->width ? window_desc->width : 1280;
    uint32_t height = window_desc->height ? window_desc->height : 720;

    if (!ctx->window) {
        create_window(window_desc);
    } else {
        SDL_SetWindowTitle(ctx->window, window_desc->title);
        SDL_SetWindowResizable(ctx->window, window_desc->resizable);
        SDL_SetWindowSize(ctx->window, (int)width, (int)height);
#if !defined(__EMSCRIPTEN__)
        configure_swapchain(ctx, ctx->window, window_desc);
#endif
    }
}

static void create_window(const SIWindow *window_desc) {
    SIEngineCtx *ctx = ecs_get_resource(SIEngineCtx);

    uint32_t width = window_desc->width ? window_desc->width : 1280;
    uint32_t height = window_desc->height ? window_desc->height : 720;
    SDL_WindowFlags flags = window_desc->resizable ? SDL_WINDOW_RESIZABLE : 0;

#if defined(__EMSCRIPTEN__)
    SDL_PropertiesID properties = SDL_CreateProperties();
    SDL_SetStringProperty(properties, SDL_PROP_WINDOW_CREATE_TITLE_STRING, window_desc->title);
    SDL_SetNumberProperty(properties, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, width);
    SDL_SetNumberProperty(properties, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, height);
    SDL_SetNumberProperty(
        properties,
        SDL_PROP_WINDOW_CREATE_FLAGS_NUMBER,
        flags | SDL_WINDOW_OPENGL | SDL_WINDOW_HIGH_PIXEL_DENSITY
    );
    char canvas_selector[130] = "#canvas";
    if (window_desc->canvas_id[0]) {
        if (window_desc->canvas_id[0] == '#') {
            SDL_strlcpy(canvas_selector, window_desc->canvas_id, sizeof(canvas_selector));
        } else {
            canvas_selector[0] = '#';
            SDL_strlcpy(canvas_selector + 1, window_desc->canvas_id, sizeof(canvas_selector) - 1);
        }
    }
    SDL_SetStringProperty(
        properties,
        SDL_PROP_WINDOW_CREATE_EMSCRIPTEN_CANVAS_ID_STRING,
        canvas_selector
    );
    ctx->window = SDL_CreateWindowWithProperties(properties);
    SDL_DestroyProperties(properties);
    EmscriptenWebGLContextAttributes attributes;
    emscripten_webgl_init_context_attributes(&attributes);
    attributes.majorVersion = 2;
    attributes.minorVersion = 0;
    attributes.enableExtensionsByDefault = true;
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE gl_context =
        emscripten_webgl_create_context(canvas_selector, &attributes);
    ctx->gl_context = (SDL_GLContext)(uintptr_t)gl_context;
    emscripten_webgl_make_context_current(gl_context);
#else
    ctx->window = SDL_CreateWindow(
        window_desc->title,
        (int)width,
        (int)height,
        flags | SDL_WINDOW_HIGH_PIXEL_DENSITY
    );
    SDL_ClaimWindowForGPUDevice(ctx->primary_gpu, ctx->window);

    configure_swapchain(ctx, ctx->window, window_desc);
#endif
}

void siwindow_poll_events(void) {
    SDL_Event e;

    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT || e.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
            ecs_quit();
            return;
        }
    }
}

static void poll_window_events_system(ecs_iter_t *it) {
    (void)it;
    siwindow_poll_events();
}

static void on_window_remove(const void *value) {
    (void)value;
    SIEngineCtx *ctx = ecs_get_resource(SIEngineCtx);

    if (ctx->window) {
#if defined(__EMSCRIPTEN__)
        emscripten_webgl_destroy_context(
            (EMSCRIPTEN_WEBGL_CONTEXT_HANDLE)(uintptr_t)ctx->gl_context
        );
        ctx->gl_context = NULL;
#else
        SDL_ReleaseWindowFromGPUDevice(ctx->primary_gpu, ctx->window);
#endif
        SDL_DestroyWindow(ctx->window);
        ctx->window = NULL;
    }
}

ECS_RESOURCE_DEFINE(SIWindow, .on_set = on_window_set, .on_remove = on_window_remove);

void siwindow_register() {
    ECS_RESOURCE_REGISTER(SIWindow);
    ecs_system(
        {
            .name = "PollWindowEvents",
            .phase = EcsPreUpdate,
            .callback = poll_window_events_system,
            .main_thread_only = true,
            .read_resources = { ecs_id(SIEngineCtx) },
        }
    );
}
