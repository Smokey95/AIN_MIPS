#include <msp430.h>
#include "..\base.h"
#include "TA0.h"

// 1. GGT von Zeitvorgaben berechnen => Zeitbasis
// 2. Berechne Teilungsfaktor := Grundfrequenz des Timers * Zeitbasis
// 3. Zerlege den Teilungsfaktor mit Hilfe der Werte aus den Mengen
//    {/1, /2, /4, /8} und {/1, /2, /3, /4, /5, /6, /7, /8} so, dass
//    der Skalierungsfaktor eine m�glichst kleine Ganzzahl ist
//
// Example:
// 1. GGT/GCD (greatest common divisor) 250ms
// 2. 614.4 kHz * 250 ms = 153.600 (Dividing factor)
// 3. 153.600 {/8} {/8} = 2400 (Scaling factor)
#define SCALING (2400 - 1)


/*
 * One should think of a suitable data structure that enables runtime efficient execution of the ISR.
 * 
 * Timebase: 250ms
 * PATTERN:
 * (x)  ON      OFF     TOTAL   PATTERN
 * (1)  2s      0.5s    2.5s    (8, 2, 0)
 * (2)  0.75s   0.75s   1.5s    (3, 3, 0)
 * (3)  0.25s   0.25s   0.5s    (1, 1, 0)
 * (4)  0.5s    2s      2.5s    (2, 8, 0)
 * (5)  0.5s    0.5s
 *      0,5s    2s      3.5s    (2,2,2,8,0)
 * (6)  0.5s    0.5s
 *      0,5s    0.5s
 *      2s              4s      (2, 2, 2, 2, 2, 8, 0)
 */
LOCAL const UChar blink_pattern[] = {
        8, 2, 0,                                                                // Pattern 1
        3, 3, 0,                                                                // Pattern 2
        1, 1, 0,                                                                // Pattern 3
        2, 8, 0,                                                                // Pattern 4
        2, 2, 2, 8, 0,                                                          // Pattern 5
        2, 2, 2, 2, 2, 8, 0                                                     // Pattern 6
};

//LOCAL const Uchar blink_ptr_arr[] = {0, 3, 6, 9, 12, 17,};
LOCAL const UChar * const blink_ptr_arr[] = {
    &blink_pattern[0],
    &blink_pattern[3],
    &blink_pattern[6],
    &blink_pattern[9],
    &blink_pattern[12],
    &blink_pattern[17]
};

LOCAL UChar pattern_index;                                                      //<! Index for the current pattern (i)
LOCAL UChar array_index;                                                        //<! Index for the current pattern value (j)
LOCAL UChar cnt_led;                                                            //<! Counter for the current LED state
LOCAL UChar cur_pattern_val;                                                    //<! Current pattern value
LOCAL UChar req_pattern_index;                                                  //<! Requested pattern index

GLOBAL Void set_blink_muster(UInt arg) {
/*
 * The function must be extended to select a blink pattern.
 * This solution depends strongly on the selected data structure.
 */
    req_pattern_index = arg;
}

#pragma FUNC_ALWAYS_INLINE(TA0_init)
GLOBAL Void TA0_init(Void) {

   array_index          = 0;
   cnt_led              = 0;
   pattern_index        = 0;
   cur_pattern_val      = 0;
   
   req_pattern_index    = 1;  // Initialise with pattern 1 (personal preference)

   CLRBIT(TA0CTL, MC0 | MC1   // stop mode
                  | TAIE      // disable interrupt
                  | TAIFG);   // clear interrupt flag
   CLRBIT(TA0CCTL0, CM1 | CM0 // no capture mode
                  | CAP       // compare mode
                  | CCIE      // disable interrupt
                  | CCIFG);   // clear interrupt flag

   TA0CCR0  = SCALING;        // set up Compare Register with SCALING Factor
   TA0EX0   = TAIDEX_7;       // set up expansion register. IDEX in TAxEX0: {/1, /2, /3, /4, /5, /6, /7, /8}
   TA0CTL   = TASSEL__ACLK    // 614.4 kHz
            | MC__UP          // Up Mode
            | ID__8           // /8     ID in TAxCTL: {/1, /2, /4, /8}
            | TACLR;          // clear and start Timer

   SETBIT(TA0CTL, TAIE        // enable interrupt
                | TAIFG);     // set interrupt flag

   // Get the third blink pattern (should be initializer but will be overwritten by req_pattern_index)
   cur_pattern_val = *(blink_ptr_arr[pattern_index] + array_index);
   SETBIT(P1OUT, BIT2);
}

#pragma vector = TIMER0_A1_VECTOR
__interrupt Void TIMER0_A1_ISR(Void) {

   /*
    * The ISR A1 Timer is defined was defined so that every 250ms a interrupt will be triggered
    */

    // Increment current LED pattern counter (every 250ms)
    cnt_led++;

    // Check if LED was on/off for the current pattern value
    if (cnt_led == cur_pattern_val)
    {
        TGLBIT(P1OUT, BIT2);
        array_index++;
        cnt_led = 0;

        // End of a muster is defined as 0 (see muster definition)
        // This will guarantee that the current blink run to end
        if (*(blink_ptr_arr[pattern_index] + array_index) == 0)
        {
            array_index = 0;
            pattern_index = req_pattern_index;
        }
        
        // Get the currently selected blink pattern
        cur_pattern_val = *(blink_ptr_arr[pattern_index] + array_index);
    }

    CLRBIT(TA0CTL, TAIFG);              // clear interrupt flag
    //__low_power_mode_off_on_exit();   // restore Active Mode on return
}
