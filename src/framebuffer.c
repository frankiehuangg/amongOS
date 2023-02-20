#include "lib-header/framebuffer.h"
#include "lib-header/stdtype.h"
#include "lib-header/stdmem.h"
#include "lib-header/portio.h"
#include "stdmem.c"

unsigned short *textmemptr;
// void framebuffer_set_cursor(uint8_t r, uint8_t c) {
//     // TODO : Implement
// }

void framebuffer_write(uint8_t row, uint8_t col, char c, uint8_t fg, uint8_t bg) {
    // TODO : Implement{
     uint16_t attrib = (bg << 4) | (fg & 0x0F);
     volatile uint16_t * where;
     where = (volatile uint16_t *)0xB8000 + (row * 80 + col) ;
     *where = c | (attrib << 8);
}

void framebuffer_clear(void) {
    unsigned blank;
    int i;
    blank = 0 << 4;

    for(i = 0; i < 1000; i++)
        memset(textmemptr + i * 400, blank, 800);
}