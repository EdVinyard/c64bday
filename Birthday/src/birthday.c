
#define uchar unsigned char

#define SCREEN ((uchar*)0x0400)
#define COLS (40)

#define CHARSET_PTR     ((uchar*)0xd018)
#define CHARSET_DEFAULT ((uchar*)0xd000)
#define CHARSET_CUSTOM  ((uchar*)0x3000)

#define EMPTY_BLOCK (96)
#define FULL_BLOCK (224)

#define INTERRUPT ((uchar*)0xdc0e)
#define INTERRUPT_DISABLE (*INTERRUPT = *INTERRUPT & 254)
#define INTERRUPT_ENABLE  (*INTERRUPT = *INTERRUPT | 1)

#define RASTER_COUNTER ((uchar*)0xd012)
#define BORDER ((uchar*)0xd020)
#define BORDER_CHANGE __asm__("inc $d020")
#define BORDER_RESET \
__asm__("lda #0"); \
__asm__("sta $d020")

#define MESSAGE_LEN (22)
// max 214
#define MARQUEE_ROW_LEN (7*MESSAGE_LEN)
uchar row0[MARQUEE_ROW_LEN];
uchar row1[MARQUEE_ROW_LEN];
uchar row2[MARQUEE_ROW_LEN];
uchar row3[MARQUEE_ROW_LEN];
uchar row4[MARQUEE_ROW_LEN];
uchar row5[MARQUEE_ROW_LEN];
uchar row6[MARQUEE_ROW_LEN];

uchar message[MESSAGE_LEN] = {
    // "HAPPY "
    8,1,16, 16,25,32,
    // "BIRTHDAY ",
    2,9,18, 20,8,4, 1,25,32,
    // "JOHN!  "
    10,15,8, 14,33,32, 32,
    };

/* 
   Copy 127 charset bitmaps to 0x3000;
   a total of 1016 bytes = 8 bytes/char * 127 chars)
*/
void copy_char_bitmaps_to_0x3000() {
    // see https://www.c64-wiki.com/wiki/Character_set#Configuring_VIC_for_a_character_set
    unsigned short i = 0;

    INTERRUPT_DISABLE;
    // Map character bitmap ROM into RAM at 0xd000
    *((uchar*)1) = *((uchar*)1) & 251; // 1111 1011

    // The character bitmaps must start at a multiple of 2048 (0x0800), but
    // NOT at 4096/$1000, 6144/$1800, 36864/$9000, or 38912/$9800, since these
    // areas are "overshadowed" by the ROM character sets as seen from the
    // VIC-II's address space.

    // We'll place the copy at 0x3000 (2048 * 6)
    // 0x3000 = 0x0800 * 6

    // Copy 127 character bitmaps at 8 bytes per character bitmap (1 KB)
    for (i = 0; i < 1023; i++) {
        CHARSET_CUSTOM[i] = CHARSET_DEFAULT[i];
    }

    // "When in text screen mode, the VIC-II looks to 53272 for information on
    // where the character set and text screen character RAM is located:
    // 
    // The four most significant bits form a 4-bit number in the range 0 thru
    // 15.  Multiplied with 1024 this gives the start address for the screen
    // character RAM.
    // 
    // Bits 1 thru 3 (weights 2 thru 8) form a 3-bit number in the range 0 thru
    // 7.  Multiplied with 2048 this gives the start address for the character 
    // set."
    //
    // - from https://www.c64-wiki.com/wiki/53272

    // Un-map character bitmap ROM out of RAM at 0xd000
    *((uchar*)1) = *((uchar*)1) | 4; // 0000 0100
    INTERRUPT_ENABLE;
}

void init_marquee_row(
    uchar* marquee_row,
    uchar char_bitmap_row,
    uchar* message, 
    uchar message_len) {

    uchar message_idx;
    uchar marquee_idx;
    uchar char_bitmap;
    uchar character;

    for (message_idx = 0; message_idx < message_len; message_idx++) {
        character = message[message_idx];
        char_bitmap = CHARSET_CUSTOM[character*8+char_bitmap_row];

        marquee_idx = 8;
        do {
            marquee_idx--;
            if (char_bitmap & 1) {
                marquee_row[message_idx*7+marquee_idx] = FULL_BLOCK;
            } else {
                marquee_row[message_idx*7+marquee_idx] = EMPTY_BLOCK;
            }
            
            char_bitmap = char_bitmap >> 1;
        } while (marquee_idx != 0);
    }
}

void init_marquee(uchar* message, uchar len) {
    init_marquee_row(row0, 0, message, len);
    init_marquee_row(row1, 1, message, len);
    init_marquee_row(row2, 2, message, len);
    init_marquee_row(row3, 3, message, len);
    init_marquee_row(row4, 4, message, len);
    init_marquee_row(row5, 5, message, len);
    init_marquee_row(row6, 6, message, len);
}

#define FAST
static uchar screen_col;
static uchar source_index;
void render_marquee_row(
    uchar* _screen,
    uchar* _source,
    uchar _offset)
{
    register uchar* screen = _screen;
    register uchar* source = _source;
#ifndef FAST
    for (screen_col = 0,
         source_index = (_offset + screen_col) % MARQUEE_ROW_LEN;
         screen_col < 40; 
         screen_col++, source_index++) 
    {
        BORDER_CHANGE;

        if (source_index >= MARQUEE_ROW_LEN) {
            source_index = 0;
        }

        screen[screen_col] = source[source_index];
    }
#else
    __asm__("ldy #39");

render_marquee_loop:
    __asm__("lda (%v),y", source);
    __asm__("sta (%v),y", screen);
    __asm__("dey");
    __asm__("bne %g", render_marquee_loop);
#endif
}

void main(void)
{
    uchar i;
    copy_char_bitmaps_to_0x3000();
    init_marquee(message, MESSAGE_LEN);
    
    for (i = 24; ; i++) {
        // while (*RASTER_COUNTER != 64);
        __asm__("lda #32");
        rasterwait:
        __asm__("cmp $d012"); // VIC2 raster index/counter
        __asm__("bne %g", rasterwait);
        BORDER_RESET;

        BORDER_CHANGE;
        render_marquee_row(SCREEN,     row0, i);
        BORDER_CHANGE;
        render_marquee_row(SCREEN+ 40, row1, i);
        BORDER_CHANGE;
        render_marquee_row(SCREEN+ 80, row2, i);
        BORDER_CHANGE;
        render_marquee_row(SCREEN+120, row3, i);
        BORDER_CHANGE;
        render_marquee_row(SCREEN+160, row4, i);
        BORDER_CHANGE;
        render_marquee_row(SCREEN+200, row5, i);
        BORDER_CHANGE;
        render_marquee_row(SCREEN+240, row6, i);
        BORDER_CHANGE;
    }
}
