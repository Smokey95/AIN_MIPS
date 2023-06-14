# Task 2: SPI

In this task the code from task 1 will expanded with the SPI interface. For this there will be a evaluation board with four BCD/7-segment displays which are connected to the SPI interface via the AS1108WL controller.

***

## Function description

### `void Button_Handler(void)`
Will handle the internal (onboard) and external (expansion-board) button events. It is a **stateless** handler which will be called from main-loop. This handler will trigger the `Number_Handler()` if the button pressed was one of the number buttons.

### `void Number_Handler(void)`
Will handle the internal counting as well as a over/underflow handling. This **stateless** handler will repeatedly called itself if there is an over/underflow which it has to handle. Afterward it will trigger the `AS1108_Handler()` which will update the display.

