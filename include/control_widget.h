#ifndef CONTROL_WIDGET_H
#define CONTROL_WIDGET_H

typedef struct ControlWidget_s {
    float size;
    bool dragging;
    int dragging_plane; // yz, xz, xy
    vec3 drag_offset;
} ControlWidget;
void control_widget_update(Entity *e, Behaviour *b);
void control_widget_mouse_listener(Entity *e, Behaviour *b, int button, int state, float x,  float y);
void control_widget_mouse_motion_listener(Entity *e, Behaviour *b, float x, float y);
ControlWidget *add_control_widget(Entity *e, float size);

#endif // CONTROL_WIDGET_H
