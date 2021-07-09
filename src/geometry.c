#include "museum.h"

vec3 *random_points(float radius, int n)
{
    // So the "random" convex polyhedra have some sort of variety, use Gram-Schmidt to create a semi-random orthonormal basis, and
    // then have r1,r2,r3 be the "random principal axes", that weight the points in each of those directions.

    // float size = frand()*1.9 + 0.1; // size multiplies the base radius.
    float size = 1;

    vec3 e1, e2, e3;
    e1 = new_vec3(frand()-0.5, frand()-0.5, frand()-0.5);
    e1 = vec3_normalize(e1);
    e2 = new_vec3(frand()-0.5, frand()-0.5, frand()-0.5);
    e2 = vec3_normalize(vec3_sub(e2, vec3_mul(e1, vec3_dot(e2, e1))));
    e3 = vec3_cross(e1, e2);
    // Extend these so the random convex polyhedra is roughly ellipsoidal.
    e1 = vec3_mul(e1, 0.5 + frand());
    e2 = vec3_mul(e2, 0.5 + frand());
    e3 = vec3_mul(e3, 0.5 + frand());

    vec3 *points = malloc(sizeof(vec3) * n);
    mem_check(points);
    float o = 50;
    vec3 offset = new_vec3(frand()*o-o/2,frand()*o-o/2,frand()*o-o/2);
    for (int i = 0; i < n; i++) {
        vec3 f = vec3_normalize(new_vec3(frand()-0.5,frand()-0.5,frand()-0.5));
        float r = frand();
        //points[i] = vec3_mul(vec3_add(vec3_add(vec3_mul(e1, frand()-0.5), vec3_mul(e2, frand()-0.5)), vec3_mul(e3, frand()-0.5)), radius * size);
        points[i] = vec3_mul(vec3_add(vec3_mul(e1, f.vals[0]), vec3_add(vec3_mul(e2, f.vals[1]), vec3_mul(e3, f.vals[2]))), radius * size * r);
        points[i] = vec3_add(points[i], offset);
    }
    return points;
}


float tetrahedron_6_times_volume(vec3 a, vec3 b, vec3 c, vec3 d)
{
    float a1,a2,a3,a4;
    a1 = a.vals[0]; a2 = b.vals[0]; a3 = c.vals[0]; a4 = d.vals[0];
    float b1,b2,b3,b4;
    b1 = a.vals[1]; b2 = b.vals[1]; b3 = c.vals[1]; b4 = d.vals[1];
    float c1,c2,c3,c4;
    c1 = a.vals[2]; c2 = b.vals[2]; c3 = c.vals[2]; c4 = d.vals[2];
    float d1,d2,d3,d4;
    d1 = a.vals[3]; d2 = b.vals[3]; d3 = c.vals[3]; d4 = d.vals[3];

    return a1*(b2*(c3-c4) - b3*(c2-c4) + b4*(c2-c3))
         - a2*(b1*(c3-c4) - b3*(c1-c4) + b4*(c1-c3))
         + a3*(b1*(c2-c4) - b2*(c1-c4) + b4*(c1-c2))
         - a4*(b1*(c2-c3) - b2*(c1-c3) + b3*(c1-c2));
}

//--------------------------------------------------------------------------------
// Closest-points methods.
//--------------------------------------------------------------------------------
vec3 closest_point_on_line_to_point(vec3 a, vec3 b, vec3 p)
{
    // This is an unlimited line.
    vec3 ab = vec3_sub(b, a);
    vec3 ap = vec3_sub(p, a);
    return vec3_add(a, vec3_mul(ab, vec3_dot(ap, ab) / vec3_dot(ab, ab)));
}
vec3 closest_point_on_line_segment_to_point(vec3 a, vec3 b, vec3 p)
{
    if (vec3_dot(vec3_sub(p, a), vec3_sub(b, a)) < 0) {
        return a;
    }
    if (vec3_dot(vec3_sub(p, b), vec3_sub(a, b)) < 0) {
        return b;
    }
    return closest_point_on_line_to_point(a, b, p);
}

vec3 barycentric_triangle(vec3 a, vec3 b, vec3 c, float wa, float wb, float wc)
{
    return vec3_mul(vec3_add(vec3_mul(a, wa), vec3_add(vec3_mul(b, wb), vec3_mul(c, wc))), 1.0/(wa + wb + wc));
}

