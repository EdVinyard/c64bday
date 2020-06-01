#define uchar unsigned char

#define SCREEN ((uchar*)0x0400)
#define COLS (40)

#define CHARSET_PTR     ((uchar*)0xd018)
#define CHARSET_DEFAULT ((uchar*)0xd000)
#define CHARSET_CUSTOM  ((uchar*)0x3000)

#define INTERRUPT ((uchar*)0xdc0e)
#define INTERRUPT_DISABLE (*INTERRUPT = *INTERRUPT & 254)
#define INTERRUPT_ENABLE  (*INTERRUPT = *INTERRUPT | 1)

#define RASTER_COUNTER ((uchar*)0xd012)
#define BORDER         ((uchar*)0xd020)
#define BORDER_CHANGE __asm__("inc $d020")
#define BORDER_RESET \
__asm__("lda #0"); \
__asm__("sta $d020")

// Copy the default charset bitmaps to 0x3000.
void setup_custom_char_set() {
    // see https://www.c64-wiki.com/wiki/Character_set#Configuring_VIC_for_a_character_set
    unsigned short i = 0;
    uchar vic2_screen_reg;
    uchar char_bitmap_row;

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

    // Now, use the custom character bitmap copy we just created.
    // NOTE: This won't work if it happens inside the bank switching section
    // above.
    vic2_screen_reg = *CHARSET_PTR;
    vic2_screen_reg = vic2_screen_reg & 241; // 1111 0001 - mask off the old bits
    vic2_screen_reg = vic2_screen_reg | 12;  // 0000 1100 - replace with 6, 0b110
    *CHARSET_PTR = vic2_screen_reg;
}

void replace_at_with_solid_block() {
    uchar i;
    for (i = 0; i < 8; i++) {
        CHARSET_CUSTOM[i] = 255; // 1111 1111
    }
}

void main(void)
{
    SCREEN[0] = 0; // '@'

    BORDER_RESET;
    setup_custom_char_set();
    replace_at_with_solid_block();
    BORDER_CHANGE;

    while (1) {
        uchar i; 

        // while (*RASTER_COUNTER != 64);
        __asm__("lda #64");
        rasterwait:
        __asm__("cmp $d012"); // VIC2 raster index/counter
        __asm__("bne %g", rasterwait);

        for (i = 0; i < 8; i++) {
            CHARSET_CUSTOM[i] = CHARSET_CUSTOM[i] + 1;
        }
    }
}
