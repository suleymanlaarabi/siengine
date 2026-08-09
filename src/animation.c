#include "engine_internal.h"
#include "siengine.h"

static void animation_on_set(
    ecs_entity_t entity,
    ecs_component_t component,
    const void *new_value,
    void *current_value
) {
    (void)component;
    (void)current_value;
    const SIAnimation *animation = new_value;
    ecs_get(entity, SISprite)->frame_index = animation->start_index;
}

ECS_CTOR(SIAnimationTimer, { .playing = true });
ECS_COMPONENT_DEFINE(SIAnimation, .on_set = animation_on_set, .inheritance = EcsInheritShared);
ECS_COMPONENT_DEFINE(SIAnimationTimer, .ops = { .ctor = ecs_ctor_id(SIAnimationTimer) });

void sianimation_update(ecs_iter_t *it) {
    SISprite *sprites = ecs_field(it, 0);
    const SIAnimation *animations = ecs_field(it, 1);
    SIAnimationTimer *timers = ecs_field(it, 2);
    for (uint32_t i = 0; i < it->count; i++) {
        if (!timers[i].playing)
            continue;
        timers[i].elapsed += it->delta_time;
        uint32_t frames = (uint32_t)(timers[i].elapsed / animations[i].frame_duration);
        timers[i].elapsed -= frames * animations[i].frame_duration;
        if (!frames)
            continue;
        if (animations[i].loop) {
            uint32_t span = animations[i].end_index - animations[i].start_index + 1;
            uint32_t relative = sprites[i].frame_index - animations[i].start_index;
            sprites[i].frame_index = animations[i].start_index + (relative + frames) % span;
        } else if (frames >= animations[i].end_index - sprites[i].frame_index) {
            sprites[i].frame_index = animations[i].end_index;
            timers[i].playing = false;
        } else {
            sprites[i].frame_index += frames;
        }
    }
}
