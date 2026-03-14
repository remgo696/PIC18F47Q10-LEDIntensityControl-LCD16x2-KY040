/*
 * File:   maincode00.c
 * Author: klnla
 *
 * Created on 13 de febrero de 2026, 05:50 PM
 */
#include <xc.h>
#include "main.h"
#include "LCD.h"
#define _XTAL_FREQ 4000000UL


#define DUTY_CYCLE_MAXIMO 100
#define DUTY_CYCLE_MINIMO 0
#define DUTY_CYCLE_PASOS 2

#define DUTY_CYCLE_1_PORCIENTO 2 // Un incremento del 1% (2us) = 2 unidades de PWM3DCH

#define ENCODER_PORT PORTB
#define ENCODER_MASK 0x03  // 0b00000011 (Filtra solo los bits 0 y 1)

/* ******************************************** */
__bit b_quieto; // El encoder está quieto
__bit b_horario;

uchar duty_cycle = 0;

volatile uchar anterior_AB;
volatile uchar actual_AB;
/* ******************************************** */

void main(void) {
    configuro();
    POS_CURSOR(1,0);
    ESCRIBE_MENSAJE2("MICRO TP 2025-0");
    POS_CURSOR(2,3);
    ESCRIBE_MENSAJE2("PWM:");
    POS_CURSOR(2,10);
    ENVIA_CHAR('%');
    mostrar_duty_cycle();
    while(1){
        actualizar_duty_cycle();
    }
}

void configuro(void){
    /* ------------------------------- *
    * Oscillator module configuration *
    * ------------------------------- */
    // OSCCON1: X|NOSC[2:0]|NDIV[3:0] -> 0011 0000 = 0x60
    OSCCON1 = 0x60; // HFINTOSC selected, NDIV: 1:1
    // OSCFRQ: XXXX|FRQ[3:0] -> 0000 0010 = 0x02
    OSCFRQ = 0x02;  // FRQ: 4MHz
    // OSCEN: EXTOEN|HFOEN|MFOEN|LFOEN|SOSCEN|ADOEN|X|PLLEN
    OSCEN = 0x40;   // HFOEN enabled
    /* ----------------- *
     * I/O configuration *
     * ----------------- */
    ANSELB = 0xFC;  // B0-1 entradas digitales
    TRISAbits.TRISA0 = 0; // RA0 como salida
    ANSELAbits.ANSELA0 = 0; // RA0 como digital
    /* --------------------------- *
     * Timer0 module configuration *
     * --------------------------- */
    // T0CON0: EN|X|OUT|MD16|OUTPS[3:0] -> 1000 0000 = 0x80 
    T0CON0 = 0x80;  // enabled, 8-bit mode, 1:1 Postscaler  
    // T0CON1: CS[2:0]|ASYNC|CKPS[3:0] -> 1101 0011 = 0xD3
    T0CON1 = 0xD3;  // MFINTOSC(500kHz), 1:4 Prescaler
    TMR0H = 100; // Cuenta para 1,6ms
    /* ------------------------ *
     * Interrupts configuration *
     * ------------------------ */
    PIE0bits.TMR0IE = 1;
    PIR0bits.TMR0IF = 0;
    INTCONbits.GIE = 1;
    /* ----------------- *
     * PPS configuration *
     * ----------------- */
    RA0PPS = 0x07;  // RA0 -> PWM3
    /* ----------------- *
    * PWM configuration *
    * ----------------- */
    PWM3CON = 0x00; // Disable PWM3 module
    T2PR = 199;     // PWM period = 200us (5kHz) 
    PWM3DCH = 0x00; 
    PWM3DCL = 0x00; // duty cycle = 0%
    TMR2IF = 0;     // Clear Timer2 interrupt flag
    T2CLKCON = 0x01; // Timer2 clock source: Fosc/4 (1MHz)
    // T2CON: ON|CKPS[2:0]|OUTPS[3:0] -> 1000 0000 = 0x80
    T2CON = 0x80;   // Timer2 enabled, 1:1 Prescaler
    PWM3CONbits.EN = 1; // Enable PWM3 module (normal operation)
    /* ------------------ *
    * Data initialization *
    * ------------------- */
    b_quieto=1;
    LCD_INIT();
    anterior_AB = ENCODER_PORT & ENCODER_MASK;
    actual_AB = anterior_AB;
}


void actualizar_duty_cycle(void){
    if(!b_quieto){
        if(b_horario){
            if(duty_cycle < DUTY_CYCLE_MAXIMO){
                duty_cycle += DUTY_CYCLE_PASOS;
                PWM3DCH += DUTY_CYCLE_1_PORCIENTO * DUTY_CYCLE_PASOS;
                mostrar_duty_cycle();
            }
        } else {
            if(duty_cycle > DUTY_CYCLE_MINIMO){
                duty_cycle -= DUTY_CYCLE_PASOS;
                PWM3DCH -= DUTY_CYCLE_1_PORCIENTO * DUTY_CYCLE_PASOS;
                mostrar_duty_cycle();
            }
        }
        b_quieto = 1; // Volvemos a marcar el encoder como quieto
    }
    return;
}

void mostrar_duty_cycle(void){
    POS_CURSOR(2,7);
    LCD_ESCRIBE_VAR_CHAR(duty_cycle,3);
    return;
}

void __interrupt() TMR0_ISR(void){
    PIR0bits.TMR0IF = 0;    //bajamos bandera TMR0IF
    actual_AB = ENCODER_PORT & ENCODER_MASK; // Leemos el estado actual del encoder
    if(actual_AB == 0x01){
        if(anterior_AB == 0x00){
            b_horario = 0; // Sentido antihorario
            b_quieto = 0;  // El encoder ya no está quieto
        }else if(anterior_AB == 0x03){
            b_horario = 1; // Sentido horario
            b_quieto = 0;  // El encoder ya no está quieto
        }
    }else if(actual_AB == 0x02){
        if(anterior_AB == 0x00){
            b_horario = 1; // Sentido horario
            b_quieto = 0;  // El encoder ya no está quieto
        }else if(anterior_AB == 0x03){
            b_horario = 0; // Sentido antihorario
            b_quieto = 0;  // El encoder ya no está quieto
        }
    }
    anterior_AB = actual_AB;
}