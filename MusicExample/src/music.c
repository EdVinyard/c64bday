
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

#define YELLOW (7);

#define SID_REGISTERS ((uchar*)0xd400)
#define TRIANGLE (16)
#define SAWTOOTH (32)
#define PULSE (64)
#define NOISE (128)

uchar voice1 = PULSE;
uchar voice2 = TRIANGLE;

// 60 frames/sec
// ~120 beats/min = ~2 beats/sec = ~1 qt note/sec = 1 quarter note / 32 frames
// 2 eigth notes / sec = 1 eigth note / 16 frames
// 4 sixteenth notes / sec = 1 sixteenth note / 8 frames

const uchar one_note[] = {
    // high byte, low byte, frame count
    16,195,72,  0,0,8,  // half note
    16,195,72,  0,0,8,

    16,195,36,  0,0,4, // quarter note
    16,195,36,  0,0,4,
    16,195,36,  0,0,4,
    16,195,36,  0,0,4,

    16,195,16,  0,0,4, // eigth note
    16,195,16,  0,0,4,
    16,195,16,  0,0,4,
    16,195,16,  0,0,4,
    16,195,16,  0,0,4,
    16,195,16,  0,0,4,
    16,195,16,  0,0,4,
    16,195,16,  0,0,4,

    16,195,8,   0,0,2, // sixteenth note
    16,195,8,   0,0,2,
    16,195,8,   0,0,2,
    16,195,8,   0,0,2,
    16,195,8,   0,0,2,
    16,195,8,   0,0,2,
    16,195,8,   0,0,2,
    16,195,8,   0,0,2,

    16,195,8,   0,0,2,
    16,195,8,   0,0,2,
    16,195,8,   0,0,2,
    16,195,8,   0,0,2,
    16,195,8,   0,0,2,
    16,195,8,   0,0,2,
    16,195,8,   0,0,2,
    16,195,8,   0,0,2,
    };


const uchar happy_birthday_music[] = {
    // high byte, low byte, frame count (40 == quarter note)

    // happy birthday to you,
    16,195,26, 0,0,4, // C4, dotted eighth
    16,195, 8, 0,0,2, // C4, sixteenth
    18,209,36, 0,0,4, // D4, quarter
    16,195,36, 0,0,4, // C4, quarter
    22, 96,36, 0,0,4, // F4, quarter
    21, 31,60,        // E4, dotted quarter

    0,0,20,           // eighth rest

    // happy birthday to you,
    16,195,26, 0,0,4, // C4, dotted eighth
    16,195, 8, 0,0,2, // C4, sixteenth
    18,209,36, 0,0,4, // D4, quarter
    16,195,36, 0,0,4, // C4, quarter
    25, 30,36, 0,0,4, // G4, quarter
    22, 96,60,        // F4, quarter

    0,0,20,           // eighth rest

    // happy birthday dear PER-SON,
    16,195,26, 0,0,4, // C4, dotted eighth
    16,195, 8, 0,0,2, // C4, sixteenth
    33,135,36, 0,0,4, // C5, quarter
    28, 49,36, 0,0,4, // A4, quarter
    22, 96,36, 0,0,4, // F4, quarter
    21, 31,36, 0,0,4, // E4, quarter
    18,209,36, 0,0,4, // D4, quarter

    0,0,40,           // quarter rest

    // happy birthday to yooouuu
    29,223,26, 0,0,4, // A#4, dotted eighth
    29,223, 8, 0,0,2, // A#4, sixteenth
    28, 49,36, 0,0,4, // A4, quarter
    22, 96,36, 0,0,4, // F4, quarter
    25, 30,36, 0,0,4, // G4, quarter
    22, 96,60,        // F4, dotted quarter

    0,0,120,          // dotted half rest

};

void clear_sid_registers() {
    uchar i;
    for (i = 0; i < 24; i++) {
        SID_REGISTERS[i] = 0;
    }
}

uchar seq_duration = 0; // duration of note, in frames (60 frames/sec)
uchar seq_index = 253; // current index into note sequence
void crummy_sequencer() {
    uchar hf;
    uchar lf;

    if (0 == seq_duration) {
        // previous note has finished; advance to the next
        seq_index += 3;

        // 𝄇 (if we just played the last note, start over)
        if (seq_index >= sizeof(happy_birthday_music)) {
            seq_index = 0; 
        }

        hf           = happy_birthday_music[seq_index+0];
        lf           = happy_birthday_music[seq_index+1];
        seq_duration = happy_birthday_music[seq_index+2];

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

    seq_duration -= 1;//2;
}

void raster_busy_wait_sequencer() {
    while (1) {
        //while (*RASTER_COUNTER != 64);
        __asm__("lda #64");
        rasterwait:
        __asm__("cmp $d012"); // VIC2 raster index/counter
        __asm__("bne %g", rasterwait);        

        *BORDER = YELLOW;
        crummy_sequencer();
        BORDER_RESET;
    }
}

void main(void)
{
    clear_sid_registers();
    voice1 = SAWTOOTH;
    SID_REGISTERS[ 5] =  0x10; // attack, decay
    SID_REGISTERS[ 6] =  0xb1; // sustain, release
    SID_REGISTERS[ 2] =  0; // pulse width low byte
    SID_REGISTERS[ 3] =  2; // pulse width high byte
    SID_REGISTERS[24] = 15; // max volume (per voice or everything?)

    __asm__("lda $DC0E");       // disabled interrupts
    __asm__("and #%%11111110");
    __asm__("sta $DC0E");
    raster_busy_wait_sequencer();
}
