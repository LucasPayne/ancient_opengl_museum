#ifndef CAMERA_H
#define CAMERA_H

typedef struct Camera_s {
    float near_plane_distance;
    float far_plane_distance;
    float near_half_width;
    float aspect_ratio;
} Camera;
void camera_update(Entity *e, Behaviour *b);
void camera_ray(Entity *camera_entity, Camera *camera, float x, float y, vec3 *origin, vec3 *direction);

#endif // CAMERA_H
