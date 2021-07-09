#include "museum.h"

// Position of the exhibition.
static vec3 ex_pos = {{-34,7,-40}};

typedef struct RigidBodyInteractor_s {
    RigidBody *rb;
    Model *model;

    bool dragging;

    vec3 drag_plane[3];
    float last_drag_x;
    float last_drag_y;
} RigidBodyInteractor;

void rigid_body_interactor_update(Entity *e, Behaviour *b)
{
    RigidBodyInteractor *rbi = b->data;

    //--- Hack: If the objects glitch through the tumbler, try put them back.
    vec3 d = vec3_sub(e->position, ex_pos);
    if (vec3_dot(d, d) > 6*6) {
        e->position = ex_pos;
        rbi->rb->linear_momentum = vec3_zero();
        rbi->rb->angular_momentum = vec3_zero();
    }
}
void rigid_body_interactor_mouse_motion_listener(Entity *e, Behaviour *b, float x, float y)
{
    RigidBodyInteractor *rbi = b->data;
    if (rbi->dragging) {
	vec3 ray_origin, ray_direction;
        camera_ray(main_camera_entity, main_camera, x, y, &ray_origin, &ray_direction);
        vec3 intersection;
        
        float rect_x, rect_y;


        vec3 rect[4] = {
            rbi->drag_plane[2],
            rbi->drag_plane[0],
            rbi->drag_plane[1],
            vec3_add(rbi->drag_plane[0], vec3_add(vec3_sub(rbi->drag_plane[1], rbi->drag_plane[0]), vec3_sub(rbi->drag_plane[2], rbi->drag_plane[0]))),
        };
        if (ray_rectangle_plane_coordinates(ray_origin,
                                      ray_direction,
                                      rect[0],
                                      rect[1],
                                      rect[2],
                                      rect[3],
                                      &rect_x, &rect_y)) {
            float dx = rect_x - rbi->last_drag_x;
            float dy = rect_y - rbi->last_drag_y;
            rbi->last_drag_x = rect_x;
            rbi->last_drag_y = rect_y;

            vec3 diff = vec3_add(vec3_mul(vec3_sub(rect[3], rect[0]), dx), vec3_mul(vec3_sub(rect[0], rect[1]), dy));
            float drag_power = 1.3;
            rbi->rb->linear_momentum = vec3_add(rbi->rb->linear_momentum, vec3_mul(diff, rbi->rb->mass * drag_power));

        } else {
            rbi->dragging = false;
        }
    }
}
void rigid_body_interactor_mouse_listener(Entity *e, Behaviour *b, int button, int state, float x,  float y)
{
    RigidBodyInteractor *rbi = b->data;
    if (rbi->dragging) {
        if (state == GLUT_UP && button == GLUT_LEFT_BUTTON) {
            rbi->dragging = false;
        }
    }
    else if (state == GLUT_DOWN && button == GLUT_LEFT_BUTTON) {
	vec3 ray_origin, ray_direction;
        camera_ray(main_camera_entity, main_camera, x, y, &ray_origin, &ray_direction);
        vec3 intersection;
        if (ray_model_intersection(ray_origin, ray_direction, rbi->model, entity_matrix(e), &intersection)) {
            rbi->dragging = true;
            rbi->drag_plane[0] = intersection;
            rbi->drag_plane[1] = vec3_add(intersection, matrix_vec3(entity_orientation(main_camera_entity), new_vec3(1,0,0)));
            rbi->drag_plane[2] = vec3_add(intersection, matrix_vec3(entity_orientation(main_camera_entity), new_vec3(0,1,0)));
            rbi->last_drag_x = 0;
            rbi->last_drag_y = 0;
        }
    }
}

static void make_rigid_body(vec3 *points, int num_points, vec3 position, float scale, float mass)
{
    Entity *e = add_entity(position, vec3_zero());
    e->scale = scale;
    Model model = polyhedron_to_model(convex_hull(points, num_points));
    model_compute_normals(&model);
    model.flat_color = WHITE;
    // Compute orthogonally projected texture coordinates.
    float *uvs = malloc(sizeof(float) * 2*model.num_vertices);
    mem_check(uvs);
    for (int i = 0; i < model.num_vertices; i++) {
        uvs[2*i + 0] = 0.3 * X(model.vertices[i]);
        uvs[2*i + 1] = 0.3 * Y(model.vertices[i]);
    }
    model.uvs = uvs;
    model.has_uvs = true;
    model.texture = load_texture("resources/rock2.bmp");
    model.textured = true;
    ModelRenderer *renderer = add_model_renderer(e, model);

    add_collider(e, model.vertices, model.num_vertices, true);
    RigidBody *rb = add_rigid_body(e, mass);

    Behaviour *rbi_b = add_behaviour(e, rigid_body_interactor_update, sizeof(RigidBodyInteractor), NoID);
    rbi_b->mouse_listener = rigid_body_interactor_mouse_listener;
    rbi_b->mouse_motion_listener = rigid_body_interactor_mouse_motion_listener;
    RigidBodyInteractor *rbi = rbi_b->data;
    rbi->rb = rb;
    rbi->model = &renderer->model;
}

