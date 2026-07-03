#ifndef SIENGINE_H
#define SIENGINE_H

/* This generated file contains includes for project dependencies */
#include "siecs.h"
#include "siengine/bake_config.h"

#ifdef __cplusplus
extern "C" {
#endif

ECS_MODULE_DECLARE(siengine, {});
ECS_COMPONENT_DECLARE(SIWindow, { char *title; });

// Transform
ECS_COMPONENT_DECLARE(Position2d, { float x, y; });
ECS_COMPONENT_DECLARE(Rotation2d, { float angle; }); // radians
ECS_COMPONENT_DECLARE(Scale2d, { float x, y; });

// Physics
ECS_COMPONENT_DECLARE(Velocity2d, { float x, y; });

#ifdef __cplusplus
}
#endif

#endif
