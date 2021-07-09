#include "museum.h"

static trackball_stop_dragging(TrackBall *tb)
{
    vec3 p = tb->last_ball_point;
    vec3 pp = tb->last_last_ball_point;
    float x = vec3_dot(p, pp) / (tb->radius*tb->radius);
    if (x > 1) x = 1;
    if (x < -1) x = -1;
    const float theta_max = 2*M_PI;
    float theta = acos(x);
    if (theta > theta_max) theta = theta_max;
    vec3 axis = vec3_normalize(vec3_cross(pp, p));
    tb->angular_velocity = vec3_mul(axis, theta / (dt == 0 ? 0.01 : dt));
    tb->dragging = false;
}

void trackball_update(Entity *e, Behaviour *b)
{
    TrackBall *tb = (TrackBall *) b->data;
    if (vec3_dot(tb->angular_velocity, tb->angular_velocity) > 1.3) {
        tb->angular_velocity = vec3_mul(tb->angular_velocity, 1.0 - 2*dt);
    }
    vec3 w = tb->angular_velocity;
    mat3x3 skew;
    fill_mat3x3_rmaj(skew, 0,    -Z(w),  Y(w),
                           Z(w),     0, -X(w),
                           -Y(w), X(w),     0);
    e->orientation = mat3x3_add(e->orientation, mat3x3_mul(mat3x3_multiply(skew, e->orientation), dt));
    mat3x3_orthonormalize(&e->orientation);
}
void trackball_mouse_listener(Entity *e, Behaviour *b, int button, int state, float x,  float y)
{

    TrackBall *tb = (TrackBall *) b->data;

    if (tb->dragging) {
        if (state == GLUT_UP && button == GLUT_LEFT_BUTTON) {
            trackball_stop_dragging(tb);
        }
    } else {
        if (state == GLUT_DOWN && button == GLUT_LEFT_BUTTON) {
            vec3 ray_origin, ray_direction;
            camera_ray(main_camera_entity, main_camera, x, y, &ray_origin, &ray_direction);
            vec3 intersection;
            vec3 dummy; // needs to be passed to ray model intersection.
            if (tb->check_model && !ray_model_intersection(ray_origin, ray_direction, tb->model, entity_matrix(e), &dummy)) {
                // The trackball is set to only be controllable when the model is clicked.
            } else if (ray_sphere_intersection(ray_origin, ray_direction, e->position, tb->radius, &intersection)) {
                tb->last_ball_point = vec3_sub(intersection, e->position);
                tb->last_last_ball_point = tb->last_ball_point;
                tb->dragging = true;
                tb->angular_velocity = vec3_zero();
	    }
        }
    }
}
void trackball_mouse_motion_listener(Entity *e, Behaviour *b, float x, float y)
{
    TrackBall *tb = (TrackBall *) b->data;
    if (tb->dragging) {
        vec3 ray_origin, ray_direction;
        camera_ray(main_camera_entity, main_camera, x, y, &ray_origin, &ray_direction);
        vec3 intersection;
        if (ray_sphere_intersection(ray_origin, ray_direction, e->position, tb->radius, &intersection)) {

            vec3 ball_point = vec3_sub(intersection, e->position);
            vec3 axis = vec3_normalize(vec3_cross(ball_point, tb->last_ball_point));
            float x = vec3_dot(ball_point, tb->last_ball_point) * 1.0 / (tb->radius*tb->radius);
            if (x > 1) x = 1;
            if (x < -1) x = -1;
            float theta = acos(x);
            e->orientation = axis_angle_rotate_mat3x3(e->orientation, axis, theta);
            mat3x3_orthonormalize(&e->orientation); // Prevent matrix-drift.

	    tb->last_last_ball_point = tb->last_ball_point;
	    tb->last_ball_point = ball_point;
        } else {
            trackball_stop_dragging(tb);
        }
    }
}

TrackBall *add_trackball(Entity *e, float radius)
{
    Behaviour *b = add_behaviour(e, trackball_update, sizeof(TrackBall), NoID);
    b->mouse_listener = trackball_mouse_listener;
    b->mouse_motion_listener = trackball_mouse_motion_listener;
    TrackBall *tb = (TrackBall *) b->data;
    tb->radius = radius;
    tb->dragging = false;
    tb->check_model = false;
    tb->model = NULL;
    return tb;
}


