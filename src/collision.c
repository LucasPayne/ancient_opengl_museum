#include "museum.h"

#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))

static mat3x3 brute_force_polyhedron_inertia_tensor(Polyhedron poly, vec3 center, float mass)
{
    float integrals[6] = {0}; // x^2, y^2, z^2, xy, xz, yz
    float volume = 0;

    PolyhedronPoint *point = poly.points.first;
    vec3 min = point->position;
    vec3 max = point->position;
    while (point != NULL) {
        for (int i = 0; i < 3; i++) {
            if (point->position.vals[i] < min.vals[i]) min.vals[i] = point->position.vals[i];
            if (point->position.vals[i] > max.vals[i]) max.vals[i] = point->position.vals[i];
        }
        point = point->next;
    }
    float min_extent = MIN(max.vals[0] - min.vals[0], max.vals[1] - min.vals[1]);
    min_extent = MIN(min_extent, max.vals[2] - min.vals[2]);
    float d = 0.05 * min_extent;
    float dcubed = d*d*d;

    for (float x = min.vals[0]; x <= max.vals[0]; x += d) {
        for (float y = min.vals[1]; y <= max.vals[1]; y += d) {
            for (float z = min.vals[2]; z <= max.vals[2]; z += d) {
                if (!point_in_convex_polyhedron(new_vec3(x,y,z), poly)) continue;
                volume += dcubed;
            }
        }
    }
    float inverse_volume = volume == 0 ? 0 : 1.0 / volume;
    float element = dcubed * inverse_volume;
    for (float x = min.vals[0]; x <= max.vals[0]; x += d) {
        for (float y = min.vals[1]; y <= max.vals[1]; y += d) {
            for (float z = min.vals[2]; z <= max.vals[2]; z += d) {
                if (!point_in_convex_polyhedron(new_vec3(x,y,z), poly)) continue;
                float xc = x - center.vals[0];
                float yc = y - center.vals[1];
                float zc = z - center.vals[2];
                integrals[0] += xc*xc * dcubed;
                integrals[1] += yc*yc * dcubed;
                integrals[2] += zc*zc * dcubed;
                integrals[3] += xc*yc * dcubed;
                integrals[4] += xc*zc * dcubed;
                integrals[5] += yc*zc * dcubed;
            }
        }
    }
    mat3x3 inertia_tensor;
    fill_mat3x3_rmaj(inertia_tensor, integrals[1]+integrals[2], -integrals[3], -integrals[4],
                                 -integrals[3], integrals[0]+integrals[2], -integrals[5],
                                 -integrals[4], -integrals[5], integrals[0]+integrals[1]);
    for (int i = 0; i < 9; i++) {
        inertia_tensor.vals[i] *= inverse_volume * mass;
    }
    return inertia_tensor;
}


//================================================================================
// Collider component. Colliders are all convex polyhedra.
Collider *add_collider(Entity *e, vec3 *points, int num_points, bool can_rotate)
{
    if (num_points < 1) {
        fprintf(stderr, "ERROR: A collider must have at least one point.\n");
        exit(EXIT_FAILURE);
    }
    Collider *collider = (Collider *) add_behaviour(e, NULL, sizeof(Collider), ColliderID)->data;
    collider->points = points;
    collider->num_points = num_points;
    // Compute a bounding sphere by finding the maximum distance from the collider origin.
    float d = 0;
    for (int i = 0; i < num_points; i++) {
        float new_d = vec3_dot(points[i], points[i]);
        if (new_d > d) {
            d = new_d;
        }
    }
    collider->radius = sqrt(d);
    // Compute an axis-aligned bounding box.
    // If the entity can rotate, a non-optimal box is computed that still bounds the collider after rotations.
    if (can_rotate) {
        collider->use_aabb = false; // If both of these flags are false for bounds checks, just use the sphere test, as this is these AABBs are worse.
        for (int i = 0; i < 3; i++) {
            collider->aabb_min[i] = -collider->radius;
            collider->aabb_max[i] = collider->radius;
        }
    } else {
        collider->use_aabb = true;
        float min_vals[3] = { UNPACK_VEC3(points[0]) };
        float max_vals[3] = { UNPACK_VEC3(points[0]) };
        for (int i = 1; i < num_points; i++) {
            for (int j = 0; j < 3; j++) {
                if (points[i].vals[j] < min_vals[j]) min_vals[j] = points[i].vals[j];
                else if (points[i].vals[j] > max_vals[j]) max_vals[j] = points[i].vals[j];
            }
        }
        memcpy(collider->aabb_min, min_vals, sizeof(float)*3);
        memcpy(collider->aabb_max, max_vals, sizeof(float)*3);
    }

    return collider;
}

