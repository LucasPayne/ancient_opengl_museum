#include "museum.h"

#define trace 0

#define in(BIT_NUMBER,MASK) ( (( MASK ) & (1 << ( BIT_NUMBER ))) != 0 )
void color_connected(int edge_component[], int vertex_mask, int vertex_colors[], int edge_colors[], int start_index, int color, int *edge_component_index)
{
    vertex_colors[start_index] = color;
    if (start_index < 4) {
        #define color_edge(INDEX) {\
            bool found = false;\
            for (int i = 0; i < (*edge_component_index); i++) {\
                if (edge_component[i] == ( INDEX )) {\
                    found = true;\
                    break;\
                }\
            }\
            if (!found && edge_colors[( INDEX )] == 0) {\
                edge_colors[( INDEX )] = color;\
                edge_component[*edge_component_index] = ( INDEX );\
                (*edge_component_index) ++;\
            }\
        }
        color_edge((start_index + 3) % 4);
        color_edge(start_index);
        color_edge(start_index + 4);
        #define propogate(INDEX) {\
            if (in(( INDEX ), vertex_mask) && vertex_colors[( INDEX )] == 0) {\
                vertex_colors[( INDEX )] = color;\
                color_connected(edge_component, vertex_mask, vertex_colors, edge_colors, ( INDEX ), color, edge_component_index);\
            }\
        }
        propogate((start_index + 1) % 4);
        propogate((start_index + 3) % 4);
        propogate(start_index + 4);
    } else {
        color_edge((start_index + 3) % 4 + 8);
        color_edge(start_index + 4);
        color_edge(start_index);
        propogate((start_index+3) % 4 + 4);
        propogate((start_index+1)%4 + 4);
        propogate(start_index - 4);
    }
#if trace
    printf("start: %d, edges_in: %d, color: %d\n", start_index, *edge_component_index, color);
    printf("vertices: [");
    for (int i = 7; i >= 0; --i) printf((vertex_mask & (1 << i)) == 0 ? "0, " : "1, ");
    printf("]\n");
    printf("component: [");
    for (int i = 0; i < (*edge_component_index); i++) printf("%d, ", edge_component[i]);
    printf("]\n");
    printf("vertex colors: (");
    for (int i = 0; i < 8; i++) {
        printf("%d, ", vertex_colors[i]);
    }
    printf(")\n");
    getchar();
#endif
}

void generate_marching_cubes_table(void)
{
    const vec3 cube_points[8] = {
        {{0,0,0}},
        {{1,0,0}},
        {{1,1,0}},
        {{0,1,0}},

        {{0,0,1}},
        {{1,0,1}},
        {{1,1,1}},
        {{0,1,1}},
    };
    const vec3 edge_points[12] = {
        {{0.5,0,0}},
        {{1,0.5,0}},
        {{0.5,1,0}},
        {{0,0.5,0}},

        {{0,0,0.5}},
        {{1,0,0.5}},
        {{1,1,0.5}},
        {{0,1,0.5}},

        {{0.5,0,1}},
        {{1,0.5,1}},
        {{0.5,1,1}},
        {{0,0.5,1}},
    };

    const int max_triangles = 32;
    int16_t *triangle_table = malloc(sizeof(int16_t) * 256*3*max_triangles);
    mem_check(triangle_table);
    for (int i = 0; i < 256*3*max_triangles; i++) triangle_table[i] = -1;
    // triangle_table[22*3*max_triangles + 3*4 + 1] is the second vertex index the 5th triangle in the 23rd triangle list (the triangle list for case 00010110).

    for (int vertices = 0; vertices <= 255; vertices++) {
        #if trace
        printf("New set.\n");
        printf("----------\n");
        #endif
        // Compute the bits representing the set of edges.
        int edges = 0;
        for (int i = 0; i < 4; i++) {
            if (in(i, vertices)) {
                edges |= ((i-1)%4) << 1;
                edges |= i << 1;
                edges |= (i+4) << 1;
            }
            if (in(i+4, vertices)) {
                edges |= (i+8) << 1;
                edges |= ((i-1)%4+8) << 1;
                edges |= (i+4) << 1;
            }
        }
        // Consider the graph of edge-midpoints where two midpoints have an edge if their corresponding edges share a vertex on the cube which is in the vertex set.
        // The faces on the convex hulls of the connected components of this graph which point inward to the cube will form the triangles.

        int vertex_colors[8] = {0};
        int edge_colors[12] = {0};
        // Construct and iterate over the connected components of this graph, adding their triangles to the triangle list.
        int tri_index = 0;
        for (int i = 0; i < 8; i++) {
            if (!in(i, vertices)) continue;
            if (vertex_colors[i] != 0) continue;
            int edge_component[12] = {0};
            int edge_component_index = 0;
            #if trace
            printf("New component.\n");
            printf("----------\n");
            #endif
            color_connected(edge_component, vertices, vertex_colors, edge_colors, i, i+1, &edge_component_index);
            vec3 points[12];
            for (int j = 0; j < edge_component_index; j++) {
                points[j] = edge_points[edge_component[j]];
            }
            // points now contains edge_component_index points which form the connected component.
            // Take the convex hull of these points. The hull algorithm leaves the print_mark of the points as their indices.
            Polyhedron hull = convex_hull(points, edge_component_index);
            PolyhedronTriangle *tri = hull.triangles.first;
            while (tri != NULL) {
                //---Check if this triangle is on a cube face. If so, don't add it.
                triangle_table[vertices*3*max_triangles + 3*tri_index + 0] = edge_component[tri->points[0]->print_mark];
                triangle_table[vertices*3*max_triangles + 3*tri_index + 1] = edge_component[tri->points[1]->print_mark];
                triangle_table[vertices*3*max_triangles + 3*tri_index + 2] = edge_component[tri->points[2]->print_mark];
                tri_index ++;
                tri = tri->next;
            }
        }
    }
    printf("#define max_marching_cube_triangles %d\n", max_triangles);
    printf("const int16_t marching_cubes_table[256][3*max_marching_cube_triangles] = {\n");
    for (int i = 0; i < 256; i++) {
        printf("    // ");
        for (int j = 7; j >= 0; --j) {
            printf((i & (1 << j)) == 0 ? "0" : "1");
        }
        printf("\n");
        printf("    {");
        for (int j = 0; j < max_triangles; j++) {
            printf("%d, %d, %d,", triangle_table[i*3*max_triangles + 3*j + 0],
                                      triangle_table[i*3*max_triangles + 3*j + 1],
                                      triangle_table[i*3*max_triangles + 3*j + 2]);
            if (j % 10 == 0) printf("\n        ");
        }
        printf(" },\n");
    }
    printf("};\n");
}

int main(void)
{
    generate_marching_cubes_table();
}

