#include "museum.h"

typedef struct BezierSurfaceRenderer_s {
    int degree_n;
    int degree_m;
    vec3 *control_points;
    bool textured;
    Texture texture;
    vec4 flat_color;
} BezierSurfaceRenderer;

typedef struct PointsController_s {
    int num_points;
    vec3 *points;
    Entity **controls;
} PointsController;

typedef struct BicubicBezierPatch_s {
    vec3 points[16];
} BicubicBezierPatch;
typedef struct BicubicBezierModel_s {
    int num_patches;
    BicubicBezierPatch *patches;
} BicubicBezierModel;

BezierSurfaceRenderer *add_bezier_surface_renderer(Entity *e, int degree_n, int degree_m, vec3 *control_points, bool copy_points);
void bezier_surface_renderer_update(Entity *e, Behaviour *b);
vec3 evaluate_bezier_patch(int n, int m, vec3 *points, float u, float v);
vec3 evaluate_bezier_patch_de_casteljau(int n, int m, vec3 *points, float u, float v);
vec3 vec3_bilerp(vec3 a, vec3 b, vec3 ap, vec3 bp, float u, float v);
PointsController *add_points_controller(Entity *e, int num_points, vec3 *points, float control_widget_size);
void points_controller_update(Entity *e, Behaviour *b);
Model load_teapot(void);
vec3 bicubic_bezier_shader(vec3 *patch, float u, float v, float w, vec3 *out_normal, vec2 *out_texcoord);
vec3 bicubic_bezier_shader_uvs(vec3 *patch, float u, float v, float w, vec3 *out_normal, vec2 *out_texcoord);
vec3 cubic_bezier_triangle_shader(vec3 *patch, float u, float v, float w, vec3 *out_normal, vec2 *out_texcoord);
vec3 evaluate_bezier_triangle(int n, vec3 *points, float u, float v);