// Test the colliders with their basic bounding volumes.
bool collider_bounding_test(Collider *A, Entity *A_entity, Collider *B, Entity *B_entity)
{
    // Bounding sphere test.
    float r = A->radius*A_entity->scale + B->radius*B_entity->scale;
    vec3 diff = vec3_sub(A_entity->position, B_entity->position);
    if (vec3_dot(diff, diff) > r * r) return false;
    if (A->use_aabb || B->use_aabb) {
        for (int i = 0; i < 3; i++) {
            if (A->aabb_min[i]*A_entity->scale + A_entity->position.vals[i] > B->aabb_max[i]*B_entity->scale + B_entity->position.vals[i]) return false;
            if (A->aabb_max[i]*A_entity->scale + A_entity->position.vals[i] < B->aabb_min[i]*B_entity->scale + B_entity->position.vals[i]) return false;
        }
    }
    return true;
}

//================================================================================
// for testing and debugging.
Polyhedron compute_minkowski_difference(Polyhedron A, Polyhedron B)
{
    int num_points = polyhedron_num_points(&A) * polyhedron_num_points(&B);
    vec3 *points = malloc(sizeof(float)*3 * num_points);
    mem_check(points);
    PolyhedronPoint *p = A.points.first;
    int i = 0;
    while (p != NULL) {
        int j = 0;
        PolyhedronPoint *q = B.points.first;
        while (q != NULL) {
            points[i + j*polyhedron_num_points(&A)] = vec3_sub(p->position, q->position);
            q = q->next;
            j++;
        }
        p = p->next;
        i++;
    }
    Polyhedron diff = convex_hull(points, num_points);
    free(points);
    return diff;
}
/*================================================================================
----------------------------------------------------------------------------------
        Convex polyhedra collision.
----------------------------------------------------------------------------------
    Collision and contact information is computed for two convex polyhedra.
    The polyhedra are given as point clouds, which define their convex hulls.
    The GJK (Gilbert-Johnson-Keerthi) algorithm is used to descend a simplex
    through the Minkowski difference, or configuration space obstacle, of the polyhedra.
    If the simplex ever bounds the origin, the polyhedra are intersecting (as for some
    points a in A and b in B, a - b = 0 => a = b).

    If they are not intersecting, the closest points of the two polyhedra can be computed from
    the closest point on the CSO to the origin.
    If they are colliding, contact information is wanted. The minimum separating vector is given,
    computed by the expanding polytope algorithm (EPA). This algorithm progressively expands
    a sub-polytope of the CSO in order to find the closest point on the CSO boundary to the origin,
    whose negative is the separating vector, the minimal translation to move the CSO so that it does not
    bound the origin. This can be used to infer the contact normal and contact points on each polyhedron.
================================================================================*/
static int support_index(vec3 *points, int num_points, mat4x4 matrix, vec3 direction)
{
    //---Basic rearrangement can allow the avoidance of most matrix-vector multiplies here.
    float d = vec3_dot(rigid_matrix_vec3(matrix, points[0]), direction); // At least one point must be given.
    int index = 0;
    for (int i = 1; i < num_points; i++) {
        float new_d = vec3_dot(rigid_matrix_vec3(matrix, points[i]), direction);
        if (new_d > d) {
            d = new_d;
            index = i;
        }
    }
    return index;
}
bool convex_hull_intersection(vec3 *A, int A_len, mat4x4 A_matrix, vec3 *B, int B_len, mat4x4 B_matrix, GJKManifold *manifold)
{
#define DEBUG 0 // Turn this flag on to visualize some things.
    // Initialize the simplex as a line segment.
    vec3 simplex[4];
    int indices_A[4];
    int indices_B[4];
    int n = 2;
    // cso: Configuration space obstacle, another name for the Minkowski difference of two sets.
    // This macro gives the support vector in the Minkowski difference, and also gives the indices of the points in A and B whose difference is that support vector.
    #define cso_support(DIRECTION,SUPPORT,INDEX_A,INDEX_B)\
    {\
        ( INDEX_A ) = support_index(A, A_len, A_matrix, ( DIRECTION ));\
        ( INDEX_B ) = support_index(B, B_len, B_matrix, vec3_neg(( DIRECTION )));\
        ( SUPPORT ) = vec3_sub(rigid_matrix_vec3(A_matrix, A[( INDEX_A )]), rigid_matrix_vec3(B_matrix, B[( INDEX_B )]));\
    }
    cso_support(new_vec3(1,1,1), simplex[0], indices_A[0], indices_B[0]);
    cso_support(vec3_neg(simplex[0]), simplex[1], indices_A[1], indices_B[1]);
    vec3 origin = vec3_zero();

    // Go into a loop, computing the closest point on the simplex and expanding it in the opposite direction (from the origin),
    // and removing simplex points to maintain n <= 4.
    int COUNTER2 = 0;
    while (1) {
        if (++COUNTER2 == 2000) return false; /////////---preventing infinite loops before making this more robust. Infinite loops will probably happen anyway unless very careful, so prevent them.
        vec3 c = closest_point_on_simplex(n, simplex, origin);
        vec3 dir = vec3_neg(c);

        // If the simplex is a tetrahedron and contains the origin, the CSO contains the origin.
        if (n == 4 && point_in_tetrahedron(simplex[0],simplex[1],simplex[2],simplex[3], origin)) {

            // Perform the expanding polytope algorithm.
            // Instead of using a fancy data-structure, the polytope is maintained by keeping
            // a pool. Entries can be nullified, and entries re-added in those empty spaces, but linear iterations still need to
            // check up to *_len features. If the arrays are not long enough, then this fails.
            int16_t points[1024 * 2] = {-1};
            const int points_n = 2;
            int points_len = 0;
            int16_t triangles[1024 * 6] = {-1};
            const int triangles_n = 6;
            int triangles_len = 0;
            int16_t edges[1024 * 2] = {-1};
            const int edges_n = 2;
            int edges_len = 0;

            // For efficiency, features are added by this simple macro'd routine. e.g.,
            //     int new_tri_index;
            //     new_feature_index(triangles, new_tri_index);
            // will get the index of an available triangle slot.
            static const char *EPA_error_string = "ERROR: Expanding polytope algorithm: Fixed-length feature lists failed to be sufficient.\n";
            #define new_feature_index(TYPE,INDEX)\
            {\
                bool found = false;\
                for (int i = 0; i < TYPE ## _len; i++) {\
                    if (TYPE [TYPE ## _n * i] == -1) {\
                        ( INDEX ) = i;\
                        found = true;\
                        break;\
                    }\
                }\
                if (!found) {\
                    if (TYPE ## _len == 1024) {\
                        fprintf(stderr, EPA_error_string);\
                        exit(EXIT_FAILURE);\
                    }\
                    ( INDEX ) = TYPE ## _len ++;\
                }\
            }
            // Point: References the indices of the points of A and B for which the difference is this point.
            #define add_point(POINT_INDEX_A,POINT_INDEX_B,INDEX) {\
                new_feature_index(points, ( INDEX ));\
                points[points_n * ( INDEX )] = ( POINT_INDEX_A );\
                points[points_n * ( INDEX ) + 1] = ( POINT_INDEX_B );\
            }
            // Edge: References its end points.
            #define add_edge(AI,BI,INDEX) {\
                new_feature_index(edges, ( INDEX ));\
                edges[edges_n * ( INDEX )] = ( AI );\
                edges[edges_n * ( INDEX ) + 1] = ( BI );\
            }
            // Triangle: References its points in anti-clockwise winding order, abc, and references its edges, ab, bc, ca.
            #define add_triangle(AI,BI,CI) {\
                int index;\
                new_feature_index(triangles, index);\
                triangles[triangles_n * index] = ( AI );\
                triangles[triangles_n * index + 1] = ( BI );\
                triangles[triangles_n * index + 2] = ( CI );\
                add_edge(( AI ),( BI ),triangles[triangles_n * index + 3]);\
                add_edge(( BI ),( CI ),triangles[triangles_n * index + 4]);\
                add_edge(( CI ),( AI ),triangles[triangles_n * index + 5]);\
            }
            float v = tetrahedron_6_times_volume(simplex[0],simplex[1],simplex[2],simplex[3]);
            if (v < 0) {
                // If the tetrahedron has negative volume, swap two entries, forcing the winding order to be anti-clockwise from outside.
                vec3 temp = simplex[0];
                int tempA = indices_A[0];
                int tempB = indices_B[0];
                simplex[0] = simplex[1];
                indices_A[0] = indices_A[1];
                indices_B[0] = indices_B[1];
                simplex[1] = temp;
                indices_A[1] = tempA;
                indices_B[1] = tempB;
            }
            //---since it is known that everything is empty, it would be more efficient to just hardcode the initial tetrahedron.
            int dummy; // since the macro saves the index.
            for (int i = 0; i < 4; i++) {
                add_point(indices_A[i], indices_B[i], dummy);
            }
            add_triangle(0,1,2);
            add_triangle(1,0,3);
            add_triangle(2,1,3);
            add_triangle(0,2,3);

            // The initial tetrahedron has been set up. Proceed with EPA.
	    int COUNTER = 0; // for debugging.
            while (1) {
                COUNTER ++;
                if (COUNTER == 500) return false; //------///////////////Preventing an infinite loop.
                // Find the closest triangle to the origin.
                float min_d = -1;
                int closest_triangle_index = -1;
                vec3 closest_point;
                for (int i = 0; i < triangles_len; i++) {
                    if (triangles[triangles_n*i] == -1) continue;

                    vec3 a = vec3_sub(rigid_matrix_vec3(A_matrix, A[points[points_n*triangles[triangles_n*i+0]]]), rigid_matrix_vec3(B_matrix, B[points[points_n*triangles[triangles_n*i+0] + 1]]));
                    vec3 b = vec3_sub(rigid_matrix_vec3(A_matrix, A[points[points_n*triangles[triangles_n*i+1]]]), rigid_matrix_vec3(B_matrix, B[points[points_n*triangles[triangles_n*i+1] + 1]]));
                    vec3 c = vec3_sub(rigid_matrix_vec3(A_matrix, A[points[points_n*triangles[triangles_n*i+2]]]), rigid_matrix_vec3(B_matrix, B[points[points_n*triangles[triangles_n*i+2] + 1]]));
                    vec3 p = point_to_triangle_plane(a,b,c, origin);
                    float new_d = vec3_dot(p, p);
                    if (min_d == -1 || new_d < min_d) {
                        min_d = new_d;
                        closest_triangle_index = i;
                        closest_point = p;
                    }
                }
                vec3 a = vec3_sub(rigid_matrix_vec3(A_matrix, A[points[points_n*triangles[triangles_n*closest_triangle_index+0]]]), rigid_matrix_vec3(B_matrix, B[points[points_n*triangles[triangles_n*closest_triangle_index+0] + 1]]));
                vec3 b = vec3_sub(rigid_matrix_vec3(A_matrix, A[points[points_n*triangles[triangles_n*closest_triangle_index+1]]]), rigid_matrix_vec3(B_matrix, B[points[points_n*triangles[triangles_n*closest_triangle_index+1] + 1]]));
                vec3 c = vec3_sub(rigid_matrix_vec3(A_matrix, A[points[points_n*triangles[triangles_n*closest_triangle_index+2]]]), rigid_matrix_vec3(B_matrix, B[points[points_n*triangles[triangles_n*closest_triangle_index+2] + 1]]));

                // Find an extreme point in the direction from the origin to the closest point on the polytope boundary.
                // The convex hull of the points of the polytope adjoined with this new point will be computed.
                // vec3 expand_to = vec3_cross(vec3_sub(b, a), vec3_sub(c, a));
                vec3 expand_to = closest_point;
                int new_point_A_index, new_point_B_index;
                vec3 new_point;
                cso_support(expand_to, new_point, new_point_A_index, new_point_B_index);
                bool new_point_on_polytope = false;
                for (int i = 0; i < points_len; i++) {
                    // Unreferenced but non-nullified points can't be on the convex hull, so they can be checked against without problems.
                    if (points[points_n*i] == -1) continue;
                    if (points[points_n*i] == new_point_A_index && points[points_n*i+1] == new_point_B_index) {
                        new_point_on_polytope = true;
                        break;
                    }
                }
                if (new_point_on_polytope) {
                    // The closest triangle is on the border of the polyhedron, so the closest point on this triangle is the closest point
                    // to the border of the polyhedron.

                    // The separating vector is the minimal translation B must make to separate from A.
                    manifold->separating_vector = closest_point;

                    // Compute the barycentric coordinates of the closest point in terms of the triangle on the boundary of the CSO.
                    // This triangle is the Minkowski difference between a triangle on A and a triangle on B. Use the same barycentric weights
                    // to calculate the corresponding points on the boundaries of A and B.
                    //---need a better way to get these points.
                    vec3 Aa = rigid_matrix_vec3(A_matrix, A[points[points_n*triangles[triangles_n*closest_triangle_index+0]]]);
                    vec3 Ab = rigid_matrix_vec3(A_matrix, A[points[points_n*triangles[triangles_n*closest_triangle_index+1]]]);
                    vec3 Ac = rigid_matrix_vec3(A_matrix, A[points[points_n*triangles[triangles_n*closest_triangle_index+2]]]);
                    vec3 Ba = rigid_matrix_vec3(B_matrix, B[points[points_n*triangles[triangles_n*closest_triangle_index+0]+1]]);
                    vec3 Bb = rigid_matrix_vec3(B_matrix, B[points[points_n*triangles[triangles_n*closest_triangle_index+1]+1]]);
                    vec3 Bc = rigid_matrix_vec3(B_matrix, B[points[points_n*triangles[triangles_n*closest_triangle_index+2]+1]]);
                    vec3 a = vec3_sub(Aa, Ba);
                    vec3 b = vec3_sub(Ab, Bb);
                    vec3 c = vec3_sub(Ac, Bc);
                    vec3 barycentric_coords = point_to_triangle_plane_barycentric(a,b,c,  origin);
                    manifold->A_closest = barycentric_triangle_v(Aa,Ab,Ac,  barycentric_coords);
                    manifold->B_closest = barycentric_triangle_v(Ba,Bb,Bc,  barycentric_coords);
                    return true;
                }

                // Remove the visible triangles and their directed edges. Points do not need to be nullified.
                for (int i = 0; i < triangles_len; i++) {
                    if (triangles[triangles_n*i] == -1) continue;
                    vec3 ap = vec3_sub(rigid_matrix_vec3(A_matrix, A[points[points_n*triangles[triangles_n*i+0]]]), rigid_matrix_vec3(B_matrix, B[points[points_n*triangles[triangles_n*i+0] + 1]]));
                    vec3 bp = vec3_sub(rigid_matrix_vec3(A_matrix, A[points[points_n*triangles[triangles_n*i+1]]]), rigid_matrix_vec3(B_matrix, B[points[points_n*triangles[triangles_n*i+1] + 1]]));
                    vec3 cp = vec3_sub(rigid_matrix_vec3(A_matrix, A[points[points_n*triangles[triangles_n*i+2]]]), rigid_matrix_vec3(B_matrix, B[points[points_n*triangles[triangles_n*i+2] + 1]]));

                    vec3 n = vec3_cross(vec3_sub(bp, ap), vec3_sub(cp, ap));
                    float v = tetrahedron_6_times_volume(ap,bp,cp, new_point);
                    if (v < 0) {
                        // Remove this triangle, as it is visible from the new point.
                        edges[edges_n*triangles[triangles_n*i+3]] = -1;
                        edges[edges_n*triangles[triangles_n*i+4]] = -1;
                        edges[edges_n*triangles[triangles_n*i+5]] = -1;
                        triangles[triangles_n*i] = -1;
                    }
                }
                int new_point_index;
                add_point(new_point_A_index, new_point_B_index, new_point_index);

                // Search for boundary edges and save them in an array.
                int boundary_len = 0;
                int16_t boundary_edges[1024];
                for (int i = 0; i < edges_len; i++) {
                    if (edges[edges_n*i] == -1) continue;
                    // Search for an incident triangle, by looking for the opposite edge (with reversed direction).
                    bool boundary = true;
                    for (int j = 0; j < edges_len; j++) {
                        if (edges[edges_n*j] == -1) continue;
                        if (   points[points_n*edges[edges_n*i]] == points[points_n*edges[edges_n*j+1]]
                            && points[points_n*edges[edges_n*i]+1] == points[points_n*edges[edges_n*j+1]+1]
                            && points[points_n*edges[edges_n*i+1]] == points[points_n*edges[edges_n*j]]
                            && points[points_n*edges[edges_n*i+1]+1] == points[points_n*edges[edges_n*j]+1]) {
                            // This is the opposite edge, so a boundary edge has not been found.
                            boundary = false;
                            break;
                        }
                    }
                    if (boundary) {
                        // Instead of adding the triangle straight away, save the boundary edge of the new triangle in the array.
                        // This is because the edges are still being iterated, and if new ones are added, they could be registered as boundary edges.
                        boundary_edges[2*boundary_len] = edges[edges_n*i+1];
                        boundary_edges[2*boundary_len+1] = edges[edges_n*i];
                        boundary_len ++; //--- not checking out-of-space.
                    }
                }
                // Add the triangles of the extending cone.
                for (int i = 0; i < boundary_len; i++) {
                    add_triangle(boundary_edges[2*i], boundary_edges[2*i+1], new_point_index);
                }
            }
            fprintf(stderr, "ERROR: Code should not reach this.\n");
            exit(EXIT_FAILURE);
        }

        // The polyhedra are not intersecting. Descend the simplex and compute the closest point on the CSO.
        int A_index, B_index;
        vec3 new_point;
        cso_support(dir, new_point, A_index, B_index);
        bool on_simplex = false;
        for (int i = 0; i < n; i++) {
            if (A_index == indices_A[i] && B_index == indices_B[i]) {
                on_simplex = true;
                break;
            }
        }
        if (n == 4 && !on_simplex) {
            int replace = simplex_extreme_index(n, simplex, c);
            simplex[replace] = new_point;
            indices_A[replace] = A_index;
            indices_B[replace] = B_index;
            ///////////////////////////////////////////////////////////////////////////////////////////////////////
            //----This check seems to fix an infinite loop bug here, but I am not sure if the reasoning is correct.
            ///////////////////////////////////////////////////////////////////////////////////////////////////////
            if (vec3_dot(new_point, dir) <= 0) {
                vec3 closest_on_poly = closest_point_on_tetrahedron_to_point(simplex[0], simplex[1], simplex[2], simplex[3], origin);
                return false;
            }
        } else if (n == 3 && on_simplex) {
            vec3 closest_on_poly = closest_point_on_triangle_to_point(simplex[0], simplex[1], simplex[2], origin);
            return false;
        } else if (n == 2 && on_simplex) {
            vec3 closest_on_poly = closest_point_on_line_segment_to_point(simplex[0], simplex[1], origin);
            return false;
        } else if (n == 1 && on_simplex) {
            return false;
        } else if (!on_simplex) {
            simplex[n] = new_point;
            indices_A[n] = A_index;
            indices_B[n] = B_index;
            n++;
        } else {
            int remove = simplex_extreme_index(n, simplex, c);
            for (int j = remove; j < n - 1; j++) {
                simplex[j] = simplex[j + 1];
                indices_A[j] = indices_A[j + 1];
                indices_B[j] = indices_B[j + 1];
            }
            n--;
        }
    }
