#ifndef GEOMETRY_H
#define GEOMETRY_H
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "helper_definitions.h"
#include "doubly_linked_list.h"
#include "mathematics.h"

/*================================================================================
    Data structure for polyhedra, with adjacency information.
    This is not designed for compactness or direct efficiency, but rather originally as a data
    structure for the iterative convex hull algorithm.  Vertices do not contain adjacency information. Marks are
    available space for algorithms to mark triangles as traversed, etc.
================================================================================*/
// Polyhedron structures. The implementation is partly based on O'Rourke, Computational Geometry in C.
struct Polyhedra_s;
struct PolyhedraPoint_s;
struct PolyhedraEdge_s;
struct PolyhedraTriangle_s;
// Polyhedron feature structs (points [vertices], edges, and triangles [faces]).
typedef struct PolyhedronEdge_s {
    struct PolyhedronEdge_s *prev;
    struct PolyhedronEdge_s *next;
    struct PolyhedronPoint_s *a;
    struct PolyhedronPoint_s *b;
    struct PolyhedronTriangle_s *triangles[2];
    int mark;
    int print_mark; //mark for use by printing/visualization functions, so they don't interfere with the use of the other marks.
} PolyhedronEdge;
typedef struct PolyhedronPoint_s {
    struct PolyhedronPoint_s *prev;
    struct PolyhedronPoint_s *next;
    vec3 position;
    int mark;
    int print_mark;
    void *saved_edge; // this edge is used in the convex hull algorithm, to avoid duplicate vertices when adding the cone.
} PolyhedronPoint;
typedef struct PolyhedronTriangle_s {
    struct PolyhedronTriangle_s *prev;
    struct PolyhedronTriangle_s *next;
    struct PolyhedronPoint_s *points[3];
    struct PolyhedronEdge_s *edges[3];
    int mark;
    int print_mark;
} PolyhedronTriangle;
// The Polyhedron struct itself is just the three doubly linked lists of features.
typedef struct Polyhedron_s {
    int num_points;
    int num_edges;
    int num_triangles; // these numbers are cached until add/remove functions are used.
    // All features _must_ only be added or removed via the add/remove functions.
    struct {
        PolyhedronPoint *first;
        PolyhedronPoint *last;
    } points;
    struct {
        PolyhedronEdge *first;
        PolyhedronEdge *last;
    } edges;
    struct {
        PolyhedronTriangle *first;
        PolyhedronTriangle *last;
    } triangles;
} Polyhedron;

// Create a new empty polyhedron.
Polyhedron new_polyhedron(void);

// Add features to the polyhedron. It is up to the user to maintain that polyhedra are only ever "incomplete", as in, they can be disconnected and have holes,
// supposedly as intermediary steps in geometric processing, but this data structure does not allow being "overcomplete", as in, having more than two triangles incident to one edge.
PolyhedronPoint *polyhedron_add_point(Polyhedron *polyhedron, vec3 point);
PolyhedronEdge *polyhedron_add_edge(Polyhedron *polyhedron, PolyhedronPoint *p1, PolyhedronPoint *p2);
PolyhedronTriangle *polyhedron_add_triangle(Polyhedron *polyhedron, PolyhedronPoint *a, PolyhedronPoint *b, PolyhedronPoint *c, PolyhedronEdge *e1, PolyhedronEdge *e2, PolyhedronEdge *e3);

// Remove features from the polyhedron. This also removes all relevant inter-references between features.
void polyhedron_remove_point(Polyhedron *poly, PolyhedronPoint *p);
void polyhedron_remove_edge(Polyhedron *poly, PolyhedronEdge *e);
void polyhedron_remove_triangle(Polyhedron *poly, PolyhedronTriangle *t);

int polyhedron_num_points(Polyhedron *poly);
int polyhedron_num_edges(Polyhedron *poly);
int polyhedron_num_triangles(Polyhedron *poly);

vec3 *polyhedron_points(Polyhedron poly);

