#include <msp430.h>
#include "Handler.h"
#include "event.h"
#include "UCA1.h"
#include "TA0.h"

#define DIGISIZE 4
#define BASE     10                  // base of the number system can be selected between 2 and 16

// data type of a constant function pointer
typedef Void (* VoidFunc)(Void);

LOCAL Int pattern_cnt;            // counter for blink pattern from task 1
LOCAL UChar button_index;         // identify the external BCD Button that was pressed
LOCAL UInt cnt_idx;               // index for the counter
LOCAL UChar bcd_cnt[DIGISIZE];    // BCD counter

// functional prototypes
LOCAL Void State0(Void);
LOCAL Void State1(Void);

LOCAL VoidFunc state;       // function pointer to the current state function
LOCAL UInt idx;             // index for the BCD counter
//@FAQ: (allows to update counter during bcd update?)
LOCAL UChar tmp[DIGISIZE];  // temporary variable for the BCD counter


// ---------------------------------------------------------------------------- Button Handling

LOCAL Void BCD_Button_Handler(TEvent arg, UChar bcd_button){
    Event_clr(arg);                 // clear the regarding button event
    button_index = bcd_button;      // set the button index
    Event_set(EVENT_UPDATE_CNT);    // set event for updating BCD
}

GLOBAL Void Button_Handler(Void) {

    // internal button BTN 2    [select blink pattern]
    if (Event_tst(EVENT_BTN2)) {
        Event_clr(EVENT_BTN2);
        if (++pattern_cnt GT MUSTER6) {
            pattern_cnt = MUSTER1;
         }
         set_blink_muster(pattern_cnt);
    }

    // internal button BTN 1    [inc/dec toggle]
    if (Event_tst(EVENT_BTN1)) {
        Event_clr(EVENT_BTN1);
        TGLBIT(P2OUT, BIT7);    // checking the PIN is cleaner than handling a second variable
    }

    // external button BTN 0    [BCD button at segment 0]
    if(Event_tst(EVENT_BTN3)) {
        BCD_Button_Handler(EVENT_BTN3, 0);
    }

    // external button BTN 1   [BCD button at segment 1]
    if(Event_tst(EVENT_BTN4)) {
        BCD_Button_Handler(EVENT_BTN4, 1);
    }

    // external button BTN 2  [BCD button at segment 2]
    if(Event_tst(EVENT_BTN5)) {
        BCD_Button_Handler(EVENT_BTN5, 2);
    }

    // external button BTN 3 [BCD button at segment 3]
    if(Event_tst(EVENT_BTN6)) {
        BCD_Button_Handler(EVENT_BTN6, 3);
    }

}

// ---------------------------------------------------------------------------- Number Handling

GLOBAL Void Number_Handler(Void) {
    
    if(Event_tst(EVENT_UPDATE_CNT)) {
        Event_clr(EVENT_UPDATE_CNT);

        //@todo: must be expand like defined in the task
        if(TSTBIT(P2OUT, BIT7))     // increment
        {
            bcd_cnt[button_index] = (bcd_cnt[button_index] + 1);
            if(bcd_cnt[button_index] GE BASE)
            {
                bcd_cnt[button_index] = 0;
            }
        }
        else                        // decrement
        {
            bcd_cnt[button_index] = (bcd_cnt[button_index] - 1);
            if(bcd_cnt[button_index] GE BASE)
            {
                bcd_cnt[button_index] = BASE - 1;
            }
        }

        Event_set(EVENT_UPDATE_BCD);
    }
    
}

// ---------------------------------------------------------------------------- BCD Handling

LOCAL Void Copy_Cnt(Void) {
    tmp[0] = bcd_cnt[0];
    tmp[1] = bcd_cnt[1];
    tmp[2] = bcd_cnt[2];
    tmp[3] = bcd_cnt[3];
}


LOCAL Void State0(Void) {
    if (Event_tst(EVENT_UPDATE_BCD)) {
        Event_clr(EVENT_UPDATE_BCD);
        Copy_Cnt();
        idx = 1;
        state = State1;
        Event_set(EVENT_DONE_BCD);
    }
}

LOCAL Void State1(Void) {
    if (Event_tst(EVENT_DONE_BCD)) {
        Event_clr(EVENT_DONE_BCD);
        if (idx LE DIGISIZE) 
        {
            UChar ch = tmp[idx - 1];
            ch += '0';                  // convert to ASCII?

            UCA1_emit(idx, ch);
            idx++;
        }
        else
        {
            state = State0;
        }
    }
}

GLOBAL Void AS1108_Handler(Void) {
    (*state)();
}


// ---------------------------------------------------------------------------- Initialisation

GLOBAL Void Handler_init(Void) {

    pattern_cnt = MUSTER1;  // Should not be changed (because of initialisations)
    state = State0;         // initial state
    idx = 1;                // initial index
    bcd_cnt[0] = 0;         // initial BCD counter
    bcd_cnt[1] = 0;
    bcd_cnt[2] = 0;
    bcd_cnt[3] = 0;
    cnt_idx = 1;            // initial counter index
}

