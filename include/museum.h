#ifndef MUSEUM_H
#define MUSEUM_H

// #include "glad/glad.h"
#include <GL/freeglut.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include "helper_definitions.h"
#include "mathematics.h"
#include "doubly_linked_list.h"
#include "entities.h"
#include "input.h"
#include "geometry.h"
#include "collision.h"
#include "camera.h"
#include "control_widget.h"
#include "player.h"
#include "textures.h"
#include "rendering.h"
#include "models.h"
#include "trackball.h"

#define GRAY new_vec4(0.5,0.5,0.5,1)
#define LIGHT_GRAY new_vec4(0.65,0.65,0.65,1)
#define PINK new_vec4(1,0.83,0.78,1);
#define DARK_GREEN new_vec4(0.03,0.34,0.0,1)
#define GREEN new_vec4(0.12,0.98,0.012,1)
#define BLUE new_vec4(0.3, 0.3, 0.95, 1)
#define RED new_vec4(1,0,0.1,1)
#define WHITE new_vec4(1,1,1,1)
#define BLACK new_vec4(0,0,0,1)

#define DEFAULT_ASPECT_RATIO 0.5616
extern float aspect_ratio;

extern vec3 player_start_position;
extern float total_time;
extern float dt;
extern int window_width;
extern int window_height;
extern mat4x4 view_matrix;
extern Camera *main_camera;
extern Entity *main_camera_entity;

extern float gravity_constant;

enum BehaviourIDs {
    NoID,
    ColliderID,
    PolyhedronRendererID,
    PlayerControllerID,
    CameraID,
    ControlWidgetID,
    RigidBodyID,
    BezierSurfaceRendererID,
    NUM_BEHAVIOUR_TYPES
};

void create_exhibit_convex_hull(void);
void create_exhibit_rigid_body_dynamics(void);
void create_exhibit_curves_and_surfaces(void);
void create_exhibit_interactions(void);

#endif // MUSEUM_H