#define BINOMIAL_COEFFICIENT_TABLE_MAX_N 12
static const uint16_t binomial_coefficient[BINOMIAL_COEFFICIENT_TABLE_MAX_N + 1][BINOMIAL_COEFFICIENT_TABLE_MAX_N + 1] = {
    { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    { 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    { 1, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    { 1, 3, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    { 1, 4, 6, 4, 1, 0, 0, 0, 0, 0, 0, 0, 0},
    { 1, 5, 10, 10, 5, 1, 0, 0, 0, 0, 0, 0, 0},
    { 1, 6, 15, 20, 15, 6, 1, 0, 0, 0, 0, 0, 0},
    { 1, 7, 21, 35, 35, 21, 7, 1, 0, 0, 0, 0, 0},
    { 1, 8, 28, 56, 70, 56, 28, 8, 1, 0, 0, 0, 0},
    { 1, 9, 36, 84, 126, 126, 84, 36, 9, 1, 0, 0, 0},
    { 1, 10, 45, 120, 210, 252, 210, 120, 45, 10, 1, 0, 0},
    { 1, 11, 55, 165, 330, 462, 462, 330, 165, 55, 11, 1, 0},
    { 1, 12, 66, 220, 495, 792, 924, 792, 495, 220, 66, 12, 1},
};

#define TRINOMIAL_COEFFICIENT_TABLE_MAX_N 12
uint16_t trinomial_coefficient[TRINOMIAL_COEFFICIENT_TABLE_MAX_N + 1][TRINOMIAL_COEFFICIENT_TABLE_MAX_N + 1][TRINOMIAL_COEFFICIENT_TABLE_MAX_N + 1] = {
    {{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // n=0
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},

    {{1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // n=1
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},

    {{1, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // n=2
        {2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},

    {{1, 3, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // n=3
        {3, 6, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {3, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},

    {{1, 4, 6, 4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // n=4
        {4, 12, 12, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {6, 12, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {4, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},

    {{1, 5, 10, 10, 5, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // n=5
        {5, 20, 30, 20, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {10, 30, 30, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {10, 20, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {5, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},

    {{1, 6, 15, 20, 15, 6, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // n=6
        {6, 30, 60, 60, 30, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {15, 60, 90, 60, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {20, 60, 60, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {15, 30, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {6, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},

    {{1, 7, 21, 35, 35, 21, 7, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // n=7
        {7, 42, 105, 140, 105, 42, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {21, 105, 210, 210, 105, 21, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {35, 140, 210, 140, 35, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {35, 105, 105, 35, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {21, 42, 21, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {7, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},

    {{1, 8, 28, 56, 70, 56, 28, 8, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // n=8
        {8, 56, 168, 280, 280, 168, 56, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {28, 168, 420, 560, 420, 168, 28, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {56, 280, 560, 560, 280, 56, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {70, 280, 420, 280, 70, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {56, 168, 168, 56, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {28, 56, 28, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {8, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},

    {{1, 9, 36, 84, 126, 126, 84, 36, 9, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // n=9
        {9, 72, 252, 504, 630, 504, 252, 72, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {36, 252, 756, 1260, 1260, 756, 252, 36, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {84, 504, 1260, 1680, 1260, 504, 84, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {126, 630, 1260, 1260, 630, 126, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {126, 504, 756, 504, 126, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {84, 252, 252, 84, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {36, 72, 36, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},

    {{1, 10, 45, 120, 210, 252, 210, 120, 45, 10, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // n=10
        {10, 90, 360, 840, 1260, 1260, 840, 360, 90, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {45, 360, 1260, 2520, 3150, 2520, 1260, 360, 45, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {120, 840, 2520, 4200, 4200, 2520, 840, 120, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {210, 1260, 3150, 4200, 3150, 1260, 210, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {252, 1260, 2520, 2520, 1260, 252, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {210, 840, 1260, 840, 210, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {120, 360, 360, 120, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {45, 90, 45, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {10, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},

    {{1, 11, 55, 165, 330, 462, 462, 330, 165, 55, 11, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // n=11
        {11, 110, 495, 1320, 2310, 2772, 2310, 1320, 495, 110, 11, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {55, 495, 1980, 4620, 6930, 6930, 4620, 1980, 495, 55, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {165, 1320, 4620, 9240, 11550, 9240, 4620, 1320, 165, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {330, 2310, 6930, 11550, 11550, 6930, 2310, 330, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {462, 2772, 6930, 9240, 6930, 2772, 462, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {462, 2310, 4620, 4620, 2310, 462, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {330, 1320, 1980, 1320, 330, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {165, 495, 495, 165, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {55, 110, 55, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {11, 11, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},

    {{1, 12, 66, 220, 495, 792, 924, 792, 495, 220, 66, 12, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // n=12
        {12, 132, 660, 1980, 3960, 5544, 5544, 3960, 1980, 660, 132, 12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {66, 660, 2970, 7920, 13860, 16632, 13860, 7920, 2970, 660, 66, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {220, 1980, 7920, 18480, 27720, 27720, 18480, 7920, 1980, 220, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {495, 3960, 13860, 27720, 34650, 27720, 13860, 3960, 495, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {792, 5544, 16632, 27720, 27720, 16632, 5544, 792, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {924, 5544, 13860, 18480, 13860, 5544, 924, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {792, 3960, 7920, 7920, 3960, 792, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {495, 1980, 2970, 1980, 495, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {220, 660, 660, 220, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {66, 132, 66, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {12, 12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
};

vec3 bicubic_bezier_shader(vec3 *patch, float u, float v, float w, vec3 *out_normal, vec2 *out_texcoord)
{
    // Does not give normals or texture coordinates.
    return evaluate_bezier_patch(4, 4, patch, u, v);
}
vec3 bicubic_bezier_shader_uvs(vec3 *patch, float u, float v, float w, vec3 *out_normal, vec2 *out_texcoord)
{
    vec3 vertex = evaluate_bezier_patch(4, 4, patch, u, v);
    float d = X(vertex)*X(vertex) + Z(vertex)*Z(vertex);
    *out_texcoord = new_vec2(0.01 * acos(X(vertex) / (d < 0.01 ? 0.01 : sqrt(d))), 0.01 * Y(vertex));
    return vertex;
}
vec3 cubic_bezier_triangle_shader(vec3 *patch, float u, float v, float w, vec3 *out_normal, vec2 *out_texcoord)
{
    return evaluate_bezier_triangle(3, patch, u, v);
}

Model load_teapot(void)
{
    FILE *file = fopen("resources/utah_teapot.txt", "r");
    if (file == NULL) {
        fprintf(stderr, "ERROR: Cannot find Utah teapot.\n");
        exit(EXIT_FAILURE);
    }
    int array_length = 16;
    BicubicBezierPatch *patches = malloc(sizeof(BicubicBezierPatch) * array_length);
    mem_check(patches);

    int patch_index = 0;
    while (1) {
        vec3 patch[4*4];
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                if (fscanf(file, "%f %f %f", &X(patch[4*i+j]),
                                             &Y(patch[4*i+j]),
                                             &Z(patch[4*i+j])) == EOF) {
                    if (i == 0 && j == 0) goto done;
		    fprintf(stderr, "ERROR: Malformed Utah teapot.\n");
		    exit(EXIT_FAILURE);
                }
            }
        }
        if (patch_index == array_length) {
            array_length *= 2;
            patches = realloc(patches, sizeof(BicubicBezierPatch) * array_length);
        }
        memcpy(patches[patch_index].points, patch, sizeof(vec3)*4*4);
        patch_index ++;
    }
    Model model = {0};
done:
    model.vertices = (vec3 *) patches;
    model.tessellated = true;
    model.num_vertices = patch_index * 16;
    model.patch_num_vertices = 16;
    model.tessellation_level = 20;
    model.tessellation_domain = Rectangular;
    model.tessellation_shader = bicubic_bezier_shader;
    return model;
}

void points_controller_update(Entity *e, Behaviour *b)
{
    PointsController *pc = b->data;
    mat4x4 inv_matrix = rigid_mat4x4_inverse(entity_matrix(e));
    for (int i = 0; i < pc->num_points; i++) {
        pc->points[i] = rigid_matrix_vec3(inv_matrix, pc->controls[i]->position);
    }
}

PointsController *add_points_controller(Entity *e, int num_points, vec3 *points, float control_widget_size)
{
    PointsController *pc = add_behaviour(e, points_controller_update, sizeof(PointsController), NoID)->data;
    pc->points = malloc(sizeof(vec3) * num_points);
    pc->num_points = num_points;
    mem_check(pc->points);
    pc->controls = malloc(sizeof(Entity *) * num_points);
    mem_check(pc->controls);
    mat4x4 matrix = entity_matrix(e);
    for (int i = 0; i < num_points; i++) {
        pc->points[i] = points[i];
        vec3 p = rigid_matrix_vec3(matrix, points[i]);
        Entity *control = add_entity(p, vec3_zero());
        ControlWidget *widget = add_control_widget(control, control_widget_size);
        pc->controls[i] = control;
    }
    return pc;
}

// Interpolate a->b by u to get A, ap->bp by u to get B, then interpolate A->B by v.
vec3 vec3_bilerp(vec3 a, vec3 b, vec3 ap, vec3 bp, float u, float v)
{
    return vec3_lerp(vec3_lerp(a, b, u), vec3_lerp(ap, bp, u), v);
}



// The de Casteljau algorithm for Bezier patches evaluates a point by repeated bilinear interpolation, a direct computation
// from the definition. Scratch space is used, and the top-left of the grid is written over each time so only this scratch space is needed.
#define MAX_NUM_BEZIER_PATCH_POINTS 1024
vec3 evaluate_bezier_patch_de_casteljau(int n, int m, vec3 *points, float u, float v)
{
    static vec3 scratch[MAX_NUM_BEZIER_PATCH_POINTS];
    if (n * m > MAX_NUM_BEZIER_PATCH_POINTS) {
        fprintf(stderr, "ERROR: Too many points in Bezier patch.\n");
        exit(EXIT_FAILURE);
    }
    memcpy(scratch, points, sizeof(vec3)*n*m);
    
    int max_degree = n > m ? n : m;

    for (int k = max_degree-1; k > 0; --k) {
        for (int i = 0; i < k; i++) {
            for (int j = 0; j < k; j++) {
                scratch[j*n + i] = vec3_bilerp(scratch[j*n + i], scratch[j*n + i+1], scratch[(j+1)*n + i], scratch[(j+1)*n + i+1], u, v);
            }
        }
    }
    if (n < max_degree) {
        for (int k = max_degree - n; k > 0; --k) {
            for (int i = 0; i < k; i++) {
                scratch[i*n] = vec3_lerp(scratch[i*n], scratch[(i+1)*n], v);
            }
        }
    } else if (m < max_degree) {
        for (int k = max_degree - m; k > 0; --k) {
            for (int i = 0; i < k; i++) {
                scratch[i] = vec3_lerp(scratch[i], scratch[i+1], u);
            }
        }
    }
    return scratch[0];
}

// From the definition by repeated bilinear interpolation, instead of computing this directly, combinatorial terms may be tabulated.
// A Bezier patch can be expressed as
//     sum{i..n-1} sum{j..m-1} n_choose_i * m_choose_j * u^i * v^j * (1-u)^(n-i) * (1-v)^(m-j) * control_point_i,j.
// This could be improved on (? Use a script to generate expanded Bernstein polynomials and rearrange with Horner's method?),
// but it should be faster than the direct de Casteljau algorithm. (? Or maybe a restructuring of de Casteljau would be faster.)
#define MAX_BEZIER_PATCH_DEGREE 63
vec3 evaluate_bezier_patch(int n, int m, vec3 *points, float u, float v)
{
    //!-----n and m are the dimensions of the control grid, _not_ the degrees.
    // Tabulate the powers of u,v,1-u,1-v that are needed.
    float u_powers[MAX_BEZIER_PATCH_DEGREE + 1];
    float v_powers[MAX_BEZIER_PATCH_DEGREE + 1];
    float one_minus_u_powers[MAX_BEZIER_PATCH_DEGREE + 1];
    float one_minus_v_powers[MAX_BEZIER_PATCH_DEGREE + 1];
    float u_power = 1;
    float one_minus_u_power = 1;
    u_powers[0] = 1;
    one_minus_u_powers[0] = 1;
    for (int i = 1; i <= n; i++) {
        u_power *= u;
        one_minus_u_power *= 1 - u;
        u_powers[i] = u_power;
        one_minus_u_powers[i] = one_minus_u_power;
    }
    float v_power = 1;
    float one_minus_v_power = 1;
    v_powers[0] = 1;
    one_minus_v_powers[0] = 1;
    for (int i = 1; i <= m; i++) {
        v_power *= v;
        one_minus_v_power *= 1 - v;
        v_powers[i] = v_power;
        one_minus_v_powers[i] = one_minus_v_power;
    }
    // Compute the point.
    vec3 total = vec3_zero();
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) {
            uint16_t bi = binomial_coefficient[n-1][i];
            uint16_t bj = binomial_coefficient[m-1][j];
            float coefficient = bi*bj*u_powers[i]*v_powers[j]*one_minus_u_powers[n-i-1]*one_minus_v_powers[m-j-1];
            X(total) += X(points[j*n + i]) * coefficient;
            Y(total) += Y(points[j*n + i]) * coefficient;
            Z(total) += Z(points[j*n + i]) * coefficient;
        }
    }
    return total;
}

#define MAX_BEZIER_TRIANGLE_DEGREE 63
vec3 evaluate_bezier_triangle(int n, vec3 *points, float u, float v)
{
    // n'th degree Bezier triangle, with (n+1)(n+2)/2 points.
    float w = 1 - u - v;
    float u_powers[MAX_BEZIER_TRIANGLE_DEGREE + 1];
    float v_powers[MAX_BEZIER_TRIANGLE_DEGREE + 1];
    float w_powers[MAX_BEZIER_TRIANGLE_DEGREE + 1];
    u_powers[0] = 1.0;
    v_powers[0] = 1.0;
    w_powers[0] = 1.0;
    float u_power = 1.0;
    float v_power = 1.0;
    float w_power = 1.0;
    for (int i = 1; i <= n; i++) {
        u_power *= u;
        v_power *= v;
        w_power *= w;
        u_powers[i] = u_power;
        v_powers[i] = v_power;
        w_powers[i] = w_power;
    }
    vec3 total = vec3_zero();
    for (int i = 0; i <= n; i++) {
        for (int j = 0; j <= n-i; j++) {
            int k = n-i-j;
            uint16_t trinomial_term = trinomial_coefficient[n][i][j];
            float coefficient = trinomial_term*u_powers[i]*v_powers[j]*w_powers[k];
            int index = ((n-j+1)*(n-j))/2 + i;
            total = vec3_add(total, vec3_mul(points[index], coefficient));
        }
    }
    return total;
}


void bezier_surface_renderer_update(Entity *e, Behaviour *b)
{
    BezierSurfaceRenderer *bs = b->data;
    vec3 *points = bs->control_points;
    int n = bs->degree_n;
    int m = bs->degree_m;

    int tess_u = 20;
    int tess_v = 20;
    float tess_u_inv = 1.0 / tess_u;
    float tess_v_inv = 1.0 / tess_v;

    prepare_entity_matrix(e);
    activate_sun();
    #define DEBUG 0
    if (bs->textured && !DEBUG) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, bs->texture.texture_id);
    } else {
        glColor3f(X(bs->flat_color), Y(bs->flat_color), Z(bs->flat_color));
    }
    glBegin(GL_TRIANGLES);
    for (int ui = 0; ui < tess_u; ui++) {
        for (int vi = 0; vi < tess_v; vi++) {
            float u1 = ui * tess_u_inv;
            float u2 = (ui+1) * tess_u_inv;
            float v1 = vi * tess_v_inv;
            float v2 = (vi+1) * tess_v_inv;

            // printf("%d, %.2f,%.2f   ;   %d, %.2f,%.2f\n", ui, u1,u2, vi, v1,v2);

            vec3 bl = evaluate_bezier_patch(n, m, points, u1, v1); //
            vec3 tl = evaluate_bezier_patch(n, m, points, u1, v2); //
            vec3 br = evaluate_bezier_patch(n, m, points, u2, v1); //
            vec3 tr = evaluate_bezier_patch(n, m, points, u2, v2); //

            float tri1[] = { 0.05 + 0.95 * u1,v1, 0.05 + 0.95 * u2,v1, 0.05 + 0.95 * u2,v2 }; //----hack to get specific texture to work.
            float tri2[] = { 0.05 + 0.95 * u1,v1, 0.05 + 0.95 * u2,v2, 0.05 + 0.95 * u1,v2 };
            // printf("tri1: %.2f %.2f ; %.2f %.2f ; %.2f %.2f\n", tri1[0], tri1[1], tri1[2], tri1[3], tri1[4], tri1[5]);
            // printf("tri2: %.2f %.2f ; %.2f %.2f ; %.2f %.2f\n", tri2[0], tri2[1], tri2[2], tri2[3], tri2[4], tri2[5]);

            if (DEBUG) glColor3f(v1,u1,0);
            if (!DEBUG && bs->textured) glTexCoord2f(tri1[0],tri1[1]);
            glVertex3fv(bl.vals);
            if (!DEBUG && bs->textured) glTexCoord2f(tri1[2],tri1[3]);
            glVertex3fv(br.vals);
            if (!DEBUG && bs->textured) glTexCoord2f(tri1[4],tri1[5]);
            glVertex3fv(tr.vals);
            if (!DEBUG && bs->textured) glTexCoord2f(tri2[0],tri2[1]);
            if (DEBUG) glColor3f(0,0,0);
            glVertex3fv(bl.vals);
            if (!DEBUG && bs->textured) glTexCoord2f(tri2[2],tri2[3]);
            glVertex3fv(tr.vals);
            if (!DEBUG && bs->textured) glTexCoord2f(tri2[4],tri2[5]);
            glVertex3fv(tl.vals);
        }
    }
    glEnd();
}
BezierSurfaceRenderer *add_bezier_surface_renderer(Entity *e, int degree_n, int degree_m, vec3 *control_points, bool copy_points)
{
    BezierSurfaceRenderer *bs = add_behaviour(e, bezier_surface_renderer_update, sizeof(BezierSurfaceRenderer), BezierSurfaceRendererID)->data;
    bs->degree_n = degree_n;
    bs->degree_m = degree_m;
    if (copy_points) {
        size_t size = sizeof(vec3) * degree_n*degree_m;
        bs->control_points = malloc(size);
        mem_check(bs->control_points);
        memcpy(bs->control_points, control_points, size);
    } else {
        // For example, this can allow a PointsController to control the surface.
        bs->control_points = control_points;
    }
    return bs;
}

static void surface2_update(Entity *e, Behaviour *b)
{
    BezierSurfaceRenderer *bs = get_behaviour(e, BezierSurfaceRendererID)->data;
    for (int i = 0; i < bs->degree_n*bs->degree_m; i++) {
        vec3 p = bs->control_points[i];
        Z(bs->control_points[i]) += (Y(p) - 15)*(1.0/15.0)*1.2*sin(X(p)+Y(p)+vec3_dot(p,p)*0.2 + total_time*1.3) * dt;
    }
}

typedef struct ExhibitMetaball_s {
    MetaballRenderer *mr;
    PointsController *controller;
    float *movement_variables;
    float *movement_speeds;
} ExhibitMetaball;
static void exhibit_metaball_update(Entity *e, Behaviour *b)
{
    ExhibitMetaball *mb = b->data;
    MetaballRenderer *mr = mb->mr;
    PointsController *c = mb->controller;
    for (int i = 0; i < c->num_points; i++) {
        c->controls[i]->position = vec3_add(c->controls[i]->position, new_vec3(mr->weights[i]*sin(total_time+mb->movement_variables[i])*dt*mb->movement_speeds[i], mr->weights[i]*cos(mb->movement_variables[i]+total_time)*dt*mb->movement_speeds[i], 0));
    }

    vec3 cube[] = {
        new_vec3(mr->box_min.vals[0], mr->box_min.vals[1], mr->box_min.vals[2]),
        new_vec3(mr->box_min.vals[0], mr->box_min.vals[1], mr->box_max.vals[2]),
        new_vec3(mr->box_min.vals[0], mr->box_max.vals[1], mr->box_max.vals[2]),
        new_vec3(mr->box_min.vals[0], mr->box_max.vals[1], mr->box_min.vals[2]),
        new_vec3(mr->box_max.vals[0], mr->box_min.vals[1], mr->box_min.vals[2]),
        new_vec3(mr->box_max.vals[0], mr->box_min.vals[1], mr->box_max.vals[2]),
        new_vec3(mr->box_max.vals[0], mr->box_max.vals[1], mr->box_max.vals[2]),
        new_vec3(mr->box_max.vals[0], mr->box_max.vals[1], mr->box_min.vals[2]),
    };

    glLineWidth(3);
    glColor3f(0,0.3,0.7);
    deactivate_sun();
    glBegin(GL_LINES);
        for (int i = 0; i < 4; i++) {
            glVertex3f(UNPACK_VEC3(cube[i]));
            glVertex3f(UNPACK_VEC3(cube[(i+1)%4]));
            glVertex3f(UNPACK_VEC3(cube[4 + i]));
            glVertex3f(UNPACK_VEC3(cube[4 + (i+1)%4]));
            glVertex3f(UNPACK_VEC3(cube[i]));
            glVertex3f(UNPACK_VEC3(cube[4 + i]));
        }
    glEnd();
}

static void make_metaballs(void)
{
    // vec3 points[10];
    // float strengths[10];
    // for (int i = 0; i < 10; i++) {
    //     points[i] = rand_vec3(2);
    //     strengths[i] = frand()*0.05;
    // }
    // MetaballRenderer *mr = add_metaball_renderer(e, 10, points, strengths, 0.5, 0.2, true);

    Entity *e = add_entity(new_vec3(-13,1.3,-50), new_vec3(0,-0.3,0));
    int n = 5;
    vec3 *points = malloc(sizeof(vec3) * n);
    mem_check(points);
    float *weights = malloc(sizeof(float) * n);
    mem_check(weights);
    for (int i = 0; i < n; i++) {
        points[i] = rand_vec3(5);
        weights[i] = frand()*0.5+0.4;
    }
    PointsController *pc = add_points_controller(e, n, points, 0.6);
    MetaballRenderer *mr = add_metaball_renderer(e, n, pc->points, weights, 0.5, 0.4, false);
    ExhibitMetaball *emb = add_behaviour(e, exhibit_metaball_update, sizeof(ExhibitMetaball), NoID)->data;
    emb->movement_speeds = malloc(sizeof(float) * n);
    mem_check(emb->movement_speeds);
    emb->movement_variables = malloc(sizeof(float) * n);
    mem_check(emb->movement_variables);
    for (int i = 0; i < n; i++) {
        emb->movement_speeds[i] = 1+frand();
        if (frand() > 0.5) emb->movement_speeds[i] = -emb->movement_speeds[i];
        emb->movement_variables[i] = 2*M_PI*frand();
    }

    emb->mr = mr;
    emb->controller = pc;
    // mr->render_grid = true;
}


typedef struct Teapot_s {
    ModelRenderer *mr;
    float timer;
    bool up;
} Teapot;
void teapot_update(Entity *e, Behaviour *b)
{
    Teapot *tp = b->data;
    Y(e->euler_angles) += dt;
    tp->timer -= dt;
    if (tp->timer <= 0) {
        if (tp->up) {
            tp->mr->model.tessellation_level ++;
            if (tp->mr->model.tessellation_level > 20) {
                tp->up = false;
            }
        } else {
            tp->mr->model.tessellation_level --;
            if (tp->mr->model.tessellation_level < 1) {
                tp->up = true;
            }
        }
        tp->timer = 0.15;
    }
}

void create_exhibit_curves_and_surfaces(void)
{
    #define SURFACE(X,Y,Z)\
        Entity *surface = add_entity(new_vec3((X), (Y), (Z)), new_vec3(0,0,0));\
        vec3 points[9];\
        for (int i = 0; i < 3; i++) {\
            for (int j = 0; j < 3; j++) {\
                points[3*i + j] = new_vec3(i,j,0);\
            }\
        }

#if 1
    {
        SURFACE(0,0,-50) PointsController *pc = add_points_controller(surface, 9, points, 0.5);
        BezierSurfaceRenderer *bezier = add_bezier_surface_renderer(surface, 3, 3, pc->points, false);
        bezier->texture = load_texture("resources/ice.bmp");
        bezier->textured = true;
    }
#endif
    {
        Entity *triangle = add_entity(new_vec3(6,1.5,-50), new_vec3(0,0.2,0));
        vec3 points[10];
        vec3 a = new_vec3(-sqrt(2)/2,-sqrt(2)/2,0);
        vec3 b = new_vec3(sqrt(2)/2,-sqrt(2)/2,0);
        vec3 c = new_vec3(0,1,0);
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j <= 3-i; j++) {
                int k = 3-i-j;
                int index = ((3-j+1)*(3-j))/2 + i;
                points[index] = vec3_mul(barycentric_triangle(a,b,c, i,j,k), 2.3);
            }
        }
        PointsController *pc = add_points_controller(triangle, 10, points, 0.5);
        Model model = {0};
        model.vertices = pc->points;
        model.num_vertices = 10;
        model.tessellated = true;
        model.patch_num_vertices = 10;
        model.tessellation_domain = Triangular;
        model.flat_color = GREEN;
        model.tessellation_shader = cubic_bezier_triangle_shader;
        model.tessellation_level = 20;
        add_model_renderer(triangle, model);
    }

#if 1
    for (int N = 0; N < 2; N++) {
        float x_along = N == 0 ? 3 : -10;
        Entity *holder = add_entity(new_vec3(x_along + 3, 19.2, -26.3), new_vec3(-0.6,0,0));
        Model holder_model = make_tessellated_block_with_uvs(7,1,2.4, 3,2,2, 10);
        model_compute_normals(&holder_model);
        holder_model.textured = true;
        holder_model.texture = load_texture("resources/rock2.bmp");
        add_model_renderer(holder, holder_model);

        Entity *surface = add_entity(new_vec3(x_along, 0, -26), new_vec3(0,M_PI,M_PI/2));
	#define DEGREE 4
        vec3 points[DEGREE*DEGREE];
        for (int i = 0; i < DEGREE; i++) {
            for (int j = 0; j < DEGREE; j++) {
                points[DEGREE*i + j] = new_vec3(7*i,2*j,0);
            }
        }
        BezierSurfaceRenderer *bezier = add_bezier_surface_renderer(surface, DEGREE, DEGREE, points, true);
        bezier->texture = load_texture("resources/banner_square.bmp");
        bezier->textured = true;
        add_behaviour(surface, surface2_update, 0, NoID);
    }
#endif

    Entity *teapot = add_entity(new_vec3(-22,-0.9,-32), vec3_zero());
    teapot->euler_controlled = true;
    Model teapot_model = load_teapot();
     
    teapot_model.flat_color = BLUE;
    //------Hack to fix the seams.
    // for (int i = 0; i < teapot_model.num_vertices / teapot_model.patch_num_vertices; i++) {
    //     vec3 center = new_vec3(0,0,0);
    //     for (int j = i*teapot_model.patch_num_vertices; j < (i+1)*teapot_model.patch_num_vertices; j++) {
    //         center = vec3_add(center, vec3_mul(teapot_model.vertices[j], 1.0 / teapot_model.patch_num_vertices));
    //     }
    //     for (int j = i*teapot_model.patch_num_vertices; j < (i+1)*teapot_model.patch_num_vertices; j++) {
    //         teapot_model.vertices[j] = vec3_add(center, vec3_mul(vec3_sub(teapot_model.vertices[j], center), 1.1));
    //     }
    // }
    teapot_model.textured = false;
    teapot_model.tessellate_normals = false;
    //teapot_model.texture = load_texture("resources/ice.bmp");
    teapot_model.tessellation_shader = bicubic_bezier_shader;
    ModelRenderer *renderer = add_model_renderer(teapot, teapot_model);
    Teapot *tp = add_behaviour(teapot, teapot_update, sizeof(Teapot), NoID)->data;
    tp->mr = renderer;

    make_metaballs();

    // Entity *spline_e = add_entity(new_vec3(0, 0, -50), new_vec3(0,0,0));
    // vec3 points[5];
    // for (int i = 0; i < 5; i++) {
    //     float r = 3;
    //     points[i] = new_vec3(frand()*r-r/2,frand()*r-r/2,frand()*r-r/2);
    // }
    // add_catmull_rom_spline_renderer(spline_e, points, 5, true);

    // add_control_widget(spline_e, 0.2);
    // Behaviour *spline_b = add_behaviour(spline_e, catmull_rom_spline_renderer_update, sizeof(CatmullRomSplineRenderer), NoID);
    // CatmullRomSplineRenderer *spline = (CatmullRomSplineRenderer *) spline_b->data;
}
