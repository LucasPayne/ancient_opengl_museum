#ifndef TRACKBALL_H
#define TRACKBALL_H

typedef struct TrackBall_s {
    bool dragging;
    float radius;
    bool check_model; // Only start dragging if the model has also been clicked.
    Model *model;

    vec3 last_ball_point;
    vec3 last_last_ball_point;
    vec3 angular_velocity;
} TrackBall;

TrackBall *add_trackball(Entity *e, float radius);
void trackball_update(Entity *e, Behaviour *b);
void trackball_mouse_listener(Entity *e, Behaviour *b, int button, int state, float x,  float y);
void trackball_mouse_motion_listener(Entity *e, Behaviour *b, float x, float y);

#endif // TRACKBALL_H
