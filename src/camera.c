#include "museum.h"

void camera_update(Entity *e, Behaviour *b)
{
    Camera *camera = (Camera *) b->data;

    // Construct the projection matrix from the camera data,
    // and make it the active projection matrix.
    float r,t,n;
    r = camera->near_half_width;
    t = camera->aspect_ratio * camera->near_half_width;
    n = camera->near_plane_distance;
    mat4x4 projection_matrix;
    fill_mat4x4_rmaj(projection_matrix, 1/r,   0,   0,    0,
                                          0,   1/t, 0,    0,
                                          0,   0,   -1/n,  -1,
                                          0,   0,   -1/n,  0);
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(projection_matrix.vals);

    // Store the view matrix globally.
    mat4x4 view_inv;
    view_matrix = rigid_mat4x4_inverse(entity_matrix(e));
}

// Bottom-left of camera rectangle is (0,0), top-right is (1,1).
// This method gives the origin and direction of a ray cast outward from the position of the camera,
// starting on the near plane.
void camera_ray(Entity *camera_entity, Camera *camera, float x, float y, vec3 *origin, vec3 *direction)
{
    mat4x4 matrix = entity_matrix(camera_entity);

    vec3 camera_space_p = new_vec3(camera->near_half_width*(2*x - 1),
                                   camera->aspect_ratio*camera->near_half_width*(2*y - 1),
                                   -camera->near_plane_distance);
    *origin = rigid_matrix_vec3(matrix, camera_space_p);
    *direction = vec3_sub(*origin, camera_entity->position);
}

