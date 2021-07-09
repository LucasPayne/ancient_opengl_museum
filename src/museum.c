/*================================================================================
    This is the main file for the museum program.
    It contains context setup code, and routines for the creation of the base scene,
    without exhibits.
================================================================================*/
#include "museum.h"

// Global variables.
//--------------------------------------------------------------------------------
// Windowing.
float aspect_ratio = DEFAULT_ASPECT_RATIO;
int window_width = 0;
int window_height = 0;
// Time.
float total_time = 0;
float dt = 0;
// Mechanics.
vec3 player_start_position = {{0,3,0}};
//vec3 player_start_position = {{-10,5,-40}};
float gravity_constant = 9.81;
// Matrices and camera.
// This program does not use the matrix stack, but rather always has one matrix.
// When the camera updates, both the projection and view matrices are updated. The view matrix needs to be stored globally
// so that GL_MODELVIEW can be set to view*model for a specific model.
mat4x4 view_matrix = {{0}};
Camera *main_camera = NULL;
Entity *main_camera_entity = NULL;
//--------------------------------------------------------------------------------

void update_time(int value) // the int parameter is needed by glut, but not used.
{
    // Update global time information.
    static bool set_time = false;
    if (!set_time) {
        total_time = glutGet(GLUT_ELAPSED_TIME) / 1000.0;
        set_time = true;
    }
    float new_time = glutGet(GLUT_ELAPSED_TIME) / 1000.0;
    dt = new_time - total_time;
    total_time = new_time;
    glutTimerFunc(1, update_time, 0); // Update the time every millisecond.
}

void update(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Update global systems.
    rigid_body_dynamics();
    // Update the entities by invoking their behaviours.
    for (int i = 0; i < entity_list_length; i++) {
        Entity *entity = &entity_list[i];
        for (int i = 0; i < entity->num_behaviours; i++) {
            if (entity->behaviours[i]->update == NULL || !entity->behaviours[i]->active) continue;
            entity->behaviours[i]->update(entity, entity->behaviours[i]);
        }
    }

    glFlush();
    glutPostRedisplay();
}

void window_reshape_callback(int width, int height)
{
    window_width = width;
    window_height = height;
    if (height * 1.0 / width > aspect_ratio) {
        // rearrange (height - 2v) / width = aspect_ratio.
        float v = -(aspect_ratio*width - height) / 2.0;
        glViewport(0, v, width, height - 2*v);
    } else {
        // rearrange height / (width - 2h) = aspect_ratio.
        float h = -(height / aspect_ratio - width) / 2.0;
        glViewport(h, 0, width - 2*h, height);
    }
}


