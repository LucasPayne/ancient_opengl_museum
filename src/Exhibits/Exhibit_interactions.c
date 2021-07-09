#include "museum.h"

void create_exhibit_interactions(void)
{
    // Create some pillars as surfaces of revolution.
    float ys[] = {0, 1, 3, 3, 4, 4.5, 14.5, 15, 16, 16, 18, 19};
    float xs[] = {10, 10, 9, 8, 7, 6, 6, 7, 8, 9, 10, 10};
    for (int i = 0; i < 12; i++) {
        ys[i] *= 0.05;
        xs[i] *= 0.05;
        ys[i] -= 0.1;
    }
    Model pillar_model = make_surface_of_revolution(xs, ys, 12, 10);
    compute_uvs_cylindrical(&pillar_model, 3,3);
    pillar_model.texture = load_texture("resources/rock.bmp");
    pillar_model.textured = true;
    vec3 pillars_position = new_vec3(-8,-1.3,-50);
    pillar_model.flat_color = BLUE;
    // model_compute_normals(&pillar_model);
    for (int i = 0; i < 5; i++) {
        vec3 pillar_pos = vec3_add(pillars_position, new_vec3(i*1.4,0,0));

        Entity *pillar = add_entity(pillar_pos, vec3_zero());
        ModelRenderer *pillar_renderer = add_model_renderer(pillar, pillar_model);
        Polyhedron block = make_block(1.1,2.1,1.1);
        add_collider(pillar, polyhedron_points(block), polyhedron_num_points(&block), false);

        Model display_model;
        switch (i) {
            case 0: display_model = make_tetrahedron(0.5); break;
            case 1: display_model = make_tessellated_block(0.7,0.7,0.7,  3,3,3); break;
            case 2: display_model = make_octahedron(0.5); break;
            case 3: display_model = make_icosahedron(0.4); break;
            case 4: display_model = make_dodecahedron(0.9); break;
        }
        compute_uvs_orthogonal(&display_model, new_vec3(sqrt(2)/2,sqrt(2)/2,0), new_vec3(-sqrt(2)/2,sqrt(2)/2,0), 3,3);
        display_model.texture = load_texture("resources/ice.bmp");
        display_model.textured = true;
        Entity *display_object = add_entity(vec3_add(pillar_pos, new_vec3(0,1.6,0)), new_vec3(0,0,0));
        ModelRenderer *renderer = add_model_renderer(display_object, display_model);
        TrackBall *tb = add_trackball(display_object, 1);
        vec3 v;
        while (vec3_dot(v, v) < 0.01) v = new_vec3(frand(), frand(), frand());
        tb->angular_velocity = vec3_mul(vec3_normalize(v), 0.12);
        tb->check_model = true;
        tb->model = &renderer->model;
    }
}
