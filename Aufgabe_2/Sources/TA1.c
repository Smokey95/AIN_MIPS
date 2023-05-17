#include <msp430.h>
#include "..\base.h"
#include "TA1.h"
#include "event.h"

#define BTN_MAX     6
#define COUNT_MAX   5

LOCAL Void Button_debounce(const Button* curr_button);

/* Port definition:
 * Port 2: Pin 7 => output, LED1
 * Port 1: Pin 2 => output, LED2
 * Port 1: Pin 1 => input,  BTN1
 * Port 1: Pin 0 => input,  BTN2
 * Port 3: Pin 0 => input,  BTN3 (external button BTN 0)
 * Port 3: Pin 1 => input,  BTN4 (external button BTN 1)
 * Port 3: Pin 2 => input,  BTN5 (external button BTN 2)
 * Port 3: Pin 3 => input,  BTN6 (external button BTN 3)
 */
// ---------------------------------------------------------------------------------> Definition of Button 1
LOCAL const button_const BTN1_CONST = { BIT1, EVENT_BTN1, (const Char *) &P1IN };
LOCAL button_var BTN1_VAR;
LOCAL const Button BTN_1 = { .btn_const = &BTN1_CONST, .btn_var = &BTN1_VAR };

// ---------------------------------------------------------------------------------> Definition of Button 2
LOCAL const button_const BTN2_CONST = { BIT0, EVENT_BTN2, (const Char *) &P1IN };
LOCAL button_var BTN2_VAR;
LOCAL const Button BTN_2 = {.btn_const = &BTN2_CONST, .btn_var = &BTN2_VAR };

// ---------------------------------------------------------------------------------> Definition of Button 3 (external button BTN 0)
LOCAL const button_const BTN3_CONST = { BIT0, EVENT_BTN3, (const Char *) &P3IN };
LOCAL button_var BTN3_VAR;
LOCAL const Button BTN_3 = {.btn_const = &BTN3_CONST, .btn_var = &BTN3_VAR };

// ---------------------------------------------------------------------------------> Definition of Button 4 (external button BTN 1)
LOCAL const button_const BTN4_CONST = { BIT1, EVENT_BTN4, (const Char *) &P3IN };
LOCAL button_var BTN4_VAR;
LOCAL const Button BTN_4 = {.btn_const = &BTN4_CONST, .btn_var = &BTN4_VAR };

// ---------------------------------------------------------------------------------> Definition of Button 5 (external button BTN 2)
LOCAL const button_const BTN5_CONST = { BIT2, EVENT_BTN5, (const Char *) &P3IN };
LOCAL button_var BTN5_VAR;
LOCAL const Button BTN_5 = {.btn_const = &BTN5_CONST, .btn_var = &BTN5_VAR };

// ---------------------------------------------------------------------------------> Definition of Button 6 (external button BTN 3)
LOCAL const button_const BTN6_CONST = { BIT3, EVENT_BTN6, (const Char *) &P3IN };
LOCAL button_var BTN6_VAR;
LOCAL const Button BTN_6 = {.btn_const = &BTN6_CONST, .btn_var = &BTN6_VAR };

// ---------------------------------------------------------------------------------> Definition for Button Array
LOCAL const Button* const BUTTONS[] = { &BTN_1, &BTN_2, &BTN_3, &BTN_4, &BTN_5, &BTN_6 };
LOCAL UChar BTN_INDEX;

// See help to struct const declaration:
// https://www.eevblog.com/forum/microcontrollers/static-const-struct-vs-static-const-of-its-members/

#pragma FUNC_ALWAYS_INLINE(TA1_init)
GLOBAL Void TA1_init(Void) {

   // Update with a loop
   BTN1_VAR.cnt = 0;
   BTN2_VAR.cnt = 0;
   BTN3_VAR.cnt = 0;
   BTN4_VAR.cnt = 0;
   BTN5_VAR.cnt = 0;
   BTN6_VAR.cnt = 0;

   BTN1_VAR.state = S0;
   BTN2_VAR.state = S0;
   BTN3_VAR.state = S0;
   BTN4_VAR.state = S0;
   BTN5_VAR.state = S0;
   BTN6_VAR.state = S0;
   
   BTN_INDEX = 0;

   CLRBIT(TA1CTL,   MC0 | MC1  // stop mode
                  | TAIE       // disable interrupt
                  | TAIFG);    // clear interrupt flag
   CLRBIT(TA1CCTL0, CM1 | CM0  // no capture mode
                  | CAP        // compare mode
                  | CCIE       // disable interrupt
                  | CCIFG);    // clear interrupt flag
   TA1CCR0  = 2-1;             // set up Compare Register
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
   
   TA1CCR0  = 2-1;                  // set up Compare Register with speed clock ()
   
   Button_debounce(BUTTONS[BTN_INDEX]);
   
   BTN_INDEX++;
   if(BTN_INDEX >= BTN_MAX)
   {
       BTN_INDEX = 0;
       
       TA1CCR0  = (1*96)-1;         // set up Compare Register with low clock (10ms sleep)
   }
   
   CLRBIT(TA1CTL, TAIFG);           // clear interrupt flag
   __low_power_mode_off_on_exit();  // restore Active Mode on return
}

#pragma FUNC_ALWAYS_INLINE(Button_debounce)
LOCAL Void Button_debounce(const Button* curr_button) {

    if(TSTBIT(*curr_button->btn_const->port, curr_button->btn_const->pin)) //--------------------> BUTTON PRESSED
    {
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
}
