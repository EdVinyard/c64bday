#define uchar unsigned char

#define SCREEN ((uchar*)0x0400)
#define COLS (40);

#define BORDER ((uchar*)0xd020)
#define RASTER_COUNTER ((uchar*)0xd012)
#define CHARSET ((uchar*)0xd800)

const uchar EMPTY_BLOCK[8]   = {  96,  96,  96,  96,  96,  96,  96,  96 };
const uchar FULL_BLOCK[8]    = { 224, 224, 224, 224, 224, 224, 224, 224 };
const uchar EMPTY_TO_FULL[8] = {  96, 103, 106, 118, 225, 245, 244, 229 };
const uchar FULL_TO_EMPTY[8] = { 224, 231, 234, 246,  97, 117, 116, 101 };
const uchar* ANIMATIONS[4] = { EMPTY_BLOCK, FULL_BLOCK, EMPTY_TO_FULL, FULL_TO_EMPTY };

#define CONTENT_LEN (8)
// 0 - EMPTY_BLOCK
// 1 - FULL_BLOCK
// 2 - EMPTY_TO_FULL
// 3 - FULL_TO_EMPTY
const uchar CONTENT[CONTENT_LEN] = { 0, 0, 0, 0,  2, 1, 3, 0 };

void main(void)
{
    static uchar screen_offset = 0;
    //uchar content_offset = 0;
    static uchar frame_index = 0;
    static uchar animation_index = 0;
    static const uchar* animation;
    static uchar new_char = 5; // 'e' for error

    while (1) {
        // draw each frame of the eight-frame animation
        for (frame_index = 0; frame_index < 8; frame_index++) {
            __asm__("lda #100");
            rasterwait:
            __asm__("cmp $d012"); // VIC2 raster index/counter
            __asm__("bne %g", rasterwait);
            __asm__("lda #0");
            __asm__("sta $d020");

            *(SCREEN+40) = frame_index + 48; // display frame index on screen

            // draw each character on the screen
            for (screen_offset = 0; screen_offset < CONTENT_LEN; screen_offset++) {
                __asm__("inc $d020");

                // draw one cell
                animation_index = CONTENT[screen_offset];
                animation = ANIMATIONS[animation_index];
                new_char = animation[frame_index];

                //*(SCREEN+80) = screen_offset + 48; // show scr_offset on screen
                *(SCREEN+screen_offset) = new_char;
            }

            __asm__("inc $d020");
        }
    }
}
