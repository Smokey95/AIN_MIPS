#include <msp430.h>
#include "..\base.h"
#include "TA1.h"
#include "event.h"

#define BTNMAX      2
#define COUNT_MAX   5

/* Port definition:
 * Port 2: Pin 7 => output, LED1
 * Port 1: Pin 2 => output, LED2
 * Port 1: Pin 0 => input,  BTN1
 * Port 1: Pin 1 => input,  BTN2
 */


// ---------------------------------------------------------------------------------> Definition of Button 1
LOCAL const button_const BTN1_CONST = { BIT1, EVENT_BTN1, (const Char *) &P1IN };
LOCAL button_var BTN1_VAR;
LOCAL Button BTN_1;
// LOCAL const Button BTN_1;  WHO CAN THIS BE SOLVED( A const pointer)?

// ---------------------------------------------------------------------------------> Definition of Button 2
LOCAL const button_const BTN2_CONST = { BIT0, EVENT_BTN2, (const Char *) &P1IN };
LOCAL button_var BTN2_VAR;
LOCAL Button BTN_2;

// ---------------------------------------------------------------------------------> Definition for Button Array
LOCAL Button BUTTONS[BTNMAX];

// See help to struct const declaration:
// https://www.eevblog.com/forum/microcontrollers/static-const-struct-vs-static-const-of-its-members/

#pragma FUNC_ALWAYS_INLINE(TA1_init)
GLOBAL Void TA1_init(Void) {

   //for(int i = 0; i < BTNMAX; i++)
   //{
//
   //}

   BTN1_VAR.cnt = 0;
   BTN2_VAR.cnt = 0;

   BTN1_VAR.state = S0;
   BTN2_VAR.state = S0;

   BTN_1.btn_var   = &BTN1_VAR;
   BTN_1.btn_const = &BTN1_CONST;

   BTN_2.btn_var   = &BTN2_VAR;
   BTN_2.btn_const = &BTN2_CONST;

   BUTTONS[0] = BTN_1;
   BUTTONS[1] = BTN_2;

   CLRBIT(TA1CTL,   MC0 | MC1  // stop mode
                  | TAIE       // disable interrupt
                  | TAIFG);    // clear interrupt flag
   CLRBIT(TA1CCTL0, CM1 | CM0  // no capture mode
                  | CAP        // compare mode
                  | CCIE       // disable interrupt
                  | CCIFG);    // clear interrupt flag
   TA1CCR0  = 96-1;            // set up Compare Register
   TA1EX0   = TAIDEX_7;        // set up expansion register
   TA1CTL   = TASSEL__ACLK     // 614.4 kHz
            | MC__UP           // Up Mode
            | ID__8            // /8
            | TACLR;           // clear and start Timer
   SETBIT(TA1CTL, TAIE);       // enable interrupt
}



#pragma vector = TIMER1_A1_VECTOR
__interrupt Void TIMER1_A1_ISR(Void) {

   /*
    * Der Inhalt der ISR ist zu implementieren (gedankenueber Datenstruktur)
    *
    * N = 5
    * | State | Taste | Cnt  | State+ | Cnt+   | Data |
      |-------|-------|------|--------|--------|------|
      | S0    | 0     | =0   | S0     | Cnt    | 0    |
      | S0    | 0     | >0   | S0     | Cnt-1  | 0    |
      | S0    | 1     | <N-1 | S0     | Cnt+1  | 0    |
      | S0    | 1     | =N-1 | S1     | Cnt    | 0    |
      | S1    | 1     | =N-1 | S1     | Cnt    | 1    |
      | S1    | 1     | <N-1 | S1     | Cnt+1  | 1    |
      | S1    | 0     | >0   | S1     | Cnt-1  | 1    |
      | S1    | 0     | =0   | S0     | Cnt    | 1    |
    */

   // Was ist wenn beide gleichzeitig gedrückt?
   LOCAL Button* curr_button;

   // Schleife in der ISR hier zulässig?
   int i;
   for(i = 0; i < BTNMAX; i++)
   {
       if(TSTBIT(*BUTTONS[i].btn_const->port, BUTTONS[i].btn_const->pin))
       {
           curr_button = &BUTTONS[i];
       }
   }

   // Würde sich auch in extra Funktion machen lassen aber dürfen wir in der ISR ja nicht :(
   if(TSTBIT(*curr_button->btn_const->port, curr_button->btn_const->pin)) { //--------------------> BUTTON PRESSED
      if(curr_button->btn_var->state == S0) //----------------------------------> State S0
      {
         if(curr_button->btn_var->cnt < COUNT_MAX)
         {
             curr_button->btn_var->cnt++;
         }
         else
         {
            curr_button->btn_var->state = S1;
            Event_set(curr_button->btn_const->event); // set up event
         }
      }
      else if(curr_button->btn_var->state == S1) //-----------------------------> State S1
      {
         if(curr_button->btn_var->cnt < COUNT_MAX)
         {
             curr_button->btn_var->cnt++;
         }
      }
   }
   else //-------------------------------------------------------------------------> BUTTON RELEASED
   {
      if(curr_button->btn_var->state == S0) //----------------------------------> State S0
      {
         if(curr_button->btn_var->cnt > 0)
         {
             curr_button->btn_var->cnt--;
         }
      }
      else if(curr_button->btn_var->state == S1) //-----------------------------> State S1
      {
         if(curr_button->btn_var->cnt > 0)
         {
             curr_button->btn_var->cnt--;
         }
         else
         {
             curr_button->btn_var->state = S0;
         }
      }
   }

   CLRBIT(TA1CTL, TAIFG);           // clear interrupt flag
   __low_power_mode_off_on_exit();  // restore Active Mode on return
}

