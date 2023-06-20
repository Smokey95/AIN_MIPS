#include <msp430.h>
#include "..\base.h"
#include "event.h"
#include "uca0.h"

LOCAL const Char * ptr;

#pragma FUNC_ALWAYS_INLINE(UCA0_init)
GLOBAL Void UCA0_init(Void) {

   SETBIT(UCA0CTLW0, UCSWRST);  // UCA0 software reset

   UCA0CTLW1 = 0x0002;          // deglitch time approximately 100 ns
   UCA0BRW   = 4;               // set clock prescaler for 9600 baud
   UCA0MCTLW = 0x00 << 8        // second modulation stage
             | 0x00             // first modulation stage
             | UCOS16;          // enable 16 times oversampling

   UCA0CTLW0 = UCPEN            // enable parity
             | UCPAR            // even parity
             | 0                // LSB first
             | 0                // 8-bit-data
             | 0                // one stop bit
             | UCMODE_0         // UART mode
             | 0                // Asynchronous mode
             | UCSSEL__ACLK     // select clock source: ACLK
             | UCRXEIE          // error interrupt enable
             | UCBRKIE          // break interrupt enable
             | 0;               // release the UCA0 for operation

   UCA0IE    = 0                // Transmit Complete Interrupt Disable
             | 0                // Start Bit Interrupt Disable
             | 0                // Transmit Interrupt Disable
             | UCRXIE;          // Receive Interrupt Enable
}

#pragma vector = USCI_A0_VECTOR
__interrupt Void UCA0_ISR(Void) {
   
   switch (__even_in_range(UCA0IV, 0x04)) {
      
      case 0x02:  // -----------------------------------------------------------------> Vector 2: Receive buffer full
         if (TSTBIT(UCA0STATW, UCBRK | UCRXERR)) {
            Char ch = UCA0RXBUF; // dummy read
            return;
         }
         if (UCA0RXBUF EQ '?') {
            Event_set(EVENT_UART);
            CLRBIT(UCA0IE, UCRXIE);                   // receive interrupt disable
            __low_power_mode_off_on_exit();           // restore Active Mode on return
         }
         break;
         
      case 0x04:  // -----------------------------------------------------------------> Vector 4: Transmit buffer empty
         if (*ptr NE '\0') {
            UCA0TXBUF = *ptr++;
            return;
         }
         CLRBIT(UCA0IE, UCTXIE);                      // transmit interrupt disable
         Char ch = UCA0RXBUF;                         // dummy read
         SETBIT(UCA0IE, UCRXIE);                      // receive interrupt enable
         break;
   }
}

GLOBAL Int UCA0_printf(const Char * str) {
   if (str EQ NULL) {
      return -1;
   }
   ptr = str;
   SETBIT(UCA0IFG, UCTXIFG); // set UCTXIFG
   SETBIT(UCA0IE,  UCTXIE);  // enable transmit interrupt
   return 0;
}
