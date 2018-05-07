#include<xc.h>           // processor SFR definitions
#include<sys/attribs.h>  // __ISR macro
#include <math.h>
#include "i2c_master_noint.h"
#include "ST7735.h"
#include <stdio.h>


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

void initExpander() {
    ANSELBbits.ANSB2 = 0;
    ANSELBbits.ANSB3 = 0;
    i2c_master_setup();
}

void setExpander(char reg, char level) {
    i2c_master_start();
    i2c_master_send(0b1101011 << 1 | 0);
    i2c_master_send(reg);
    i2c_master_send(level);
    i2c_master_stop();
    
}

unsigned char getExpander() {
    i2c_master_start();
    i2c_master_send(0b1101011 << 1 | 0);
    i2c_master_send(0x0F);
    i2c_master_restart();
    i2c_master_send(0b1101011 << 1 | 1);
    unsigned char rec = i2c_master_recv();
    i2c_master_ack(1);
    i2c_master_stop();
    
    
    return rec;
}

void imuRead(unsigned char add, unsigned char reg, unsigned char* data, int length) {
    i2c_master_start();
    i2c_master_send(0b1101011 << 1 | 0);
    i2c_master_send(reg);
    i2c_master_restart();
    i2c_master_send(0b1101011 << 1 | 1);
    int i = 0;
    while (i < (length - 1)) {
        data[i] = i2c_master_recv();
        i2c_master_ack(0);
        i++;
    }
    data[i] = i2c_master_recv();
    i2c_master_ack(1);
    i2c_master_stop();
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
    //Note: Pins should already default to input, so above line may be unneccesary 
    
    initExpander();
    
    setExpander(0x10,0b10000010); //set up accelarometer
    setExpander(0x11,0b10001000);  //set all outputs as high
    setExpander(0x12,0b00000100);
    
    LCD_init();
    LCD_clearScreen(0x0000);
    
    __builtin_enable_interrupts();

    //Set PIC32 internal clock to 0
    _CP0_SET_COUNT(0);
    
    char message[30];
    unsigned char rawData[14];
    short data[7];
    
    sprintf(message, "Initialized");
    drawString(1,1,message,BLACK,WHITE);
    while(1) {

        
        if (getExpander() != 0x69) { 
            sprintf(message, "uh-oh");
            drawString(1,10,message,RED, BLACK);
            while(1) {
                ;
            }
            
        }
        
       

        
  
        if (_CP0_GET_COUNT() > 1200000){
            LATAINV = 0b10000;
            
            _CP0_SET_COUNT(0);
            
             imuRead(0b1101011, 0x20, rawData, 14);
        
            int i = 0;
            for (i = 0; i <= 6; i++) {
                data[i] = (rawData[i*2 + 1] << 8) | data[i*2];
            }
        
            sprintf(message, "x %d y %d", data[4], data[5]);
            drawString(1, 20, message, WHITE, BLACK);
        }
    }
}