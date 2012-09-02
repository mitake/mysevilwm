#ifndef MARK_H_
#define MARK_H_

void mark_client(Client *c);
void goto_mark(Client *c);

Client *find_marked_client(char mark);

#define MARKCHAR_PREVCLIENT     '\''

#endif // ! MARK_H_