#undef DEBUG
}

static void resolve_rigid_body_collisions(void)
{
    for_behaviour(RigidBody, A, A_entity)
        mat4x4 A_matrix = entity_matrix(A_entity);

        // for_behaviour(RigidBody, B, B_entity)
        for_behaviour(Collider, B, B_entity)
            if (A_entity == B_entity) continue;
            if (!collider_bounding_test(A->collider, A_entity, B, B_entity)) continue;

            // If the bodies are colliding, manifold will contain contact information.
            mat4x4 B_matrix = entity_matrix(B_entity);
            GJKManifold manifold;
            bool colliding = convex_hull_intersection(A->collider->points, A->collider->num_points, A_matrix,
                                                      B->points, B->num_points, B_matrix, &manifold);
            if (!colliding) continue;
            // Try to find a rigidbody on the other entity. If there isn't one, the other collider is treated like an immovable rigidbody with infinite mass.
            RigidBody B_rigid_body = {0};
            bool found_rigid_body = false;
            for (int i = 0; i < MAX_NUM_ENTITY_BEHAVIOURS; i++) {
                if (B_entity->behaviours[i] != NULL && B_entity->behaviours[i]->type == RigidBodyID) {
                    B_rigid_body = *((RigidBody *) B_entity->behaviours[i]->data); // Copy the rigid body over.
                    found_rigid_body = true;
                    break;
                }
            }
            if (!found_rigid_body) {
                // Create a rigid body for this collider. This collider has infinite mass and is not moving.
                B_rigid_body.collider = B;
            }

            // va,vb: The linear velocities of the rigid bodies.
            vec3 va = vec3_mul(A->linear_momentum, A->inverse_mass);
            vec3 vb = vec3_mul(B_rigid_body.linear_momentum, B_rigid_body.inverse_mass);
            // Transform the inverse inertia tensors to world space via the rotation matrix and scale of this entity.
            mat3x3 A_rotation_matrix = entity_orientation(A_entity);
            mat3x3 A_worldspace_inverse_inertia_tensor = mat3x3_mul(mat3x3_multiply3(A_rotation_matrix, A->inverse_inertia_tensor, mat3x3_transpose(A_rotation_matrix)), A_entity->scale);
            mat3x3 B_rotation_matrix = entity_orientation(B_entity);
            mat3x3 B_worldspace_inverse_inertia_tensor = mat3x3_mul(mat3x3_multiply3(B_rotation_matrix, B_rigid_body.inverse_inertia_tensor, mat3x3_transpose(B_rotation_matrix)), B_entity->scale);

            // wa,wb: The angular velocities of the rigid bodies.
            vec3 wa = matrix_vec3(A_worldspace_inverse_inertia_tensor, A->angular_momentum);
            vec3 wb = matrix_vec3(B_worldspace_inverse_inertia_tensor, B_rigid_body.angular_momentum);
	    // Separate the objects.
            vec3 p;
            if (A->mass + B_rigid_body.mass == 0) {
                // Two "immovable" objects are colliding. Just separate them in a simple way without taking masses into account.
                //vec3 sep = vec3_mul(vec3_neg(manifold.separating_vector), 1.1); //
                vec3 sep = vec3_neg(manifold.separating_vector);
                A_entity->position = vec3_add(A_entity->position, sep);
                p = vec3_add(manifold.A_closest, sep);
            } else {
                float inv_total_mass =  1.0 / (A->mass + B_rigid_body.mass);
                float a_weighting = A->mass * inv_total_mass;
                float b_weighting = B_rigid_body.mass * inv_total_mass;
                vec3 a_sep = vec3_mul(manifold.separating_vector, -a_weighting);
                vec3 b_sep = vec3_mul(manifold.separating_vector, b_weighting);
                A_entity->position = vec3_add(A_entity->position, a_sep);
                B_entity->position = vec3_add(B_entity->position, b_sep);
                p = vec3_add(manifold.A_closest, a_sep);
            }

            // n: The normalized direction of separation.
            vec3 n = vec3_normalize(manifold.separating_vector);
            // a_pos,b_pos: The relative positions of the point of contact from A and B.
            vec3 a_pos = vec3_sub(p, A_entity->position);
            vec3 b_pos = vec3_sub(p, B_entity->position);
            // dpa,dpb: The linear velocities at the points of contact.
            vec3 dpa = vec3_add(va, vec3_cross(wa, a_pos));
            vec3 dpb = vec3_add(vb, vec3_cross(wb, b_pos));
            // relative_velocity: If the point of contact of A were hitting B in a frame where B is still, then this is A's velocity.
            //                    This is needed to calculate the force required for "bouncing A off of B".
            vec3 relative_velocity = vec3_sub(dpa, dpb);
            if (vec3_dot(relative_velocity, n) > 0) {
                // Colliding contact.

                // Separate the objects.
                // Move each object away from each other along the separating vector, taking into account the relative momentums at the points of contact.
                // Calculate the linear momentums at the points of contact.
                /*
                vec3 momentum_pa = vec3_add(A->linear_momentum, vec3_cross(a_pos, A->angular_momentum));
                vec3 momentum_pb = vec3_add(B->linear_momentum, vec3_cross(b_pos, B->angular_momentum));
                float dpan = vec3_dot(dpa, n);
                float dpbn = vec3_dot(dpb, n);
                float momentum_pan = vec3_dot(momentum_pa, n);
                float momentum_pbn = vec3_dot(momentum_pb, n);
                if (momentum_pan - momentum_pbn < 0.001) {
                    Transform_move(t, vec3_mul(vec3_neg(manifold.separating_vector), 1.1)); //---Offset because resting contact crashes. Seriously need to work on robustness.
                } else {
                    float a_sep = momentum_pan / (momentum_pan - momentum_pbn);
                    float b_sep = momentum_pbn / (momentum_pan - momentum_pbn);
                    Transform_move(t, vec3_mul(manifold.separating_vector, -a_sep * 1.1));
                    Transform_move(t2, vec3_mul(manifold.separating_vector, b_sep * 1.1));
                }*/


                float coefficient_of_restitution = 0;
                vec3 rel_A = a_pos;
                vec3 rel_B = b_pos;
                vec3 kA = vec3_cross(rel_A, n);
                vec3 kB = vec3_cross(rel_B, n);
                vec3 uA = matrix_vec3(A_worldspace_inverse_inertia_tensor, kA);
                vec3 uB = matrix_vec3(B_worldspace_inverse_inertia_tensor, kB);
                float impulse_magnitude = -(1 + coefficient_of_restitution) * (vec3_dot(n, vec3_sub(va, vb)) + vec3_dot(wa, kA) - vec3_dot(wb, kB))
                                          / (A->inverse_mass + B_rigid_body.inverse_mass + vec3_dot(kA, uA) + vec3_dot(kB, uB));
                vec3 impulse = vec3_mul(n, impulse_magnitude);
                A->linear_momentum = vec3_add(A->linear_momentum, impulse);
                B_rigid_body.linear_momentum = vec3_sub(B_rigid_body.linear_momentum, impulse);
                A->angular_momentum = vec3_add(A->angular_momentum, vec3_mul(kA, impulse_magnitude));
                B_rigid_body.angular_momentum = vec3_sub(B_rigid_body.angular_momentum, vec3_mul(kB, impulse_magnitude));

/*
                    float coefficient_of_restitution = 0.5;
                    float numerator = -(1 + coefficient_of_restitution) * (vec3_dot(relative_velocity, n) + vec3_dot(wa, vec3_cross(a_pos, n)) - vec3_dot(wb, vec3_cross(b_pos, n)));
                    float denominator = A->inverse_mass + B->inverse_mass + vec3_dot(vec3_cross(a_pos, n), matrix_vec3(A_worldspace_inverse_inertia_tensor, vec3_cross(a_pos, n)))
                                                                          + vec3_dot(vec3_cross(b_pos, n), matrix_vec3(B_worldspace_inverse_inertia_tensor, vec3_cross(b_pos, n)));
                    if (denominator == 0) {
                        fprintf(stderr, ERROR_ALERT "Denominator in calculation of impulse magnitude between rigid bodies should never be zero.\n");
                        exit(EXIT_FAILURE);
                    }
                    float impulse_magnitude = numerator / denominator;
                    //printf("impulse_magnitude: %.6f\n", impulse_magnitude);
                    //printf("A mass: %.6f\n", A->mass);
                    //printf("B mass: %.6f\n", B->mass);
                    //printf("A inertia tensor:\n");
                    //print_matrix3x3f(&A->inertia_tensor);
                    //printf("A inverse inertia tensor:\n");
                    //print_matrix3x3f(&A->inverse_inertia_tensor);
                    //printf("B inertia tensor:\n");
                    //print_matrix3x3f(&B->inertia_tensor);
                    //printf("B inverse inertia tensor:\n");
                    //print_matrix3x3f(&B->inverse_inertia_tensor);
                    //getchar();
                    vec3 impulse_force = vec3_mul(n, impulse_magnitude);
                    A->linear_momentum = vec3_add(A->linear_momentum, impulse_force);
                    B->linear_momentum = vec3_sub(B->linear_momentum, impulse_force);
                    A->angular_momentum = vec3_add(A->angular_momentum, vec3_mul(vec3_cross(a_pos, n), impulse_magnitude));
                    B->angular_momentum = vec3_sub(B->angular_momentum, vec3_mul(vec3_cross(b_pos, n), impulse_magnitude));
*/
            }
        end_for_behaviour()
    end_for_behaviour()
}

