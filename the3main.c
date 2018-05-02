/* 
 * File:   the3main.c
 * 2036192 Sena Temucin
 * 1881556 Yasir Tosun
 *
 * We designed a state machine to control the work flow
 * of the program. We used Timer0 and Timer1 interrupts for time-based
 * operations. We configured the analog to digital converter and planned the
 * ADC ISR to work as described in the homework text. We used the LCD.c, LCD.h,
 * and Includes.h files shared in the recitation to configure and use the LCD.
 * 
 * There is a little bug where during pin setting and pin entry modes the last digit
 * sometimes appears twice. 
 * 
 * Created on April 22, 2018, 5:29 PM
 */

#include <p18cxxx.h>
#include <p18f8722.h>
#include <xc.h>
#include <string.h>
#pragma config OSC = HSPLL, FCMEN = OFF, IESO = OFF, PWRT = OFF, BOREN = OFF, WDT = OFF, MCLRE = ON, LPT1OSC = OFF, LVP = OFF, XINST = OFF, DEBUG = OFF

#define _XTAL_FREQ   40000000

#include "Includes.h"
#include "LCD.h"

#define tl1 RH0
#define tl2 RH1
#define tl3 RH2
#define tl4 RH3

unsigned int const lookup[10] = {0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F};
int flag;

int state = 0;
int buttonSet = 0;
int debug=0;
int tmr0helper = 0;
int tmr1helper = 0;
int currpin=0;
int blank=0;
int digit=0;
char number='#';
int pin[4];
int test=0,att=2;
int msgblink=0,penalty=0;
int a=0,b,c=2,d,e=1,f,g=0,h,timeleft=120;



void Init(){        //Initialize SFR values, timers, and interrupts
    TRISA = 0xFF;
    TRISF = 0xFF;
    PORTE = 0;
    PORTB = 0;
    TRISB = 0xC0;
    TRISE = 0x02;
    INTCON = 0x68;
    INTCON2 = 0;
    INTCON3 = 0;
    PIR1 = 0;
    PIE1 = 1;
    IPR1 = 0;
    ADCON0 = 0x31;
    ADCON1 = 0;
    ADCON2 = 0x1A;
    return;
}  

