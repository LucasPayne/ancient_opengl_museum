#include "museum.h"

/*--------------------------------------------------------------------------------
    This is a "coroutine" variant of the convex hull code using the iterative algorithm.
    The only state needed is the unfinished hull polyhedron and the position in the point index.
--------------------------------------------------------------------------------*/
// Definitions of marks this algorithm makes on features, during processing.
#define VISIBLE true
#define INVISIBLE false
#define NEEDED 0x1
#define BOUNDARY 0x2
static vec3 convex_hull_visualizer_coroutine(vec3 *points, int num_points, Polyhedron *poly, int point_index)
{
    if (num_points < 4) {
        fprintf(stderr, "ERROR: Can only visualize convex hull algorithm if num_points > 4.\n");
        exit(EXIT_FAILURE);
    }
    if (point_index < 4) {
        fprintf(stderr, "ERROR: Visualizer must be initialized at point_index 4.\n");
        exit(EXIT_FAILURE);
    }
    if (point_index == 4) {
        // Start up a polyhedron data structure as a tetrahedron.
        *poly = new_polyhedron();
        PolyhedronPoint *tetrahedron_points[4];
        for (int i = 0; i < 4; i++) {
            tetrahedron_points[i] = polyhedron_add_point(poly, points[i]);
        }
        bool negative = tetrahedron_6_times_volume(points[0], points[1], points[2], points[3]) < 0;
        if (negative) {
            // Fix the orientation by swapping two of the points.
            PolyhedronPoint *temp = tetrahedron_points[0];
            tetrahedron_points[0] = tetrahedron_points[1];
            tetrahedron_points[1] = temp;
        }
        PolyhedronEdge *e1 = polyhedron_add_edge(poly, tetrahedron_points[0], tetrahedron_points[1]);
        PolyhedronEdge *e2 = polyhedron_add_edge(poly, tetrahedron_points[1], tetrahedron_points[2]);
        PolyhedronEdge *e3 = polyhedron_add_edge(poly, tetrahedron_points[2], tetrahedron_points[0]);
        polyhedron_add_triangle(poly, tetrahedron_points[0], tetrahedron_points[1], tetrahedron_points[2], e1, e2, e3);
        PolyhedronEdge *e4 = polyhedron_add_edge(poly, tetrahedron_points[0], tetrahedron_points[3]);
        PolyhedronEdge *e5 = polyhedron_add_edge(poly, tetrahedron_points[1], tetrahedron_points[3]);
        PolyhedronEdge *e6 = polyhedron_add_edge(poly, tetrahedron_points[2], tetrahedron_points[3]);
        polyhedron_add_triangle(poly, tetrahedron_points[3], tetrahedron_points[1], tetrahedron_points[0], e1, e5, e4);
        polyhedron_add_triangle(poly, tetrahedron_points[3], tetrahedron_points[2], tetrahedron_points[1], e2, e6, e5);
        polyhedron_add_triangle(poly, tetrahedron_points[3], tetrahedron_points[0], tetrahedron_points[2], e3, e4, e6);
        return tetrahedron_points[3]->position;
    }
    int i = point_index;
    // FOR i=point_index .. num_points-1: This is where the loop would usually be.
        // Clean up the marks for points and edges (not neccessary for triangles, since they are always set).
        PolyhedronPoint *p = poly->points.first;
        while (p != NULL) {
            p->mark = 0;
            p->saved_edge = NULL;
            p = p->next;
        }
        PolyhedronEdge *e = poly->edges.first;
        while (e != NULL) { 
            e->mark = 0;
            e = e->next;
        }
        // For each triangle, mark as visible or invisible from the point of view of the new point.
        // If none are visible, then this point is in the hull so far, so continue.
        // Otherwise, use the marks scratched during the triangle checks to remove all visible triangles and
        // all other uneccessary features.
        // Then, add a new triangle for each border edge, which is an edge whose adjacent triangles have opposing visibility/invisibility.
        bool any_visible = false;
        PolyhedronTriangle *t = poly->triangles.first;
        while (t != NULL) {
            float v = tetrahedron_6_times_volume(t->points[0]->position, t->points[1]->position, t->points[2]->position, points[i]);
            if (v < 0) {
                t->mark = VISIBLE;
                any_visible = true;
            } else {
                t->mark = INVISIBLE;
                // mark edges and vertices of this triangle as necessary (they won't be deleted).
                t->points[0]->mark |= NEEDED;
                t->points[1]->mark |= NEEDED;
                t->points[2]->mark |= NEEDED;
                t->edges[0]->mark |= NEEDED;
                t->edges[1]->mark |= NEEDED;
                t->edges[2]->mark |= NEEDED;
            }
            t = t->next;
        }
        if (!any_visible) {
            return points[i];
            // continue; // The point is in the hull so far.
        }
        // Before deleting anything, use the visibility information to mark the borders. Then when triangles are deleted, the borders are still known.
        // This could be done more cleanly without looping over features so many times.
        e = poly->edges.first;
        while (e != NULL) {
            if (e->triangles[0]->mark != e->triangles[1]->mark) {
                e->mark |= BOUNDARY;
            }
            e = e->next;
        }
        // Remove all visible triangles (that weren't just added), and unneeded points and edges.
        t = poly->triangles.first;
        while (t != NULL) {
            if (t->mark == VISIBLE) {
                PolyhedronTriangle *to_remove = t;
                t = t->next;
                polyhedron_remove_triangle(poly, to_remove);
            } else t = t->next;
        }
        e = poly->edges.first;
        while (e != NULL) {
            if ((e->mark & NEEDED) == 0) {
                PolyhedronEdge *to_remove = e;
                e = e->next;
                polyhedron_remove_edge(poly, to_remove);
                continue;
            }
            e = e->next;
        }
        p = poly->points.first;
        while (p != NULL) {
            if ((p->mark & NEEDED) == 0) {
                PolyhedronPoint *to_remove = p;
                p = p->next;
                polyhedron_remove_point(poly, to_remove);
                continue;
            }
            p = p->next;
        }
        // Add the new point, and new edges and triangles to create a cone from the new point to the border.
        PolyhedronPoint *new_point = polyhedron_add_point(poly, points[i]);
        e = poly->edges.first;
        while (e != NULL) {
            if ((e->mark & BOUNDARY) != 0) {
                // Determine whether a->b is in line with the winding of the invisible triangle incident to ab, and adjust accordingly.
                bool reverse = false;
                for (int i = 0; i < 2; i++) {
                    if (e->triangles[i] != NULL) {
                        for (int j = 0; j < 3; j++) {
                            if (e->a == e->triangles[i]->points[j] && e->b == e->triangles[i]->points[(j+1)%3]) reverse = true;
                        }
                        break;
                    }
                }
                // In winding order, introduce two new edges, unless an edge has already been made.
                PolyhedronPoint *p1 = reverse ? e->b : e->a;
                PolyhedronPoint *p2 = reverse ? e->a : e->b;
                PolyhedronEdge *e1 = p1->saved_edge;
                if (e1 == NULL) {
                    e1 = polyhedron_add_edge(poly, new_point, p1);
                    p1->saved_edge = e1;
                }
                PolyhedronEdge *e2 = p2->saved_edge;
                if (e2 == NULL) {
                    e2 = polyhedron_add_edge(poly, new_point, p2);
                    p2->saved_edge = e2;
                }
                // Use these to form a new triangle.
                polyhedron_add_triangle(poly, new_point, p1, p2, e1, e, e2);
            }
            e = e->next;
        }
    return points[i];
}

