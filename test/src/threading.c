#include "test.h"
#include "../../src/render_internal.h"
#include <stdatomic.h>
#include <threads.h>

ECS_COMPONENT_DECLARE(ThreadMutationA, { int value; });
ECS_COMPONENT_DECLARE(ThreadMutationB, { int value; });
ECS_COMPONENT_DECLARE(ThreadAdded, { int value; });
ECS_COMPONENT_DECLARE(ThreadRemoved, { int value; });
ECS_COMPONENT_DECLARE(ThreadValue, { int value; });
ECS_RELATION_DECLARE(ThreadRelation);

ECS_RESOURCE_DECLARE(ThreadReadResource, { int value; });
ECS_RESOURCE_DECLARE(ThreadWriteResource, { int value; });
ECS_RESOURCE_DECLARE(ThreadOtherResource, { int value; });

ECS_COMPONENT_DEFINE(ThreadMutationA);
ECS_COMPONENT_DEFINE(ThreadMutationB);
ECS_COMPONENT_DEFINE(ThreadAdded);
ECS_COMPONENT_DEFINE(ThreadRemoved);
ECS_COMPONENT_DEFINE(ThreadValue);
ECS_RELATION_DEFINE(ThreadRelation, {});
ECS_RESOURCE_DEFINE(ThreadReadResource);
ECS_RESOURCE_DEFINE(ThreadWriteResource);
ECS_RESOURCE_DEFINE(ThreadOtherResource);

static ecs_entity_t mutation_created;
static ecs_entity_t mutation_victim;
static ecs_entity_t mutation_killed;
static ecs_entity_t mutation_consumed;
static ecs_entity_t mutation_relation_target;
static bool mutation_saw_old_state;
static bool mutation_after_saw_added;

static void mutation_producer(ecs_iter_t *it) {
    (void)ecs_field(it, 0);
    mutation_created = ecs_new();
    ecs_add(mutation_created, ThreadAdded);
    ecs_set(mutation_created, ThreadValue, { .value = 7 });
    ecs_relate(mutation_created, ThreadRelation, mutation_relation_target);
    ecs_remove(mutation_victim, ThreadRemoved);
    ecs_kill(mutation_killed);
    ecs_set(mutation_consumed, ThreadValue, { .value = 9 });
}

static void mutation_observer(ecs_iter_t *it) {
    (void)ecs_field(it, 0);
    mutation_saw_old_state = ecs_has(mutation_victim, ThreadRemoved) &&
                             ecs_is_alive(mutation_killed) &&
                             ecs_get(mutation_consumed, ThreadValue)->value == 1;
}

static void mutation_after_observer(ecs_iter_t *it) {
    (void)ecs_field(it, 0);
    mutation_after_saw_added = ecs_has(mutation_victim, ThreadAdded);
}

static void mutation_after_producer(ecs_iter_t *it) {
    (void)ecs_field(it, 0);
    ecs_add(mutation_victim, ThreadAdded);
}

static void register_mutation_types(void) {
    ECS_COMPONENT_REGISTER(
        ThreadMutationA,
        ThreadMutationB,
        ThreadAdded,
        ThreadRemoved,
        ThreadValue
    );
    ECS_RELATION_REGISTER(ThreadRelation);
}

void threading_structural_mutations_are_deferred(void) {
    ecs_with_features({ .target_fps = 60, .worker_threads = 2 });
    register_mutation_types();

    mutation_victim = ecs_new();
    ecs_add(mutation_victim, ThreadRemoved);
    mutation_killed = ecs_new();
    mutation_consumed = ecs_new();
    ecs_set(mutation_consumed, ThreadValue, { .value = 1 });
    mutation_relation_target = ecs_new();

    ecs_entity_t producer = ecs_new();
    ecs_add(producer, ThreadMutationA);
    ecs_entity_t observer = ecs_new();
    ecs_add(observer, ThreadMutationB);

    ecs_system({
        .name = "ThreadMutationProducer",
        .phase = EcsOnUpdate,
        .callback = mutation_producer,
        .query = { .terms = { ecs_inout(ThreadMutationA) } },
    });
    ecs_system({
        .name = "ThreadMutationObserver",
        .phase = EcsOnUpdate,
        .callback = mutation_observer,
        .query = { .terms = { ecs_inout(ThreadMutationB) } },
    });

    ecs_run_phase(EcsOnUpdate);

    test_true(mutation_saw_old_state);
    test_true(mutation_created != SI_INVALID_HANDLE);
    test_true(ecs_is_alive(mutation_created));
    test_true(ecs_has(mutation_created, ThreadAdded));
    test_int(7, ecs_get(mutation_created, ThreadValue)->value);
    test_false(ecs_has(mutation_victim, ThreadRemoved));
    test_false(ecs_is_alive(mutation_killed));
    test_int(mutation_relation_target, ecs_target(mutation_created, ThreadRelation));
    test_int(9, ecs_get(mutation_consumed, ThreadValue)->value);

    ecs_fini();
}

