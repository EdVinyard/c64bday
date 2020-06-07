
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

const uchar voice1 = PULSE;
const uchar voice2 = TRIANGLE;

const uchar single_voice_data[] = {
    25,177,250,
    28,214,250,

    25,177,250,
    25,177,250,

    25,177,120,
    28,214,120,

    32,94,750,
    25,177,250,
    19,63,250,
    19,63,250,

    21,154,60, // originally 63
    24,63,60,
    
    25,177,250,
    24,63,120,

    19,63,250
    };

void single_voice_example() {
    uchar i;
    uchar hf;
    uchar lf;
    uchar dr;
    uchar t;
    
    SID_REGISTERS[5] = 9;
    SID_REGISTERS[6] = 0;
    SID_REGISTERS[2] = 0; // pulse width low byte
    SID_REGISTERS[3] = 4; // pulse width high byte
    SID_REGISTERS[24] = 15; // max volume (per voice or everything?)

    for (i = 0; i < sizeof(single_voice_data); i += 3) {
        hf = single_voice_data[i+0];
        lf = single_voice_data[i+1];
        dr = single_voice_data[i+2];

        SID_REGISTERS[1] = hf;
        SID_REGISTERS[0] = lf;
        SID_REGISTERS[4] = voice1 | 1;

        // wait with sound playing
        for (; dr > 0; dr -= 10) {
            //while (*RASTER_COUNTER != 64);
            __asm__("lda #64");
            rasterwait:
            __asm__("cmp $d012"); // VIC2 raster index/counter
            __asm__("bne %g", rasterwait);
        }

        SID_REGISTERS[4] = voice1;

        // wait in silence
        for (dr = 50; dr > 0; dr -= 10) {
            //while (*RASTER_COUNTER != 64);
            __asm__("lda #64");
            rasterwait2:
            __asm__("cmp $d012"); // VIC2 raster index/counter
            __asm__("bne %g", rasterwait2);
        }
    }
}

void clear_sid_registers() {
    uchar i;
    for (i = 0; i < 24; i++) {
        SID_REGISTERS[i] = 0;
    }
}

uchar hf;
uchar lf;
void two_voice_example() {
    uchar i;
    uchar dr;
    short f;
    uchar t;
    uchar lf_up_a_fifth;
    uchar hf_up_a_fifth;
    
    SID_REGISTERS[5] = 9; // voice 1 attack/decay
    SID_REGISTERS[6] = 0; // voice 1 sustain/release
    SID_REGISTERS[2] = 0; // voice 1 pulse width low byte
    SID_REGISTERS[3] = 8; // voice 1 pulse width high byte    

    SID_REGISTERS[5+7] = 9; // voice 2 attack/decay
    SID_REGISTERS[6+7] = 0; // voice 2 sustain/release

    SID_REGISTERS[24] = 15; // set max volume

    for (i = 0; i < sizeof(single_voice_data); i += 3) {
        BORDER_CHANGE;

        hf = single_voice_data[i+0];
        lf = single_voice_data[i+1];
        dr = single_voice_data[i+2];

        SID_REGISTERS[1] = hf; // voice 1 frequency high byte
        SID_REGISTERS[0] = lf; // voice 1 frequency low byte
        SID_REGISTERS[4] = voice1 | 1;

        // up a perfect fifth
        lf_up_a_fifth = lf + (lf >> 1);
        hf_up_a_fifth = hf + (hf >> 1);
        if (lf_up_a_fifth < lf) {
            hf_up_a_fifth += 1;
        }

        SID_REGISTERS[1+7] = hf_up_a_fifth; // voice 2 freq high byte
        SID_REGISTERS[0+7] = lf_up_a_fifth; // voice 2 freq low byte
        SID_REGISTERS[4+7] = voice2 | 1; // voice 2 + attack (1)

        BORDER_RESET;

        // wait with sound playing
        for (; dr > 0; dr -= 10) {
            //while (*RASTER_COUNTER != 64);
            __asm__("lda #64");
            rasterwait:
            __asm__("cmp $d012"); // VIC2 raster index/counter
            __asm__("bne %g", rasterwait);
        }

        SID_REGISTERS[4] = voice1; // voice 1 decay initiated
        SID_REGISTERS[4+7] = voice2; // voice 2 decay initiated

        // wait in silence
        for (dr = 50; dr > 0; dr -= 10) {
            //while (*RASTER_COUNTER != 64);
            __asm__("lda #64");
            rasterwait2:
            __asm__("cmp $d012"); // VIC2 raster index/counter
            __asm__("bne %g", rasterwait2);
        }
    }    
}

void main(void)
{
    clear_sid_registers();
    // while (1) single_voice_example();
    while (1) two_voice_example();
}
