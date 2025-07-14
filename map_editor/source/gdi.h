//extern int windows_width, windows_height;
//byte fullscreen, redraw_scene;
//extern double deltatime;
extern float delta_time_f;
extern u8 i_cursor_key_pressed, i_key_pressed, i_modifier_key_pressed, mouse_moved, i_mouse_button_pressed, enable_mouse_cursor;
extern u16 i_direction_pressed;
extern int i_cursor_dx, i_cursor_dy;
extern u8 mouse_capture;

// i_direction_pressed
#define KEY_W 1
#define KEY_S (1 << 1)
#define KEY_A (1 << 2)
#define KEY_D (1 << 3)

#define KEY_Q (1 << 4)
#define KEY_E (1 << 5)

#define KEY_NUMPAD_1 (1 << 6)
#define KEY_NUMPAD_2 (1 << 7)

#define KEY_NUMPAD_4 (1 << 8)
#define KEY_NUMPAD_5 (1 << 9)
#define KEY_NUMPAD_3 (1 << 10)
#define KEY_NUMPAD_6 (1 << 11)

// i_cursor_key_pressed
#define KEY_UP 1
#define KEY_DOWN 2
#define KEY_LEFT 4
#define KEY_RIGHT 8

#define KEY_Z 16
#define KEY_X 32

// i_modifier_key_pressed
#define KEY_SHIFT 1

// i_key_pressed
#define KEY_RETURN 1
#define KEY_BACK 2
#define KEY_SPACE 3
#define KEY_CONTROL 4
#define KEY_C 5
#define KEY_V 6
#define KEY_B 7
#define KEY_M 8
#define KEY_P 9

#define KEY_COMMA 10
#define KEY_POINT 11
#define KEY_MINUS 12

// i_mouse_button_pressed
#define LEFT_MOUSE_BUTTON 1
#define RIGHT_MOUSE_BUTTON 2