// Get the barycentric coordinates of the projection of the point into the plane spanned by the triangle.
vec3 point_to_triangle_plane_barycentric(vec3 a, vec3 b, vec3 c, vec3 p)
{
    //---all three collinear gives a divison by zero?
    vec3 n = vec3_cross(vec3_sub(b, a), vec3_sub(c, a));
    vec3 ap = vec3_sub(p, a);
    vec3 bp = vec3_sub(p, b);
    vec3 cp = vec3_sub(p, c);
    vec3 w;
    w.vals[0] = vec3_dot(vec3_cross(bp, cp), n);
    w.vals[1] = vec3_dot(vec3_cross(cp, ap), n);
    w.vals[2] = vec3_dot(vec3_cross(ap, bp), n);
    float winv = 1.0 / (w.vals[0] + w.vals[1] + w.vals[2]);
    w.vals[0] *= winv; w.vals[1] *= winv; w.vals[2] *= winv;
    return w;
}
// Get the cartesian coordinates of the projection of the point into the plane spanned by the triangle.
vec3 point_to_triangle_plane(vec3 a, vec3 b, vec3 c, vec3 p)
{
    vec3 w = point_to_triangle_plane_barycentric(a, b, c, p);
    return barycentric_triangle(a, b, c, w.vals[0], w.vals[1], w.vals[2]);
}

vec3 closest_point_on_triangle_to_point(vec3 a, vec3 b, vec3 c, vec3 p)
{
    // Get the barycentric coordinates of the projection of p into the triangle's plane,
    // then use barycentric regions then tests against normals to determine the Voronoi region,
    // and return the closest point on the relevant feature.

    vec3 w = point_to_triangle_plane_barycentric(a, b, c, p);
    float wa,wb,wc;
    wa = w.vals[0]; wb = w.vals[1]; wc = w.vals[2];
    if (wa < 0) {
       if (vec3_dot(vec3_sub(p, b), vec3_sub(c, b)) < 0) return b;
       if (vec3_dot(vec3_sub(p, c), vec3_sub(b, c)) < 0) return c;
       return closest_point_on_line_to_point(b, c, p);
    }
    if (wb < 0) {
       if (vec3_dot(vec3_sub(p, c), vec3_sub(a, c)) < 0) return c;
       if (vec3_dot(vec3_sub(p, a), vec3_sub(c, a)) < 0) return a;
       return closest_point_on_line_to_point(c, a, p);
    }
    if (wc < 0) {
        if (vec3_dot(vec3_sub(p, a), vec3_sub(b, a)) < 0) return a;
        if (vec3_dot(vec3_sub(p, b), vec3_sub(a, b)) < 0) return b;
        return closest_point_on_line_to_point(a, b, p);
    }
    return barycentric_triangle(a,b,c, wa,wb,wc);
}

// Order: a,b,c,d,  p
// This is for strict inclusion, not on the boundary.
bool point_in_tetrahedron(vec3 a, vec3 b, vec3 c, vec3 d, vec3 p)
{
    float wa = tetrahedron_6_times_volume(a, b, c, p);
    float wb = tetrahedron_6_times_volume(b, a, d, p);
    float wc = tetrahedron_6_times_volume(c, b, d, p);
    float wd = tetrahedron_6_times_volume(a, c, d, p);
    // Return whether or not the weights all have the same sign.
    return (wa != 0 && wa < 0 == wb < 0 && wb < 0 == wc < 0 && wc < 0 == wd < 0);
}
vec3 closest_point_on_tetrahedron_to_point(vec3 a, vec3 b, vec3 c, vec3 d, vec3 p)
{
    // This method just takes all the closest points on each triangle, and takes the one of minium distance, unless the point is inside the tetrahedron.
    // This could definitely be better.
    if (point_in_tetrahedron(a,b,c,d, p)) return p;

    vec3 close_points[4];
    close_points[0] = closest_point_on_triangle_to_point(a,b,c, p);
    close_points[1] = closest_point_on_triangle_to_point(a,b,d, p);
    close_points[2] = closest_point_on_triangle_to_point(a,c,d, p);
    close_points[3] = closest_point_on_triangle_to_point(b,c,d, p);
    float mindis = -1;
    int min_index = 0;
    for (int i = 0; i < 4; i++) {
        float dis = vec3_dot(vec3_sub(close_points[i], p), vec3_sub(close_points[i], p));
        if (mindis < 0 || dis < mindis) {
            mindis = dis; min_index = i;
        }
    }
    return close_points[min_index];
}

