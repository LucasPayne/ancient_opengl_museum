/*--------------------------------------------------------------------------------
--------------------------------------------------------------------------------*/
#include "museum.h"

bool arrow_keys_down[4] = {false};
bool alt_arrow_keys_down[4] = {false};
bool ___shift_key_down = false;
bool ___space_key_down = false;

bool shift_key_down(void)
{
    return ___shift_key_down;
}
bool space_key_down(void)
{
    return ___space_key_down;
}

void special_key_up_input_callback(int key, int x, int y)
{
    switch(key) {
        case GLUT_KEY_UP: arrow_keys_down[Up] = false; break;
        case GLUT_KEY_DOWN: arrow_keys_down[Down] = false; break;
        case GLUT_KEY_LEFT: arrow_keys_down[Left] = false; break;
        case GLUT_KEY_RIGHT: arrow_keys_down[Right] = false; break;
    }
}
void special_key_input_callback(int key, int x, int y)
{
    switch(key) {
        case GLUT_KEY_UP: arrow_keys_down[Up] = true; break;
        case GLUT_KEY_DOWN: arrow_keys_down[Down] = true; break;
        case GLUT_KEY_LEFT: arrow_keys_down[Left] = true; break;
        case GLUT_KEY_RIGHT: arrow_keys_down[Right] = true; break;
    }
}
void key_up_input_callback(unsigned char key, int x, int y)
{
    switch(key) {
        case 'q':
            glutLeaveMainLoop();
            break;
        case 'k': arrow_keys_down[Up] = false; break;
        case 'j': arrow_keys_down[Down] = false; break;
        case 'h': arrow_keys_down[Left] = false; break;
        case 'l': arrow_keys_down[Right] = false; break;
        case 'w': alt_arrow_keys_down[Up] = false; break;
        case 's': alt_arrow_keys_down[Down] = false; break;
        case 'a': alt_arrow_keys_down[Left] = false; break;
        case 'd': alt_arrow_keys_down[Right] = false; break;
        case ' ': ___space_key_down = false; break;
    }
}
void key_input_callback(unsigned char key, int x, int y)
{
    switch(key) {
        case GLUT_KEY_ESC:
            glutLeaveMainLoop();
            break;
        case 'k': arrow_keys_down[Up] = true; break;
        case 'j': arrow_keys_down[Down] = true; break;
        case 'h': arrow_keys_down[Left] = true; break;
        case 'l': arrow_keys_down[Right] = true; break;
        case 'w': alt_arrow_keys_down[Up] = true; break;
        case 's': alt_arrow_keys_down[Down] = true; break;
        case 'a': alt_arrow_keys_down[Left] = true; break;
        case 'd': alt_arrow_keys_down[Right] = true; break;
        case ' ': ___space_key_down = true; break;
    }

    // Look for active key-listeners attached to entities, then send them the key event.
    for (int i = 0; i < entity_list_length; i++) {
        Entity *entity = &entity_list[i];
        for (int i = 0; i < entity->num_behaviours; i++) {
            if (entity->behaviours[i]->key_listener == NULL) continue;
            entity->behaviours[i]->key_listener(entity, entity->behaviours[i], key);
        }
    }
}

void window_to_screen(int window_x, int window_y, float *x, float *y)
{
    float h = window_height * 1.0 / window_width < aspect_ratio ? -(window_height / aspect_ratio - window_width) / 2.0 : 0;
    float v = window_height * 1.0 / window_width > aspect_ratio ? -(aspect_ratio*window_width - window_height) / 2.0 : 0;

    *x = (window_x - h) * 1.0 / (window_width - 2*h);
    *y = 1 - (window_y - v) * 1.0 / (window_height - 2*v);
}
void mouse_input_callback(int button, int state, int window_x,  int window_y)
{
    // Convert to window coordinates
    // --------[1,1]
    // -           -
    // -           -
    // [0,0]--------
    // before passing to entities.
    float x, y;
    window_to_screen(window_x, window_y, &x, &y);

    // Look for active mouse-listeners attached to entities, then send them the mouse event.
    for (int i = 0; i < entity_list_length; i++) {
        Entity *entity = &entity_list[i];
        for (int i = 0; i < entity->num_behaviours; i++) {
            if (entity->behaviours[i]->mouse_listener == NULL) continue;
            entity->behaviours[i]->mouse_listener(entity, entity->behaviours[i], button, state, x, y);
        }
    }
}


// This callback is only triggered when motion occurs while a mouse button is held.
void mouse_motion_callback(int window_x, int window_y)
{
    float x, y;
    window_to_screen(window_x, window_y, &x, &y);
    
    
    // Look for active mouse-motion-listeners attached to entities, then send them the mouse motion event.
    for (int i = 0; i < entity_list_length; i++) {
        Entity *entity = &entity_list[i];
        for (int i = 0; i < entity->num_behaviours; i++) {
            if (entity->behaviours[i]->mouse_motion_listener == NULL) continue;
            entity->behaviours[i]->mouse_motion_listener(entity, entity->behaviours[i], x, y);
        }
    }
}
