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
    SIMaterial2D material_desc{};
    material_desc.texture = texture;
    material_desc.filter = SI_FILTER_NEAREST;
    auto material = ecs::entity::create()
                        .set(material_desc)
                        .set(SISpriteSheet{ 4, 1, 128, 128, 0, 0, 0, 0 });

    ecs::entity PlayerBase = ecs::entity::create()
                                 .set(
                                     SISprite{},
                                     Velocity{ 50 }
                                 )
                                 .abstract();

    for (int x = 0; x < 200; x++) {
        for (int i = 0; i < 200; i++) {
            ecs::entity::create()
                .set(SITransform2D::from_xy(-200 - x * 2, i - 100).with_scale(0.02f))
                .is_a(PlayerBase)
                .relate<Layer>(SILayerActors)
                .relate<Material>(material);
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