//--------------------------------------------------------------------------------
// Simplex methods.
//--------------------------------------------------------------------------------
#define bad_simplex_error() {\
    fprintf(stderr, "ERROR: simplex method: Bad input. Input must be a simplex of 1,2,3, or 4 vertices.\n");\
    exit(EXIT_FAILURE);\
}
vec3 closest_point_on_simplex(int n, vec3 points[], vec3 p)
{
    if (n == 1) return points[0];
    if (n == 2) return closest_point_on_line_segment_to_point(points[0], points[1], p);
    if (n == 3) return closest_point_on_triangle_to_point(points[0], points[1], points[2], p);
    if (n == 4) return closest_point_on_tetrahedron_to_point(points[0], points[1], points[2], points[3], p);
    bad_simplex_error();
}
int simplex_extreme_index(int n, vec3 points[], vec3 dir)
{
    // Returns the index of a support vector in the simplex vertices in the given direction.
    if (n < 1 || n > 4) bad_simplex_error();
    float dist = vec3_dot(points[0], dir);
    int index = 0;
    for (int i = 1; i < n; i++) {
        float new_dist = vec3_dot(points[i], dir);
        if (new_dist > dist) {
            index = i;
            dist = new_dist;
        }
    }
    return index;
}

/*--------------------------------------------------------------------------------
    Intersection methods.
--------------------------------------------------------------------------------*/
// Ray intersection methods.
//--------------------------------------------------------------------------------
// Test whether the weights are a convex combination of the triangle points.
#define barycentric_triangle_convex(WA,WB,WC)\
    (0 <= ( WA ) && ( WA ) <= 1 && 0 <= ( WB ) && ( WB ) <= 1 && 0 <= ( WC ) && ( WC ) <= 1)
#define barycentric_triangle_convex_v(W)\
    barycentric_triangle_convex(( W ).vals[0], ( W ).vals[1], ( W ).vals[2])
    
// Get the intersection of the ray with the plane the triangle defines, in barycentric coordinates.
bool ray_triangle_plane_intersection_barycentric(vec3 origin, vec3 direction, vec3 a, vec3 b, vec3 c, vec3 *intersection)
{
    float wa = vec3_dot(direction, vec3_cross(vec3_sub(b, origin), vec3_sub(c, origin)));
    float wb = vec3_dot(direction, vec3_cross(vec3_sub(c, origin), vec3_sub(a, origin)));
    float wc = vec3_dot(direction, vec3_cross(vec3_sub(a, origin), vec3_sub(b, origin)));
    const float epsilon = 0.001;
    float w = wa + wb + wc;
    if (ABS(w) < epsilon || vec3_dot(vec3_sub(barycentric_triangle(a,b,c, wa,wb,wc), origin), direction) < 0) return false;
    float winv = 1.0 / w;
    wa *= winv;
    wb *= winv;
    wc *= winv;
    *intersection = new_vec3(wa,wb,wc);
    return true;
}
// Give the intersection as cartesian coordinates.
bool ray_triangle_plane_intersection(vec3 origin, vec3 direction, vec3 a, vec3 b, vec3 c, vec3 *intersection)
{
    vec3 inter;
    if (!ray_triangle_plane_intersection_barycentric(origin, direction, a, b, c, &inter)) return false;
    *intersection = barycentric_triangle_v(a,b,c, inter);
    return true;
}
// Get the intersection of the ray with the triangle, in barycentric coordinates,
bool ray_triangle_intersection_barycentric(vec3 origin, vec3 direction, vec3 a, vec3 b, vec3 c, vec3 *intersection)
{
    vec3 weights;
    if (!ray_triangle_plane_intersection_barycentric(origin, direction, a, b, c, &weights)) return false;
    if (!barycentric_triangle_convex_v(weights)) return false;
    *intersection = weights;
    return true;
}
// Give the intersection as cartesian coordinates.
bool ray_triangle_intersection(vec3 origin, vec3 direction, vec3 a, vec3 b, vec3 c, vec3 *intersection)
{
    vec3 weights;
    if (!ray_triangle_plane_intersection_barycentric(origin, direction, a, b, c, &weights)) return false;
    if (!barycentric_triangle_convex_v(weights)) return false;
    *intersection = barycentric_triangle_v(a,b,c, weights);
    return true;
}

