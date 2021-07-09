#include "museum.h"

#define FLYING_ENABLED true

void player_controller_key_listener(Entity *e, Behaviour *b, unsigned char key)
{
    float jumpiness = 6.5;
    PlayerController *player = (PlayerController *) b->data;
    #if FLYING_ENABLED
    if (key == ' ') {
        if (!player->flying) Y(player->velocity) = jumpiness;
        if (player->toggle_flying_timer > 0) {
            player->flying = !player->flying;
            player->toggle_flying_timer = 0;
        }
        player->toggle_flying_timer = 0.7;
    }
    #endif
    if (key == 'r') {
        e->position = player_start_position;
        e->euler_angles = vec3_zero();
        player->velocity = vec3_zero();
    }
    // if (key == 'c') {
    //     printf("Player position:\n");
    //     print_vec3(e->position);
    //     getchar();
    // }
}
void player_controller_update(Entity *e, Behaviour *b)
{
    PlayerController *player = (PlayerController *) b->data;
    #if FLYING_ENABLED
    player->toggle_flying_timer -= dt;
    #endif

    #if FLYING_ENABLED
    if (!player->flying) Y(player->velocity) -= gravity_constant * dt;
    #else
    Y(player->velocity) -= gravity_constant * dt;
    #endif 
    if (vec3_dot(player->velocity, player->velocity) > 0.1*0.1) e->position = vec3_add(e->position, vec3_mul(player->velocity, dt));

    for_behaviour(Collider, collider, collider_entity)
        if (collider == player->collider) continue;
        if (!collider_bounding_test(player->collider, e, collider, collider_entity)) continue;

        #if 0
        // Visualize culling, for debugging.
        prepare_entity_matrix(collider_entity);
        glDisable(GL_LIGHTING);
        glDisable(GL_DEPTH);
        glBegin(GL_LINE_STRIP);
        glColor3f(0,1,0);
        for (int i = 0; i < collider->num_points; i++) {
            glVertex3f(UNPACK_VEC3(collider->points[i]));
        }
        glEnd();
        #endif

        GJKManifold contact_manifold;
        mat4x4 player_matrix = entity_matrix(e);
        mat4x4 object_matrix = entity_matrix(collider_entity);
        if (!convex_hull_intersection(player->collider->points,
                                      player->collider->num_points,
                                      player_matrix,
                                      collider->points,
                                      collider->num_points,
                                      object_matrix,
                                      &contact_manifold)) continue;
        if (vec3_dot(player->velocity, contact_manifold.separating_vector) <= 0) continue;
        vec3 n = vec3_normalize(contact_manifold.separating_vector);
        player->velocity = vec3_sub(player->velocity, vec3_mul(n, vec3_dot(player->velocity, n)));

        e->position = vec3_add(e->position, vec3_neg(contact_manifold.separating_vector));
    end_for_behaviour()

    // Turning.
    float turn_speed = 2;
    if (arrow_key_down(Left)) {
        Y(e->euler_angles) -= turn_speed * dt;
    }
    if (arrow_key_down(Right)) {
        Y(e->euler_angles) += turn_speed * dt;
    }

    // Walking forward and backward, and strafing.
    float walk_speed = 6;
    float strafe_speed = walk_speed;
    float slow_down_factor = 7.8;

    // Slow the player down instead of abruptly stopping.
    if (X(player->velocity) > 0) {
        X(player->velocity) -= walk_speed * slow_down_factor * dt;
        if (X(player->velocity) < 0) X(player->velocity) = 0;
    } else if (X(player->velocity) < 0) {
        X(player->velocity) += walk_speed * slow_down_factor * dt;
        if (X(player->velocity) > 0) X(player->velocity) = 0;
    }
    if (Z(player->velocity) > 0) {
        Z(player->velocity) -= walk_speed * slow_down_factor * dt;
        if (Z(player->velocity) < 0) Z(player->velocity) = 0;
    } else if (Z(player->velocity) < 0) {
        Z(player->velocity) += walk_speed * slow_down_factor * dt;
        if (Z(player->velocity) > 0) Z(player->velocity) = 0;
    }

    mat3x3 orientation = entity_orientation(e);
    vec3 forward = matrix_vec3(orientation, new_vec3(0,0,-1));
    vec3 right = matrix_vec3(orientation, new_vec3(1,0,0));
    bool walking = false;
    #if FLYING_ENABLED
    if (alt_arrow_key_down(Up)) {
    #else
    if (alt_arrow_key_down(Up) || arrow_key_down(Up)) {
    #endif
        X(player->velocity) = X(forward) * walk_speed;
        Z(player->velocity) = Z(forward) * walk_speed;
        walking = true;
    }
    #if FLYING_ENABLED
    if (alt_arrow_key_down(Down)) {
    #else
    if (alt_arrow_key_down(Down) || arrow_key_down(Down)) {
    #endif
        X(player->velocity) = (walking ? X(player->velocity) : 0) + X(forward) * -walk_speed;
        Z(player->velocity) = (walking ? Z(player->velocity) : 0) + Z(forward) * -walk_speed;
        walking = true;
    }
    if (alt_arrow_key_down(Right)) {
        X(player->velocity) = (walking ? X(player->velocity) : 0) + X(right) * strafe_speed;
        Z(player->velocity) = (walking ? Z(player->velocity) : 0) + Z(right) * strafe_speed;
        walking = true;
    }
    if (alt_arrow_key_down(Left)) {
        X(player->velocity) = (walking ? X(player->velocity) : 0) + X(right) * -strafe_speed;
        Z(player->velocity) = (walking ? Z(player->velocity) : 0) + Z(right) * -strafe_speed;
        walking = true;
    }
    if (walking) {
        if (player->bob_up) {
            player->bob += 0.23 * dt;
            if (player->bob > 0.1) {
                player->bob = 0.1;
                player->bob_up = false;
            }
        } else {
            player->bob -= 0.23 * dt;
            if (player->bob < -0.1) {
                player->bob = -0.1;
                player->bob_up = true;
            }
        }
    } else {
        player->bob_up = true;
        player->bob = player->bob * (1.0 - 0.29*dt);
    }

    #if FLYING_ENABLED
    float fly_vertical_speed = 5;
    if (player->flying) {
        Y(player->velocity) = 0;
        if (!arrow_key_down(Up) || !arrow_key_down(Down)) {
            if (arrow_key_down(Up)) Y(player->velocity) = fly_vertical_speed;
            else if (arrow_key_down(Down)) Y(player->velocity) = -fly_vertical_speed;
        }
    }
    #endif
}

void create_player(vec3 start_position, float camera_near_plane, float camera_far_plane, float camera_near_half_width)
{
    // Create the player as an entity with update and key-listening functions.
    Entity *player = add_entity(start_position, new_vec3(0,0,0));
    player->euler_controlled = true;

    // The player collider is a capsule. This means that the player can walk up things like stairs.
    Model player_capsule = make_capsule(0.6, 1.9);
    Collider *collider = add_collider(player, player_capsule.vertices, player_capsule.num_vertices, true);

    Behaviour *controller_behaviour = add_behaviour(player, player_controller_update, sizeof(PlayerController), PlayerControllerID);
    controller_behaviour->key_listener = player_controller_key_listener;
    PlayerController *controller = (PlayerController *) controller_behaviour->data;
    controller->collider = collider; // Let the player controller reference the player collider.

    // The camera is a separate behaviour added to the player. This is so the camera logic isn't tied to the player, the player just controls its specific camera.
    // The main camera can then be set to another camera in the world if wanted.
    Camera *player_camera = (Camera *) (add_behaviour(player, camera_update, sizeof(Camera), CameraID)->data);
    player_camera->near_plane_distance = camera_near_plane;
    player_camera->far_plane_distance = camera_far_plane;
    player_camera->near_half_width = camera_near_half_width;
    player_camera->aspect_ratio = aspect_ratio;

    // Set the global camera to the player camera.
    main_camera = player_camera;
    main_camera_entity = player;
}

