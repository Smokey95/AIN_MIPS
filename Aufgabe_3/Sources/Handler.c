#include <msp430.h>
#include "Handler.h"
#include "event.h"
#include "UCA1.h"
#include "TA0.h"

#define DIGISIZE 4
#define BASE     10                  // base of the number system can be selected between 2 and 16

// data type of a constant function pointer
typedef Void (* VoidFunc)(Void);

LOCAL Int   pattern_cnt;            // counter for blink pattern from task 1
LOCAL UChar button_index;           // identify the external BCD Button that was pressed
LOCAL UChar bcd_cnt[DIGISIZE];      // BCD counter

// functional prototypes
LOCAL Void State0(Void);
LOCAL Void State1(Void);

LOCAL VoidFunc state;               // function pointer to the current state function
LOCAL UInt idx;                     // index for the BCD counter

// ---------------------------------------------------------------------------- Button Handling

static void BCD_Button_Handler(TEvent arg, UChar bcd_button){
    if(Event_tst(arg)) {
        Event_clr(arg);                 // clear the regarding button event
        button_index = bcd_button;      // set the button index
        Event_set(EVENT_UPDATE_CNT);    // set event for updating BCD
    }
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
        TGLBIT(P2OUT, BIT7);
    }

    BCD_Button_Handler(EVENT_BTN3, 0);
    BCD_Button_Handler(EVENT_BTN4, 1);
    BCD_Button_Handler(EVENT_BTN5, 2);
    BCD_Button_Handler(EVENT_BTN6, 3);

}

// ---------------------------------------------------------------------------- Number Handling

GLOBAL Void Number_Handler(Void) {
    
    if(Event_tst(EVENT_UPDATE_CNT)) {
        Event_clr(EVENT_UPDATE_CNT);

        // checking the PIN is cleaner than handling a second variable
        if(!TSTBIT(P2OUT, BIT7)) //--------------------------------- increment
        {
            bcd_cnt[button_index] = (bcd_cnt[button_index] + 1);
            
            // check on overflow
            if(bcd_cnt[button_index] GE BASE)
            {
                bcd_cnt[button_index] = 0;
                button_index++;
                // check if end of number space is reached
                if(button_index GE DIGISIZE)
                {
                    button_index = 0;
                }
                else
                {
                    Event_set(EVENT_UPDATE_CNT);
                }
            }
        }
        else //--------------------------------------------------- decrement
        {
            bcd_cnt[button_index] = (bcd_cnt[button_index] - 1);
            
            // check on underflow
            if(bcd_cnt[button_index] GE BASE)
            {
                bcd_cnt[button_index] = BASE - 1;
                button_index++;
                
                if(button_index GE DIGISIZE)
                {
                    button_index = 0;
                }
                else
                {
                    Event_set(EVENT_UPDATE_CNT);
                }
            }
        }

        // if there is no more EVENT_UPDATE_CNT pending display can updated
        if(!Event_tst(EVENT_UPDATE_CNT))
        {
            Event_set(EVENT_UPDATE_BCD);
        }
        
    }
    
}

// ---------------------------------------------------------------------------- BCD Handling


static void State0(void) {
    if (Event_tst(EVENT_UPDATE_BCD)) {
        Event_clr(EVENT_UPDATE_BCD);
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
            UChar ch = bcd_cnt[idx - 1];
            //ch += '0';                  // convert to ASCII? Will not display decimal point anymore (is set with D7 = 0 regarding datasheet)
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
    pattern_cnt = MUSTER1;  // Should not be changed (because of initialisation)
    state = State0;         // initial state
    idx = 1;                // initial index
    
    bcd_cnt[0] = 0;         // initial BCD counter
    bcd_cnt[1] = 0;
    bcd_cnt[2] = 0;
    bcd_cnt[3] = 0;
}