// Ray-rectangle.
// --------------------------------------------------------------------------------
// Give the intersection of the ray with the plane spanned by a rectangle, given in terms of rectangular coordinates where tl (top-left)
// is (0,0), and br (bottom-right) is (1,1).
// tl----------tr
// |            | => (3/13, 2/3)
// |  x         |
// bl----------br
//-------Coordinates corrected.
bool ray_rectangle_plane_coordinates(vec3 origin, vec3 direction, vec3 tl, vec3 bl, vec3 br, vec3 tr, float *x, float *y)
{
    vec3 p;
    if (!ray_triangle_plane_intersection(origin, direction, tl, bl, br, &p)) return false;
    vec3 top_vector = vec3_sub(tr, tl);
    vec3 side_vector = vec3_sub(bl, tl);
    // Get the x-coordinate by projecting onto the line tl->tr.
    *x = vec3_dot(vec3_sub(p, tl), top_vector) / vec3_dot(top_vector, top_vector);
    // Get the y-coordinate by projecting onto the line tl->bl.
    *y = 1 - vec3_dot(vec3_sub(p, tl), side_vector) / vec3_dot(side_vector, side_vector);
    return true;
}
// Only detect intersection and give coordinates when the ray actually intersects with the rectangle, not just the plane it spans.
bool ray_rectangle_coordinates(vec3 origin, vec3 direction, vec3 tl, vec3 bl, vec3 br, vec3 tr, float *x, float *y)
{
    float xx,yy;
    if (!ray_rectangle_plane_coordinates(origin, direction, tl, bl, br, tr, &xx, &yy)) return false;
    if (xx < 0 || xx > 1 || yy < 0 || yy > 1) return false;
    *x = xx;
    *y = yy;
    return true;
}
// Give the point of intersection instead of coordinates.
bool ray_rectangle_intersection(vec3 origin, vec3 direction, vec3 tl, vec3 bl, vec3 br, vec3 tr, vec3 *intersection)
{
    float xx,yy;
    if (!ray_rectangle_plane_coordinates(origin, direction, tl, bl, br, tr, &xx, &yy)) return false;
    if (xx < 0 || xx > 1 || yy < 0 || yy > 1) return false;
    *intersection = vec3_lerp(vec3_lerp(tl, tr, xx), vec3_lerp(bl, br, xx), yy);
    return true;
}

bool ray_sphere_intersection(vec3 origin, vec3 direction, vec3 center, float radius, vec3 *intersection)
{
    // The sphere origin defines a plane orthogonal to the ray direction which the origin lies on. Compute
    // the intersection of the line with this plane, and do a circle test to check whether the line intersects the sphere.
    vec3 n = vec3_normalize(direction);
    vec3 p = vec3_add(origin, vec3_mul(n, vec3_dot(vec3_sub(center, origin), n)));
    vec3 pp = vec3_sub(p, center);
    float r_squared = vec3_dot(pp, pp);
    float radius_squared = radius*radius;
    if (r_squared > radius_squared) return false;
    // The intersection points are lifted off of the intersected plane.
    float h = sqrt(radius_squared - r_squared);
    // Return the closest intersection point to the ray origin, if it intersects.
    vec3 inter1 = vec3_sub(p, vec3_mul(n, h));
    if (vec3_dot(n, vec3_sub(inter1, origin)) >= 0) {
        *intersection = inter1;
        return true;
    }
    vec3 inter2 = vec3_add(p, vec3_mul(n, h));
    if (vec3_dot(n, vec3_sub(inter2, origin)) >= 0) {
        *intersection = inter2;
        return true;
    }
    return false;
}



/*--------------------------------------------------------------------------------
    Polyhedron methods.
--------------------------------------------------------------------------------*/
Polyhedron new_polyhedron(void)
{
    Polyhedron poly = {0};
    // If this function is not used to create a polyhedron, then these values triggering the calculation of number of features
    // won't be set.
    poly.num_points = -1;
    poly.num_edges = -1;
    poly.num_triangles = -1;
    return poly;
}


