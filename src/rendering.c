#include "museum.h"
#include "generated/marching_cubes_table.h"

void activate_sun(void)
{
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    float sun_position[4] = { -100,100,0,1 };
    glLightfv(GL_LIGHT0, GL_POSITION, sun_position);
}
void deactivate_sun(void)
{
    glDisable(GL_LIGHTING);
}


void polyhedron_renderer_update(Entity *e, Behaviour *b)
{
    PolyhedronRenderer *renderer = (PolyhedronRenderer *) b->data;
    PolyhedronTriangle *t = renderer->polyhedron.triangles.first;
    prepare_entity_matrix(e);
    glColor3f(X(renderer->color), Y(renderer->color), Z(renderer->color));
    glBegin(GL_TRIANGLES);
    while (t != NULL) {
        for (int i = 0; i < 3; i++) {
            vec3 n = vec3_normalize(vec3_cross(vec3_sub(t->points[1]->position, t->points[0]->position), vec3_sub(t->points[2]->position, t->points[0]->position)));
            glNormal3f(X(n), Y(n), Z(n));
            glVertex3f(X(t->points[i]->position), Y(t->points[i]->position), Z(t->points[i]->position));
        }
        t = t->next;
    }
    glEnd();
}
PolyhedronRenderer *add_polyhedron_renderer(Entity *e, Polyhedron polyhedron, vec4 color)
{
    PolyhedronRenderer *renderer = add_behaviour(e, polyhedron_renderer_update, sizeof(PolyhedronRenderer), PolyhedronRendererID)->data;
    renderer->polyhedron = polyhedron;
    renderer->color = color;
    return renderer;
}

void draw_cubic_bezier_curve(vec3 a, vec3 b, vec3 c, vec3 d, int tessellation, vec4 color, float line_width)
{
    glColor3f(X(color), Y(color), Z(color));
    deactivate_sun();
    glLineWidth(line_width);
    vec3 points[4] = { a,b,c,d };
    vec3 scratch[4] = { a,b,c,d };
    glBegin(GL_LINE_STRIP);
    // de Casteljau's algorithm.
    for (int i = 0; i <= tessellation; i++) {
        float t = i * 1.0 / tessellation;
        if (i > 0) for (int j = 0; j < 4; j++) scratch[j] = points[j];
        for (int j = 3; j > 0; --j) {
            for (int k = 0; k < j; k++) {
                scratch[k] = vec3_lerp(scratch[k], scratch[k+1], t);
            }
        }
        glVertex3f(X(scratch[0]), Y(scratch[0]), Z(scratch[0]));
    }
    glEnd();
}
void catmull_rom_spline_renderer_update(Entity *e, Behaviour *b)
{
    CatmullRomSplineRenderer *spline = (CatmullRomSplineRenderer *) b->data;
    if (spline->num_points == 0) return;

    if (spline->controlled) {
        mat4x4 inv_matrix = rigid_mat4x4_inverse(entity_matrix(e));
        for (int i = 0; i < spline->num_points; i++) {
            spline->points[i] = rigid_matrix_vec3(inv_matrix, spline->controls[i]->position);
        }
    }
    prepare_entity_matrix(e);
    for (int i = 1; i < spline->num_points - 2; i++) {
        vec3 p0 = spline->points[i - 1];
        vec3 p1 = spline->points[i];
        vec3 p2 = spline->points[i + 1];
        vec3 p3 = spline->points[i + 2];
        vec3 a = vec3_add(p1, vec3_mul(vec3_add(vec3_sub(p1, p0), vec3_sub(p2, p1)), 1.5));
        vec3 b = vec3_sub(p2, vec3_mul(vec3_add(vec3_sub(p2, p1), vec3_sub(p3, p2)), 1.5));
        draw_cubic_bezier_curve(p1, a, b, p2, 20, new_vec4(1,1,0,1), 30);
    }
}

CatmullRomSplineRenderer *add_catmull_rom_spline_renderer(Entity *e, vec3 *points, int num_points, bool controlled)
{
    Behaviour *spline_b = add_behaviour(e, catmull_rom_spline_renderer_update, sizeof(CatmullRomSplineRenderer), NoID);
    CatmullRomSplineRenderer *spline = (CatmullRomSplineRenderer *) spline_b->data;
    spline->num_points = num_points;
    spline->points = malloc(sizeof(vec3) * num_points);
    mem_check(spline->points);
    memcpy(spline->points, points, sizeof(vec3) * num_points);
    spline->controlled = controlled;
    if (spline->controlled) {
        spline->controls = malloc(sizeof(Entity *) * num_points);
        mem_check(spline->controls);
        mat4x4 matrix = entity_matrix(e);
        for (int i = 0; i < spline->num_points; i++) {
            Entity *control_point = add_entity(rigid_matrix_vec3(matrix, spline->points[i]), vec3_zero());
            ControlWidget *control_widget = add_control_widget(control_point, 0.5);
            spline->controls[i] = control_point;
        }
    }
    return spline;
}

