#include "engine_internal.h"
#include <SDL3/SDL_events.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <siengine.h>
#include <siui_sdl_gpu.h>

#define SIUI_DEFAULT_FONT "/usr/share/fonts/noto/NotoSans-Regular.ttf"

ECS_RESOURCE_DECLARE(SIUIState, {
    ecs_entity_t owner;
    siui_sdlgpu_t *backend;
    TTF_Font *font;
});

ECS_RESOURCE_DEFINE(SIUIState);

static SIUIState *siui_state(void) { return ecs_resource(SIUIState); }

static void siui_release(void) {
    SIUIState *state = siui_state();

    siui_sdlgpu_destroy(state->backend);
    TTF_CloseFont(state->font);
    state->owner = 0;
    state->backend = NULL;
    state->font = NULL;
}

static void siui_mount_window(ecs_entity_t entity, sicomponent_fn_t render) {
    SIEngineCtx *engine = ecs_resource(SIEngineCtx);
    SIWindowHandle *window = ecs_try_get(entity, SIWindowHandle);
    SIUIState *state = siui_state();

    if (!window || !render || state->backend)
        return;

    TTF_Font *font = TTF_OpenFont(SIUI_DEFAULT_FONT, 16.0f);
    siui_sdlgpu_t *backend = siui_sdlgpu_create(&(siui_sdlgpu_desc_t){
        .device = engine->primary_gpu,
        .window = window->handle,
        .default_font = font,
        .default_font_size = 16.0f,
    });

    if (!backend) {
        TTF_CloseFont(font);
        return;
    }

    state->owner = entity;
    state->backend = backend;
    state->font = font;
    siui_mount_impl(render, NULL, 0);
}

void siui_attach_window(ecs_entity_t entity) {
    SIUIRoot *root = ecs_try_get(entity, SIUIRoot);
    if (root)
        siui_mount_window(entity, root->render);
}

void siui_window_destroying(ecs_entity_t entity) {
    if (siui_state()->owner == entity)
        siui_release();
}

static SDL_WindowID siui_event_window_id(const SDL_Event *event) {
    switch (event->type) {
    case SDL_EVENT_MOUSE_MOTION:
        return event->motion.windowID;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
        return event->button.windowID;
    case SDL_EVENT_MOUSE_WHEEL:
        return event->wheel.windowID;
    default:
        return 0;
    }
}

bool siui_handle_event(const SDL_Event *event) {
    SIUIState *state = siui_state();
    SIWindowHandle *window;
    siinput_event_t input = { 0 };

    if (!state->backend || !event)
        return false;

    window = ecs_try_get(state->owner, SIWindowHandle);
    if (!window || SDL_GetWindowID(window->handle) != siui_event_window_id(event))
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
        input.delta_x = event->wheel.x;
        input.delta_y = event->wheel.y;
        break;
    default:
        return false;
    }

    return siui_dispatch(&input);
}

bool siui_render_window(
    ecs_entity_t entity,
    SDL_GPUCommandBuffer *command,
    SDL_GPUTexture *texture,
    uint32_t pixel_width,
    uint32_t pixel_height,
    SDL_FColor clear_color,
    bool clear_target
) {
    SIUIState *state = siui_state();

    if (state->owner != entity)
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

static void on_ui_root_set(
    ecs_entity_t entity,
    ecs_component_t component,
    const void *new_value,
    void *old_value
) {
    SIUIState *state = siui_state();

    if (state->owner == entity) {
        siui_release();
        siui_attach_window(entity);
    } else {
        siui_attach_window(entity);
    }
}

static void on_ui_root_remove(ecs_entity_t entity, ecs_component_t component, void *data) {
    siui_window_destroying(entity);
}

ECS_COMPONENT_DEFINE(SIUIRoot, .on_set = on_ui_root_set, .on_remove = on_ui_root_remove);

static void SyncPendingUI(ecs_iter_t *it) {
    SIUIState *state = siui_state();
    if (state->backend)
        return;

    for (uint32_t i = 0; i < it->count; i++)
        siui_attach_window(it->entities[i]);
}

void siui_register() {
    TTF_Init();
    ECS_RESOURCE_REGISTER(SIUIState);
    ecs_set_resource(SIUIState, {});
    ECS_COMPONENT_REGISTER(SIUIRoot);
    ecs_system(
        {
            .name = "SyncPendingUI",
            .query.terms = { ecs_in(SIWindowHandle), ecs_in(SIUIRoot) },
            .phase = EcsPreRender,
            .callback = SyncPendingUI,
        }
    );
}

void siui_shutdown() {
    siui_release();
    TTF_Quit();
}
