
#define uchar unsigned char

#define SCREEN ((uchar*)0x0400)
#define COLORS ((uchar*)0xD800)
#define CURSOR_COLOR ((uchar*)0x0286)
#define CURSOR ((uchar*)0x00cc)
#define ROWS (25)
#define COLS (40)

#define CHARSET_PTR     ((uchar*)0xd018)
#define CHARSET_DEFAULT ((uchar*)0xd000)
#define CHARSET_CUSTOM  ((uchar*)0x3000)

#define EMPTY_BLOCK (96)
#define FULL_BLOCK (224)

#define INTERRUPT ((uchar*)0xdc0e)
#define INTERRUPT_DISABLE (*INTERRUPT = *INTERRUPT & 254)
#define INTERRUPT_ENABLE  (*INTERRUPT = *INTERRUPT | 1)

#define HORIZONTAL_SCROLL ((uchar*)0xd016)

#define RASTER_COUNTER ((uchar*)0xd012)
#define CLEAR_SCREEN __asm__("jsr $e544")
#define BACKGROUND ((uchar*)0xd021)
#define BORDER ((uchar*)0xd020)
#define BORDER_CHANGE __asm__("inc $d020")
#define BORDER_RESET \
__asm__("lda #0"); \
__asm__("sta $d020")

#define BLACK (0)
#define YELLOW (7)
#define DARK_GREY (11)
#define GREY (12)
#define LIGHT_GREY (15)

#define SID_REGISTERS ((uchar*)0xd400)
#define TRIANGLE (16)
#define SAWTOOTH (32)
#define PULSE (64)
#define NOISE (128)

uchar voice1 = PULSE;
uchar voice2 = SAWTOOTH;

#define MESSAGE_LEN (22)
// max 214
#define MARQUEE_ROW_LEN (7*MESSAGE_LEN + 40)
uchar row0[256];
uchar row1[256];
uchar row2[256];
uchar row3[256];
uchar row4[256];
uchar row5[256];
uchar row6[256];

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

    // fill the whole marquee row with blanks
    marquee_idx = 255;
    do {
        marquee_row[marquee_idx--] = EMPTY_BLOCK;
    } while (marquee_idx != 0);
    marquee_row[marquee_idx] = EMPTY_BLOCK;

    // blanks for the first full screen width of columns
    marquee_row = marquee_row + COLS; 

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

void render_marquee_row(
    uchar* _screen,
    uchar* _source,
    uchar _offset)
{
    register uchar* screen = _screen;
    register uchar* source = _source + _offset;
    __asm__("ldy #40");

render_marquee_loop:
    __asm__("lda (%v),y", source);
    __asm__("sta (%v),y", screen);
    __asm__("dey");
    __asm__("bne %g", render_marquee_loop);

    __asm__("lda (%v),y", source);
    __asm__("sta (%v),y", screen);
}

// 60 frames/sec
// ~120 beats/min = ~2 beats/sec = ~1 qt note/sec = 1 quarter note / 32 frames
// 2 eigth notes / sec = 1 eigth note / 16 frames
// 4 sixteenth notes / sec = 1 sixteenth note / 8 frames

/*
    This is the "sequencer" input data, in the form of three-byte 
    "tuples".  Each tuple consists of:

    - frequency high byte
    - frequency low byte
    - frame count (40 == quarter note)
    
    The frequency values here are from the C64 Programmer's 
    Reference Guide, starting at page 384.
*/
const uchar happy_birthday_lead[] = {
    // happy birthday to you,
    16,195,26, 0,0,4, // C4, dotted eighth
    16,195, 8, 0,0,2, // C4, sixteenth
    18,209,36, 0,0,4, // D4, quarter
    16,195,36, 0,0,4, // C4, quarter
    22, 96,36, 0,0,4, // F4, quarter
    21, 31,76, 0,0,4, // E4, half

    // happy birthday to you,
    16,195,26, 0,0,4, // C4, dotted eighth
    16,195, 8, 0,0,2, // C4, sixteenth
    18,209,36, 0,0,4, // D4, quarter
    16,195,36, 0,0,4, // C4, quarter
    25, 30,36, 0,0,4, // G4, quarter
    22, 96,76, 0,0,4, // F4, quarter

    // happy birthday dear PER-SON,
    16,195,26, 0,0,4, // C4, dotted eighth
    16,195, 8, 0,0,2, // C4, sixteenth
    33,135,36, 0,0,4, // C5, quarter
    28, 49,36, 0,0,4, // A4, quarter
    22, 96,36, 0,0,4, // F4, quarter
    21, 31,36, 0,0,4, // E4, quarter
    18,209,76, 0,0,4, // D4, half (hold)

    0,0,40,           // quarter rest

    // happy birthday to yooouuu
    29,223,26, 0,0,4, // A#4, dotted eighth
    29,223, 8, 0,0,2, // A#4, sixteenth
    28, 49,36, 0,0,4, // A4, quarter
    22, 96,36, 0,0,4, // F4, quarter
    25, 30,36, 0,0,4, // G4, quarter
    22, 96,80,        // F4, half

    0,0,120,          // rest 3 beats
};

/* harmony by Marcia Karr */
const uchar happy_birthday_harmony[] = {
    // happy
    0,0,40,           // quarter rest
    // birthday to
    11,48,76,  0,0,4, // F3, half
    0,0,40,
    // you,
    8,97,76,   0,0,4, // C3, half
    // happy
    0,0,40,           // quarter rest
    // birthday to
    11,48,76,  0,0,4, // F3, half
    0,0,40,           // quarter rest
    // you
    14,24,76,  0,0,4, // A3, half
    // happy birthday dear
    0,0,160,          // rest 4 beats
    // PER-SON
    14,239,116, 0,0,4, // A#3, dotted half (hold)
    0,0,40,           // quarter rest
    // happy
    0,0,40,
    // birthday
    11,48,76,  0,0,4, // F3, half
    // to
    16,195,36, 0,0,4, // C4, dotted eighth
    // you
    //11,48,80,         // F3, half
    5,152,80,         // F2, half
    0,0,120,          // rest 3 beats
};

