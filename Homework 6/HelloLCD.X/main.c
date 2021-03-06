#include<xc.h>           // processor SFR definitions
#include<sys/attribs.h>  // __ISR macro
#include <stdio.h>
#include "ST7735.h"


// DEVCFG0
#pragma config DEBUG = OFF // no debugging
#pragma config JTAGEN = OFF // no jtag
#pragma config ICESEL = ICS_PGx1 // use PGED1 and PGEC1
#pragma config PWP = OFF // no write protect
#pragma config BWP = OFF // no boot write protect
#pragma config CP = OFF // no code protect

// DEVCFG1
#pragma config FNOSC = PRIPLL // use primary oscillator with pll
#pragma config FSOSCEN = OFF // turn off secondary oscillator
#pragma config IESO = OFF // no switching clocks
#pragma config POSCMOD = HS // high speed crystal mode
#pragma config OSCIOFNC = OFF // disable secondary osc
#pragma config FPBDIV = DIV_1 // divide sysclk freq by 1 for peripheral bus clock
#pragma config FCKSM = CSDCMD // do not enable clock switch
#pragma config WDTPS = PS1048576 // use slowest wdt
#pragma config WINDIS = OFF // wdt no window mode
#pragma config FWDTEN = OFF // wdt disabled
#pragma config FWDTWINSZ = WINSZ_25 // wdt window at 25%

// DEVCFG2 - get the sysclk clock to 48MHz from the 8MHz crystal
#pragma config FPLLIDIV = DIV_2 // divide input clock to be in range 4-5MHz
#pragma config FPLLMUL = MUL_24 // multiply clock after FPLLIDIV
#pragma config FPLLODIV = DIV_2 // divide clock after FPLLMUL to get 48MHz
#pragma config UPLLIDIV = DIV_2 // divider for the 8MHz input clock, then multiplied by 12 to get 48MHz for USB
#pragma config UPLLEN = ON // USB clock on

// DEVCFG3
#pragma config USERID = 420 // some 16bit userid, doesn't matter what. Also Ayy Lmao
#pragma config PMDL1WAY = OFF // allow multiple reconfigurations
#pragma config IOL1WAY = OFF // allow multiple reconfigurations
#pragma config FUSBIDIO = ON // USB pins controlled by USB module
#pragma config FVBUSONIO = ON // USB BUSON controlled by USB module


void drawchar(short x, short y, char c, short color_1, short color_2) {
    c = c - 0x20; //ascii conversion
    int i;
    for (i = 0; i < 5; i++) {
        char bitmap = ASCII[c][i];
        int j;
        for (j = 0; j < 8; j++) {
            
            if ((bitmap >> j & 1) == 1) {
                LCD_drawPixel(x+i, y+j, color_1);             
            }
            else {
                LCD_drawPixel(x+i, y+j, color_2);
            }
        }
    }
}

void drawString(short x, short y, char* message, short color_1, short color_2) {
    int i = 0;
    while(message[i] && i < 26) { //we can only print a max of 26 chars
        drawchar(x+i*5, y, message[i], color_1, color_2);
        i++;
    }
}

void drawBox(short x, short y, short length, short height, short color) {
    int i, j;
    for (i = 0; i < length; i++) {
        for (j = 0; j < height; j++) {
            LCD_drawPixel(x+i, y+j, color);               
            
        }
    }
}



int main() {

    __builtin_disable_interrupts();

    // set the CP0 CONFIG register to indicate that kseg0 is cacheable (0x3)
    __builtin_mtc0(_CP0_CONFIG, _CP0_CONFIG_SELECT, 0xa4210583);

    // 0 data RAM access wait states
    BMXCONbits.BMXWSDRM = 0x0;

    // enable multi vector interrupts
    INTCONbits.MVEC = 0x1;

    // disable JTAG to get pins back
    DDPCONbits.JTAGEN = 0;

    // do your TRIS and LAT commands here
    //Set TRIS register to declare I/O, 0 is output, 1 is input.
    TRISAbits.TRISA4 = 0;   //Declare RA4 (LED) as output
    TRISBbits.TRISB4 = 1;   //Declare RB4 (Push Button)
    //Note: Pins should already default to input, so above line may be unnecessary 

    LCD_init();
    LCD_clearScreen(0x0000);
    char message[26];
    sprintf(message, "Hello World");
    drawString(35, 65, message, 0xFFFF, 0x0000);
    
    drawBox(20, 100, 88, 10, WHITE);
    drawBox(21, 101, 86, 8, BLACK);
    
    __builtin_enable_interrupts();

    //Set PIC32 internal clock to 0
    _CP0_SET_COUNT(0);
    
    int count = 0;
    u_int last_tick = 0;
    u_int cur_tick = 0;
    
    while(1) {
        if (count < 86) {
            drawBox(21 + count, 101, 1, 8, RED);
        }
        
        cur_tick = _CP0_GET_COUNT();

        sprintf(message, "fps:%g", 24000000/((float)cur_tick - last_tick));
        drawString(1,1, message, WHITE, BLACK);
        last_tick = cur_tick;
        
        if (_CP0_GET_COUNT() > 2400000){
            _CP0_SET_COUNT(0);
            LATAINV = 0b10000;
            count++;
            last_tick = 0;
        }
    }
}