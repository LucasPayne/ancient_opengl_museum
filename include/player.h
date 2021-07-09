#ifndef PLAYER_H
#define PLAYER_H

typedef struct PlayerController_s {
    Collider *collider;
    vec3 velocity;
    bool flying;
    float toggle_flying_timer;

    float bob;
    bool bob_up;
} PlayerController;
void player_controller_key_listener(Entity *e, Behaviour *b, unsigned char key);
void player_controller_update(Entity *e, Behaviour *b);
void create_player(vec3 start_position, float camera_near_plane, float camera_far_plane, float camera_near_half_width);

#endif // PLAYER_H