void interrupt mainISR(){       
    if(TMR0IF){         //Timer0 ISR checks the states and performs necessary operations
        TMR0IF=0;       //based on current state
        TMR0L = 0x3D;
        tmr0helper+=1;
        if (((tmr0helper%20)==0)&&((state==1)||(state==3))){    //ADC conversion request once every 100 ms
            /*ADC code*/
            ADCON0bits.GO = 1;
        }
        if (((tmr0helper%50)==0)&&((state==1)||(state==3))){    //Pin entry blinking once every 250 ms
            if(currpin<7){
                WriteCommandToLCD(0x8B+currpin/2);
            }
            if(blank){
                WriteDataToLCD(digit+48);
                blank=0;
            }
            else{
                WriteDataToLCD(' ');
                blank=1;
            }
        }
        if((tmr0helper%100==0)&&(state==2)){        //Pin set complete, 3 seconds of blinking
            if(blank){
                WriteStringToLCD(" The new pin is ");
                WriteCommandToLCD(0xC0);  
                WriteStringToLCD("   ---");
                WriteDataToLCD(pin[0]+48);
                WriteDataToLCD(pin[1]+48);
                WriteDataToLCD(pin[2]+48);
                WriteDataToLCD(pin[3]+48);
                WriteStringToLCD("---   ");
                blank=0;
                currpin=0;
                }
            else{
                ClearLCDScreen();
                blank=1;}
            msgblink+=1;
            if(msgblink==6){        //Change the message and the state after 3 seconds
                ClearLCDScreen();
                WriteCommandToLCD(0x80);
                WriteStringToLCD(" Enter pin:#### ");
                WriteCommandToLCD(0xC0);                
                WriteStringToLCD("  Attempts:");
                WriteDataToLCD(att+48);
                
                state=3;
            }
            tmr0helper=0;
            }
    }
    if(TMR1IF){     //7-segment operations
        TMR1IF=0;
        TMR1 = 0x9E58;
        if(state<3){        //Display "----"
            PORTJ=64;
            tl1=1;
            tl2=1;
            tl3=1;
            tl4=1;
        }
        else if(state>2){       //Countdown from 120
            tmr1helper+=1;
            PORTJ=lookup[g];
            tl1=1;
            __delay_ms(3);
            tl1=0;
            
            PORTJ=lookup[e];
            tl2=1;
            __delay_ms(3);
            tl2=0;

            PORTJ=lookup[c];
            tl3=1;
            __delay_ms(3);
            tl3=0;

            PORTJ=lookup[a];
            tl4=1;
            __delay_ms(3);
            tl4=0;
            if((tmr1helper==50)&&(state==5)){       //20 seconds wait
                penalty+=1;
                currpin=0;}
            if(penalty==20){        //Continue after 20 sec
                state=3; 
                penalty=0;
                att=2;
                ClearLCDScreen();
                WriteCommandToLCD(0x80);
                WriteStringToLCD(" Enter pin:#### ");
                WriteCommandToLCD(0xC0);                
                WriteStringToLCD("  Attempts:");
                WriteDataToLCD(att+48);
                }
            if((tmr1helper==50)&&(state!=4)){       //7-segment display 120 secs counter code
                if(!timeleft){      //Reset program after 120 seconds
                    msgblink=0;
                    currpin=0;
                    state=1;
                    timeleft=120;
                    ClearLCDScreen();
                    WriteCommandToLCD(0x80);
                    WriteStringToLCD(" Set a pin:#### ");
                }
                timeleft-=1;
                tmr1helper=0;
                /*7-segment update code*/
                a=timeleft%10;//4th digit is saved here
                b=timeleft/10;
                c=b%10;//3rd digit is saved here
                d=b/10;
                e=d%10; //2nd digit is saved here
                f=d/10;
                g=f%10; //1st digit is saved here
                h=f/10;
            }
        }
    }
    if(INTCONbits.RBIF){        //RB Port change interrupt ISR
        RBIF=0;
        /*rb6&rb7 isr code*/
        if((!PORTBbits.RB6)&&(state==1)){       //Set digit
            WriteCommandToLCD(0x8B+currpin/2);
            WriteDataToLCD(digit+48);
            pin[currpin/2] = digit;
            if(currpin<7){
                currpin+=1;}
        }
        else if((!PORTBbits.RB6)&&(state==3)){      //Test current digit
            WriteCommandToLCD(0x8B+currpin/2);
            WriteDataToLCD(digit+48);
            if((pin[currpin/2])==digit){test+=1;}
            if(currpin<7){
                currpin+=1;}
            if(currpin>7){
                currpin=7;
            }
        }
        if((!PORTBbits.RB7)&&(currpin>6)&&(state==1)){      //Finish pin setting
            state=2;
            currpin=0;
        }
        else if((!PORTBbits.RB7)&&(currpin>6)&&(state==3)){     //Test pin
            if(test>7){
                ClearLCDScreen();
                WriteCommandToLCD(0x80);
                WriteStringToLCD("Safe is opening!");
                WriteCommandToLCD(0xC0);                
                WriteStringToLCD("$$$$$$$$$$$$$$$$");
                state=4;
                currpin=0;
            }
            else{       //Decrease remaining attempt count
                att=att-1;
                WriteCommandToLCD(0xCB);
                WriteDataToLCD((att)+48);
                if(!att){
                    WriteCommandToLCD(0x8B);
                    WriteStringToLCD("XXXX");
                    WriteCommandToLCD(0xC0);
                    WriteStringToLCD("Try after 20 sec");
                    state=5;
                }
                currpin=0;
            }
        }
    }
    if(PIR1bits.ADIF){      //AD Converter ISR
        ADIF=0;
        /*adcon code*/
        digit = ADRESL|(ADRESH<<8);
        if(digit<100){digit=0;}
        else if(digit<200){digit=1;}
        else if(digit<300){digit=2;}
        else if(digit<400){digit=3;}
        else if(digit<500){digit=4;}
        else if(digit<600){digit=5;}
        else if(digit<700){digit=6;}
        else if(digit<800){digit=7;}
        else if(digit<900){digit=8;}
        else{digit=9;}
        debug=1;
    }
}


void buttonE(){     //RE1 button task
    if(PORTEbits.RE1==0 && buttonSet==1){
        for(int i=0;i<30091;i++){
            for(int j=0;j<81;j++){}
        }
        buttonSet=0;
        state=1;
    }
    else if(PORTEbits.RE1==1){
        buttonSet=1;
        return;
    }
    else{
        return;
    }
    
}

int main(int argc, char** argv) {
    InitLCD();
    ClearLCDScreen();
    Init();
    WriteStringToLCD(" $>Very  Safe<$ ");
    WriteCommandToLCD(0xC0);  
    WriteStringToLCD(" $$$$$$$$$$$$$$ ");
    
    while(!state){      //Wait until RE1 is pushed
        buttonE();
    }
    
    ClearLCDScreen();       
    WriteCommandToLCD(0x80);
    WriteStringToLCD(" Set a pin:#### ");       //Start pin setting phase
    TMR0L = 0x3D;
    TMR1 = 0x9E58;
    T0CON = 0xC7;
    T1CON = 0xF9;
    ADIF = 0;
    ADIE = 1;
    RBIE = 1;
    ei();       //Enable interrupts
    while(1){
    }
}
