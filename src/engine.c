#include "engine_internal.h"
#include "assets_internal.h"
#include "siecs.h"
#include "siengine.h"

ECS_MODULE_DEFINE(siengine);

static void on_engine_remove(const void *ptr) {
    SIEngineCtx *ctx = (SIEngineCtx *)ptr;
    siplatform_shutdown(ctx);
}

ECS_RESOURCE_DEFINE(SIEngineCtx, .on_remove = on_engine_remove);

void siengine_import(const siengine_props_t *) {
    ECS_MODULE_IMPORT(siscene2d, {});
    SIEngineCtx context = siplatform_init();

    ECS_RESOURCE_REGISTER(SIEngineCtx);
    ecs_set_resource_rid(ecs_id(SIEngineCtx), &context);
    siwindow_register();
    siassets_register();
    sirender_register();
}

void siengine_run(void) { siplatform_run(); }
