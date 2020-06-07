
#define uchar unsigned char

#define SCREEN ((uchar*)0x0400)
#define COLS (40)

#define RASTER_COUNTER ((uchar*)0xd012)
#define CHARSET ((uchar*)0xd800)
#define BORDER ((uchar*)0xd020)
#define BORDER_CHANGE __asm__("inc $d020")
#define BORDER_RESET \
__asm__("lda #0"); \
__asm__("sta $d020")

#define SPRITE_ENABLE  ((uchar*)0xd015)
#define SPRITE0_BITMAP ((uchar*)0x07f8)
#define SPRITE0_COLOR  ((uchar*)0xd027)
#define SPRITE0_X      ((uchar*)0xd000)
#define SPRITE0_Y      ((uchar*)0xd001)
#define SPRITE_X_MSB   ((uchar*)0xd010)

#define RED (2)
#define YELLOW (7)

/*
    Fill Sprite 0 with "on" bits,
    set its color to red,
    set its position, and
    turn it on
*/
void init_sprite0() {
    int i;
    
    // "draw" the sprite bitmap
    for (i = 0; i < 63; i++) {
        // each iteration alters one third of one row
        SPRITE0_BITMAP[i] = 0xff; // 1111 1111
    }

    // set the color
    *SPRITE0_COLOR = YELLOW;

    // turn the first sprite on
    *SPRITE_ENABLE = *SPRITE_ENABLE | 1;
}

void main(void)
{
    uchar x = 30; // position
    uchar y = 200;
    uchar v_x = 2; // velocity
    uchar v_y = 0;
    uchar pause_frame = 0;

    init_sprite0();

    while (1) {
        pause_frame = !pause_frame;

        //while (*RASTER_COUNTER != 64);
        __asm__("lda #64");
        rasterwait:
        __asm__("cmp $d012"); // VIC2 raster index/counter
        __asm__("bne %g", rasterwait);

        if (pause_frame) {
            continue; // 30 fps instead of 60 fps
        }

        BORDER_RESET;

        *SPRITE0_X = x;
        *SPRITE0_Y = y;
        BORDER_CHANGE;

        x += v_x;
        y += v_y;
        //v_y = (v_y + 1) & 0x1f; // cap y velocity
        BORDER_CHANGE;
    }
}
