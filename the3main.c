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
            if(currpin!=4){
                WriteCommandToLCD(0x8B+currpin);
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
        if(tmr0helper==100&&state==2){
            tmr0helper=0;
            if(blank){
                WriteStringToLCD(" The new pin is ");
                WriteCommandToLCD(0xC0);  
                WriteStringToLCD("   ---");
                WriteStringToLCD(pin);
                WriteStringToLCD("---   ");
                }
            else{
                ClearLCDScreen();}
            }
        
    }
    if(TMR1IF){
        TMR1IF=0;
        TMR1 = 0x0BDC;
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
        if(PORTBbits.RB6){
            for(int i=0;i++;i<(3-currpin)){
                digit=digit*10;
            }
            pin = pin+digit;
            if(currpin!=4){
                currpin+=1;}
        }
        if(PORTBbits.RB7&&(currpin==4)){
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
    TMR1 = 0x0BDC;
    T0CON = 0xC7;
    T1CON = 0xF9;
    ADIF = 0;
    ADIE = 1;
    RBIE = 1;
    ei();
    while(1){
    }
    
}