void make_tree(vec3 position, float size)
{
    // A tree is made from a half-capsule with no collider, and a cylindrical trunk with a collider.
    Entity *tree = add_entity(position, vec3_zero());

    // Create the leaves model as a half capsule.
    Model leaves_model = make_half_capsule(size, size*2);
    for (int i = 0; i < leaves_model.num_vertices; i++) leaves_model.vertices[i].vals[1] += size;
    model_compute_normals(&leaves_model);
    // Project a texture of leaves onto the tree.
    leaves_model.uvs = malloc(sizeof(float) * 2*leaves_model.num_vertices);
    mem_check(leaves_model.uvs);
    for (int i = 0; i < leaves_model.num_vertices; i++) {
        vec3 v = leaves_model.vertices[i];
        leaves_model.uvs[2*i] = 5 * acos(0.25 * X(v) / sqrt(X(v)*X(v) + Z(v)*Z(v)));
        leaves_model.uvs[2*i+1] = Y(v);
    }
    leaves_model.has_uvs = true;
    leaves_model.texture = load_texture("resources/leaves.bmp");
    leaves_model.textured = true;
    ModelRenderer *leaves_renderer = add_model_renderer(tree, leaves_model);

    // Create the trunk as a cylinder.
    Model trunk_model = make_cylinder(size * 0.2, size);
    // for (int i = 0; i < trunk_model.num_vertices; i++) Y(trunk_model.vertices[i]) += size/2.0;
    model_compute_normals(&trunk_model);
    // Project a bark texture onto the cylinder.
    trunk_model.uvs = malloc(sizeof(float) * 2*trunk_model.num_vertices);
    mem_check(trunk_model.uvs);
    for (int i = 0; i < trunk_model.num_vertices; i++) {
        trunk_model.uvs[2*i] = 0.4 * 2*acos(X(trunk_model.vertices[i]) / (size * 0.2 * 2));
        trunk_model.uvs[2*i+1] = 0.4 * Y(trunk_model.vertices[i]);
    }
    trunk_model.has_uvs = true;
    trunk_model.texture = load_texture("resources/bark.bmp");
    trunk_model.textured = true;
    ModelRenderer *trunk_renderer = add_model_renderer(tree, trunk_model);
    // Add a collider to the trunk.
    add_collider(tree, trunk_model.vertices, trunk_model.num_vertices, false);
}
void create_nature(void)
{
    // Create the map floor.
    {
        Entity *floor = add_entity(new_vec3(0, -10,0), new_vec3(0,0,0));
        Model floor_model = make_tessellated_block_with_uvs(400,10,400, 50,2,50, 30);
        floor_model.textured = false;
        // floor_model.flat_color = new_vec4(0.8,0.8,0.9,1);
        floor_model.flat_color = new_vec4(0.73,0.73,0.73,1);
        model_compute_normals(&floor_model);
        ModelRenderer *renderer = add_model_renderer(floor, floor_model);
        Model collider = make_tessellated_block(1000,10,1000, 2,2,2);
        add_collider(floor, collider.vertices, collider.num_vertices, false);
    }
    // Create some hilly terrain. Some semi-random rolly hills are wanted, and this is done by using random-number seeds,
    // and generating random point clouds, and taking their convex hulls.
    const int num_seeds = 2;
    const unsigned int seeds[] = {
        0x340984,
        0x322011,
    };
    for (int k = 0; k < num_seeds; k++) {
        #define N 1024
        vec3 hilly_points[N];
        unsigned int seed = seeds[k];
        srand(seed);
        float s = 10;
        for (int i = 0; i < N; i++) {
            float random[3];
            for (int j = 0; j < 3; j++) random[j] = frand();
            random[1] *= random[0]*random[0] + random[1]*random[1];
            hilly_points[i] = new_vec3(10*random[0]*s, 0.8 * random[1]*s, 10*random[2]*s);
        }
        Model hills_model = polyhedron_to_model(convex_hull(hilly_points, N));
        model_compute_normals(&hills_model);
        // Orthogonally project a texture onto the hill from above.
        float *uvs = malloc(sizeof(float) * 2*hills_model.num_vertices);
        mem_check(uvs);
        for (int i = 0; i < hills_model.num_vertices; i++) {
            vec3 v = hills_model.vertices[i];
            uvs[2*i] =  0.1 * X(v);
            uvs[2*i+1] =  0.1 * Z(v);
        }
        hills_model.uvs = uvs;
        hills_model.has_uvs = true;
        hills_model.texture = load_texture("resources/snow.bmp");
        hills_model.textured = true;

        Entity *hills = add_entity(new_vec3(-60,-14,10), new_vec3(0,-M_PI/2,0));
        ModelRenderer *renderer = add_model_renderer(hills, hills_model);
        add_collider(hills, hills_model.vertices, hills_model.num_vertices, true);
        #undef N
    }
    // Create trees.
    make_tree(new_vec3(-10,-4,-15), 2);
    make_tree(new_vec3(-13,-4.5,-13), 2.6);
    make_tree(new_vec3(7,-4.2,-13), 2.2);
    make_tree(new_vec3(11,-3.5,-12), 4);

    // Create the sky.
    Entity *skybox = add_entity(new_vec3(0,0,-130), new_vec3(0,M_PI/2,0));
    add_skybox(skybox, "resources/snow/top.bmp",
                       "resources/snow/bottom.bmp",
                       "resources/snow/right.bmp",
                       "resources/snow/left.bmp",
                       "resources/snow/front.bmp",
                       "resources/snow/back.bmp", 10000);
}

