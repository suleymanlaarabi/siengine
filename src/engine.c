#include "engine_internal.h"
#include "siecs.h"
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_hints.h>
#include <SDL3/SDL_init.h>
#include <siengine.h>
#include <stdio.h>
#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#endif

ECS_MODULE_DEFINE(siengine);

static void on_engine_remove(const void *ptr) {
    SIEngineCtx *ctx = (SIEngineCtx *)ptr;
    siassets_shutdown();
    sirender_shutdown();
    siwindow_shutdown();
#if !defined(__EMSCRIPTEN__)
    SDL_DestroyGPUDevice(ctx->primary_gpu);
#else
    (void)ctx;
#endif
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

ECS_RESOURCE_DEFINE(SIEngineCtx, .on_remove = on_engine_remove);

void siengine_import(const siengine_props_t *) {
    ECS_MODULE_IMPORT(siscene2d, {});
#if !defined(__EMSCRIPTEN__)
    SDL_SetHintWithPriority(SDL_HINT_VIDEO_DRIVER, "x11,wayland", SDL_HINT_OVERRIDE);
#endif
    SDL_Init(SDL_INIT_VIDEO);
    fprintf(stderr, "siengine: SDL video driver: %s\n", SDL_GetCurrentVideoDriver());

#if !defined(__EMSCRIPTEN__)
#ifdef NDEBUG
    const bool gpu_debug = false;
#else
    const bool gpu_debug = true;
#endif
    SDL_GPUDevice *gpu = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, gpu_debug, "vulkan");
#endif

    ECS_RESOURCE_REGISTER(SIEngineCtx);
#if !defined(__EMSCRIPTEN__)
    ecs_set_resource(SIEngineCtx, { .primary_gpu = gpu });
#else
    ecs_set_resource(SIEngineCtx, {});
#endif

    siwindow_register();
    siassets_register();
    sirender_register();
}

#if defined(__EMSCRIPTEN__)
static void siengine_web_frame(void *ctx) {
    (void)ctx;
    if (!ecs_progress())
        emscripten_cancel_main_loop();
}
#endif

void siengine_run(void) {
#if defined(__EMSCRIPTEN__)
    emscripten_set_main_loop_arg(siengine_web_frame, NULL, 0, true);
#else
    ecs_run();
#endif
}
