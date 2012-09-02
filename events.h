#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#ifndef EVENT_H_
#define EVENT_H_

void handle_key_event(XKeyEvent *e);
void handle_button_event(XEvent *e);
void handle_client_message(XClientMessageEvent *e);
void handle_configure_request(XConfigureRequestEvent *e);
void handle_enter_event(XCrossingEvent *e);
void handle_focus_change_event(XFocusChangeEvent *e);
void handle_map_request(XMapRequestEvent *e);
void handle_property_change(XPropertyEvent *e);
void handle_unmap_event(XUnmapEvent *e);
void handle_destroy_event(XDestroyWindowEvent *e);

typedef char* EvArgs;

/* This file will be parsed by ev_handler.scm.
 * EVENT_FUNCTION macro is the place holder for the parser.
 */

#define EVENT_FUNCTION

void EVENT_FUNCTION ev_move_window_x(EvArgs args);
void EVENT_FUNCTION ev_move_window_y(EvArgs args);
void EVENT_FUNCTION ev_move_window_upleft(EvArgs args);
void EVENT_FUNCTION ev_move_window_upright(EvArgs args);
void EVENT_FUNCTION ev_move_window_downleft(EvArgs args);
void EVENT_FUNCTION ev_move_window_downright(EvArgs args);
void EVENT_FUNCTION ev_delete_window(EvArgs args);
void EVENT_FUNCTION ev_info_window(EvArgs args);
void EVENT_FUNCTION ev_maximise_window(EvArgs args);
void EVENT_FUNCTION ev_maximise_window_vert(EvArgs args);
void EVENT_FUNCTION ev_fix_window(EvArgs args);
void EVENT_FUNCTION ev_exec_command(EvArgs args);
void EVENT_FUNCTION ev_next_focus(EvArgs args);
void EVENT_FUNCTION ev_switch_vdesk(EvArgs args);
void EVENT_FUNCTION ev_next_vdesk(EvArgs args);
void EVENT_FUNCTION ev_prev_vdesk(EvArgs args);
void EVENT_FUNCTION ev_fix_next_vdesk(EvArgs args);
void EVENT_FUNCTION ev_fix_prev_vdesk(EvArgs args);
void EVENT_FUNCTION ev_up_focus(EvArgs args);
void EVENT_FUNCTION ev_down_focus(EvArgs args);
void EVENT_FUNCTION ev_right_focus(EvArgs args);
void EVENT_FUNCTION ev_left_focus(EvArgs args);
void EVENT_FUNCTION ev_upleft_focus(EvArgs args);
void EVENT_FUNCTION ev_upright_focus(EvArgs args);
void EVENT_FUNCTION ev_downleft_focus(EvArgs args);
void EVENT_FUNCTION ev_downright_focus(EvArgs args);
void EVENT_FUNCTION ev_wm_quit(EvArgs args);
void EVENT_FUNCTION ev_wm_restart(EvArgs args);
void EVENT_FUNCTION ev_mark_client(EvArgs args);
void EVENT_FUNCTION ev_goto_mark(EvArgs args);
void EVENT_FUNCTION ev_warp_pointer_x(EvArgs args);
void EVENT_FUNCTION ev_warp_pointer_y(EvArgs args);
void EVENT_FUNCTION ev_set_mode(EvArgs args);
void EVENT_FUNCTION ev_raise_window(EvArgs args);
void EVENT_FUNCTION ev_lower_window(EvArgs args);
/* void EVENT_FUNCTION ev_iconize(EvArgs args); */
void EVENT_FUNCTION ev_mouse_move(EvArgs args);
void EVENT_FUNCTION ev_mouse_resize(EvArgs args);
void EVENT_FUNCTION ev_resize_window_x(EvArgs args);
void EVENT_FUNCTION ev_resize_window_y(EvArgs args);
void EVENT_FUNCTION ev_do_nothing(EvArgs args);

#undef EVENT_FUNCTION

#endif // ! EVENT_H_
