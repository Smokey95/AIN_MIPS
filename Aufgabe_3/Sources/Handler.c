#include <msp430.h>
#include "Handler.h"
#include "event.h"
#include "UCA0.h"
#include "UCA1.h"
#include "TA0.h"

#define BASE     10                  // base of the number system can be selected between 2 and 16

// data type of a constant function pointer
typedef Void (* VoidFunc)(Void);

LOCAL Int   pattern_cnt;            // counter for blink pattern from task 1
LOCAL UChar button_index;           // identify the external BCD Button that was pressed
LOCAL UChar bcd_cnt[DIGISIZE];      // BCD counter
LOCAL Char  bcd_uart[DIGISIZE + 2]; // BCD counter array for UART TX (2 additional chars for '\r' and '\n')

// functional prototypes
LOCAL Void State0(Void);
LOCAL Void State1(Void);

LOCAL VoidFunc state;               // function pointer to the current state function
LOCAL UInt idx;                     // index for the BCD counter

LOCAL UInt error;                   // error variable for UART

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
        Event_set(EVENT_TXD);
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

GLOBAL Void get_bcd_cnt(Void) {
    
    // returns the BCD counter as a string with UART line ending (LSB first) e.g. "1234\r\n"
    // the string is stored in bcd_uart
    bcd_uart[0] = bcd_cnt[3] + '0';
    bcd_uart[1] = bcd_cnt[2] + '0';
    bcd_uart[2] = bcd_cnt[1] + '0';
    bcd_uart[3] = bcd_cnt[0] + '0';
    bcd_uart[4] = '\r';
    bcd_uart[5] = '\n';

}

// ---------------------------------------------------------------------------- UART Handling
GLOBAL Void UART_Handler(Void) {
    
    if(Event_tst(EVENT_RXD)) {
        Event_clr(EVENT_RXD);
        bcd_cnt[0] = rx_buf[0] - '0';
        bcd_cnt[1] = rx_buf[1] - '0';
        bcd_cnt[2] = rx_buf[2] - '0';
        bcd_cnt[3] = rx_buf[3] - '0';
        Event_set(EVENT_UPDATE_BCD);
    }
    
    if(Event_tst(EVENT_TXD)) {
        Event_clr(EVENT_TXD);
        get_bcd_cnt();
        UCA0_printf(bcd_uart);
    }
}


// ---------------------------------------------------------------------------- Error Handling
GLOBAL Void Error_Handler(Void) {
    
    if(Event_tst(EVENT_ERR)) {
        Event_clr(EVENT_ERR);
        
        // error handling
        if       (error == BREAK_ERROR) {
            set_blink_muster(MUSTER3);
        } else if(error == FROVPAR_ERROR) {
            set_blink_muster(MUSTER6);
        } else if(error == CHARACTOR_ERROR) {
            set_blink_muster(MUSTER5);
        } else if(error == BUFFER_ERROR) {
            set_blink_muster(MUSTER4);
        } else {
            set_blink_muster(MUSTER1);
        }
    }
}

GLOBAL Void set_error(UChar err) {
    error = err;
    Event_set(EVENT_ERR);
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

