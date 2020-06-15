
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
uchar voice2 = SAWTOOTH;

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

uchar lead_duration = 0; // duration of note, in frames (60 frames/sec)
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

    lead_duration -= 1;
}

uchar harmony_duration = 0; // duration of note, in frames (60 frames/sec)
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

    harmony_duration -= 1;
}

uchar rbws_frame = 0;
void raster_busy_wait_sequencer() {
    while (1) {
        //while (*RASTER_COUNTER != 64);
        __asm__("lda #64");
        rasterwait:
        __asm__("cmp $d012"); // VIC2 raster index/counter
        __asm__("bne %g", rasterwait);        

        *BORDER = YELLOW;
        if (rbws_frame & 1) {
            lead_sequencer();
            harmony_sequencer();
        }
        rbws_frame++;
        BORDER_RESET;
    }
}

void main(void)
{
    clear_sid_registers();
    voice1 = SAWTOOTH;
    SID_REGISTERS[ 5] =  0x10; // lead attack, decay
    SID_REGISTERS[ 6] =  0xb1; // lead sustain, release
    SID_REGISTERS[ 2] =  0; // pulse width low byte
    SID_REGISTERS[ 3] =  2; // pulse width high byte

    SID_REGISTERS[5+7] = 9; // harmony attack/decay
    SID_REGISTERS[6+7] = 0xe1; // harmony sustain/release

    SID_REGISTERS[24] = 15; // max volume (per voice or everything?)

    __asm__("lda $DC0E");       // disabled interrupts
    __asm__("and #%%11111110");
    __asm__("sta $DC0E");
    raster_busy_wait_sequencer();
}
