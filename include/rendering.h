#ifndef RENDERING_H
#define RENDERING_H
#include "museum.h"

void deactivate_sun(void);
void activate_sun(void);

typedef struct PolyhedronRenderer_s {
    Polyhedron polyhedron;
    vec4 color;
} PolyhedronRenderer;
PolyhedronRenderer *add_polyhedron_renderer(Entity *e, Polyhedron polyhedron, vec4 color);
void polyhedron_renderer_update(Entity *e, Behaviour *b);

void draw_cubic_bezier_curve(vec3 a, vec3 b, vec3 c, vec3 d, int tessellation, vec4 color, float line_width);
typedef struct CatmullRomSplineRenderer_s {
    int num_points;
    vec3 *points;
    bool controlled;
    Entity **controls;
} CatmullRomSplineRenderer;
CatmullRomSplineRenderer *add_catmull_rom_spline_renderer(Entity *e, vec3 *points, int num_points, bool controlled);
void catmull_rom_spline_renderer_update(Entity *e, Behaviour *b);

enum TessellationDomains {
    Rectangular,
    Triangular,
};
typedef uint8_t TessellationDomain;
// vec3 tessellation_shader(vec3 *patch, float u, float v, float w, vec3 *out_normal, vec2 *out_texcoord)
typedef vec3 (*TessellationShader)(vec3 *, float, float, float, vec3 *, vec2 *);

// Model with a mesh, vertex attributes (UV coordinates, normals), and optional associated texture.
typedef struct Model_s {
    int num_vertices;
    vec3 *vertices;

    // If not textured, this will be the color of the model.
    vec4 flat_color;

    // Optional texturing.
    bool textured;
    Texture texture;

    // Triangle model values.
    int num_triangles;
    uint16_t *triangles; // length is 3*num_triangles.

    // Stored vertex attributes.
    bool has_normals; // Otherwise these will be computed if needed.
    vec3 *normals;
    bool has_uvs;
    float *uvs; // length is 2*num_vertices.

    // Tessellation options.
    bool tessellated;
    int patch_num_vertices;
    TessellationDomain tessellation_domain;
    int tessellation_level;
    bool tessellate_normals;
    bool tessellate_uvs;
    TessellationShader tessellation_shader;
} Model;
void render_model(Model *model);
void render_wireframe_model(Model *model, float line_width);
void model_compute_normals(Model *model);

typedef struct ModelRenderer_s {
    bool wireframe;
    float wireframe_width;
    Model model;
} ModelRenderer;
ModelRenderer *add_model_renderer(Entity *e, Model model);
void model_renderer_update(Entity *e, Behaviour *b);


#define max_marching_cube_triangles 32
typedef struct MetaballRenderer_s {
    int num_points;
    vec3 *points;
    float *weights;
    float threshold;

    float box_size;

    // Just restrict isosurface tessellation to this box.
    vec3 box_min;
    vec3 box_max;

    bool render_grid;
    bool render_points;
} MetaballRenderer;
void metaball_renderer_update(Entity *e, Behaviour *b);
float evaluate_metaball_function(MetaballRenderer *mr, vec3 point);
MetaballRenderer *add_metaball_renderer(Entity *e, int num_points, vec3 *points, float *weights, float threshold, float box_size, bool copy_points);


#endif // RENDERING_H
