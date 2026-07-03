
#include "engine_internal.h"
#include "siecs.h"
#include "siengine.h"

ECS_MODULE_DEFINE(sitransform);

ECS_COMPONENT_DEFINE(SIPosition2d);
ECS_COMPONENT_DEFINE(SIScale2d);
ECS_COMPONENT_DEFINE(SIRotation2d);

void sitransform_import(ecs_world_t *world, const sitransform_props_t *props) {
    ECS_COMPONENT_REGISTER(world, SIPosition2d);
    ECS_COMPONENT_REGISTER(world, SIScale2d);
    ECS_COMPONENT_REGISTER(world, SIRotation2d);
}
