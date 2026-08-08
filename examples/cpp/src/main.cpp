#include "siecs.h"
#include "siecs_rest.h"
#include "siengine.h"

int main(int argc, char *argv[]) {
    ecs::init({ .target_fps = 60 });

    ecs::import<siengine>();
    ecs::import<sirest>();

    ecs::set_resource(SIWindow("Hello"));
    ecs::set_resource(SIAssetRoot("../../../../assets"));


    ecs::entity::create().add<SICamera2D>();

    struct Velocity {
        float x, y;
    };

    ecs::entity::create().set(
        SISprite{ siengine_load_texture("hero.png", SI_FILTER_NEAREST) },
        SISpriteSheet{
            .columns = 4,
            .rows = 1,
            .frame_width = 128,
            .frame_height = 128,
        },
        Velocity{50}
    );

    ecs::system()
        .phase(EcsOnUpdate)
        .each([](SITransform2D &transform, const Velocity &vel, ecs::res<DeltaTime> time) {
            transform.x += vel.x * time->value;
            transform.y += vel.y * time->value;
        });

    ecs::run();
}
