
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
  // 757 = 1200 baud
  //  15 = 57600 baud

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

#define RING0_BASE 0x30003000   // collapsering
#define RING1_BASE 0x30003100   // ringosc

#define RING_COUNT_OFFSET 0x00
#define RING_CONTROL_OFFSET 0x04
#define RING_TRIMA_OFFSET 0x08
#define RING_TRIMB_OFFSET 0x0c

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
    asm("nop");
    REG(RING0_BASE + RING_CONTROL_OFFSET) = 0b000;
    REG(RING1_BASE + RING_CONTROL_OFFSET) = (0b11 << 8) | 0b000;  // clk div 8
    asm("nop");

    // Trim
    REG(RING0_BASE + RING_TRIMA_OFFSET) = 0x000000ff;
    REG(RING0_BASE + RING_TRIMB_OFFSET) = 0x0000000f;

    REG(RING1_BASE + RING_TRIMA_OFFSET) = 0x00000000;

    // Trigger rings.
    REG(RING0_BASE + RING_CONTROL_OFFSET) = 0b010;
    REG(RING1_BASE + RING_CONTROL_OFFSET) = (0b11 << 8) | 0b010;  // clk div 8

    // Wait for collapse.
    uint32_t last = REG(RING0_BASE + RING_COUNT_OFFSET);
    while (true) {
      delay(CLOCK_FREQ_HZ / 100);
      uint32_t current = REG(RING0_BASE + RING_COUNT_OFFSET);
      if (current == last) {
        break;
      }
      last = current;
    }

    // Freeze counters, wait for things to settle.
    REG(RING0_BASE + RING_CONTROL_OFFSET) = 0b000;
    REG(RING1_BASE + RING_CONTROL_OFFSET) = 0b000;
    asm("nop");
    asm("nop");
    asm("nop");
    asm("nop");

    uint32_t ring0 = REG(RING0_BASE + RING_COUNT_OFFSET);
    uint32_t ring1 = REG(RING1_BASE + RING_COUNT_OFFSET);

    // printf("RING0=%8u RING1=%8u\r\n", ring0, ring1);
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
