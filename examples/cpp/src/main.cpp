#include "siecs.h"
#include "siengine.h"
#include <format>

struct Velocity {
    float x, y;
};

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

    auto texture = siengine_load_texture("hero.png", SI_FILTER_NEAREST);

    ecs::entity PlayerBase = ecs::entity::create()
                                 .set(
                                     SISprite{ texture },
                                     SISpriteSheet{
                                         .columns = 4,
                                         .rows = 1,
                                         .frame_width = 128,
                                         .frame_height = 128,
                                     },
                                     Velocity{ 50 }
                                 )
                                 .abstract();

    for (int x = 0; x < 200; x++) {
        for (int i = 0; i < 200; i++) {
            ecs::entity::create()
                .set(SITransform2D::from_xy(-200 - x * 2 , (i * 1) - 100).with_scale(0.02))
                .is_a(PlayerBase);
        }
    }


    ecs::system()
        .phase(EcsPreUpdate)
        .each([](SITransform2D &transform, const Velocity &vel, ecs::res<DeltaTime> time) {
            transform.x += vel.x * time->value;
            transform.y += vel.y * time->value;
        });

    siengine_run();
}
