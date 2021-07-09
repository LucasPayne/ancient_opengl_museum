/*--------------------------------------------------------------------------------
    Procedural model creation and loading module.
--------------------------------------------------------------------------------*/
#include "museum.h"

// Read OFF models with triangle faces. Comments are allowed.
Model load_OFF_model(char *filename)
{    
    #define load_error(ERROR_STRING) {\
        fprintf(stderr, "Error loading OFF file: " ERROR_STRING "\n");\
        exit(EXIT_FAILURE);\
    }
    FILE *file = fopen(filename, "r");
    if (file == NULL) load_error("File doesn't exist.");
    #define line_size 512
    char line_buffer[line_size];
    #define get_line() {\
        while (1) {\
            if (fgets(line_buffer, line_size, file) == NULL) load_error("Couldn't get a line.");\
            break;\
        }\
    }
    get_line();
    if (strcmp(line_buffer, "OFF\n") != 0) load_error("OFF files must start with the line \"OFF\".");
    get_line();
    int num_vertices, num_faces, num_edges;
    if (sscanf(line_buffer, "%d %d %d\n", &num_vertices, &num_faces, &num_edges) == EOF) load_error("Line should consist of 3 numbers, for #vertices, #faces, #edges.");
    vec3 *vertices = malloc(sizeof(vec3) * num_vertices);
    mem_check(vertices);
    for (int i = 0; i < num_vertices; i++) {
        get_line();
        float x,y,z;
        if (sscanf(line_buffer, "%f %f %f\n", &x, &y, &z) == EOF) load_error("Malformed vertex entry.");
        vertices[i] = new_vec3(x,y,z);
    }
    uint16_t *triangles = malloc(sizeof(uint16_t)* 3*num_faces);
    mem_check(triangles);
    for (int i = 0; i < num_faces; i++) {
        get_line();
        uint16_t a,b,c;
        if (sscanf(line_buffer, "3 %hu %hu %hu\n", &a, &b, &c) == EOF) load_error("Malformed face entry. Faces must be triangles.");
        triangles[3*i + 0] = a;
        triangles[3*i + 1] = b;
        triangles[3*i + 2] = c;
    }
    Model model = {0};
    model.num_vertices = num_vertices;
    model.vertices = vertices;
    model.num_triangles = num_faces;
    model.triangles = triangles;
    return model;
}

Model make_surface_of_revolution(float *xs, float *ys, int num_points, int tessellation)
{
    // int model_num_points = num_points*tessellation + 2;
    int model_num_points = num_points*tessellation;
    vec3 *points = malloc(sizeof(vec3) * model_num_points);
    mem_check(points);
    int model_num_triangles = ((num_points - 1)*2) * tessellation;
    uint16_t *triangles = malloc(sizeof(uint16_t) * 3*model_num_triangles);
    mem_check(triangles);
    for (int j = 0; j < tessellation; j++) {
        float theta = 2*M_PI * j * 1.0 / tessellation;
        float sin_theta = sin(theta);
        float cos_theta = cos(theta);
        for (int i = 0; i < num_points; i++) {
            points[i*tessellation + j] = new_vec3(xs[i] * cos_theta, ys[i], xs[i] * sin_theta);
            if (i < num_points-1) {
                triangles[3*(2*(i*tessellation + j))] = i*tessellation + j;
                triangles[3*(2*(i*tessellation + j))+1] = (i+1)*tessellation + j;
                triangles[3*(2*(i*tessellation + j))+2] = (i+1)*tessellation + (j+1)%tessellation;
                triangles[3*(2*(i*tessellation + j)+1)] = i*tessellation + j;
                triangles[3*(2*(i*tessellation + j)+1)+1] = (i+1)*tessellation + (j+1)%tessellation;
                triangles[3*(2*(i*tessellation + j)+1)+2] = i*tessellation + (j+1)%tessellation;
            }
        }
    }
    // points[num_points*tessellation] = new_vec3(0, ys[0], 0);
    // points[num_points*tessellation+1] = new_vec3(0, ys[num_points-1], 0);
    // for (int i = 0; i < tessellation; i++) {
    //     triangles[3*((num_points - 1)*2) * tessellation + 0] = num_points*tessellation;
    //     triangles[3*((num_points - 1)*2) * tessellation + 1] = i;
    //     triangles[3*((num_points - 1)*2) * tessellation + 2] = (i+1)%tessellation;

    //     triangles[3*((num_points - 1)*2+1) * tessellation + 0] = num_points*tessellation;
    //     triangles[3*((num_points - 1)*2+1) * tessellation + 1] = (num_points-1)*tessellation + i;
    //     triangles[3*((num_points - 1)*2+1) * tessellation + 2] = (num_points-1)*tessellation + (i+1)%tessellation;
    // }
    Model model = {0};
    model.num_vertices = model_num_points;
    model.num_triangles = model_num_triangles;
    model.vertices = points;
    model.triangles = triangles;
    return model;
}

