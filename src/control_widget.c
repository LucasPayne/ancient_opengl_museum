#include "museum.h"

static const vec3 widget_axis_colors[] = {{{1,0,0}},{{0,1,0}},{{0,0,1}}};
static const vec3 widget_axes[] = {{{1,0,0}}, {{0,1,0}}, {{0,0,1}}};
void control_widget_update(Entity *e, Behaviour *b)
{
    ControlWidget *widget = (ControlWidget *) b->data;
    prepare_entity_matrix(e);
    glLineWidth(10);
    deactivate_sun();
    glDisable(GL_DEPTH);
    glBegin(GL_LINES);
    for (int i = 0; i < 3; i++) {
        glColor3f(UNPACK_VEC3(widget_axis_colors[i]));
        glVertex3f(0,0,0);
        glVertex3f(UNPACK_VEC3(vec3_mul(widget_axes[i], widget->size)));
    }
    glEnd();
    
    glBegin(GL_QUADS);
    for (int i = 0; i < 3; i++) {
        vec3 e1 = vec3_mul(widget_axes[i], widget->size);
        vec3 e2 = vec3_mul(widget_axes[(i+1)%3], widget->size);
        vec3 color = vec3_lerp(widget_axis_colors[i], widget_axis_colors[(i+1)%3], 0.5);
        vec3 points[4];
        points[0] = vec3_zero();
        float multiplier = widget->dragging && widget->dragging_plane == i ? 1 : 0.78;
        points[1] = vec3_mul(e1, multiplier);
        points[3] = vec3_mul(e2, multiplier);
        points[2] = vec3_add(points[1], points[3]);
        glColor3f(UNPACK_VEC3(color));
        for (int j = 0; j < 4; j++) glVertex3f(UNPACK_VEC3(points[j]));
    }
    glEnd();
/*
    glLineWidth(20);
    if (widget->dragging) {
        glColor3f(0,0,0);
        glBegin(GL_LINES);
            glVertex3f(0,0,0);
            glVertex3f(UNPACK_VEC3(rigid_matrix_vec3(rigid_mat4x4_inverse(entity_matrix(e)), widget->drag_start_point)));
        glEnd();
    }
*/
    glEnable(GL_DEPTH);
    activate_sun();
}
void control_widget_mouse_listener(Entity *e, Behaviour *b, int button, int state, float x,  float y)
{
    ControlWidget *widget = (ControlWidget *) b->data;
    if (widget->dragging) {
        if (state == GLUT_UP && button == GLUT_LEFT_BUTTON) {
            widget->dragging = false;
        }
    } else {
        if (state == GLUT_DOWN && button == GLUT_LEFT_BUTTON) {
            // When clicking, check a ray against three axis-aligned rectangles. If any are intersected, the closest one is
            // the one chosen.
	    vec3 ray_origin, ray_direction;
            camera_ray(main_camera_entity, main_camera, x, y, &ray_origin, &ray_direction);
            int clicked = -1; 
            float min_distance = 0;
            mat4x4 matrix = entity_matrix(e);
	    vec3 drag_start_point;
            for (int i = 0; i < 3; i++) {
                vec3 points[4];
                points[0] = vec3_zero();
                points[1] = vec3_mul(widget_axes[i], 0.78 * widget->size);
                points[3] = vec3_mul(widget_axes[(i+1)%3], 0.78 * widget->size);
                points[2] = vec3_add(points[1], points[3]);
                for (int i = 0; i < 4; i++) points[i] = rigid_matrix_vec3(matrix, points[i]);
	        vec3 intersection;
                if (!ray_rectangle_intersection(ray_origin, ray_direction, points[0], points[1], points[2], points[3], &intersection)) continue;

                float distance = vec3_dot(vec3_sub(intersection, ray_origin), vec3_sub(intersection, ray_origin)); // distance-squared.
                if (clicked == -1 || distance < min_distance) {
                    clicked = i;
                    min_distance = distance;
                    drag_start_point = intersection;
                }
            }
            if (clicked != -1) {
                widget->dragging = true;
                widget->dragging_plane = clicked;
                widget->drag_offset = vec3_sub(e->position, drag_start_point);
            }
        }
    }
}
void control_widget_mouse_motion_listener(Entity *e, Behaviour *b, float x, float y)
{
    ControlWidget *widget = (ControlWidget *) b->data;
    mat4x4 matrix = entity_matrix(e);
    if (widget->dragging) {
        vec3 ray_origin, ray_direction;
        camera_ray(main_camera_entity, main_camera, x, y, &ray_origin, &ray_direction);
        vec3 a = rigid_matrix_vec3(matrix, vec3_zero());
        vec3 b = rigid_matrix_vec3(matrix, widget_axes[widget->dragging_plane]);
        vec3 c = rigid_matrix_vec3(matrix, widget_axes[(widget->dragging_plane+1)%3]);
        vec3 intersection;
	if (ray_triangle_plane_intersection(ray_origin, ray_direction, a, b, c, &intersection)) {
            e->position = vec3_add(widget->drag_offset, intersection);
        }
    }
}

ControlWidget *add_control_widget(Entity *e, float size)
{
    Behaviour *widget_b = add_behaviour(e, control_widget_update, sizeof(ControlWidget), ControlWidgetID);
    widget_b->mouse_listener = control_widget_mouse_listener;
    widget_b->mouse_motion_listener = control_widget_mouse_motion_listener;
    ControlWidget *widget = (ControlWidget *) widget_b->data;
    widget->size = size;
    return widget;
}