static void split_model(Model model, vec3 point, vec3 normal, Model *part_a, Model *part_b)
{
    int a_count = 0;
    int b_count = 0;
    for (int i = 0; i < model.num_vertices; i++) {
        if (vec3_dot(vec3_sub(model.vertices[i], point), normal) > 0) {
            a_count ++;
        } else {
            b_count ++;
        }
    }
    Model part_a_model = model;
    part_a_model.num_vertices = a_count;
    part_a_model.vertices = malloc(sizeof(vec3) * a_count);
    mem_check(part_a_model.vertices);
    Model part_b_model = model;
    part_b_model.num_vertices = b_count;
    part_b_model.vertices = malloc(sizeof(vec3) * b_count);
    mem_check(part_b_model.vertices);
    int a_index = 0;
    int b_index = 0;
    for (int i = 0; i < model.num_vertices; i++) {
        if (vec3_dot(vec3_sub(model.vertices[i], point), normal) > 0) {
            part_a_model.vertices[a_index] = model.vertices[i];
            part_a_model.uvs[2*a_index] = model.uvs[2*i];
            part_a_model.uvs[2*a_index+1] = model.uvs[2*i+1];
            a_index ++;
        } else {
            part_b_model.vertices[b_index] = model.vertices[i];
            part_b_model.uvs[2*b_index] = model.uvs[2*i];
            part_b_model.uvs[2*b_index+1] = model.uvs[2*i+1];
            b_index ++;
        }
    }
    // Construct another model as an easy way to get the triangles.
    Model ___part_a_model = polyhedron_to_model(convex_hull(part_a_model.vertices, part_a_model.num_vertices));
    part_a_model.triangles = ___part_a_model.triangles;
    part_a_model.num_triangles = ___part_a_model.num_triangles;
    // Construct another model as an easy way to get the triangles.
    Model ___part_b_model = polyhedron_to_model(convex_hull(part_b_model.vertices, part_b_model.num_vertices));
    part_b_model.triangles = ___part_b_model.triangles;
    part_b_model.num_triangles = ___part_b_model.num_triangles;
    *part_a = part_a_model;
    *part_b = part_b_model;

    //-----Destroy ___part_b_model.
    // int a_count = 0;
    // int b_count = 0;
    // for (int i = 0; i < model.num_vertices; i++) {
    //     if (vec3_dot(vec3_sub(model.vertices[i], point), normal) > 0) {
    //         a_count ++;
    //     } else {
    //         b_count ++;
    //     }
    // }
    // Model part_a_model = model;
    // part_a_model.num_vertices = a_count;
    // part_a_model.vertices = malloc(sizeof(vec3) * a_count);
    // mem_check(part_a_model.vertices);
    // Model part_b_model = model;
    // part_b_model.num_vertices = b_count;
    // part_b_model.vertices = malloc(sizeof(vec3) * b_count);
    // mem_check(part_b_model.vertices);
    // int a_index = 0;
    // int b_index = 0;
    // for (int i = 0; i < model.num_vertices; i++) {
    //     if (vec3_dot(vec3_sub(model.vertices[i], point), normal) > 0) {
    //         part_a_model.vertices[a_index] = model.vertices[i];
    //         part_a_model.uvs[2*a_index] = model.uvs[2*i];
    //         part_a_model.uvs[2*a_index+1] = model.uvs[2*i+1];
    //         a_index ++;
    //     } else {
    //         part_b_model.vertices[b_index] = model.vertices[i];
    //         part_b_model.uvs[2*b_index] = model.uvs[2*i];
    //         part_b_model.uvs[2*b_index+1] = model.uvs[2*i+1];
    //         b_index ++;
    //     }
    // }
    // // Construct another model as an easy way to get the triangles.
    // Model ___part_a_model = polyhedron_to_model(convex_hull(part_a_model.vertices, part_a_model.num_vertices));
    // part_a_model.triangles = ___part_a_model.triangles;
    // part_a_model.num_triangles = ___part_a_model.num_triangles;
    // //-----Destroy ___part_a_model.
    // Entity *part_a = add_entity(e->position, e->euler_angles);
    // part_a->scale = e->scale;
    // ModelRenderer *part_a_renderer = add_model_renderer(part_a, part_a_model);
    // // Construct another model as an easy way to get the triangles.
    // Model ___part_b_model = polyhedron_to_model(convex_hull(part_b_model.vertices, part_b_model.num_vertices));
    // part_b_model.triangles = ___part_b_model.triangles;
    // part_b_model.num_triangles = ___part_b_model.num_triangles;
    //-----Destroy ___part_b_model.
    *part_a = part_a_model;
    *part_b = part_b_model;
}