void render_model(Model *model)
{
    bool normaled = model->has_normals;
    bool textured = model->textured && model->has_uvs;
    if (textured) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, model->texture.texture_id);
    }
    activate_sun();
    glBegin(GL_TRIANGLES);
    if (!model->textured) {
        glColor3f(X(model->flat_color), Y(model->flat_color), Z(model->flat_color));
    } else {
        glColor3f(1,1,1);
    }
    for (int i = 0; i < model->num_triangles; i++) {
        int ai = model->triangles[3*i];
        int bi = model->triangles[3*i+1];
        int ci = model->triangles[3*i+2];
        vec3 a = model->vertices[ai];
        vec3 b = model->vertices[bi];
        vec3 c = model->vertices[ci];
        if (!normaled) {
            // Compute a normal.
            vec3 n = vec3_normalize(vec3_cross(vec3_sub(b, a), vec3_sub(c, a)));
            glNormal3f(UNPACK_VEC3(n));
        }
        if (textured) glTexCoord2f(model->uvs[2*ai], model->uvs[2*ai+1]);
        if (normaled) glNormal3f(UNPACK_VEC3(model->normals[ai]));
        glVertex3f(UNPACK_VEC3(a));
        if (textured) glTexCoord2f(model->uvs[2*bi], model->uvs[2*bi+1]);
        if (normaled) glNormal3f(UNPACK_VEC3(model->normals[bi]));
        glVertex3f(UNPACK_VEC3(b));
        if (textured) glTexCoord2f(model->uvs[2*ci], model->uvs[2*ci+1]);
        if (normaled) glNormal3f(UNPACK_VEC3(model->normals[ci]));
        glVertex3f(UNPACK_VEC3(c));
    }
    glEnd();
    if (textured) {
        glDisable(GL_TEXTURE_2D);
    }
}
void render_wireframe_model(Model *model, float line_width)
{
    deactivate_sun();
    glLineWidth(line_width);
    glBegin(GL_LINES);
    glColor3f(X(model->flat_color), Y(model->flat_color), Z(model->flat_color));
    for (int i = 0; i < model->num_triangles; i++) {
        for (int j = 0; j < 3; j++) {
            glVertex3f(UNPACK_VEC3(model->vertices[model->triangles[3*i+j]]));
            glVertex3f(UNPACK_VEC3(model->vertices[model->triangles[3*i+(j+1)%3]]));
        }
    }
    glEnd();
}