PolyhedronPoint *polyhedron_add_point(Polyhedron *polyhedron, vec3 point)
{
    PolyhedronPoint *p = calloc(1, sizeof(PolyhedronPoint));
    mem_check(p);
    p->position = point;
    dl_add(&polyhedron->points, p);
}
// It is up to the user of the polyhedron structure to maintain the fact that this is really does represent a polyhedron.
PolyhedronEdge *polyhedron_add_edge(Polyhedron *polyhedron, PolyhedronPoint *p1, PolyhedronPoint *p2)
{
    PolyhedronEdge *e = calloc(1, sizeof(PolyhedronEdge));
    mem_check(e);
    e->a = p1;
    e->b = p2;
    dl_add(&polyhedron->edges, e);
}
// Triangles are added through their edges, so these edges must actually form a triangle for this to make sense.

PolyhedronTriangle *polyhedron_add_triangle(Polyhedron *polyhedron, PolyhedronPoint *a, PolyhedronPoint *b, PolyhedronPoint *c, PolyhedronEdge *e1, PolyhedronEdge *e2, PolyhedronEdge *e3)
{
    PolyhedronTriangle *t = calloc(1, sizeof(PolyhedronTriangle));
    mem_check(t);
    // Get three unequal points from the edges (this complication is because in the polyhedron structure, feature ordering does not matter, only adjacency).
    // t->points[0] = e1->a;
    // t->points[1] = e2->a != e1->a ? e2->a : e2->b;
    // t->points[2] = e3->a != e1->a && e3->a != e2->a ? e3->a : e3->b;
    t->points[0] = a;
    t->points[1] = b;
    t->points[2] = c;
    // Add the triangle to the empty triangle slots of each edge.
    // if (e1->triangles[0] != NULL) t->adj[0] = e1->triangles[0];
    // if (e1->triangles[1] != NULL) t->adj[0] = e1->triangles[1];
    // if (e2->triangles[0] != NULL) t->adj[1] = e2->triangles[0];
    // if (e2->triangles[1] != NULL) t->adj[1] = e2->triangles[1];
    // if (e3->triangles[0] != NULL) t->adj[2] = e3->triangles[0];
    // if (e3->triangles[1] != NULL) t->adj[2] = e3->triangles[1];
    t->edges[0] = e1;
    t->edges[1] = e2;
    t->edges[2] = e3;

    e1->triangles[e1->triangles[0] == NULL ? 0 : 1] = t;
    e2->triangles[e2->triangles[0] == NULL ? 0 : 1] = t;
    e3->triangles[e3->triangles[0] == NULL ? 0 : 1] = t;
    dl_add(&polyhedron->triangles, t);
}
void polyhedron_remove_point(Polyhedron *poly, PolyhedronPoint *p)
{
    dl_remove(&poly->points, p);
}
void polyhedron_remove_edge(Polyhedron *poly, PolyhedronEdge *e)
{
    // Nullify incident triangles' reference to this edge.
    for (int i = 0; i < 2; i++) {
        if (e->triangles[i] == NULL) continue;
        for (int j = 0; j < 3; j++) {
            if (e->triangles[i]->edges[j] == e) e->triangles[i]->edges[j] = NULL;
        }
    }
    dl_remove(&poly->edges, e);
}
void polyhedron_remove_triangle(Polyhedron *poly, PolyhedronTriangle *t)
{
    // Nullify incident edges reference to this triangle.
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 2; j++) {
            if (t->edges[i] != NULL) if (t->edges[i]->triangles[j] == t) t->edges[i]->triangles[j] = NULL;
        }
    }
    dl_remove(&poly->triangles, t);
}
int polyhedron_num_points(Polyhedron *poly)
{
    if (poly->num_points == -1) {
        int n = 0;
        PolyhedronPoint *p = poly->points.first;
        while (p != NULL) { n++; p = p->next; }
        poly->num_points = n;
        return n;
    } else return poly->num_points;
}
int polyhedron_num_edges(Polyhedron *poly)
{
    if (poly->num_edges == -1) {
        int n = 0;
        PolyhedronEdge *e = poly->edges.first;
        while (e != NULL) { n++; e = e->next; }
        poly->num_edges = n;
        return n;
    } else return poly->num_edges;
}
int polyhedron_num_triangles(Polyhedron *poly)
{
    if (poly->num_triangles == -1) {
        int n = 0;
        PolyhedronTriangle *t = poly->triangles.first;
        while (t != NULL) { n++; t = t->next; }
        poly->num_triangles = n;
        return n;
    } else return poly->num_triangles;
}