void model_compute_normals(Model *model)
{
    // Use this if vertex-normals instead of face-normals are wanted. This gives smoother shading but needs neighbourhood information, so
    // precompute them here.
    model->normals = calloc(1, sizeof(vec3) * model->num_vertices);
    mem_check(model->normals);
    // Compute each vertex normal as the average of the normals of incident faces.
    for (int i = 0; i < model->num_triangles; i++) {
        vec3 a = model->vertices[model->triangles[3*i + 0]];
        vec3 b = model->vertices[model->triangles[3*i + 1]];
        vec3 c = model->vertices[model->triangles[3*i + 2]];
        vec3 n = vec3_normalize(vec3_cross(vec3_sub(b, a), vec3_sub(c, a)));
        for (int j = 0; j < 3; j++) {
            model->normals[model->triangles[3*i+j]] = vec3_add(model->normals[model->triangles[3*i+j]], n);
        }
    }
    for (int i = 0; i < model->num_vertices; i++) {
        model->normals[i] = vec3_normalize(model->normals[i]);
    }
    model->has_normals = true;
}

Model make_capsule(float radius, float height)
{
    if (height < 2*radius) height = 2*radius; // Make sure the capsule is at least a sphere.
    float xs[16];
    float ys[16];
    for (int i = 0; i < 8; i++) {
        float theta = i * 0.5*M_PI / 8.0;
        xs[i] = radius * sin(theta);
        xs[16-i-1] = xs[i];
        ys[i] = -(height - 2*radius)/2.0 - radius*cos(theta);
        ys[16-i-1] = -ys[i];
    }
    return make_surface_of_revolution(xs, ys, 16, 8);
}
Model make_half_capsule(float radius, float height)
{
    if (height < radius) height = radius; // Make sure the half-capsule is at least a hemisphere.
    float xs[10];
    float ys[10];
    xs[0] = 0;
    ys[0] = -(height - radius)/2.0;
    xs[1] = radius;
    ys[1] = -(height - radius)/2.0;
    for (int i = 0; i < 8; i++) {
        float theta = i * 0.5*M_PI / 8.0;
        xs[10-i-1] = radius * sin(theta);
        ys[10-i-1] = (height - radius)/2.0 + radius*cos(theta);
    }
    return make_surface_of_revolution(xs, ys, 9, 8);
}
Model make_cylinder(float radius, float height)
{
    float xs[2];
    xs[0] = radius;
    xs[1] = radius;
    float ys[2];
    ys[0] = -height/2.0;
    ys[1] = height/2.0;
    return make_surface_of_revolution(xs, ys, 2, 8);
}


