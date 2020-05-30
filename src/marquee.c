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

const uchar EMPTY_BLOCK[8]   = {  96,  96,  96,  96,  96,  96,  96,  96 };
const uchar FULL_BLOCK[8]    = { 224, 224, 224, 224, 224, 224, 224, 224 };
const uchar EMPTY_TO_FULL[8] = {  96, 103, 106, 118, 225, 245, 244, 229 };
const uchar FULL_TO_EMPTY[8] = { 224, 231, 234, 246,  97, 117, 116, 101 };
const uchar* ANIMATIONS[4] = { EMPTY_BLOCK, FULL_BLOCK, EMPTY_TO_FULL, FULL_TO_EMPTY };

#define CONTENT_LEN (64)
// 0 - EMPTY_BLOCK
// 1 - FULL_BLOCK
// 2 - EMPTY_TO_FULL
// 3 - FULL_TO_EMPTY
const uchar CONTENT[CONTENT_LEN] = { 
    2, 3, 0, 0,  2, 3, 0, 0,  2, 3, 0, 0,  2, 3, 0, 0,
    2, 1, 3, 0,  2, 1, 3, 0,  2, 1, 3, 0,  2, 1, 3, 0,
    2, 1, 1, 3,  0, 0, 0, 0,  2, 1, 1, 3,  0, 0, 0, 0,
    2, 1, 1, 1,  1, 1, 1, 3,  2, 1, 1, 1,  1, 1, 1, 3, 
    };

void main(void)
{
    static uchar screen_offset = 0;
    static uchar content_offset = 0;
    static uchar content_index = 0;
    static uchar frame_index = 0;
    static uchar cell_animation_type = 0;
    static const uchar* cell_animation;
    static uchar new_char = 5; // 'e' for error

    while (1) {
        // walk the content right-to-left across the screen
        for (content_offset = 0; content_offset < 40; content_offset++) {
            
            // draw each frame of the eight-frame animation for a smooth transition
            for (frame_index = 0; frame_index < 8; frame_index++) {
                //while (*RASTER_COUNTER != 64);
                __asm__("lda #100");
                rasterwait:
                __asm__("cmp $d012"); // VIC2 raster index/counter
                __asm__("bne %g", rasterwait);
                
                BORDER_RESET;

                *(SCREEN+80) = frame_index + 48; // display frame index on screen

                // draw each character in the row
                for (screen_offset = 0; screen_offset < COLS; screen_offset++) {
                    BORDER_CHANGE;

                    // draw one cell
                    content_index = (CONTENT_LEN-1) & (screen_offset + content_offset); // KLUDGE: cheap modulo
                    cell_animation_type = CONTENT[content_index];
                    cell_animation = ANIMATIONS[cell_animation_type];
                    new_char = cell_animation[frame_index];

                    *(SCREEN+screen_offset) = new_char;
                    __asm__("inc %v", content_index);
                }

                BORDER_CHANGE;
            }
        }
    }
}