void threading_after_observes_structural_mutation(void) {
    ecs_with_features({ .target_fps = 60, .worker_threads = 2 });
    register_mutation_types();
    mutation_after_saw_added = false;

    mutation_victim = ecs_new();
    ecs_entity_t producer = ecs_new();
    ecs_add(producer, ThreadMutationA);
    ecs_entity_t observer = ecs_new();
    ecs_add(observer, ThreadMutationB);

    ecs_system_id_t producer_system = ecs_system({
        .name = "ThreadAfterProducer",
        .phase = EcsOnUpdate,
        .callback = mutation_after_producer,
        .query = { .terms = { ecs_inout(ThreadMutationA) } },
    });
    ecs_system({
        .name = "ThreadAfterObserver",
        .phase = EcsOnUpdate,
        .callback = mutation_after_observer,
        .after = { producer_system },
        .query = { .terms = { ecs_inout(ThreadMutationB) } },
    });

    ecs_progress();
    test_true(mutation_after_saw_added);
    test_true(ecs_has(mutation_victim, ThreadAdded));

    ecs_fini();
}

static atomic_int resource_active;
static atomic_int resource_max_active;

static void resource_enter(void) {
    int active = atomic_fetch_add(&resource_active, 1) + 1;
    int maximum = atomic_load(&resource_max_active);
    while (active > maximum &&
           !atomic_compare_exchange_weak(&resource_max_active, &maximum, active)) {}
}

static void resource_leave(void) { atomic_fetch_sub(&resource_active, 1); }

static void resource_overlap(void) {
    resource_enter();
    for (volatile uint32_t i = 0; i < 100000; i++) {}
    resource_leave();
}

static void resource_read_a(ecs_iter_t *it) {
    (void)ecs_field(it, 0);
    (void)ecs_get_resource_read(ThreadReadResource);
    resource_overlap();
}

static void resource_read_b(ecs_iter_t *it) {
    (void)ecs_field(it, 0);
    (void)ecs_get_resource_read(ThreadReadResource);
    resource_overlap();
}

static void resource_read_write(ecs_iter_t *it) {
    (void)ecs_field(it, 0);
    (void)ecs_get_resource_read(ThreadWriteResource);
    resource_overlap();
}

static void resource_write_a(ecs_iter_t *it) {
    (void)ecs_field(it, 0);
    ecs_get_resource(ThreadWriteResource)->value++;
    resource_overlap();
}

static void resource_write_b(ecs_iter_t *it) {
    (void)ecs_field(it, 0);
    ecs_get_resource(ThreadWriteResource)->value++;
    resource_overlap();
}

static void resource_other(ecs_iter_t *it) {
    (void)ecs_field(it, 0);
    (void)ecs_get_resource_read(ThreadOtherResource);
    resource_overlap();
}

static void render_state_writer_a(ecs_iter_t *it) {
    (void)ecs_field(it, 0);
    ecs_get_resource(SIRenderState)->instances.size++;
    resource_overlap();
}

static void render_state_writer_b(ecs_iter_t *it) {
    (void)ecs_field(it, 0);
    ecs_get_resource(SIRenderState)->instances.size++;
    resource_overlap();
}