// Create a block with more vertices and faces than is needed for the geometry. This
// allows better OpenGL-2 vertex-lighting-based shading.
// Vertices are duplicated at the edges. This makes the creation of the model easier, and also facilitates
// different texture mappings on faces.
static Model ___make_tessellated_block(float w, float h, float d, int tw, int th, int td, bool compute_uvs, float texture_size)
{
    if (tw < 2) tw = 2;
    if (th < 2) th = 2;
    if (td < 2) td = 2;

    int num_vertices = 2*(tw*th + tw*td + th*td);
    vec3 *vertices = malloc(sizeof(vec3) * num_vertices);
    mem_check(vertices);
    int num_triangles = 2*2*((tw-1)*(th-1) + (tw-1)*(td-1) + (th-1)*(td-1));
    uint16_t *triangles = malloc(sizeof(uint16_t) * 3 * num_triangles);
    
    float *uvs;
    if (compute_uvs) {
        uvs = malloc(sizeof(float) * 2 * num_vertices);
        mem_check(uvs);
    }
    struct {
        int x_index;
        int y_index;
        int other_index;
    } modes[3] = {
        { 1, 0, 2 },
        { 2, 1, 0 },
        { 2, 0, 1 },
    };
    int tessellations[3] = { tw, th, td };
    float extents[3] = { w, h, d };
    int vertex = 0;
    int triangle = 0;
    for (int N = 0; N < 3; N++) {
        int tx = tessellations[modes[N].x_index];
        int ty = tessellations[modes[N].y_index];
        float x = extents[modes[N].x_index];
        float y = extents[modes[N].y_index];
        float z = extents[modes[N].other_index];
        for (int side = -1; side <= 1; side += 2) {
            for (int i = 0; i < tx; i++) {
                for (int j = 0; j < ty; j++) {
                    if (i < tx - 1 && j < ty - 1) {
                        triangles[3*triangle + 0] = vertex;
                        triangles[3*triangle + 1] = vertex + ty;
                        triangles[3*triangle + 2] = vertex + ty + 1;
                        triangle ++;
                        triangles[3*triangle + 0] = vertex;
                        triangles[3*triangle + 1] = vertex + ty + 1;
                        triangles[3*triangle + 2] = vertex + 1;
                        triangle ++;
                    }
                    float vx = x*i*1.0/(tx-1) - x/2.0;
                    float vy = y*j*1.0/(ty-1) - y/2.0;
                    vertices[vertex].vals[modes[N].x_index] = vx;
                    vertices[vertex].vals[modes[N].y_index] = vy;
                    vertices[vertex].vals[modes[N].other_index] = side * z/2.0;
                    if (compute_uvs) {
                        uvs[2*vertex + 0] = vx / texture_size;
                        uvs[2*vertex + 1] = vy / texture_size;
                    }
                    vertex ++;
                }
            }
        }
    }
    Model model = {0};
    model.num_vertices = num_vertices;
    model.num_triangles = num_triangles;
    model.vertices = vertices;
    model.triangles = triangles;
    if (compute_uvs) {
        model.has_uvs = true;
        model.uvs = uvs;
    }
    return model;
}
Model make_tessellated_block(float w, float h, float d, int tw, int th, int td)
{
    return ___make_tessellated_block(w,h,d,  tw,th,td,  false,0);
}
// Generate texture coordinates for the tessellated block. The coordinates are for a tiling of a texture_size x texture_size texture on each face.
Model make_tessellated_block_with_uvs(float w, float h, float d, int tw, int th, int td, float texture_size)
{
    return ___make_tessellated_block(w,h,d,  tw,th,td,  true,texture_size);
}

static void icosahedron_points(vec3 points[], float radius) // points must be of length 12
{
    float theta0 = 2.0*M_PI / 5.0;
    float theta1 = 4.0*M_PI / 5.0;
    float x = sqrt(2 - 2*cos(theta0));
    float r = 1.0 / x;
    
    float y = r * sqrt(1 - 2*cos(theta0));
    float z = 2*y - sqrt(1 - (sin(theta0) - sin(theta1))*(sin(theta0) - sin(theta1)) - (sin(theta0) - sin(theta1))*(sin(theta0) - sin(theta1)));

    points[0] = new_vec3(0,0,-1.5*y);//------this is not correct. Might as well hardcode all of these points.
    points[1] = new_vec3(0,0,z+1.5*y);//------
    for (int i = 0; i < 5; i++) {
        float theta_a = 2.0*M_PI*i/5.0;
        float theta_b = 2.0*M_PI*(i+0.5)/5.0;
        points[2+2*i] = new_vec3(r*cos(theta_a), r*sin(theta_a), y);
        points[2+2*i+1] = new_vec3(r*cos(theta_b), r*sin(theta_b), z - y);
    }
    for (int i = 0; i < 12; i++) points[i] = vec3_mul(vec3_sub(points[i], new_vec3(0,0,z/2)), radius);
}
Model make_icosahedron(float radius)
{
    vec3 points[12];
    icosahedron_points(points, radius);
    return polyhedron_to_model(convex_hull(points, 12));
}
Model make_dodecahedron(float radius)
{
    vec3 ico_points[12];
    icosahedron_points(ico_points, radius);
    Polyhedron ico_poly = convex_hull(ico_points, 12);
    vec3 points[20];
    PolyhedronTriangle *tri = ico_poly.triangles.first;
    int i = 0;
    while (tri != NULL) {
        points[i] = vec3_mul(vec3_add(vec3_add(tri->points[0]->position, tri->points[1]->position), tri->points[2]->position), 1.0/3.0);
        tri = tri->next;
        i++;
    }
    //-------destroy the polyhedra!
    return polyhedron_to_model(convex_hull(points, 20));
}
Model make_tetrahedron(float size)
{
    vec3 points[4];
    for (int i = 0; i < 3; i++) points[i] = new_vec3(cos(2*M_PI*i/3.0), sin(2*M_PI*i/3.0), 0);
    vec3 centroid = vec3_mul(vec3_add(points[0], vec3_add(points[1], points[2])), 1.0/3.0);
    float b = sqrt(vec3_dot(vec3_sub(centroid, points[0]), vec3_sub(centroid, points[0])));
    float x = sqrt(vec3_dot(vec3_sub(points[1], points[0]), vec3_sub(points[1], points[0])));
    float y = sqrt(3)*0.5*x;
    float a = y - b;
    float h = sqrt(x*x - a*a);
    points[3] = new_vec3(0,0,h);
    for (int i = 0; i < 4; i++) points[i] = vec3_mul(points[i], size);
    return polyhedron_to_model(convex_hull(points, 4));
}
Model make_octahedron(float size)
{
    float x = size/2;
    vec3 points[6];
    points[0] = new_vec3(-x,x,0);
    points[1] = new_vec3(-x,-x,0);
    points[2] = new_vec3(x,-x,0);
    points[3] = new_vec3(x,x,0);
    float h = 2*x / sqrt(2);
    points[4] = new_vec3(0,0,h);
    points[5] = new_vec3(0,0,-h);
    return polyhedron_to_model(convex_hull(points, 6));
}

