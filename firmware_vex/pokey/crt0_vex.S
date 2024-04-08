.global main
.global _start

_start:
  j crt_init

.global  trap_entry
trap_entry:
  mret

.text

crt_init:
  la sp, _fstack
  la a0, trap_entry
  csrw mtvec, a0

data_init:
  la a0, _fdata
  la a1, _edata
  la a2, _fdata_rom
data_loop:
  beq a0,a1,data_done
  lw a3,0(a2)
  sw a3,0(a0)
  add a0,a0,4
  add a2,a2,4
  j data_loop
data_done:

bss_init:
  la a0, _fbss
  la a1, _ebss
bss_loop:
  beq a0,a1,bss_done
  sw zero,0(a0)
  add a0,a0,4
#ifndef SIM
  j bss_loop
#endif
bss_done:

  //li a0, 0x880  //880 enable timer + external interrupt sources (until mstatus.MIE is set, they will never trigger an interrupt)
  //csrw mie,a0

  call main
infinit_loop:
  j infinit_loop
