#ifndef _MINI_CONSOLE_H_
#define _MINI_CONSOLE_H_

#include <stdint.h>

void mini_console(void);
void mini_console_ext(int show_header);
void mini_console_terminate(void);
uint32_t console_write(char *buf, uint32_t len);
uint32_t console_read(char *buf, uint32_t len);

#endif /* _MINI_CONSOLE_H_ */
