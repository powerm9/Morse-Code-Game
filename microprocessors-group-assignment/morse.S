#include "hardware/regs/addressmap.h"
#include "hardware/regs/io_bank0.h"
#include "hardware/regs/timer.h"
#include "hardware/regs/m0plus.h"

.syntax unified
.cpu    cortex-m0plus
.thumb
.global main_asm
.align  4

.equ    DFLT_ALARM_TIME,  1000000     @ Specify the default alarm timeout
.equ    MORSE_DOT_TIME,   150000      @ Time threshold for dot 0.1->(in microsecond
.equ    MORSE_DASH_TIME,  450000      @ Time threshold for dash 0.3sec->(in microseconds)
.equ    MORSE_PAUSE_TIME, 1000000     @ Time threshold for pause 1sec->(in microseconds)
.equ    DFLT_INPUT_LENGTH,    0       @ Default input length 


.equ    GPIO_BTN_MKEYFAL_MSK, 0x00800000   @ Bit-22 for falling-edge event on GP21
.equ    GPIO_BTN_MKEYRIS_MSK, 0x00400000   @ rising edge event on gp 21 which is double falling edge


.equ    GPIO_BTN_MKEY,  21              @ Specify pin for the "morse key" button

.equ    GPIO_LED_PIN, 25              @ Specify pin for the built-in LED
.equ    GPIO_DIR_IN,   0              @ Specify input direction for a GPIO pin
.equ    GPIO_DIR_OUT,  1              @ Specify output direction for a GPIO pin

.equ    GPIO_ISR_OFFSET, 0x74         @ GPIO is int #13 (vector table entry 29)
.equ    ALRM_ISR_OFFSET, 0x40         @ ALARM0 is int #0 (vector table entry 16)

@ Entry point to the ASM portion of the program
main_asm:
    push    {lr}
    bl      init_btn                    @ Jump to init_BTN
    bl      install_alarm               @ Jump to install_ALRM
    bl      install_gpio                @ Jump to install_GPIO
    pop     {pc}
    @ wfi                                 @ Wait for first input 

@ main_loop:
@     bl    alarm_setup                   @ Branch to alarm setup
@     wfi                                 @ Wait for interrupt
@     b     main_loop                     @ Jump back to beginning main_loop


init_btn:                               @ Enter subroutine and store link register to stack
    push    {lr}
    movs    r0, #GPIO_BTN_MKEY          @ Move 21 to r0        
    bl      asm_gpio_init               @ Branch to C function to initialize pin 21 (gp21)    
    movs    r0, #GPIO_BTN_MKEY          @ Move 21 to r0       
    movs    r1, #GPIO_DIR_IN            @ Move 0 to r1    
    bl      asm_gpio_set_dir            @ Branch to C function which sets pin to be input or output    
    movs    r0, #GPIO_BTN_MKEY          @ Move 21 to r0        
    bl      asm_gpio_set_irq_rise       @ Set rising edge interrupt for 21 
    movs    r0, #GPIO_BTN_MKEY          @ Move 21 to r0
    bl      asm_gpio_set_irq_fall       @ Set falling edge interrupt for 21
    pop     {pc}

alarm_setup:
    movs    r1, #1                                     @ Store 1 value to r1
    ldr     r2, =(TIMER_BASE + TIMER_INTE_OFFSET)      @ Load address of TIMER_INTE_OFFSET into r2
    str     r1, [r2]                                   @ Set TIMER_INTE_OFFSET to value in r1 to enable alarm timer interrupts
    ldr     r2, = (TIMER_BASE + TIMER_TIMELR_OFFSET)   @ Load TIMER_TIMELR_OFFSET
    ldr     r1, [r2]                                   @ Derefence memory to load address at memory into r1
    ldr     r0, =ltimer                                @ Load default timer value address into r0
    ldr     r0, [r0]                                   @ Load load value from timer value address into r0
    adds    r0, r1                                     @ Add into r0, default timer value and current timer value
    ldr     r2, =(TIMER_BASE + TIMER_ALARM0_OFFSET)    @ Load address of TIMER_ALARM0_OFFSET into r2
    str     r0, [r2]                                   @ Set TIMER_ALARM0_OFFSET to updated value in r1
    bx      lr                                         @ Return from subroutine

install_alarm:
    ldr     r2, =(PPB_BASE + M0PLUS_VTOR_OFFSET)       @ Load Vector Table Offset Register address into r1
    ldr     r1, [r2]                                   @ Load value from Vector Table Offset Register into r1
    movs    r2, #ALRM_ISR_OFFSET                       @ Load ALRM_ISR_OFFSET into r2
    add     r2, r1                                     @ Add offset to base address to get address of ALRM ISR
    ldr     r0, =alarm_isr                             @ Load address of alrm_isr into r0
    str     r0, [r2]                                   @ Store address of alrm_isr in interrupt vector table
    ldr     r2, =(PPB_BASE + M0PLUS_NVIC_ICPR_OFFSET)  @ Load address of NVIC_ICPR register into r2
    movs    r1, #1                                     @ Load value 1 into r1    
    str     r1, [r2]                                   @ Set interrupt clear-pending register to clear any pending interrupts
    ldr     r2, =(PPB_BASE + M0PLUS_NVIC_ISER_OFFSET)  @ Load address of NVIC_ISER register into r2
    movs    r1, #1                                     @ Load value 1 into r1    
    str     r1, [r2]                                   @ Set interrupt enable register to enable corresponding interrupt
    bx      lr                                         @ Return from function

.thumb_func
alarm_isr:                                             @ Alarm Interrupt Service Routine
    push    {lr}                                       @ Enter subroutine and store link register to stack
    ldr     r2, =(TIMER_BASE + TIMER_INTR_OFFSET)      @ Add TIMER_BASE and TIMER_INTR_OFFSET and store it in r2
    movs    r1, #1                                     @ Load 1 into r1 to set the ICPR (interrupt clear-pending register) high
    str     r1, [r2]                                   @ Store 
    movs    r0, #3                                     @ Load " " into r0
    adds    r7, #1
    bl      morse_parse                                    @ Print value in r0
    bl      alarm_setup                                @ set alarm for the input                                     @ Add #1 into r7
    cmp     r7, #2                                     @ Compare r7 and #2
    beq     morse_input                                @ Branch if they are equal (two alarm cycles have passed and the input is finished)
    pop     {pc}                                       @ Pop program counter from stack and return

install_gpio:
    ldr     r2, =(PPB_BASE + M0PLUS_VTOR_OFFSET)       @ Load Vector Table Offset Register address into r1
    ldr     r1, [r2]                                   @ Load value from Vector Table Offset Register into r1
    movs    r2, #GPIO_ISR_OFFSET                       @ Load GPIO_ISR_OFFSET into r2
    add     r2, r1                                     @ Add offset to base address to get address of ALRM ISR
    ldr     r0, =gpio_isr                              @ Load address of gpio_isr into r0
    str     r0, [r2]                                   @ Store address of gpio_isr in interrupt vector table
    ldr     r2, =(PPB_BASE + M0PLUS_NVIC_ICPR_OFFSET)  @ Load address of NVIC_ICPR register into r2
    movs    r0, #1                                     @ Load value 1 into r1
    lsls    r0, #13                                    @ Shift bits left by 13 positions (Since GPIO is int #13)
    str     r0, [r1]                                   @ Set interrupt clear-pending register to clear any pending interrupts
    ldr     r1, =(PPB_BASE + M0PLUS_NVIC_ISER_OFFSET)  @ Load address of NVIC_ISER register into r2
    str     r0, [r1]                                   @ Set interrupt enable register to enable corresponding interrupt
    bx      lr                                         @ Return from function

.thumb_func
gpio_isr:                                                           @ Alarm Interrupt Service Routine
    push    {lr}                                                    @ Enter subroutine and store link register to stack
    ldr     r0, =(IO_BANK0_BASE + IO_BANK0_PROC0_INTS2_OFFSET)      @ Load GPIO interrupt register address into r0
    ldr     r0, [r0]                                                @ Load GPIO interrupt register value
@-------------KeyPressed?-------------
    ldr     r4, =GPIO_BTN_MKEYRIS_MSK                               @ Load bitmask for morse key rising edge event
    cmp     r0, r4                                                  @ Compare result with 0
    beq     rising_event                                            @ Branch to rising event equal
    ldr     r4, =GPIO_BTN_MKEYFAL_MSK                               @ Load bitmask for morse key falling edge event
    cmp     r0, r4                                                  @ Compare result with 0 (value loaded from interupt)
    beq     falling_event                                           @ Branch to falling event if equal

rising_event:
    bl      stop_BTN_check                                          @ Clear interrupt
    bl      time_check1                                             @ Get first timestamp
    movs    r5, r0                                                  @ Put in r5 for later
    bl      watchdog_update
    pop     {pc}                                                    @ Go back to main loop and wait for next interrupt (falling edge)

falling_event:
    bl      stop_BTN_check                                          @ Clear button isr handler
    bl      time_check2                                             @ Second time stamp
    bl      time_diff                                               @ Time difference between time stamp 1 and 2 in r0 now
    movs    r6, r0                                                  @ Save in r6 for later
    bl      alarm_setup                                             @ Branch to alarm setup which will print spaces or end input now
    bl      detect_morse                                            @ Branch to value comparor 

@HowLongThough?------------------------------
detect_morse:
    movs    r0, r6                        @ Get time difference into r0
    ldr     r1, =ldottime                 @ Load dot time threshold address value into r1
    ldr     r1, [r1]                      @ Get value from address
    cmp     r0, r1                        @ Compare time difference with dot time threshold
    blt     short_press                   @ If current timer value is less than dot time threshold jump to short_press to print dot

    ldr     r1, =ldashtime                @ Load dash time threshold address into r1
    ldr     r1, [r1]                      @ Dereference
    cmp     r0, r1                        @ Compare time difference with dash time threshold
    blt     long_press                    @ If current timer value is less than dash time threshold, jump to long_press to print dash
    b       end_detect_morse              @ If greater than end detection, invalid input and return to main loop
short_press:
    movs    r0, #1                  @ Load dot message
    bl      morse_parse                        @ Print
    movs    r7, #0                        @ restart alarm pass counter  
    b       end_detect_morse              @ Return main loop 
long_press:
    movs    r0, #2                 @ Load dash message
    bl      morse_parse                        @ Print
    movs    r7, #0                        @ restart alarm pass counter  
    b       end_detect_morse              @ Return main loop 
end_detect_morse:
    pop     {pc}                          @ Return main loop

@-------------------------------------------
stop_BTN_check:                                             
    ldr     r0, =(IO_BANK0_BASE + IO_BANK0_INTR2_OFFSET)    @ Load GPIO raw interrupt register address into r0
    str     r4, [r0]                                        @ Store value in r1 to r0 memory address
    bx      lr                                              @ Pop program counter from stack and return

morse_input:
    movs    r7, #0                      @ Reset alarm pass counter
    movs    r0, #4                @ Load return message
    bl      morse_parse                     @ Print
    pop     {pc}                        @ Return to main loop

.align 4
dot_msg:     .asciz "."                 @ Define dot message
dash_msg:    .asciz "-"                 @ Define dash message
spac_msg:    .asciz " "                 @ Define space message     
rtn_msg:     .asciz "\n"                @ Define new line message

.data
inputlength: .word  DFLT_INPUT_LENGTH      @ Define input length time variable
ldottime:    .word  MORSE_DOT_TIME         @ Define dot time variable
ldashtime:   .word  MORSE_DASH_TIME        @ Define dash time
lspacetime:  .word  MORSE_PAUSE_TIME       @ Define default alarm time variable
ltimer:      .word  DFLT_ALARM_TIME        @ Define default alarm time variable
