
/*
    Entity *spline_e = add_entity(new_vec3(-25, 2, -35), new_vec3(0,0,0));
    vec3 points[5];
    for (int i = 0; i < 5; i++) {
        float r = 3;
        points[i] = new_vec3(frand()*r-r/2,frand()*r-r/2,frand()*r-r/2);
    }
    add_catmull_rom_spline_renderer(spline_e, points, 5, true);
    // add_control_widget(spline_e, 0.2);
    // Behaviour *spline_b = add_behaviour(spline_e, catmull_rom_spline_renderer_update, sizeof(CatmullRomSplineRenderer), NoID);
    // CatmullRomSplineRenderer *spline = (CatmullRomSplineRenderer *) spline_b->data;
*/

/*
    // Create some pillars as surfaces of revolution.
    float ys[] = {0, 1, 3, 3, 4, 4.5, 14.5, 15, 16, 16, 18, 19};
    float xs[] = {10, 10, 9, 8, 7, 6, 6, 7, 8, 9, 10, 10};
    for (int i = 0; i < 12; i++) {
        ys[i] *= 0.05;
        xs[i] *= 0.05;
        ys[i] -= 0.1;
    }
    Model pillar_model = make_surface_of_revolution(xs, ys, 12, 10);
    vec3 pillars_position = new_vec3(0,-1,-40);
    pillar_model.flat_color = BLUE;
    // model_compute_normals(&pillar_model);
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 3; j++) {
            Entity *pillar = add_entity(vec3_add(pillars_position, new_vec3(i*1.4,0,j*1.4)), vec3_zero());
            ModelRenderer *pillar_renderer = add_model_renderer(pillar, pillar_model);
            Polyhedron block = make_block(1.1,2.1,1.1);
            add_collider(pillar, polyhedron_points(block), polyhedron_num_points(&block));

            pillar_renderer->wireframe = true;
            pillar_renderer->wireframe_width = 1;
            // Polyhedron hull = convex_hull(pillar_geometry.vertices, pillar_geometry.num_vertices);
            // add_collider(pillar, hull);
        }
    }
*/

static void grow_tree(Entity *branch, float length, int amount)
{
    static bool set_texture = false;
    static Texture bark_texture;
    if (!set_texture) {
        bark_texture = load_texture("resources/bark.bmp");
        set_texture = true;
    }

    vec3 branch_up = matrix_vec3(entity_orientation(branch), new_vec3(0,1,0));

    for (int i = 0; i < amount; i++) {
        float theta = crand();
        float along = -length/2.0 + frand()*length;

        Model model = make_cylinder(length * 0.1, length);
        for (int i = 0; i < model.num_vertices; i++) Y(model.vertices[i]) += length/2.0;
        model_compute_normals(&model);
        // Project a bark texture onto the cylinder.
        model.uvs = malloc(sizeof(float) * 2*model.num_vertices);
        mem_check(model.uvs);
        for (int i = 0; i < model.num_vertices; i++) {
            model.uvs[2*i] = 0.2 * 2*acos(X(model.vertices[i]) / (length * 0.1 * 2));
            model.uvs[2*i+1] = 0.2 * Y(model.vertices[i]);
        }
        model.has_uvs = true;
        model.texture = bark_texture;
        model.textured = true;

        vec3 pos = vec3_add(branch->position, vec3_mul(branch_up, amount));
        Entity *new_branch = add_entity(pos, new_vec3(0, 0, 0));
        new_branch->scale = branch->scale * 0.5;
        add_model_renderer(new_branch, model);
        
        int next_amount = amount - rand()%3;
        if (next_amount > 0) grow_tree(branch, length * (frand()*0.5+0.25), next_amount);
    }
}
