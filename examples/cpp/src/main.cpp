#include "siecs.h"
#include "siecs_rest.h"
#include "siengine.h"
#include <format>


struct Velocity {
    float x, y;
};

int main(int argc, char *argv[]) {
    ecs::init({ .target_fps = 120 });

    ecs::import<siengine>();
    ecs::import<sirest>();

    ecs::set_resource(SIWindow("Hello"));
    ecs::set_resource(SIAssetRoot("../../../../assets"));

    ecs::entity::create().add<SICamera2D>();


    auto texture = siengine_load_texture("hero.png", SI_FILTER_NEAREST);
    for (int i = 0; i < 10; i++) {
        ecs::entity::create(std::format("hero{}", i).c_str()).set(
            SISprite{texture},
            SISpriteSheet{
                .columns = 4,
                .rows = 1,
                .frame_width = 128,
                .frame_height = 128,
            },
            Velocity{50},
            SITransform2D::from_xy(-200, (i * 25) - 100).with_scale(0.5)

        );
    }

    auto circle = ecs::entity::create().set(SICircle{24.0f}, SITransform2D::from_xy(-80, 0));
    circle.get<SIColor>() = SIColor{};
    circle.get<SIColor>().r = 0.95f;
    circle.get<SIColor>().g = 0.2f;
    circle.get<SIColor>().b = 0.2f;

    auto rectangle =
        ecs::entity::create().set(SIRectangle{56.0f, 28.0f}, SITransform2D::from_xy(0, 0));
    rectangle.get<SIColor>() = SIColor{};
    rectangle.get<SIColor>().r = 0.2f;
    rectangle.get<SIColor>().g = 0.85f;
    rectangle.get<SIColor>().b = 0.3f;

    auto triangle =
        ecs::entity::create().set(SITriangle{48.0f, 48.0f}, SITransform2D::from_xy(80, 0));
    triangle.get<SIColor>() = SIColor{};
    triangle.get<SIColor>().r = 0.2f;
    triangle.get<SIColor>().g = 0.45f;
    triangle.get<SIColor>().b = 1.0f;

    ecs::system()
        .phase(EcsPreUpdate)
        .each([](SITransform2D &transform, const Velocity &vel, ecs::res<DeltaTime> time) {
            transform.x += vel.x * time->value;
            transform.y += vel.y * time->value;
        });

    ecs::run();
}