void print_polyhedron(Polyhedron *p)
{
    PolyhedronPoint *point = p->points.first;
    printf("Points\n");
    printf("--------------------------------------------------------------------------------\n");
    int num_points = 0;
    while (point != NULL) {
        point->print_mark = num_points; // give this point a number.
        num_points ++;
        point = point->next;
    }
    printf("%d points\n", num_points);
    printf("Edges\n");
    printf("--------------------------------------------------------------------------------\n");
    PolyhedronEdge *edge = p->edges.first;
    int num_edges = 0;
    while (edge != NULL) {
        printf("e%d: %d->%d\n", num_edges, edge->a->print_mark, edge->b->print_mark);
        edge->print_mark = num_edges;
        num_edges ++;
        edge = edge->next;
    }
    printf("Triangles\n");
    printf("--------------------------------------------------------------------------------\n");
    PolyhedronTriangle *t = p->triangles.first;
    int num_triangles = 0;
    while (t != NULL) {
        printf("t%d : %d, %d, %d\n", num_triangles, t->points[0]->print_mark, t->points[1]->print_mark, t->points[2]->print_mark);
        t->print_mark = num_triangles;
        num_triangles ++;
        t = t->next;
    }
}


/*================================================================================
    3-dimensional convex hull. Returns the hull as a polyhedron.
================================================================================*/
// Definitions of marks this algorithm makes on features, during processing.
#define VISIBLE true
#define INVISIBLE false
#define NEEDED 0x1
#define BOUNDARY 0x2
Polyhedron convex_hull(vec3 *points, int num_points)
{
    //note: The auxilliary print marks are left as the indices of the points on the hull, if the caller wants these.
    Polyhedron poly = new_polyhedron();
    if (num_points == 3) {
        //---Assumes the triangle is non-degenerate.
        // Returns the triangle as a polyhedron structure.
        PolyhedronPoint *ps[3];
        for (int i = 0; i < 3; i++) {
            ps[i] = polyhedron_add_point(&poly, points[i]);
            ps[i]->print_mark = i;
        }
        PolyhedronEdge *e1 = polyhedron_add_edge(&poly, ps[0], ps[1]);
        PolyhedronEdge *e2 = polyhedron_add_edge(&poly, ps[1], ps[2]);
        PolyhedronEdge *e3 = polyhedron_add_edge(&poly, ps[2], ps[0]);
        polyhedron_add_triangle(&poly, ps[0], ps[1], ps[2], e1, e2, e3);
        return poly;
    }
    if (num_points < 3) {
        //--- Currently just returning the points.
        for (int i = 0; i < num_points; i++) {
            PolyhedronPoint *p = polyhedron_add_point(&poly, points[i]);
            p->print_mark = i;
        }
        return poly;
    }
    // Start up a polyhedron data structure as a tetrahedron.
    {
        PolyhedronPoint *tetrahedron_points[4];
        for (int i = 0; i < 4; i++) {
            tetrahedron_points[i] = polyhedron_add_point(&poly, points[i]);
            tetrahedron_points[i]->print_mark = i;
        }
        bool negative = tetrahedron_6_times_volume(points[0], points[1], points[2], points[3]) < 0;
        if (negative) {
            // Fix the orientation by swapping two of the points.
            PolyhedronPoint *temp = tetrahedron_points[0];
            tetrahedron_points[0] = tetrahedron_points[1];
            tetrahedron_points[1] = temp;
        }
        PolyhedronEdge *e1 = polyhedron_add_edge(&poly, tetrahedron_points[0], tetrahedron_points[1]);
        PolyhedronEdge *e2 = polyhedron_add_edge(&poly, tetrahedron_points[1], tetrahedron_points[2]);
        PolyhedronEdge *e3 = polyhedron_add_edge(&poly, tetrahedron_points[2], tetrahedron_points[0]);
        polyhedron_add_triangle(&poly, tetrahedron_points[0], tetrahedron_points[1], tetrahedron_points[2], e1, e2, e3);
        PolyhedronEdge *e4 = polyhedron_add_edge(&poly, tetrahedron_points[0], tetrahedron_points[3]);
        PolyhedronEdge *e5 = polyhedron_add_edge(&poly, tetrahedron_points[1], tetrahedron_points[3]);
        PolyhedronEdge *e6 = polyhedron_add_edge(&poly, tetrahedron_points[2], tetrahedron_points[3]);
        polyhedron_add_triangle(&poly, tetrahedron_points[3], tetrahedron_points[1], tetrahedron_points[0], e1, e5, e4);
        polyhedron_add_triangle(&poly, tetrahedron_points[3], tetrahedron_points[2], tetrahedron_points[1], e2, e6, e5);
        polyhedron_add_triangle(&poly, tetrahedron_points[3], tetrahedron_points[0], tetrahedron_points[2], e3, e4, e6);
    }
    for (int i = 4; i < num_points; i++) {
        // Clean up the marks for points and edges (not neccessary for triangles, since they are always set).
        PolyhedronPoint *p = poly.points.first;
        while (p != NULL) {
            p->mark = 0;
            p->saved_edge = NULL;
            p = p->next;
        }
        PolyhedronEdge *e = poly.edges.first;
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
        PolyhedronTriangle *t = poly.triangles.first;
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
            continue; // The point is in the hull so far.
        }
        // Before deleting anything, use the visibility information to mark the borders. Then when triangles are deleted, the borders are still known.
        // This could be done more cleanly without looping over features so many times.
        e = poly.edges.first;
        while (e != NULL) {
            if (e->triangles[0]->mark != e->triangles[1]->mark) {
                e->mark |= BOUNDARY;
            }
            e = e->next;
        }
        // Remove all visible triangles (that weren't just added), and unneeded points and edges.
        t = poly.triangles.first;
        while (t != NULL) {
            if (t->mark == VISIBLE) {
                PolyhedronTriangle *to_remove = t;
                t = t->next;
                polyhedron_remove_triangle(&poly, to_remove);
            } else t = t->next;
        }
        e = poly.edges.first;
        while (e != NULL) {
            if ((e->mark & NEEDED) == 0) {
                PolyhedronEdge *to_remove = e;
                e = e->next;
                polyhedron_remove_edge(&poly, to_remove);
                continue;
            }
            e = e->next;
        }
        p = poly.points.first;
        while (p != NULL) {
            if ((p->mark & NEEDED) == 0) {
                PolyhedronPoint *to_remove = p;
                p = p->next;
                polyhedron_remove_point(&poly, to_remove);
                continue;
            }
            p = p->next;
        }
        // Add the new point, and new edges and triangles to create a cone from the new point to the border.
        PolyhedronPoint *new_point = polyhedron_add_point(&poly, points[i]);
        new_point->print_mark = i;
        e = poly.edges.first;
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
                    e1 = polyhedron_add_edge(&poly, new_point, p1);
                    p1->saved_edge = e1;
                }
                PolyhedronEdge *e2 = p2->saved_edge;
                if (e2 == NULL) {
                    e2 = polyhedron_add_edge(&poly, new_point, p2);
                    p2->saved_edge = e2;
                }
                // Use these to form a new triangle.
                polyhedron_add_triangle(&poly, new_point, p1, p2, e1, e, e2);
            }
            e = e->next;
        }
    }
    return poly;
}