#define eval_vertex(N) {\
    if (model->tessellate_normals) glNormal3f(UNPACK_VEC3(normals[(N)]));\
    if (model->tessellate_uvs) glTexCoord2f(uvs[(N)].vals[0], uvs[(N)].vals[1]);\
    glVertex3f(UNPACK_VEC3(vertices[(N)]));\
}
void render_tessellated_model(Model *model)
{
    TessellationShader shader = model->tessellation_shader;
    int patch_n = model->patch_num_vertices;

    float tess = model->tessellation_level;
    float inv_tess_plus_one = 1.0 / (tess + 1);
    float inv_tess = 1.0 / tess;

    if (model->textured) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, model->texture.texture_id);
    } else {
        glColor3f(X(model->flat_color),Y(model->flat_color),Z(model->flat_color));
    }
    if (model->tessellation_domain == Rectangular) {
        // tl, bl, br, tr
        vec3 vertices[4];
        vec3 normals[4];
        vec2 uvs[4];
        glBegin(GL_TRIANGLES);
        for (int i = 0; i < model->num_vertices; i += patch_n) {
            for (int ui = 0; ui < tess+1; ui++) {
                float u = ui * inv_tess_plus_one;
                float up = (ui+1) * inv_tess_plus_one;
                for (int vi = 0; vi < tess+1; vi++) {
                    float v = vi * inv_tess_plus_one;
                    float vp = (vi+1) * inv_tess_plus_one;
		    vertices[0] = shader(&model->vertices[i], u, v, 0, &normals[0], &uvs[0]);
		    vertices[1] = shader(&model->vertices[i], u, vp, 0, &normals[1], &uvs[1]);
		    vertices[2] = shader(&model->vertices[i], up, vp, 0, &normals[2], &uvs[2]);
		    vertices[3] = shader(&model->vertices[i], up, v, 0, &normals[3], &uvs[3]);
                    if (!model->tessellate_normals) {
                        vec3 n = vec3_normalize(vec3_cross(vec3_sub(vertices[1], vertices[0]), vec3_sub(vertices[2], vertices[0])));
                        glNormal3f(UNPACK_VEC3(n));
                    }
                    eval_vertex(0); eval_vertex(1); eval_vertex(2);
                    if (!model->tessellate_normals) {
                        vec3 n = vec3_normalize(vec3_cross(vec3_sub(vertices[2], vertices[0]), vec3_sub(vertices[3], vertices[0])));
                        glNormal3f(UNPACK_VEC3(n));
                    }
                    eval_vertex(0); eval_vertex(2); eval_vertex(3);
                }
            }
        }
        glEnd();
    } else if (model->tessellation_domain == Triangular) {
        vec3 vertices[4];
        vec3 normals[4];
        vec2 uvs[4];
        glBegin(GL_TRIANGLES);
        for (int i = 0; i < model->num_vertices; i += patch_n) {
            for (int ui = 1; ui <= tess; ui++) {
                for (int vi = 0; vi <= tess - ui; vi++) {
                    int wi = tess - ui - vi;
                    float x = inv_tess;
                    vertices[0] = shader(&model->vertices[i], ui*x, vi*x, wi*x, &normals[0], &uvs[0]);
                    vertices[1] = shader(&model->vertices[i], (ui-1)*x, vi*x, (wi+1)*x, &normals[1], &uvs[1]);
                    vertices[2] = shader(&model->vertices[i], (ui-1)*x, (vi+1)*x, wi*x, &normals[2], &uvs[2]);
                    eval_vertex(0); eval_vertex(1); eval_vertex(2);
                    if (vi < tess && wi > 0) {
                        vertices[3] = shader(&model->vertices[i], ui*x, (vi+1)*x, (wi-1)*x, &normals[3], &uvs[3]);
                        eval_vertex(0); eval_vertex(2); eval_vertex(3);
                    }
                }
            }
        }
        glEnd();
    }
    if (model->textured) {
        glDisable(GL_TEXTURE_2D);
    }
}
#undef eval_vertex

ModelRenderer *add_model_renderer(Entity *e, Model model)
{
    ModelRenderer *renderer = (ModelRenderer *) add_behaviour(e, model_renderer_update, sizeof(ModelRenderer), NoID)->data;
    renderer->model = model;
    return renderer;
}
void model_renderer_update(Entity *e, Behaviour *b)
{
    ModelRenderer *renderer = (ModelRenderer *) b->data;
    prepare_entity_matrix(e);
    if (renderer->model.tessellated) {
        render_tessellated_model(&renderer->model);
    } else if (renderer->wireframe) {
        render_wireframe_model(&renderer->model, renderer->wireframe_width);
    } else {
        render_model(&renderer->model);
    }
}

MetaballRenderer *add_metaball_renderer(Entity *e, int num_points, vec3 *points, float *weights, float threshold, float box_size, bool copy_points)
{
    MetaballRenderer *mr = add_behaviour(e, metaball_renderer_update, sizeof(MetaballRenderer), NoID)->data;
    mr->num_points = num_points;
    if (copy_points) {
        // Copy both the points and the weights. If, for example, it is wanted for them to be controlled, it could be useful to not copy them.
        mr->points = malloc(sizeof(vec3) * num_points);
        mem_check(mr->points);
        mr->weights = malloc(sizeof(float) * num_points);
        mem_check(mr->weights);
        for (int i = 0; i < num_points; i++) {
            mr->points[i] = points[i];
            mr->weights[i] = weights[i];
        }
    } else {
        mr->points = points;
        mr->weights = weights;
    }
    mr->threshold = threshold;
    mr->box_size = box_size;

    //---
    vec3 min_vals = points[0];
    vec3 max_vals = points[0];
    for (int i = 1; i < num_points; i++) {
        vec3 p = points[i];
        for (int j = 0; j < 3; j++) {
            if (p.vals[j] < min_vals.vals[j]) min_vals.vals[j] = p.vals[j];
            if (p.vals[j] > max_vals.vals[j]) max_vals.vals[j] = p.vals[j];
        }
    }
    mr->box_min = min_vals;
    mr->box_max = max_vals;
    //----hack for specific exhibit.
    mr->box_min = vec3_sub(mr->box_min, new_vec3(0.77,0.77,0.77));
    mr->box_max = vec3_add(mr->box_max, new_vec3(0.77,0.77,0.77));
    return mr;
}