// This is not a behavioural update, since finer control over when rigid bodies are updated is wanted.
static void rigid_body_update(RigidBody *rb, Entity *e)
{
    // Euler's method updating for rigid body positions and orientations.
    X(e->position) += rb->linear_momentum.vals[0] * rb->inverse_mass * dt;
    Y(e->position) += rb->linear_momentum.vals[1] * rb->inverse_mass * dt;
    Z(e->position) += rb->linear_momentum.vals[2] * rb->inverse_mass * dt;

    // Transform the inverse inertia tensor to world space via the rotation matrix of this body.
    mat3x3 rotation_matrix = entity_orientation(e);
    mat3x3 worldspace_inverse_inertia_tensor = mat3x3_multiply3(rotation_matrix, rb->inverse_inertia_tensor, mat3x3_transpose(rotation_matrix));

    // Calculate the angular velocity from angular momentum and the (inverse) inertia tensor.
    vec3 angular_velocity = matrix_vec3(worldspace_inverse_inertia_tensor, rb->angular_momentum);

    float dwx,dwy,dwz;
    dwx = angular_velocity.vals[0] * dt;
    dwy = angular_velocity.vals[1] * dt;
    dwz = angular_velocity.vals[2] * dt;
    mat3x3 skew;
    fill_mat3x3_cmaj(skew, 0,   -dwz,  dwy,
                           dwz,    0, -dwx,
                           -dwy, dwx,    0);
    skew = mat3x3_transpose(skew);
    e->orientation = mat3x3_add(e->orientation, mat3x3_multiply(skew, e->orientation));
    // Orthonormalize to prevent matrix drift.
    mat3x3_orthonormalize(&e->orientation);
}

