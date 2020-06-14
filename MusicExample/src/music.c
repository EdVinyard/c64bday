
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

const uchar before = 3;
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
const uchar after = 4;

void render_one_note_as_chars(uchar* screen) {
    int i;
    for (i = 0; i < sizeof(one_note); i++) {
        screen[i] = one_note[i];
    }
}

void clear_sid_registers() {
    uchar i;
    for (i = 0; i < 24; i++) {
        SID_REGISTERS[i] = 0;
    }
}

uchar seq_duration = 0; // duration of note, in frames (60 frames/sec)
uchar seq_index = 0; // current index into note sequence
void crummy_sequencer() {
    uchar hf;
    uchar lf;

    if (0 == seq_duration) {
        // previous note has finished; advance to the next
        seq_index += 3;

        // 𝄇 (if we just played the last note, start over)
        if (seq_index >= sizeof(one_note)) {
            seq_index = 0; 
        }

        hf           = one_note[seq_index+0];
        lf           = one_note[seq_index+1];
        seq_duration = one_note[seq_index+2];

        SCREEN[seq_index] = hf;
        SCREEN[seq_index+1] = lf;
        SCREEN[seq_index+2] = seq_duration;

        if (0 == hf) {
            // release
            SID_REGISTERS[4] = voice1;
        } else {
            // attack
            SID_REGISTERS[1] = 5; //hf;
            SID_REGISTERS[0] = 0; //lf;
            SID_REGISTERS[4] = voice1 | 1;
        }
    }

    seq_duration -= 1;//2;
}

void raster_busy_wait_sequencer() {
    uchar frame;

    for (frame = 0; ; frame++) {
        //while (*RASTER_COUNTER != 64);
        __asm__("lda #64");
        rasterwait:
        __asm__("cmp $d012"); // VIC2 raster index/counter
        __asm__("bne %g", rasterwait);        
        BORDER_RESET;

        if (frame & 1) {
            BORDER_CHANGE;
            crummy_sequencer();
            BORDER_CHANGE;
        }
    }
}

void raster_interrupt_handler() {
    // from https://www.c64-wiki.com/wiki/Raster_interrupt#Using_a_single_interrupt_service_routine

    // change a border scanline yellow to mark the timing of this handler
    *BORDER = 7; // yellow
    // __asm__("ldx #255"); // busy-wait ~200 ms
    // rih_busywait:
    // __asm__("dex");
    // __asm__("beq %g", rih_busywait);

    // The data in one_note looks OK here, but is messed up if we invoke
    // crummy_sequencer.  The C compiler depends on having zero-page set
    // up just so, and I think the callback from raster interrupt breaks
    // some assumption about the state of the machine.
    SCREEN[0] = one_note[3];
    SCREEN[1] = one_note[4];
    SCREEN[2] = one_note[2];
    crummy_sequencer();

    *BORDER = 0; // black

    // "Acknowledge" the interrupt by clearing the VIC's interrupt flag.
    __asm__("ASL $D019");
    
    // Jump into KERNAL's standard interrupt service routine to handle keyboard
    // scan, cursor display etc.
    __asm__("JMP $EA31");
}

void enable_raster_interrupt() {
    // from https://www.c64-wiki.com/wiki/Raster_interrupt#Setting_up_a_raster_interrupt

    // Switch off interrupts signals from CIA-1
    __asm__("LDA #%%01111111");
    __asm__("STA $DC0D");
    
    // Clear most significant bit in VIC's raster register 
	__asm__("AND $D011");
    __asm__("STA $D011");

    // Set the raster line number where interrupt should occur
	__asm__("LDA #64"); // TODO: function parameter?
    __asm__("STA $D012");

    // Set the interrupt vector to point to custom interrupt service routine
	__asm__("LDA #<%v", raster_interrupt_handler); // TODO: parameter?
    __asm__("STA $0314");
    __asm__("LDA #>%v", raster_interrupt_handler);
    __asm__("STA $0315");

    // Enable raster interrupt signals from VIC
	__asm__("LDA #%%00000001");
    __asm__("STA $D01A");
}

// #define INTERRUPT_SEQUENCER
void main(void)
{
    clear_sid_registers();
    voice1 = SAWTOOTH;
    SID_REGISTERS[ 5] =  0x10; // attack, decay
    SID_REGISTERS[ 6] =  0xb1; // sustain, release
    SID_REGISTERS[ 2] =  0; // pulse width low byte
    SID_REGISTERS[ 3] =  2; // pulse width high byte
    SID_REGISTERS[24] = 15; // max volume (per voice or everything?)

    render_one_note_as_chars(SCREEN+320);
#ifndef INTERRUPT_SEQUENCER
    __asm__("lda $DC0E");       // disabled interrupts
    __asm__("and #%%11111110");
    __asm__("sta $DC0E");
    raster_busy_wait_sequencer();
#else
    enable_raster_interrupt();
#endif
}
