#include<xc.h>           // processor SFR definitions
#include<sys/attribs.h>  // __ISR macro

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

void initSPI1() {
    RPB13Rbits.RPB13R = 0b0011; //B13 is SPI1
    TRISBbits.TRISB15 = 0; //B15 is CS, B14 is SCK1
    SPI1CON = 0;
    SPI1BUF;
    SPI1BRG = 1;
    SPI1CONbits.CKE = 1;
    SPI1CONbits.MSTEN = 1;
    SPI1CONbits.ON = 1;
    
    CS = 1; //pull low to send data
    
}

char SPI1_IO(char c) {
    SPI1BUF = c;
    while(!SPI1STATbits.SPIRBF) {
        ;//do nothing but wait.
    }
    return SPI1BUF //return bit you get back
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
    
    
    initSPI1();
    
    __builtin_enable_interrupts();

    //Set PIC32 internal clock to 0
    _CP0_SET_COUNT(0);

    while(1) {
	// use _CP0_SET_COUNT(0) and _CP0_GET_COUNT() to test the PIC timing
	// remember the core timer runs at half the sysclk
        
        //each cycle check for button press -> Low
        //PORTBbits.RB4 reads RB4. While low, do nothing
        while (PORTBbits.RB4 == 0) {
            //do nothing
        }

        //12000 is 0.5 ms. 
        //48MHz Clock Speed -> 24MHz Timer tick speed
        //we want 2kHz as the alternating rate, therefore 12,000 ticks/cycle
        if (_CP0_GET_COUNT() >= 12000) {
            //once it hits that many counts invert the output, and reset the clock
            LATAINV = 0b10000;
            _CP0_SET_COUNT(0);
        }
    }
}