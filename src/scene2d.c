#include "engine_internal.h"
#include "siengine.h"

ECS_MODULE_DEFINE(siscene2d);

ECS_COMPONENT_DEFINE(SITransform2D);
ECS_COMPONENT_DEFINE(SIWorldTransform2D);
ECS_COMPONENT_DEFINE(SICamera2D);
ECS_COMPONENT_DEFINE(SIActiveCamera);
ECS_COMPONENT_DEFINE(SIRenderOrder);
ECS_COMPONENT_DEFINE(SIColor);

void siscene2d_import(const siscene2d_props_t *props) {
    (void)props;

    ECS_COMPONENT_REGISTER(SITransform2D);
    ECS_COMPONENT_REGISTER(SIWorldTransform2D);
    ECS_COMPONENT_REGISTER(SICamera2D);
    ECS_COMPONENT_REGISTER(SIActiveCamera);
    ECS_COMPONENT_REGISTER(SIRenderOrder);
    ECS_COMPONENT_REGISTER(SIColor);

    ecs_with(ecs_id(SITransform2D), ecs_id(SIWorldTransform2D));
    ecs_with(ecs_id(SICamera2D), ecs_id(SITransform2D));
}