static void resource_setup(void) {
    ECS_COMPONENT_REGISTER(ThreadMutationA, ThreadMutationB);
    ECS_RESOURCE_REGISTER(ThreadReadResource);
    ECS_RESOURCE_REGISTER(ThreadWriteResource);
    ECS_RESOURCE_REGISTER(ThreadOtherResource);
    ecs_set_resource(ThreadReadResource, { .value = 1 });
    ecs_set_resource(ThreadWriteResource, { .value = 1 });
    ecs_set_resource(ThreadOtherResource, { .value = 1 });
    atomic_store(&resource_active, 0);
    atomic_store(&resource_max_active, 0);
}

static void run_resource_pair(
    void (*first)(ecs_iter_t *),
    void (*second)(ecs_iter_t *),
    ecs_resource_t *first_read,
    ecs_resource_t *first_write,
    ecs_resource_t *second_read,
    ecs_resource_t *second_write
) {
    ecs_entity_t first_entity = ecs_new();
    ecs_add(first_entity, ThreadMutationA);
    ecs_entity_t second_entity = ecs_new();
    ecs_add(second_entity, ThreadMutationB);
    ecs_system_desc_t first_desc = {
        .name = "ResourceFirst",
        .phase = EcsOnUpdate,
        .callback = first,
        .query = { .terms = { ecs_inout(ThreadMutationA) } },
    };
    ecs_system_desc_t second_desc = {
        .name = "ResourceSecond",
        .phase = EcsOnUpdate,
        .callback = second,
        .query = { .terms = { ecs_inout(ThreadMutationB) } },
    };
    if (first_read)
        first_desc.read_resources[0] = *first_read;
    if (first_write)
        first_desc.write_resources[0] = *first_write;
    if (second_read)
        second_desc.read_resources[0] = *second_read;
    if (second_write)
        second_desc.write_resources[0] = *second_write;
    ecs_system_init(&first_desc);
    ecs_system_init(&second_desc);
    ecs_run_phase(EcsOnUpdate);
}

void threading_read_read_resources_can_overlap(void) {
    ecs_with_features({ .target_fps = 60, .worker_threads = 2 });
    resource_setup();
    ecs_resource_t read = ecs_id(ThreadReadResource);
    run_resource_pair(resource_read_a, resource_read_b, &read, NULL, &read, NULL);
    test_true(atomic_load(&resource_max_active) == 2);
    ecs_fini();
}

void threading_read_write_resources_are_serialized(void) {
    ecs_with_features({ .target_fps = 60, .worker_threads = 2 });
    resource_setup();
    ecs_resource_t write = ecs_id(ThreadWriteResource);
    run_resource_pair(resource_read_write, resource_write_a, NULL, &write, NULL, &write);
    test_int(1, atomic_load(&resource_max_active));
    ecs_fini();
}

void threading_write_write_resources_are_serialized(void) {
    ecs_with_features({ .target_fps = 60, .worker_threads = 2 });
    resource_setup();
    ecs_resource_t write = ecs_id(ThreadWriteResource);
    run_resource_pair(resource_write_a, resource_write_b, NULL, &write, NULL, &write);
    test_int(1, atomic_load(&resource_max_active));
    ecs_fini();
}

void threading_unrelated_resources_can_overlap(void) {
    ecs_with_features({ .target_fps = 60, .worker_threads = 2 });
    resource_setup();
    ecs_resource_t read = ecs_id(ThreadReadResource);
    ecs_resource_t other = ecs_id(ThreadOtherResource);
    run_resource_pair(resource_read_a, resource_other, &read, NULL, &other, NULL);
    test_true(atomic_load(&resource_max_active) == 2);
    ecs_fini();
}

void threading_sirender_state_writers_are_serialized(void) {
    ecs_with_features({ .target_fps = 60, .worker_threads = 2 });
    ECS_MODULE_IMPORT(siengine, {});
    resource_setup();
    ecs_resource_t render_state = ecs_id(SIRenderState);
    run_resource_pair(
        render_state_writer_a,
        render_state_writer_b,
        NULL,
        &render_state,
        NULL,
        &render_state
    );
    test_int(1, atomic_load(&resource_max_active));
    ecs_fini();
}
