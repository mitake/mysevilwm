#ifndef SCREEN_H_
#define SCREEN_H_

void drag(Client *c);
void draw_outline(Client *c);
void get_mouse_position(int *x, int *y);
void move(Client *c, int set);
void recalculate_sweep(Client *c, int x1, int y1, int x2, int y2);
void resize(Client *c, int set);
void maximise_vert(Client *c);
void maximise_horiz(Client *c);
void show_info(Client *c);
void sweep(Client *c);
void unhide(Client *c, int raise);
void next(Client *c);
void arrow_next(Client* c, int x, int y);
void hide(Client *c);
void switch_vdesk(int v);
void iconize(Client* c);
void change_vdesk(Client *c, int vd);

void focus(Client* c);
void set_focus(Client* c);

void change_state(Client* client,
                  int x, int y, int w, int h, int vd, int focused, char mark);

#endif // ! SCREEN_H_