float evaluate_metaball_function(MetaballRenderer *mr, vec3 point)
{
    float total = 0;
    for (int i = 0; i < mr->num_points; i++) {
        vec3 p = mr->points[i];
        float w = mr->weights[i];
        vec3 diff = vec3_sub(p, point);
        float rsq = vec3_dot(diff, diff);
        total += w / rsq;

        // float x = (1 - rsq/(0.2*0.2));
        // total += x*x*x;
        // total += w * 1.0 / vec3_dot(diff, diff);
    }
    // printf("%.2f\n", total);
    return total;
}
static void render_metaball_cell(MetaballRenderer *mr, vec3 points[])
{
    uint8_t vertices = 0;
    float evaluations[8];
    for (int i = 0; i < 8; i++) {
        float val = evaluate_metaball_function(mr, points[i]);
        evaluations[i] = val;
        bool inside = val >= mr->threshold;
        if (inside) vertices |= 1 << i;
    }
    // for (int i = 0; i < 8; i++) printf("evaluation %d: %.2f\n", i, evaluations[i]);
    if (vertices == 0 || vertices == 0xFF) return;
    for (int i = 0; i < max_marching_cube_triangles; i++) {
        if (marching_cubes_table[vertices][3*i] == -1) return;
        int16_t *triangle = &marching_cubes_table[vertices][3*i];
        vec3 tri_points[3];
        for (int j = 0; j < 3; j++) {
            // Get the indices of the points from the edge index in the triangle.
            int index_a, index_b;
            if (triangle[j] < 4) {
                index_a = triangle[j];
                index_b = (triangle[j]+1)%4;
            } else if (triangle[j] < 8) {
                index_a = triangle[j] - 4;
                index_b = triangle[j];
            } else {
                index_a = triangle[j] - 4;
                index_b = (triangle[j]+1)%4 + 4;
            }
            float a = evaluations[index_a];
            float b = evaluations[index_b];
            float t = (mr->threshold - a) / (b - a); // threshold = t(point a) + (1 - t)(point b).
            if (t < 0) t = 0;
            if (t > 1) t = 1; //---Don't know why saturating this works, but it seems to.
            tri_points[j] = vec3_lerp(points[index_a], points[index_b], 1-t);
        }
        vec3 n = vec3_normalize(vec3_cross(vec3_sub(tri_points[1], tri_points[0]), vec3_sub(tri_points[2], tri_points[0])));
        glNormal3f(UNPACK_VEC3(n));
        for (int j = 0; j < 3; j++) {
            glVertex3f(UNPACK_VEC3(tri_points[j]));
        }
    }
}

void metaball_renderer_update(Entity *e, Behaviour *b)
{
    MetaballRenderer *mr = b->data;
    float s = mr->box_size;
    
    prepare_entity_matrix(e);
    activate_sun();
    glColor3f(0,1,1);
    glBegin(GL_TRIANGLES);
    vec3 points[8];
    for (float x = X(mr->box_min); x < X(mr->box_max); x += s) {
        for (float y = Y(mr->box_min); y < Y(mr->box_max); y += s) {
            for (float z = Z(mr->box_min); z < Z(mr->box_max); z += s) {
                points[0] = new_vec3(x, y, z);
                points[1] = new_vec3(x+s, y, z);
                points[2] = new_vec3(x+s, y+s, z);
                points[3] = new_vec3(x, y+s, z);
                points[4] = new_vec3(x, y, z+s);
                points[5] = new_vec3(x+s, y, z+s);
                points[6] = new_vec3(x+s, y+s, z+s);
                points[7] = new_vec3(x, y+s, z+s);
                render_metaball_cell(mr, points);
            }
        }
    }
    glEnd();

    if (mr->render_grid) {
        // Render a "heatmap" with values of the function at each grid point.
        glPointSize(2);
        deactivate_sun();
        glDisable(GL_DEPTH);
        glBegin(GL_POINTS);
        for (float x = X(mr->box_min); x < X(mr->box_max)+mr->box_size; x += s) {
            for (float y = Y(mr->box_min); y < Y(mr->box_max)+mr->box_size; y += s) {
                for (float z = Z(mr->box_min); z < Z(mr->box_max)+mr->box_size; z += s) {
                    float val =  evaluate_metaball_function(mr, new_vec3(x, y, z));
                    if (val >= mr->threshold) glColor3f(1,0,0);
                    else glColor3f(1,1,1);
                    glVertex3f(x, y, z);
                }
            }
        }
        glEnd();
        glEnable(GL_DEPTH);
    }
    if (mr->render_points) {
        glDisable(GL_DEPTH);
        glPointSize(5);
        glColor3f(1,0,1);
        glBegin(GL_POINTS);
        // Render the points of each metaball.
        for (int i = 0; i < mr->num_points; i++) {
            glVertex3f(X(mr->points[i]), Y(mr->points[i]), Z(mr->points[i]));
        }
        glEnd();
        glEnable(GL_DEPTH);
    }
}