typedef struct ConvexHullVisualizer_s {
    Model model;
    vec3 *points;
    int num_points;
    float timer;
    bool has_hull;
    
    vec3 prev_points[3];

    Polyhedron hull;
    int point_index;

    Model hull_model;
} ConvexHullVisualizer;

static void visualize_convex_hull(Entity *e, Behaviour *b)
{
    static bool set_texture = false;
    static Texture ice_texture;
    if (!set_texture) {
        ice_texture = load_texture("resources/ice.bmp");
        set_texture = true;
    }

    float rate = 200.0;
    ConvexHullVisualizer *v = (ConvexHullVisualizer *) b->data;

    vec3 hull_color = new_vec3(0,0,0);

    prepare_entity_matrix(e);
    const vec3 colorA = {{ 0.7, 0.7, 0.98 }};
    if (v->has_hull) {
        render_model(&v->hull_model);
    } else {
        render_wireframe_model(&v->model, 1);
        deactivate_sun();
        glLineWidth(5);
        glBegin(GL_LINES);
        glColor3f(UNPACK_VEC3(hull_color));
        PolyhedronEdge *edge = (PolyhedronEdge *) v->hull.edges.first;
        while (edge != NULL) {
            vec3 a = edge->a->position;
            vec3 b = edge->b->position;
            glVertex3f(UNPACK_VEC3(a));
            glVertex3f(UNPACK_VEC3(b));
            edge = edge->next;
        }
        glEnd();
        glLineWidth(1);
        glDisable(GL_DEPTH_TEST);
        glBegin(GL_LINES);
        glColor3f(UNPACK_VEC3(colorA));
        edge = (PolyhedronEdge *) v->hull.edges.first;
        while (edge != NULL) {
            vec3 a = edge->a->position;
            vec3 b = edge->b->position;
            glVertex3f(UNPACK_VEC3(a));
            glVertex3f(UNPACK_VEC3(b));
            edge = edge->next;
        }
        glEnd();
        glEnable(GL_DEPTH_TEST);
        if (v->num_points > 1) {
            glLineWidth(15);
            glBegin(GL_LINES);
            glColor3f(UNPACK_VEC3(colorA));
            for (int i = 0; i < v->num_points && i < 2; i++) {
                vec3 a = v->prev_points[i];
                vec3 b = v->prev_points[i+1];
                glVertex3f(UNPACK_VEC3(a));
                glVertex3f(UNPACK_VEC3(b));
            }
            glEnd();
        }
    }
    v->timer -= dt;
    if (v->timer <= 0) {
        if (v->has_hull) {
            #if 0
            // Blow up the hull.
            Model parts[4];
            split_model(v->hull_model, polytope_center_of_mass(v->hull_model.vertices, v->hull_model.num_vertices), new_vec3(frand()-0.5,frand()-0.5,frand()-0.5), &parts[0], &parts[2]);
            split_model(parts[0], polytope_center_of_mass(parts[0].vertices, parts[0].num_vertices), new_vec3(frand()-0.5,frand()-0.5,frand()-0.5), &parts[0], &parts[1]);
            split_model(parts[2], polytope_center_of_mass(parts[2].vertices, parts[0].num_vertices), new_vec3(frand()-0.5,frand()-0.5,frand()-0.5), &parts[2], &parts[3]);
            for (int i = 0; i < 4; i++) {
                if (parts[i].num_vertices == 0) continue;
                Entity *part = add_entity(e->position, e->euler_angles);
                part->scale = e->scale;
                ModelRenderer *part_renderer = add_model_renderer(part, parts[i]);
                add_collider(part, parts[i].vertices, parts[i].num_vertices, true);
                add_rigid_body(part, 1);
            }
            #endif

            //----Destroy the hull.
            v->has_hull = false;
            v->point_index = 4;
            v->timer = 1.0 / rate;
        } else {
            if (v->point_index == v->num_points) {
                v->timer = 5;
                v->has_hull = true;
                Model model = polyhedron_to_model(v->hull);
                model_compute_normals(&model);
                model.uvs = malloc(sizeof(float) * 2*model.num_vertices);
                mem_check(model.uvs);
                for (int i = 0; i < model.num_vertices; i++) {
                    model.uvs[2*i] = 0.29 * e->scale * X(model.vertices[i]);
                    model.uvs[2*i+1] = 0.29 * e->scale * Z(model.vertices[i]);
                }
                model.has_uvs = true;
                model.textured = true;
                model.texture = ice_texture;
                v->hull_model = model;
            } else {
                vec3 new_point = convex_hull_visualizer_coroutine(v->points, v->num_points, &v->hull, v->point_index);
                v->prev_points[2] = v->prev_points[1];
                v->prev_points[1] = v->prev_points[0];
                v->prev_points[0] = new_point;
                v->point_index ++;
                v->timer = 1.0 / rate;
            }
        }
    }
}

static ConvexHullVisualizer *make_visualizer(Model model, vec3 position, vec3 orientation, float scale)
{
    model_compute_normals(&model);
    model.flat_color = WHITE;
    Entity *e = add_entity(position, orientation);
    e->euler_controlled = true;
    e->scale = scale;
    ConvexHullVisualizer *visualizer = (ConvexHullVisualizer *) add_behaviour(e, visualize_convex_hull, sizeof(ConvexHullVisualizer), NoID)->data;
    visualizer->points = model.vertices;
    visualizer->num_points = model.num_vertices;
    visualizer->point_index = 4;
    visualizer->has_hull = false;
    visualizer->model = model;
    return visualizer;
}
void create_exhibit_convex_hull(void)
{
    make_visualizer(load_OFF_model("resources/stanford_bunny_low.off"), new_vec3(15,-3,-36), new_vec3(0,M_PI/2,0), 50);
    //make_visualizer(load_OFF_model("resources/dolphins.off"), new_vec3(6,10,-45), new_vec3(-M_PI/2,0,0), 0.01);
    make_visualizer(load_OFF_model("resources/dolphins.off"), new_vec3(15,10,-39), new_vec3(-M_PI/2,0,M_PI/2), 0.01);
}