bool point_in_convex_polyhedron(vec3 p, Polyhedron poly)
{
    // Uses "visibility" from the point. If any triangle is visible (as in, from the point of view of the point, the triangle is non-degenerate and in anti-clockwise order),
    // then the point is outside the convex polyhedron.
    bool any_visible = false;
    PolyhedronTriangle *t = poly.triangles.first;
    while (t != NULL) {
        float v = tetrahedron_6_times_volume(t->points[0]->position, t->points[1]->position, t->points[2]->position, p);
        if (v < 0) {
            any_visible = true;
        }
        t = t->next;
    }
    return !any_visible;
}

float polyhedron_volume(Polyhedron poly)
{
    float volume = 0.0;
    PolyhedronTriangle *t = poly.triangles.first;
    vec3 zero = vec3_zero();
    while (t != NULL) {
        volume += tetrahedron_6_times_volume(t->points[0]->position, t->points[1]->position, t->points[2]->position, zero);
        t = t->next;
    }
    return volume / 6.0;
}

// This is for a general polyhedron, so no hill-climbing is done.
vec3 polyhedron_extreme_point(Polyhedron poly, vec3 direction)
{
    PolyhedronPoint *p = poly.points.first;
    float d = vec3_dot(p->position, direction);
    vec3 v = p->position;
    while ((p = p->next) != NULL) {
        float new_d = vec3_dot(p->position, direction);
        if (new_d > d) {
            d = new_d;
            v = p->position;
        }
    }
    return v;
}

