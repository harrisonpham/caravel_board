
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../defs.h"
#include "printf.h"

#define CLOCK_FREQ_HZ 10000000

void init_ram_text() {
  extern uint8_t _ram_text_start[];
  extern uint8_t _ram_text_end[];
  extern uint8_t _ram_text_rom[];

  memcpy(_ram_text_start, _ram_text_rom, _ram_text_end - _ram_text_start);
}

static inline void delay(const int d) {
  // Single shot count down mode.
  reg_timer0_config = 0;
  reg_timer0_data = d;
  reg_timer0_config = 1;

  // Wait until counter reaches 0.
  reg_timer0_update = 1;
  while (reg_timer0_value > 0) {
    reg_timer0_update = 1;
  }
}

__attribute__((always_inline)) static inline void delay_bit() {
  //   757 = 1200 baud
  //    15 = 57600 baud
  // empty = 1 mbaud

  // for (int i = 0; i < 757; ++i) {
  //   asm("nop");
  // }

  for (int i = 0; i < 15; ++i) {
    asm("nop");
  }
}

__attribute__ ((section (".ram_text"))) __attribute__((optimize(2))) void _bitbang_putc(char c) {
  int d = (0b1 << 10) | (c << 2) | (0b01);
  for (int i = 0; i < 11; ++i) {
    reg_gpio_out = d;
    d >>= 1;
    delay_bit();
  }
}

int putc(int c, FILE* f) {
  _bitbang_putc(c);
  return 0;
}

void _putchar(char c) {
  _bitbang_putc(c);
}

int puts(const char* s) {
  while (*s != '\0') {
    _bitbang_putc(*s++);
  }
  return 0;
}

#define REG(addr) (*((volatile uint32_t*)(addr)))

// https://github.com/harrisonpham/randsack_b1/blob/mpw-4-main/ip/randsack/rtl/digitalcore_macro.v
#define RING0_BASE 0x30003000   // collapsering
#define RING1_BASE 0x30003100   // ringosc
#define RING2_BASE 0x30003200   // collapsering, controls rings 2-9
#define RING3_BASE 0x30003300   // collapsering, only supports count reset / read
#define RING4_BASE 0x30003400   // collapsering, only supports count reset / read
#define RING5_BASE 0x30003500   // collapsering, only supports count reset / read
#define RING6_BASE 0x30003600   // ringosc, only supports count reset / read
#define RING7_BASE 0x30003700   // ringosc, only supports count reset / read
#define RING8_BASE 0x30003800   // ringosc, only supports count reset / read
#define RING9_BASE 0x30003900   // ringosc, only supports count reset / read

#define RING_COUNT_OFFSET 0x00
#define RING_CONTROL_OFFSET 0x04
#define RING_TRIMA_OFFSET 0x08
#define RING_TRIMB_OFFSET 0x0c

void wait_collapse(int address) {
  uint32_t last = REG(address + RING_COUNT_OFFSET);
  while (true) {
    delay(CLOCK_FREQ_HZ / 1000);
    uint32_t current = REG(address + RING_COUNT_OFFSET);
    delay(CLOCK_FREQ_HZ / 1000);
    uint32_t current2 = REG(address + RING_COUNT_OFFSET);
    if (current == last && current2 == last) {
      return;
    }
    last = current;
  }
}

