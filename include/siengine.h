#ifndef SIENGINE_H
#define SIENGINE_H

/* This generated file contains includes for project dependencies */
#include "siecs.h"
#include "siengine/bake_config.h"

#ifdef __cplusplus
extern "C" {
#endif

ECS_MODULE_DECLARE(siengine, {});
ECS_COMPONENT_DECLARE(SIWindow, { uint64_t handle; });

SIWindow siengine_create_window(ecs_world_t *world, const char *title);

#ifdef __cplusplus
}
#endif

#endif
