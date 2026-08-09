#include "engine_internal.h"
#include "siecs.h"
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_hints.h>
#include <SDL3/SDL_init.h>
#include <stdio.h>

#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#endif

SIEngineCtx siplatform_init(void) {
    SIEngineCtx context = {};
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
    context.primary_gpu = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, gpu_debug, "vulkan");
#endif
    return context;
}

void siplatform_shutdown(SIEngineCtx *context) {
#if !defined(__EMSCRIPTEN__)
    SDL_WaitForGPUIdle(context->primary_gpu);
    SDL_DestroyGPUDevice(context->primary_gpu);
#else
    (void)context;
#endif
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

#if defined(__EMSCRIPTEN__)
static void siplatform_web_frame(void *context) {
    (void)context;
    if (!ecs_progress())
        emscripten_cancel_main_loop();
}
#endif

void siplatform_run(void) {
#if defined(__EMSCRIPTEN__)
    emscripten_set_main_loop_arg(siplatform_web_frame, NULL, 0, true);
#else
    ecs_run();
#endif
}