void clear_sid_registers() {
    uchar i;
    for (i = 0; i < 24; i++) {
        SID_REGISTERS[i] = 0;
    }
}

uchar lead_duration = 0; // remaining duration of note, in frames (60 frames/sec)
uchar lead_index = 253; // current index into note sequence
void lead_sequencer() {
    uchar hf;
    uchar lf;

    if (0 == lead_duration) {
        // previous note has finished; advance to the next
        lead_index += 3;

        // 𝄇 (if we just played the last note, start over)
        if (lead_index >= sizeof(happy_birthday_lead)) {
            lead_index = 0; 
        }

        hf            = happy_birthday_lead[lead_index+0];
        lf            = happy_birthday_lead[lead_index+1];
        lead_duration = happy_birthday_lead[lead_index+2];

        if (0 == hf) {
            // release
            SID_REGISTERS[4] = voice1;
        } else {
            // attack
            SID_REGISTERS[1] = hf;
            SID_REGISTERS[0] = lf;
            SID_REGISTERS[4] = voice1 | 1;
        }
    }

    lead_duration -= 2;
}

uchar harmony_duration = 0; // remaining duration of note, in frames (60 frames/sec)
uchar harmony_index = 253; // current index into note sequence
void harmony_sequencer() {
    uchar hf;
    uchar lf;

    if (0 == harmony_duration) {
        // previous note has finished; advance to the next
        harmony_index += 3;

        // 𝄇 (if we just played the last note, start over)
        if (harmony_index >= sizeof(happy_birthday_harmony)) {
            harmony_index = 0; 
        }

        hf               = happy_birthday_harmony[harmony_index+0];
        lf               = happy_birthday_harmony[harmony_index+1];
        harmony_duration = happy_birthday_harmony[harmony_index+2];

        if (0 == hf) {
            // release
            SID_REGISTERS[4+7] = voice2;
        } else {
            // attack
            SID_REGISTERS[1+7] = hf;
            SID_REGISTERS[0+7] = lf;
            SID_REGISTERS[4+7] = voice2 | 1;
        }
    }

    harmony_duration -= 2;
}

/*
    Draw a horizontal, full-character-height colored row/bar
    across the screen (full 40-column width).
 */
void render_color_row(
    uchar* screen_row, 
    uchar* color_row,
    uchar color)
{
    uchar i;
    for (i = 0; i < 40; i++) {
        screen_row[i] = FULL_BLOCK;
        color_row[i] = color;
    }
}

void main(void)
{
    uchar i;
    uchar frame;

    // marquee setup
    CLEAR_SCREEN;
    *BORDER = BLACK;
    *BACKGROUND = BLACK;

    copy_char_bitmaps_to_0x3000();
    init_marquee(message, MESSAGE_LEN);

    *HORIZONTAL_SCROLL = *HORIZONTAL_SCROLL & 247; // enable 38 column mode

    // music setup
    clear_sid_registers();
    voice1 = SAWTOOTH;
    SID_REGISTERS[ 5] =  0x10;  // lead attack, decay
    SID_REGISTERS[ 6] =  0xb1;  // lead sustain, release
    SID_REGISTERS[ 2] =  0;     // pulse width low byte
    SID_REGISTERS[ 3] =  2;     // pulse width high byte

    SID_REGISTERS[5+7] = 9;     // harmony attack/decay
    SID_REGISTERS[6+7] = 0xe1;  // harmony sustain/release

    SID_REGISTERS[24] = 15;     // max volume!

    __asm__("lda $DC0E");       // disable interrupts
    __asm__("and #%%11111110");
    __asm__("sta $DC0E");

    render_color_row(SCREEN, COLORS, 7);

    while (1) {
        // one cycle of the entire marquee message
        for (i = 0; i < MARQUEE_ROW_LEN; i++) {

            // marque moves one whole character to the left
            //for (frame = 8; frame != 0; frame--) {
            frame = 8;
            do {
                frame--;
                // while (*RASTER_COUNTER != 190);
                __asm__("lda #180");
                // 190 for text at bottom of screen
                rasterwait:
                __asm__("cmp $d012"); // VIC2 raster index/counter
                __asm__("bne %g", rasterwait);
                
                // BORDER_RESET;       // raster timing
                // *BORDER = YELLOW;

                *HORIZONTAL_SCROLL = (*HORIZONTAL_SCROLL & 248) | frame;

                if (7 == frame) {
                    // advance one key-frame of the marquee
                    render_marquee_row(SCREEN+320, row0, i);
                    render_marquee_row(SCREEN+360, row1, i);
                    render_marquee_row(SCREEN+400, row2, i);
                    render_marquee_row(SCREEN+440, row3, i);
                    render_marquee_row(SCREEN+480, row4, i);
                    render_marquee_row(SCREEN+520, row5, i);
                    render_marquee_row(SCREEN+560, row6, i);
                    // last screen row starts at SCREEN+960
                    // BORDER_CHANGE;
                } else if (0 == (frame & 1)) {
                    // advance the music sequencers
                    lead_sequencer();
                    harmony_sequencer();
                }

                // BORDER_RESET;       // raster timing
            } while (frame != 0);
        }
    }
}