typedef struct RBDExhibit_s {
    float speed;
    bool slowing;
} RBDExhibit;
void rigid_body_dynamics_exhibit_update(Entity *e, Behaviour *b)
{
    RBDExhibit *ex = (RBDExhibit *) b->data;
    if (ex->slowing) {
        ex->speed -= 0.8 * dt;
        if (ex->speed < 0.1 ) ex->slowing = false;
    } else {
        ex->speed += 0.8 * dt;
        if (ex->speed > 1.8 ) ex->slowing = true;
    }
    Z(e->euler_angles) += dt * ex->speed;
}
void create_exhibit_rigid_body_dynamics(void)
{
    vec3 ex_orientation = new_vec3(0,-M_PI/2,0);
    float base_speed = 0.2;
        
    Entity *tumbler = add_entity(ex_pos, ex_orientation);
    Behaviour *ex_b = add_behaviour(tumbler, rigid_body_dynamics_exhibit_update, sizeof(RBDExhibit), NoID);
    RBDExhibit *ex = (RBDExhibit *) ex_b->data;
    ex->speed = 0;
    tumbler->euler_controlled = true;
    vec3 side_positions[4] = {
        {{-5,0,0}},
        {{0,5,0}},
        {{5,0,0}},
        {{0,-5,0}},
    };
    float side_extents[4*3] = {
        1,11,5,
        11,1,5,
        1,11,5,
        11,1,5,
    };
    for (int i = 0; i < 4; i++) {
        Model side_model = make_tessellated_block_with_uvs(side_extents[3*i],side_extents[3*i+1],side_extents[3*i+2], 5,5,5, 5);
        Model side_collider = make_tessellated_block(side_extents[3*i],side_extents[3*i+1],side_extents[3*i+2], 2,2,2);
        for (int j = 0; j < side_model.num_vertices; j++) {
            side_model.vertices[j] = vec3_add(side_model.vertices[j], side_positions[i]);
        }
        for (int j = 0; j < side_collider.num_vertices; j++) {
            side_collider.vertices[j] = vec3_add(side_collider.vertices[j], side_positions[i]);
        }
        model_compute_normals(&side_model);
        side_model.textured = true;
        side_model.texture = load_texture("resources/rock.bmp");
        side_model.flat_color = GRAY;
        ModelRenderer *renderer = add_model_renderer(tumbler, side_model);
        add_collider(tumbler, side_collider.vertices, side_collider.num_vertices, true);
    }
    for (float z = -2.5; z < 3; z += 5) {
        Model block = make_tessellated_block(11,11,0.2, 2,2,2);
        for (int j = 0; j < block.num_vertices; j++) Z(block.vertices[j]) += z;
        add_collider(tumbler, block.vertices, block.num_vertices, true);
    }

#if 0
    for (int i = 0; i < 7; i++) {
        int n = 30;
        vec3 *points = random_points(1, n);
        make_rigid_body(points, n, player_start_position, 2, 1);
    }
#endif
    Model icosahedron = make_icosahedron(1);
    make_rigid_body(icosahedron.vertices, icosahedron.num_vertices, ex_pos, 2, 0.9);
    Model cube = make_tessellated_block(1,1,1, 2,2,2);
    make_rigid_body(cube.vertices, cube.num_vertices, ex_pos, 2, 1);
    Model dodecahedron = make_dodecahedron(1);
    make_rigid_body(dodecahedron.vertices, dodecahedron.num_vertices, ex_pos, 2, 0.9);
    Model tetrahedron = make_tetrahedron(1);
    make_rigid_body(tetrahedron.vertices, tetrahedron.num_vertices, ex_pos, 2, 0.9);
    Model octahedron = make_octahedron(1);
    make_rigid_body(octahedron.vertices, octahedron.num_vertices, ex_pos, 2, 0.6);
    
    // Make a viewing platform.
    for (int N = 0; N < 2; N++) {
        float h = 7;
        float w = 4;
        const vec3 points1[8] = {
            {{0,0,0 }},
            {{0,h,0 }},
            {{w,h,0 }},
            {{w,0,0 }},
            {{0,0,12}},
            {{0,h,3 }},
            {{w,h,3 }},
            {{w,0,12}},
        };
        const vec3 points2[8] = {
            {{0-0.5,0-0.1,0 }},
            {{0-0.5,h-0.1,0 }},
            {{w+0.5,h-0.1,0 }},
            {{w+0.5,0-0.1,0 }},
            {{0-0.5,0-0.1,12}},
            {{0-0.5,h-0.1,3 }},
            {{w+0.5,h-0.1,3 }},
            {{w+0.5,0-0.1,12}},
        };
        vec3 *points = N == 0 ? points1 : points2;
        Entity *ramp = add_entity(vec3_add(ex_pos, new_vec3(10,-8.5,1)), new_vec3(0,-M_PI/2,0));
        Model ramp_model = polyhedron_to_model(convex_hull(points, 8));
        model_compute_normals(&ramp_model);
        ramp_model.textured = true;
        float *uvs = malloc(sizeof(float) * 2*ramp_model.num_vertices);
        mem_check(uvs);
        for (int i = 0; i < ramp_model.num_vertices; i++) {
            if (N == 0) {
                uvs[2*i + 0] = 0.3 * Z(ramp_model.vertices[i]);
                uvs[2*i + 1] = 0.3 * X(ramp_model.vertices[i]);
            } else {
                uvs[2*i + 0] = 0.1 * Y(ramp_model.vertices[i]);
                uvs[2*i + 1] = 0.1 * Z(ramp_model.vertices[i]);
            }
        }
        ramp_model.uvs = uvs;
        ramp_model.has_uvs = true;
        if (N == 0) ramp_model.texture = load_texture("resources/rock.bmp");
        else ramp_model.texture = load_texture("resources/floor.bmp");
        add_model_renderer(ramp, ramp_model);
        add_collider(ramp, ramp_model.vertices, ramp_model.num_vertices, true);
    }
    for (int N = 0; N < 2; N++) {
        Model rod_model = make_tessellated_block_with_uvs(13,0.7,0.7, 4,2,2,  10);
        model_compute_normals(&rod_model);
        rod_model.texture = load_texture("resources/floor.bmp");
        rod_model.textured = true;
        Entity *rod = add_entity(vec3_add(ex_pos, new_vec3(18,-5.6, N == 0 ? 1.3 : -3.2)), new_vec3(0,0,-0.65));
        add_model_renderer(rod, rod_model);
    }
    for (int N = 0; N < 2; N++) {
        Model rod_model = make_tessellated_block_with_uvs(3,0.7,0.7, 4,2,2,  10);
        model_compute_normals(&rod_model);
        rod_model.texture = load_texture("resources/floor.bmp");
        rod_model.textured = true;
        Entity *rod = add_entity(vec3_add(ex_pos, new_vec3(11.56,-1.74, N == 0 ? 1.3 : -3.2)), vec3_zero());
        add_model_renderer(rod, rod_model);
    }
    Model back_model = make_tessellated_block_with_uvs(0.7,10,5.23, 4,2,2,  10);
    model_compute_normals(&back_model);
    back_model.texture = load_texture("resources/floor.bmp");
    back_model.textured = true;
    Entity *back = add_entity(vec3_add(ex_pos, new_vec3(10,-6.4,-0.94)), vec3_zero());
    add_model_renderer(back, back_model);

    #if 0
    {
        vec3 positions[3] = {
            {{10,-7.5,0}},
            {{10,-10.5,-4}},
            {{10,-10.5,4}},
        };
        Model b1 = make_tessellated_block_with_uvs(3,10,3, 4,4,4, 10);
        Model b1collider = make_tessellated_block(3,10,3, 2,2,2);
        for (int i = 0; i < 3; i++) {
            Entity *e1 = add_entity(vec3_add(ex_pos, positions[i]), vec3_zero());
            model_compute_normals(&b1);
            b1.textured = true;
            b1.texture = load_texture("resources/rock.bmp");
            add_model_renderer(e1, b1);
            add_collider(e1, b1collider.vertices, b1collider.num_vertices, false);
        }
    }
    #endif
}