void create_museum(void)
{
    // Create foundations for the museum.
    {
        Entity *foundations = add_entity(new_vec3(-10,-6.4,-40), new_vec3(0,0,0));
        float w,h,d;
        w = 60; h = 10; d = 30;
        Model foundations_model = make_tessellated_block_with_uvs(w,h,d, 30,30,30, 13);
        foundations_model.texture = load_texture("resources/floor.bmp");
        foundations_model.textured = true;
        foundations_model.flat_color = GRAY;
        model_compute_normals(&foundations_model);
        ModelRenderer *renderer = add_model_renderer(foundations, foundations_model);
        Model collider_model = make_tessellated_block(w,h,d, 2,2,2);
        add_collider(foundations, collider_model.vertices, collider_model.num_vertices, false);
    }
    // Create pillars to hold up the roof. These are surfaces of revolution designed on grid paper.
    {
        vec3 pillar_positions[4] = {
            {{-35,-1.4,-29}},
            {{15,-1.4,-29}},
            {{-35,-1.4,-49}},
            {{15,-1.4,-49}},
        };
        float ys[] = {0, 1, 3, 3, 4, 4.5, 9.5, 14.5, 19.5, 24.5, 29.5, 34.5, 39.5, 44.5, 49.5, 54.5, 59.5, 64.5, 65, 66, 66, 68, 69};
        float xs[] = {10, 10, 9, 8, 7, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 8, 9, 10, 10};
        Model pillar_model = make_surface_of_revolution(xs, ys, 23, 10);
        // Project a texture around the pillar.
        float *uvs = malloc(sizeof(float) * 2*pillar_model.num_vertices);
        mem_check(uvs);
        for (int i = 0; i < pillar_model.num_vertices; i++) {
            vec3 v = pillar_model.vertices[i];
            uvs[2*i] = acos(X(v) / (X(v)*X(v) + Z(v)*Z(v)));
            uvs[2*i+1] = 0.03*Y(v);
        }
        pillar_model.uvs = uvs;
        pillar_model.has_uvs = true;
        pillar_model.texture = load_texture("resources/rock.bmp");
        pillar_model.textured = true;
        // Vertex normals are not computed. This is so they are computed when rendered, and the pillar looks stoney and flat.
        for (int i = 0; i < 4; i++) {
            Entity *pillar = add_entity(pillar_positions[i], new_vec3(0,M_PI/4,0));
            pillar->scale = 0.3;
            ModelRenderer *renderer = add_model_renderer(pillar, pillar_model);
            // The pillar is not convex, so break it apart into approximate convex pieces for its collider geometry.
            {
                Model collider_model = make_tessellated_block(14.5,4,14.5, 2,2,2);
                for (int i = 0; i < collider_model.num_vertices; i++) {
                    Y(collider_model.vertices[i]) += 2;
                }
                Collider *collider = add_collider(pillar, collider_model.vertices, collider_model.num_vertices, false);
            }
            {
                Model collider_model = make_tessellated_block(16.5,4,16.5, 2,2,2);
                for (int i = 0; i < collider_model.num_vertices; i++) {
                    Y(collider_model.vertices[i]) -= 0.5;
                }
                Collider *collider = add_collider(pillar, collider_model.vertices, collider_model.num_vertices, false);
            }
            {
                float pillar_collider_ys[2] = {0, 69};
                float pillar_collider_xs[2] = {6.1, 6.1};
                Model collider_model = make_surface_of_revolution(pillar_collider_xs, pillar_collider_ys, 2, 10);
                Collider *collider = add_collider(pillar, collider_model.vertices, collider_model.num_vertices, false);
            }
        }
    }
    // Add a roof.
    {
        Entity *roof = add_entity(new_vec3(-10,21,-40), new_vec3(0,0,0));
        Model roof_model = make_tessellated_block_with_uvs(60,3,30, 10,10,10, 13);
        roof_model.texture = load_texture("resources/rock.bmp");
        roof_model.textured = true;
        model_compute_normals(&roof_model);
        ModelRenderer *renderer = add_model_renderer(roof, roof_model);
        
        Model collider_model = make_tessellated_block(60,3,30, 2,2,2);
        add_collider(roof, collider_model.vertices, collider_model.num_vertices, false);
    }
    // Create a staircase.
    {
        Model step_model = make_tessellated_block_with_uvs(6,3,1.6,  3,3,3,  4);
        step_model.textured = true;
        step_model.texture = load_texture("resources/rock.bmp");
        vec3 pos = new_vec3(0,-5.5,-18.9);
        for (int i = 0; i < 9; i++) {
            Entity *step = add_entity(vec3_add(new_vec3(0,0.3*i,-0.8*i), pos), new_vec3(0,-0.36,0));
            ModelRenderer *renderer = add_model_renderer(step, step_model);
            add_collider(step, step_model.vertices, step_model.num_vertices, false);
        }
    }
}

void museum_initialize(void)
{
    init_entity_system();
    create_player(player_start_position, 0.2, 10, 0.3);
    create_nature();
    create_museum();
    create_exhibit_convex_hull();
    create_exhibit_rigid_body_dynamics();
    create_exhibit_curves_and_surfaces();
    create_exhibit_interactions();
}

void initialize(int argc, char *argv[])
{
    glutInit(&argc, argv);            
    glutInitDisplayMode(GLUT_SINGLE | GLUT_DEPTH);  
    glutInitWindowSize(600, 600);
    glutInitWindowPosition(0, 0);
    glutCreateWindow("Museum");

    glutDisplayFunc(update);
    glutReshapeFunc(window_reshape_callback);
    glutKeyboardFunc(key_input_callback);
    glutSpecialFunc(special_key_input_callback);
    glutKeyboardUpFunc(key_up_input_callback);
    glutSpecialUpFunc(special_key_up_input_callback);
    glutMouseFunc(mouse_input_callback);
    glutMotionFunc(mouse_motion_callback);

    glClearColor(0,0,0,1);
    activate_sun();

    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);
    glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_COLOR_MATERIAL);
    float white[4] = {1,1,1,1};
    glMaterialfv(GL_FRONT, GL_SPECULAR, white);
    glMaterialf(GL_FRONT, GL_SHININESS, 50);

    window_width = glutGet(GLUT_WINDOW_WIDTH);
    window_height = glutGet(GLUT_WINDOW_HEIGHT);

    update_time(0);
}

void run_tests(void)
{
    // Put initialization tests here.
}

int main(int argc, char *argv[])
{
    initialize(argc, argv);
    run_tests();
    museum_initialize();
    glutMainLoop();
}
