#include "siecs_rest.h"
#include "siengine.h"

int main(int argc, char *argv[]) {
    ecs::init({ .target_fps = 60 });

    ecs::import<siengine>();
    ecs::import<sirest>();

    ecs::set_resource(SIWindow{ "Hello" });
    ecs::set_resource(SIAssetRoot("../../../../assets"));

    SITextureHandle texture = siengine_load_texture("hero.png", SI_FILTER_NEAREST);

    ecs::entity::create().set(
        SICamera2D{},
        SIVirtualResolution{
            .width = 320,
            .height = 180,
            .enabled = true,
            .pixel_perfect = true,
        }
    );

    ecs::entity::create().set(
        SISprite{ texture },
        SISpriteSheet{
            .columns = 4,
            .rows = 1,
            .frame_width = 128,
            .frame_height = 128,
        }
    );

    ecs::run();
}