// Polyhedron structures are more useful for interacting with the geometry. If the geometry is completed and it is wanted to
// be rendered, a model may be more useful.
Model polyhedron_to_model(Polyhedron polyhedron)
{
    Model model = {0};
    model.vertices = polyhedron_points(polyhedron);
    PolyhedronPoint *p = polyhedron.points.first;
    int pi = 0;
    while (p != NULL) {
        p->print_mark = pi;
        pi ++;
        p = p->next;
    }
    model.num_vertices = pi;
    model.num_triangles = polyhedron_num_triangles(&polyhedron);
    model.triangles = malloc(sizeof(uint16_t) * 3 * model.num_triangles);
    mem_check(model.triangles);
    PolyhedronTriangle *t = polyhedron.triangles.first;
    int ti = 0;
    while (t != NULL) {
        for (int i = 0; i < 3; i++) model.triangles[3*ti + i] = t->points[i]->print_mark;
        ti ++;
        t = t->next;
    }
    model.num_triangles = ti;
    return model;
}

bool ray_model_intersection(vec3 origin, vec3 direction, Model *model, mat4x4 model_matrix, vec3 *intersection)
{
    //---could rather transform the ray into model-space.
    for (int i = 0; i < model->num_triangles; i++) {
        vec3 a = rigid_matrix_vec3(model_matrix, model->vertices[model->triangles[3*i+0]]);
        vec3 b = rigid_matrix_vec3(model_matrix, model->vertices[model->triangles[3*i+1]]);
        vec3 c = rigid_matrix_vec3(model_matrix, model->vertices[model->triangles[3*i+2]]);
        if (ray_triangle_intersection(origin, direction, a, b, c, intersection)) return true;
    }
    return false;
}

// This is an orthogonal projection only when right and up are unit-length and orthogonal.
void compute_uvs_orthogonal(Model *model, vec3 right, vec3 up, float x_size, float y_size)
{
    float *uvs = malloc(sizeof(float) * 2*model->num_vertices);
    mem_check(uvs);

    float x_size_inv = 1.0 / x_size;
    float y_size_inv = 1.0 / y_size;

    for (int i = 0; i < model->num_vertices; i++) {
        vec3 v = model->vertices[i];
        uvs[2*i] = x_size_inv * vec3_dot(v, right);
        uvs[2*i+1] = y_size_inv * vec3_dot(v, up);
    }

    model->uvs = uvs;
    model->has_uvs = true;
}

void compute_uvs_cylindrical(Model *model, float x_size, float y_size)
{
    float *uvs = malloc(sizeof(float) * 2*model->num_vertices);
    mem_check(uvs);

    float x_size_inv = 1.0 / x_size;
    float y_size_inv = 1.0 / y_size;
    
    for (int i = 0; i < model->num_vertices; i++) {
        vec3 v = model->vertices[i];
        float d = sqrt(X(v)*X(v) + Z(v)*Z(v));
        uvs[2*i] = d <= 0.001 ? 0 : x_size_inv * acos(X(v) / d);
        uvs[2*i+1] = y_size_inv * Y(v);
    }

    model->uvs = uvs;
    model->has_uvs = true;
}