void rigid_body_dynamics(void)
{
    // Gravity updates here for now for testing.
    for_behaviour(RigidBody, rb, rb_entity)
        rb->linear_momentum.vals[1] -= rb->mass * dt * gravity_constant;
        rigid_body_update(rb, rb_entity);
    end_for_behaviour()
    resolve_rigid_body_collisions();
}

RigidBody *add_rigid_body(Entity *e, float mass)
{
    Collider *collider = NULL;
    for (int i = 0; i < MAX_NUM_ENTITY_BEHAVIOURS; i++) {
        if (e->behaviours[i] != NULL && e->behaviours[i]->type == ColliderID) {
            collider = (Collider *) e->behaviours[i]->data;
            break;
        }
    }
    if (collider == NULL) {
        fprintf(stderr, "ERROR: A collider must be attached to an entity before it can become a rigid body.\n");
        exit(EXIT_FAILURE);
    }
    RigidBody *rb = (RigidBody *) add_behaviour(e, NULL, sizeof(RigidBody), RigidBodyID)->data;
    memset(rb, 0, sizeof(RigidBody));
    rb->collider = collider;
    rb->mass = mass;
    rb->inverse_mass = mass == 0 ? 0 : 1.0 / mass;

    vec3 center_of_mass = polytope_center_of_mass(rb->collider->points, rb->collider->num_points);

    rb->center_of_mass = center_of_mass;
    // Update the entity center. This is by default (0,0,0), but the center can be changed to make adjustments to the entity matrix.
    // This is useful because then geometry (in application or in vram) does not need to be changed for a change of center of rotation.
    e->center = center_of_mass;

    mat3x3 inertia_tensor = brute_force_polyhedron_inertia_tensor(convex_hull(rb->collider->points, rb->collider->num_points), center_of_mass, mass);
    if (mass == 0) {
        memset(&rb->inertia_tensor, 0, sizeof(mat3x3));
        memset(&rb->inverse_inertia_tensor, 0, sizeof(mat3x3));
    } else {
        rb->inertia_tensor = inertia_tensor;
        rb->inverse_inertia_tensor = mat3x3_inverse(inertia_tensor);
    }
    return rb;
}

