/*******************************************************************************
  MPLAB Harmony Application Source File
  
  Company:
    Microchip Technology Inc.
  
  File Name:
    app.c

  Summary:
    This file contains the source code for the MPLAB Harmony application.

  Description:
    This file contains the source code for the MPLAB Harmony application.  It 
    implements the logic of the application's state machine and it may call 
    API routines of other MPLAB Harmony modules in the system, such as drivers,
    system services, and middleware.  However, it does not call any of the
    system interfaces (such as the "Initialize" and "Tasks" functions) of any of
    the modules in the system or make any assumptions about when those functions
    are called.  That is the responsibility of the configuration-specific system
    files.
 *******************************************************************************/

// DOM-IGNORE-BEGIN
/*******************************************************************************
Copyright (c) 2013-2014 released Microchip Technology Inc.  All rights reserved.

Microchip licenses to you the right to use, modify, copy and distribute
Software only when embedded on a Microchip microcontroller or digital signal
controller that is integrated into your product or third party product
(pursuant to the sublicense terms in the accompanying license agreement).

You should refer to the license agreement accompanying this Software for
additional information regarding your rights and obligations.

SOFTWARE AND DOCUMENTATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF
MERCHANTABILITY, TITLE, NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE.
IN NO EVENT SHALL MICROCHIP OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER
CONTRACT, NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR
OTHER LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE OR
CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT OF
SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
(INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.
 *******************************************************************************/
// DOM-IGNORE-END


// *****************************************************************************
// *****************************************************************************
// Section: Included Files 
// *****************************************************************************
// *****************************************************************************

#include "app.h"
#include <math.h>
#include "i2c_master_noint.h"
#include "ST7735.h"
#include <stdio.h>

// *****************************************************************************
// *****************************************************************************
// Section: Global Data Definitions
// *****************************************************************************
// *****************************************************************************
int horiz;
int vert;
char message[30];
unsigned char rawData[14];
short data[7];
// *****************************************************************************
/* Application Data

  Summary:
    Holds application data

  Description:
    This structure holds the application's data.

  Remarks:
    This structure should be initialized by the APP_Initialize function.
    
    Application strings and buffers are be defined outside this structure.
*/

APP_DATA appData;

// *****************************************************************************
// *****************************************************************************
// Section: Application Callback Functions
// *****************************************************************************
// *****************************************************************************

/* TODO:  Add any necessary callback functions.
*/

// *****************************************************************************
// *****************************************************************************
// Section: Application Local Functions
// *****************************************************************************
// *****************************************************************************


/* TODO:  Add any necessary local functions.
*/


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


// *****************************************************************************
// *****************************************************************************
// Section: Application Initialization and State Machine Functions
// *****************************************************************************
// *****************************************************************************

/*******************************************************************************
  Function:
    void APP_Initialize ( void )

  Remarks:
    See prototype in app.h.
 */

void APP_Initialize ( void )
{
    /* Place the App state machine in its initial state. */
    appData.state = APP_STATE_INIT;
    
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
    

    
    drawBox(14, 87, 100, 6, WHITE);
    drawBox(62, 40, 6, 100, WHITE);
    drawBox(62, 87, 6, 6, RED);

    
    /* TODO: Initialize your application's state machine and other
     * parameters.
     */
}


/******************************************************************************
  Function:
    void APP_Tasks ( void )

  Remarks:
    See prototype in app.h.
 */

void APP_Tasks ( void )
{

    /* Check the application's current state. */
    switch ( appData.state )
    {
        /* Application's initial state. */
        case APP_STATE_INIT:
        {
            bool appInitialized = true;
       
        
            if (appInitialized)
            {
            
                appData.state = APP_STATE_SERVICE_TASKS;
            }
            break;
        }

        case APP_STATE_SERVICE_TASKS:
        {
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
                data[i] = (rawData[i*2 + 1] << 8) | rawData[i*2];
            }
        
            sprintf(message, "x %d z %d", data[4], data[6]);
            drawString(1, 1, message, WHITE, BLACK);
            
            horiz = -1*data[4]*47.0/16000;
            vert = data[6]*47.0/16000;
            if(horiz > 3) {
                drawBox(14, 87, 47, 6, WHITE);
                drawBox(67, 87, horiz-3,6, RED);
                drawBox(67+horiz-3, 87, 50-horiz, 6, WHITE);
            }
            else if (horiz < -3) {
                drawBox(14, 87, 50+horiz, 6, WHITE);
                drawBox(61+horiz+3, 87, -1* horiz, 6, RED);
                drawBox(68, 87, 47, 6, WHITE);
            }
            else {
                drawBox(14, 87, 48, 6, WHITE);
                drawBox(68, 87, 46, 6, WHITE);
            }
 
            if(vert > 3) {
                drawBox(62, 40, 6, 47, WHITE);
                drawBox(62, 93, 6, vert-3, RED);
                drawBox(62, 93+vert-3, 6, 50-vert, WHITE);
            }
            else if (vert < -3) {
                drawBox(62, 40, 6, 50+vert, WHITE);
                drawBox(62, 87+vert+3, 6, -1*vert, RED);
                drawBox(62, 93, 6, 47, WHITE);
            }
            else {
                drawBox(62, 40, 6, 47, WHITE);
                drawBox(62, 93, 6, 47, WHITE);
            }
            
            
        }
            break;
        }

        /* TODO: implement your application state machine.*/
        

        /* The default state should never be executed. */
        default:
        {
            /* TODO: Handle error in application's state machine. */
            break;
        }
    }
}

 

/*******************************************************************************
 End of File
 */
