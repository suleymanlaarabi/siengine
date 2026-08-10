#include "siecs.h"
#include "siengine.h"
#include <format>

int main(int argc, char *argv[]) {
    ecs::init({ .target_fps = 120 });

    ecs::import<siengine>();
    ecs::set_resource(SIWindow("Hello", "siengine-canvas"));
#if defined(__EMSCRIPTEN__)
    ecs::set_resource(SIAssetRoot("/assets"));
#else
    ecs::set_resource(SIAssetRoot("../../../../assets"));
#endif

    ecs::entity::create().add<SICamera2D>();

    ecs::entity::create()
        .add<Dynamic>()
        .set(
            SIColor::from_rgb(255, 0, 0),
            SICircle(5.),
            Position{ 0, 100 },
            CircleCollider{ .radius = 2 }
        )
        .relate<Material>(SI2DDefaultMaterial);

    ecs::entity::create()
        .add<Static>()
        .set(
            SIColor::from_rgb(0, 200, 0),
            SIRectangle{ 50, 3 },
            BoxCollider{ 50, 3 },
            Position{ 0, -25 }
        )
        .relate<Material>(SI2DDefaultMaterial);
    siengine_run();
}
