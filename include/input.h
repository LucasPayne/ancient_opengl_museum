#ifndef INPUT_H 
#define INPUT_H 

enum ArrowKeys {
    Up,
    Down,
    Left,
    Right
};
extern bool arrow_keys_down[];
extern bool alt_arrow_keys_down[];
#define arrow_key_down(KEY) ( arrow_keys_down[( KEY )] )
#define alt_arrow_key_down(KEY) ( alt_arrow_keys_down[( KEY )] )
bool space_key_down(void);
bool shift_key_down(void);

// Escape and enter keys are given as regular ASCII keys. They are not handled in the special key callback.
#define GLUT_KEY_ESC 27
#define GLUT_KEY_ENTER 32

// GLUT callback functions.
void special_key_up_input_callback(int key, int window_x, int window_y);
void special_key_input_callback(int key, int window_x, int window_y);
void key_up_input_callback(unsigned char key, int window_x, int window_y);
void key_input_callback(unsigned char key, int window_x, int window_y);
void mouse_input_callback(int button, int state, int window_x,  int window_y);
void mouse_motion_callback(int window_x, int window_y);

void window_to_screen(int window_x, int window_y, float *x, float *y);

#endif // INPUT_H 