// Extract the points from the polyhedron as an array on the heap.
vec3 *polyhedron_points(Polyhedron poly)
{
    int n = polyhedron_num_points(&poly);
    vec3 *points = malloc(sizeof(vec3) * n);
    mem_check(points);
    PolyhedronPoint *p = poly.points.first;
    int i = 0;
    while (p != NULL) {
        points[i++] = p->position;
        p = p->next;
    }
    return points;
}

vec3 polyhedron_center_of_mass(Polyhedron poly)
{
    // Calculate the center of mass (assuming the mass is uniform).
    PolyhedronTriangle *tri = poly.triangles.first;
    vec3 center_of_mass = new_vec3(0,0,0);
    // The center of mass is found by taking the weighted sum of the centroids of tetrahedra connecting the origin to each triangle,
    // weighted by the signed volumes of each tetrahedron.
    // ---I think this method works, at least for convex polyhedra, since if the origin is in the center,
    // ---then this works, and it would not become incorrect when moving the origin past the boundary.
    float volume_times_6 = 0;
    while (tri != NULL) {
        // Calculate the centroid of the tetrahedron containing the origin and the points of the triangle.
        vec3 centroid = vec3_mul(vec3_add(vec3_add(tri->points[0]->position, tri->points[1]->position), tri->points[2]->position), 0.25);
        // Calculate the volume of this same tetrahedron.
        float v = tetrahedron_6_times_volume(new_vec3(0,0,0), tri->points[0]->position, tri->points[1]->position, tri->points[2]->position);
        volume_times_6 += v;
        center_of_mass = vec3_add(center_of_mass, vec3_mul(centroid, v));
        tri = tri->next;
    }
    center_of_mass = vec3_mul(center_of_mass, 1.0 / volume_times_6);
    return center_of_mass;
}


/*--------------------------------------------------------------------------------
    Polytope methods. Polytopes are represented by only their points, and their polyhedron can be recovered at any time by taking the convex hull.
    (Polyhedron representation may not be consistent due to triangulation of faces).
--------------------------------------------------------------------------------*/
vec3 polytope_center_of_mass(vec3 *points, int num_points)
{
    Polyhedron hull = convex_hull(points, num_points);
    vec3 center_of_mass = polyhedron_center_of_mass(hull);
    //----------////// Destroy the polyhedron.
    return center_of_mass;
}

vec3 polytope_extreme_point(vec3 *points, int num_points, vec3 direction)
{
    if (num_points == 0) {
        fprintf(stderr, "ERROR: polytope_extreme_point: Need at least one point.\n");
        exit(EXIT_FAILURE);
    }
    vec3 p = points[0];
    float d = vec3_dot(p, direction);
    for (int i = 1; i < num_points; i++) {
        float new_d = vec3_dot(points[i], direction);
        if (new_d > d) {
            d = new_d;
            p = points[i];
        }
    }
    return p;
}

// Shapes and object creation.
// ---------------------------
Polyhedron make_block(float width, float height, float depth)
{
    vec3 points[8];
    points[0] = new_vec3(-width/2, height/2, depth/2);
    points[1] = new_vec3(-width/2, -height/2, depth/2);
    points[2] = new_vec3(width/2, -height/2, depth/2);
    points[3] = new_vec3(width/2, height/2, depth/2);
    points[4] = new_vec3(-width/2, height/2, -depth/2);
    points[5] = new_vec3(-width/2, -height/2, -depth/2);
    points[6] = new_vec3(width/2, -height/2, -depth/2);
    points[7] = new_vec3(width/2, height/2, -depth/2);
    return convex_hull(points, 8);
}
