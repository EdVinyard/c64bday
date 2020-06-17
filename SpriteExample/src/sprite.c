
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

#define SPRITE0_BITMAP ((uchar*)0x07f8)
#define SPRITE0_X      ((uchar*)0xd000)
#define SPRITE0_Y      ((uchar*)0xd001)
#define SPRITE1_X      ((uchar*)0xd002)
#define SPRITE1_Y      ((uchar*)0xd003)
#define SPRITE2_X      ((uchar*)0xd004)
#define SPRITE2_Y      ((uchar*)0xd005)
#define SPRITE3_X      ((uchar*)0xd006)
#define SPRITE3_Y      ((uchar*)0xd007)
#define SPRITE4_X      ((uchar*)0xd008)
#define SPRITE4_Y      ((uchar*)0xd009)
#define SPRITE5_X      ((uchar*)0xd00a)
#define SPRITE5_Y      ((uchar*)0xd00b)
#define SPRITE6_X      ((uchar*)0xd00c)
#define SPRITE6_Y      ((uchar*)0xd00d)
#define SPRITE7_X      ((uchar*)0xd00e)
#define SPRITE7_Y      ((uchar*)0xd00f)
#define SPRITE_X_MSB   ((uchar*)0xd010)
#define SPRITE_ENABLE  ((uchar*)0xd015)
#define SPRITE0_COLOR  ((uchar*)0xd027)

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
    uchar x = 167; // position
    uchar y = 245;
    uchar v_x = 0; // velocity
    uchar v_y = 20;
    uchar pause_frame = 0;
    uchar explode = 0;
    init_sprite0();

    while (1) {
        pause_frame++;

        //while (*RASTER_COUNTER != 64);
        __asm__("lda #64");
        rasterwait:
        __asm__("cmp $d012"); // VIC2 raster index/counter
        __asm__("bne %g", rasterwait);

        if (pause_frame & 1) {
            continue; // 30 fps instead of 60 fps
        }

        BORDER_RESET;

        BORDER_CHANGE;

        if (explode) {
            *SPRITE_ENABLE = 0x1f;
            *SPRITE0_X = *SPRITE0_X + 4;
            *SPRITE1_X = *SPRITE1_X - 4;
            *SPRITE2_Y = *SPRITE2_Y + 3;
            *SPRITE3_Y = *SPRITE3_Y - 3;

            *SPRITE4_X = *SPRITE4_X - 2;
            *SPRITE4_Y = *SPRITE4_Y - 2;
        } else {
            *SPRITE0_X = x;
            *SPRITE0_Y = y;
            x += v_x;
            y -= v_y;

            if (v_y > 0) {
                v_y -= 2;
            } else {
                explode = 1;

                *SPRITE1_X = *SPRITE0_X;
                *SPRITE1_Y = *SPRITE0_Y;

                *SPRITE2_X = *SPRITE0_X;
                *SPRITE2_Y = *SPRITE0_Y;

                *SPRITE3_X = *SPRITE0_X;
                *SPRITE3_Y = *SPRITE0_Y;

                *SPRITE4_X = *SPRITE0_X;
                *SPRITE4_Y = *SPRITE0_Y;
            }
            //v_y = (v_y + 1) & 0x1f; // cap y velocity
        }
        BORDER_CHANGE;
    }
}
