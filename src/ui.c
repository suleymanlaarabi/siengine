#include "engine_internal.h"
#include <SDL3/SDL_events.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <siengine.h>
#include <siui_sdl_gpu.h>

#define SIUI_DEFAULT_FONT "/usr/share/fonts/noto/NotoSans-Regular.ttf"

ECS_RESOURCE_DECLARE(SIUIState, {
    siui_sdlgpu_t *backend;
    TTF_Font *font;
});

ECS_RESOURCE_DEFINE(SIUIState);

static SIUIState *siui_state(void) { return ecs_resource(SIUIState); }

static void siui_release(void) {
    SIUIState *state = siui_state();

    siui_sdlgpu_destroy(state->backend);
    TTF_CloseFont(state->font);
    state->backend = NULL;
    state->font = NULL;
}

void siui_attach_window() {
    SIEngineCtx *engine = ecs_resource(SIEngineCtx);
    SIUIRoot *root = ecs_try_get_resource(SIUIRoot);
    SIUIState *state = siui_state();

    if (!engine->window || !root || !root->render || state->backend)
        return;

    TTF_Font *font = TTF_OpenFont(SIUI_DEFAULT_FONT, 16.0f);
    siui_sdlgpu_t *backend = siui_sdlgpu_create(&(siui_sdlgpu_desc_t){
        .device = engine->primary_gpu,
        .window = engine->window,
        .default_font = font,
        .default_font_size = 16.0f,
    });

    if (!backend) {
        TTF_CloseFont(font);
        return;
    }

    state->backend = backend;
    state->font = font;
    siui_mount_impl(root->render, NULL, 0);
}

bool siui_handle_event(const SDL_Event *event) {
    SIUIState *state = siui_state();
    siinput_event_t input = { 0 };

    if (!state->backend || !event)
        return false;

    switch (event->type) {
    case SDL_EVENT_MOUSE_MOTION:
        input.type = SIUI_EVENT_POINTER_MOVE;
        input.x = event->motion.x;
        input.y = event->motion.y;
        input.buttons = event->motion.state;
        break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        input.type = SIUI_EVENT_POINTER_DOWN;
        input.x = event->button.x;
        input.y = event->button.y;
        input.button = event->button.button;
        input.clicks = event->button.clicks;
        break;
    case SDL_EVENT_MOUSE_BUTTON_UP:
        input.type = SIUI_EVENT_POINTER_UP;
        input.x = event->button.x;
        input.y = event->button.y;
        input.button = event->button.button;
        input.clicks = event->button.clicks;
        break;
    case SDL_EVENT_MOUSE_WHEEL:
        input.type = SIUI_EVENT_SCROLL;
        input.x = event->wheel.mouse_x;
        input.y = event->wheel.mouse_y;
        input.delta_x = event->wheel.x * 12;
        input.delta_y = event->wheel.y * 12;
        break;
    default:
        return false;
    }

    return siui_dispatch(&input);
}

bool siui_render(
    SDL_GPUCommandBuffer *command,
    SDL_GPUTexture *texture,
    uint32_t pixel_width,
    uint32_t pixel_height,
    SDL_FColor clear_color,
    bool clear_target
) {
    SIUIState *state = siui_state();

    if (!state->backend)
        return true;

    return siui_sdlgpu_render(
        state->backend,
        command,
        texture,
        pixel_width,
        pixel_height,
        clear_color,
        clear_target
    );
}

static void on_ui_root_set(const void *new_value) {
    if (siui_state()->backend)
        siui_release();
    siui_attach_window();
}

ECS_RESOURCE_DEFINE(SIUIRoot, .on_set = on_ui_root_set);

void siui_register() {
    TTF_Init();
    ECS_RESOURCE_REGISTER(SIUIState);
    ecs_set_resource(SIUIState, {});
    ECS_RESOURCE_REGISTER(SIUIRoot);
}

void siui_shutdown() {
    siui_release();
    TTF_Quit();
}
