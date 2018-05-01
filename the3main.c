/* 
 * File:   the3main.c
 * Author: Sena Temuçin
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
int pin;

void Init(){
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
    ADCON2 = 0x0A;
    return;
}

void sevenseg(int flag){
    int a,b,c,d,e,f,g,h,timeleft=120;
    if(flag==0){
        PORTJ=64;
        tl1=1;
    }
    else{
        a=timeleft%10;//4th digit is saved here
        b=timeleft/10;
        c=b%10;//3rd digit is saved here
        d=b/10;
        e=d%10; //2nd digit is saved here
        f=d/10;
        g=f%10; //1st digit is saved here
        h=f/10;
        
        PORTJ=lookup[g];
        tl1=1;
        
        PORTJ=lookup[e];
        tl2=1;
    
        PORTJ=lookup[c];
        tl3=1;

        PORTJ=lookup[a];
        tl4=1; 
 
    }
}

void interrupt mainISR(){
    if(TMR0IF){
        TMR0IF=0;
        TMR0L = 0x3D;
        tmr0helper+=1;
        if ((tmr0helper%20)==0&&state==1){
            /*ADC code*/
            ADCON0bits.GO = 1;
        }
        if ((tmr0helper%50==0)&&state==1){
            if(currpin!=8){
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
        if(tmr0helper==100){
            if(blank){
                WriteStringToLCD(" The new pin is ");
                WriteCommandToLCD(0xC0);  
                WriteStringToLCD("   ---");
                WriteStringToLCD(pin);
                WriteStringToLCD("---   ");
                }
            else{
                ClearLCDScreen();}
            tmr0helper=0;
            }
    }
    if(TMR1IF){
        TMR1IF=0;
        TMR1 = 0x0BDC;
        PORTJ=64;
        tl1=1;
        tl2=1;
        tl3=1;
        tl4=1;
        if(state==3){
            tmr1helper+=1;
            if(tmr1helper==20){
                tmr1helper=0;
                /*7-segment update code*/
                
            }
        }
    }
    if(INTCONbits.RBIF){
        RBIF=0;
        /*rb6&rb7 isr code*/
        if(!PORTBbits.RB6){
            WriteCommandToLCD(0x8B+currpin/2);
            WriteDataToLCD(digit+48);
            for(int i=0;i++;i<(3-currpin/2)){
                digit=digit*10;
            }
            pin = pin+digit;
            if(currpin!=8){
                currpin+=1;}
        }
        if(PORTBbits.RB7&&(currpin==8)){
            state=2;
        }
    }
    if(PIR1bits.ADIF){
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


void buttonE(){
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
    
    while(!state){
        buttonE();
    }
    
    ClearLCDScreen();
    WriteCommandToLCD(0x80);
    WriteStringToLCD(" Set a pin:#### ");
    TMR0L = 0x3D;
    TMR1L = 0xDC;
    TMR1H = 0x0B;
    T0CON = 0xC7;
    T1CON = 0xF9;
    ADIF = 0;
    ADIE = 1;
    RBIE = 1;
    ei();
    while(1){
    }
    
}

