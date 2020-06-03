#define uchar unsigned char

#define SCREEN ((uchar*)0x0400)
#define COLS (40)

#define CHARSET_PTR     ((uchar*)0xd018)
#define CHARSET_DEFAULT ((uchar*)0xd000)
#define CHARSET_CUSTOM  ((uchar*)0x3000)

#define INTERRUPT ((uchar*)0xdc0e)
#define INTERRUPT_DISABLE (*INTERRUPT = *INTERRUPT & 254)
#define INTERRUPT_ENABLE  (*INTERRUPT = *INTERRUPT | 1)

#define HORIZONTAL_SCROLL ((uchar*)0xd016)

#define RASTER_COUNTER ((uchar*)0xd012)
#define BORDER         ((uchar*)0xd020)
#define BORDER_CHANGE __asm__("inc $d020")
#define BORDER_RESET \
__asm__("lda #0"); \
__asm__("sta $d020")

void main(void)
{
    uchar scroll_position = 0;
    char scroll_direction = 1;

    *HORIZONTAL_SCROLL = *HORIZONTAL_SCROLL & 247; // enable 38 column mode

    while (1) {
        BORDER_RESET;

        // while (*RASTER_COUNTER != 64);
        __asm__("lda #200");
        rasterwait:
        __asm__("cmp $d012"); // VIC2 raster index/counter
        __asm__("bne %g", rasterwait);

        BORDER_CHANGE;

        // calculate new X scroll position
        scroll_position = scroll_position + scroll_direction;
        if (scroll_position == 0 || scroll_position == 7) {
            scroll_direction = -scroll_direction;
        }

        // set new X scroll position
        *HORIZONTAL_SCROLL = (*HORIZONTAL_SCROLL & 248) | scroll_position;
    }
}