void main() {
  init_ram_text();

  // Enable wishbone.
  reg_wb_enable = 1;

  // GPIO output.
  reg_gpio_mode1 = 1;  // pad mode ieb
  reg_gpio_mode0 = 0;  // pad mode oeb
  reg_gpio_ien = 1;
  reg_gpio_oe = 1;

  delay(CLOCK_FREQ_HZ / 10);
  printf("\r\nBoot\r\n");

  while (true) {
    // Reset rings.
    REG(RING0_BASE + RING_CONTROL_OFFSET) = 0b001;
    REG(RING1_BASE + RING_CONTROL_OFFSET) = 0b001;
    REG(RING2_BASE + RING_CONTROL_OFFSET) = 0b001;
    REG(RING3_BASE + RING_CONTROL_OFFSET) = 0b001;
    REG(RING4_BASE + RING_CONTROL_OFFSET) = 0b001;
    REG(RING5_BASE + RING_CONTROL_OFFSET) = 0b001;
    REG(RING6_BASE + RING_CONTROL_OFFSET) = 0b001;
    REG(RING7_BASE + RING_CONTROL_OFFSET) = 0b001;
    REG(RING8_BASE + RING_CONTROL_OFFSET) = 0b001;
    REG(RING9_BASE + RING_CONTROL_OFFSET) = 0b001;
    asm("nop");
    REG(RING0_BASE + RING_CONTROL_OFFSET) = (0b01 << 8) | 0b000;  // div 2
    REG(RING1_BASE + RING_CONTROL_OFFSET) = (0b11 << 8) | 0b000;  // div 8
    REG(RING2_BASE + RING_CONTROL_OFFSET) = (0b01 << 8) | 0b000;  // div 2
    REG(RING3_BASE + RING_CONTROL_OFFSET) = 0b000;
    REG(RING4_BASE + RING_CONTROL_OFFSET) = 0b000;
    REG(RING5_BASE + RING_CONTROL_OFFSET) = 0b000;
    REG(RING6_BASE + RING_CONTROL_OFFSET) = 0b000;
    REG(RING7_BASE + RING_CONTROL_OFFSET) = 0b000;
    REG(RING8_BASE + RING_CONTROL_OFFSET) = 0b000;
    REG(RING9_BASE + RING_CONTROL_OFFSET) = 0b000;
    asm("nop");

    // Trim rings.
    REG(RING0_BASE + RING_TRIMA_OFFSET) = 0x000000ff;
    REG(RING0_BASE + RING_TRIMB_OFFSET) = 0x0000000f;

    REG(RING2_BASE + RING_TRIMA_OFFSET) = 0x000000f3;
    REG(RING2_BASE + RING_TRIMB_OFFSET) = 0x0000000f;

    REG(RING1_BASE + RING_TRIMA_OFFSET) = 0xffffffff;
    asm("nop");

    // Trigger rings.
    REG(RING0_BASE + RING_CONTROL_OFFSET) = (0b01 << 8) | 0b010;  // div 2
    REG(RING1_BASE + RING_CONTROL_OFFSET) = (0b11 << 8) | 0b010;  // div 8
    // REG(RING2_BASE + RING_CONTROL_OFFSET) = (0b01 << 8) | 0b010;  // div 2

    // Wait for collapse.
    wait_collapse(RING0_BASE);
    // wait_collapse(RING2_BASE);
    // wait_collapse(RING3_BASE);
    // wait_collapse(RING4_BASE);
    // wait_collapse(RING5_BASE);

    // Freeze counters, wait for things to settle.
    REG(RING0_BASE + RING_CONTROL_OFFSET) = 0b000;
    REG(RING1_BASE + RING_CONTROL_OFFSET) = 0b000;
    REG(RING2_BASE + RING_CONTROL_OFFSET) = 0b000;
    asm("nop");
    asm("nop");
    asm("nop");
    asm("nop");

    uint32_t ring0 = REG(RING0_BASE + RING_COUNT_OFFSET);
    uint32_t ring1 = REG(RING1_BASE + RING_COUNT_OFFSET);
    // uint32_t ring2 = REG(RING2_BASE + RING_COUNT_OFFSET);
    // uint32_t ring3 = REG(RING3_BASE + RING_COUNT_OFFSET);
    // uint32_t ring4 = REG(RING4_BASE + RING_COUNT_OFFSET);
    // uint32_t ring5 = REG(RING5_BASE + RING_COUNT_OFFSET);
    // uint32_t ring6 = REG(RING6_BASE + RING_COUNT_OFFSET);
    // uint32_t ring7 = REG(RING7_BASE + RING_COUNT_OFFSET);
    // uint32_t ring8 = REG(RING8_BASE + RING_COUNT_OFFSET);
    // uint32_t ring9 = REG(RING9_BASE + RING_COUNT_OFFSET);

    // printf("%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\r\n", ring0, ring1, ring2, ring3, ring4, ring5, ring6, ring7, ring8, ring9);

    printf("%d,%d\r\n", ring0, ring1);

    // printf("Hello world!\r\n");
    // putc(0xaa);
    // puts("Hello world!\r\n");
    // delay(CLOCK_FREQ_HZ / 10000);

    // reg_gpio_out = 1;
    // delay(CLOCK_FREQ_HZ);
    // reg_gpio_out = 0;
    // delay(CLOCK_FREQ_HZ);
  }
}