/*================================================================================
    Polyhedron algorithms.
================================================================================*/
Polyhedron convex_hull(vec3 *points, int num_points);
bool point_in_convex_polyhedron(vec3 p, Polyhedron poly);
float polyhedron_volume(Polyhedron poly);
vec3 polyhedron_extreme_point(Polyhedron poly, vec3 direction);
// Assuming uniform mass of the polyhedron.
vec3 polyhedron_center_of_mass(Polyhedron poly);

/*================================================================================
    Polytope methods. Polytopes are represented by only their points, and their polyhedron
    can be recovered at any time by taking the convex hull.
    (Polyhedron representation may not be consistent due to triangulation of faces).
================================================================================*/
vec3 polytope_center_of_mass(vec3 *points, int num_points);
vec3 polytope_extreme_point(vec3 *points, int num_points, vec3 direction);

/*================================================================================
    Closest-points methods.
================================================================================*/
vec3 closest_point_on_line_to_point(vec3 a, vec3 b, vec3 p);
vec3 closest_point_on_line_segment_to_point(vec3 a, vec3 b, vec3 p);
vec3 closest_point_on_triangle_to_point(vec3 a, vec3 b, vec3 c, vec3 p);
vec3 closest_point_on_tetrahedron_to_point(vec3 a, vec3 b, vec3 c, vec3 d, vec3 p);

/*================================================================================
    Projection methods.
================================================================================*/
vec3 point_to_triangle_plane(vec3 a, vec3 b, vec3 c, vec3 p);
vec3 point_to_triangle_plane_barycentric(vec3 a, vec3 b, vec3 c, vec3 p);

/*================================================================================
    Simplex methods.
================================================================================*/
vec3 closest_point_on_simplex(int n, vec3 points[], vec3 p);
int simplex_extreme_index(int n, vec3 points[], vec3 dir);
bool point_in_tetrahedron(vec3 a, vec3 b, vec3 c, vec3 d, vec3 p);
//note: 6-times the volume is easier for checking signs since it is the factor achieved from the 4x4 determinant formulation.
float tetrahedron_6_times_volume(vec3 a, vec3 b, vec3 c, vec3 d);

/*================================================================================
    Coordinate methods.
================================================================================*/
// wa+wb+wc != 0 is required. The sum is normalized to one by this function.
vec3 barycentric_triangle(vec3 a, vec3 b, vec3 c, float wa, float wb, float wc);
// Alternatively give the barycentric weights as a vector.
#define barycentric_triangle_v(A,B,C,W) barycentric_triangle(A,B,C, ( W ).vals[0], ( W ).vals[1], ( W ).vals[2])

/*================================================================================
    Intersection methods.
================================================================================*/
bool ray_triangle_plane_intersection_barycentric(vec3 origin, vec3 direction, vec3 a, vec3 b, vec3 c, vec3 *intersection);
bool ray_triangle_plane_intersection(vec3 origin, vec3 direction, vec3 a, vec3 b, vec3 c, vec3 *intersection);
bool ray_triangle_intersection_barycentric(vec3 origin, vec3 direction, vec3 a, vec3 b, vec3 c, vec3 *intersection);
bool ray_triangle_intersection(vec3 origin, vec3 direction, vec3 a, vec3 b, vec3 c, vec3 *intersection);
bool ray_rectangle_plane_coordinates(vec3 origin, vec3 direction, vec3 tl, vec3 bl, vec3 br, vec3 tr, float *x, float *y);
bool ray_rectangle_coordinates(vec3 origin, vec3 direction, vec3 tl, vec3 bl, vec3 br, vec3 tr, float *x, float *y);
bool ray_rectangle_intersection(vec3 origin, vec3 direction, vec3 tl, vec3 bl, vec3 br, vec3 tr, vec3 *intersection);
bool ray_sphere_intersection(vec3 origin, vec3 direction, vec3 center, float radius, vec3 *intersection);

/*================================================================================
    Geometric object creation.
================================================================================*/
Polyhedron make_block(float width, float height, float depth);

/*================================================================================
    Semi-random routines, for creating test objects.
================================================================================*/
vec3 *random_points(float radius, int n);

#endif // GEOMETRY_H
