#ifndef COLLISION_H
#define COLLISION_H
/*================================================================================
    Collision.
================================================================================*/
#include "mathematics.h"
#include "geometry.h"
#include "museum.h"

typedef struct GJKManifold_s {
    vec3 separating_vector;
    vec3 A_closest;
    vec3 B_closest;
} GJKManifold;
bool convex_hull_intersection(vec3 *A, int A_len, mat4x4 A_matrix, vec3 *B, int B_len, mat4x4 B_matrix, GJKManifold *manifold);

// Debugging and visualization.
Polyhedron compute_minkowski_difference(Polyhedron A, Polyhedron B);

typedef struct Collider_s {
    vec3 *points;
    int num_points;
    float radius; // Gives a bounding sphere from the collider origin.
    bool use_aabb;
    float aabb_min[3];
    float aabb_max[3];
} Collider;
Collider *add_collider(Entity *e, vec3 *points, int num_points, bool can_rotate);
bool collider_bounding_test(Collider *A, Entity *A_entity, Collider *B, Entity *B_entity);

/*--------------------------------------------------------------------------------
A RigidBody is simulated according to rigid body dynamics. The geometry need not be the
same as what the object is being rendered as. Currently rigid bodies are restricted to being
convex polyhedra, although the algorithms can be generalized to convex sets, and constraints can
be incorporated along with convex decomposition to allow concave objects.
--------------------------------------------------------------------------------*/
typedef struct RigidBody_s {
    Collider *collider;

    vec3 linear_momentum;
    vec3 angular_momentum;
    float mass;
    float inverse_mass;
    vec3 center_of_mass;
    mat3x3 inertia_tensor;
    mat3x3 inverse_inertia_tensor;
} RigidBody;
void rigid_body_dynamics(void);
RigidBody *add_rigid_body(Entity *e, float mass);

#endif // COLLISION_H
