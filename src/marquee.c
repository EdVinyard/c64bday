
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

// Each value in ANIMATIONS is a PETSCII character index.
const uchar ANIMATIONS[32]   = {
     96,  96,  96,  96,  96,  96,  96,  96, // EMPTY_BLOCK
    224, 224, 224, 224, 224, 224, 224, 224, // FULL_BLOCK
     96, 103, 106, 118, 225, 245, 244, 229, // EMPTY_TO_FULL
    224, 231, 234, 246,  97, 117, 116, 101, // FULL_TO_EMPTY
};

#define CONTENT_LEN (64)
// each CONTENT value is a base index into ANIMATIONS
//  0 - EMPTY_BLOCK
//  8 - FULL_BLOCK
// 16 - EMPTY_TO_FULL
// 24 - FULL_TO_EMPTY
const uchar CONTENT[CONTENT_LEN] = { 
    16, 24,  0,  0,  16, 24,  0,  0,  16, 24,  0,  0,  16, 24,  0,  0,
    16,  8, 24,  0,  16,  8, 24,  0,  16,  8, 24,  0,  16,  8, 24,  0,
    16,  8,  8, 24,   0,  0,  0,  0,  16,  8,  8, 24,   0,  0,  0,  0,
    16,  8,  8,  8,   8,  8,  8, 24,  16,  8,  8,  8,   8,  8,  8, 24, 
    };

uchar frame_index;

// RenderRow arguments
uchar  RenderRow_content_offset = 0; // leftmost screen char shows content[offset]
const uchar* RenderRow_content; // pointer to start of row content
uchar* RenderRow_screen;  // pointer to leftmost char in screen row
#undef RR_SLOW
void RenderRow() {
    register uchar* screen;
    register const uchar* content;
    static uchar col;
    static uchar content_index;
#ifdef RR_SLOW
    static uchar animation;
    static uchar new_char;
#endif

    screen = RenderRow_screen;
    content = RenderRow_content;

    // draw each character in the row
    for (col = 0; col < COLS; col++) {
        //BORDER_CHANGE;

        // draw one character/cell
#ifdef RR_SLOW
        content_index = (CONTENT_LEN-1) & (col + RenderRow_content_offset); // HACK: cheap modulo
#else
        __asm__ ("lda %v", col);  // A = col
        __asm__ ("clc");
        __asm__ ("adc %v", RenderRow_content_offset); // A = A + content_offset
        __asm__ ("and #%b", CONTENT_LEN-1); // A = A & 63 // cheap modulo
// SLOW __asm__ ("sta %v", content_index);  // content_index = A
        // content_index is in register A
#endif

#ifdef RR_SLOW
        animation = RenderRow_content[content_index]; // was CONTENT[...]
#else
        __asm__ ("tay");
        __asm__ ("lda (%v),y", content);
// SLOW __asm__ ("sta %v", animation);
        // animation is in register A
#endif

#ifdef RR_SLOW
        new_char = ANIMATIONS[animation + frame_index];
#else
        __asm__ ("clc");
        __asm__ ("adc %v", frame_index);
        __asm__ ("tay");
        __asm__ ("lda %v,y", ANIMATIONS);
// SLOW __asm__ ("sta %v", new_char);
        // new_char is in register A
#endif

#ifdef RR_SLOW
        *(SCREEN+col) = new_char;
#else
        __asm__ ("ldy %v", col);
        __asm__ ("sta (%v),y", screen);
#endif
    }    
}

void main(void)
{
    RenderRow_content = CONTENT;

    while (1) {
        // walk the content right-to-left across the screen
        for (RenderRow_content_offset = 0; 
             RenderRow_content_offset < CONTENT_LEN; 
             RenderRow_content_offset++
        ) {
            // draw each frame of the eight-frame animation for a smooth transition
            for (frame_index = 0; frame_index < 8; frame_index++) {
                //while (*RASTER_COUNTER != 64);
                __asm__("lda #64");
                rasterwait:
                __asm__("cmp $d012"); // VIC2 raster index/counter
                __asm__("bne %g", rasterwait);
                
                BORDER_RESET;

                *(SCREEN+200) = frame_index + 48; // display frame index on screen

                BORDER_CHANGE;
                RenderRow_screen = SCREEN;
                RenderRow();

                BORDER_CHANGE;
                RenderRow_screen = SCREEN+40;
                RenderRow();

                BORDER_CHANGE;
                RenderRow_screen = SCREEN+80;
                RenderRow();

                BORDER_CHANGE;
                RenderRow_screen = SCREEN+120;
                RenderRow();

                BORDER_CHANGE;
            }
        }
    }
}
