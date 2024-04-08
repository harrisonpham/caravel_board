
# https://github.com/xpack-dev-tools/riscv-none-elf-gcc-xpack/releases/download/v13.2.0-2/xpack-riscv-none-elf-gcc-13.2.0-2-linux-x64.tar.gz
TOOLCHAIN_PATH=/home/harrison/workspace/riscv/xpack-riscv-none-elf-gcc-13.2.0-2/bin/
#TOOLCHAIN_PATH=/usr/bin/
#TOOLCHAIN_PATH=/opt/riscv32imc/bin/
# TOOLCHAIN_PATH=/ef/apps/bin/

# Set the prefix for `riscvXX-unknown-elf-*`
# On installations using `multilib`, this will be `riscv64` even for compiling to 32-bit targets
TOOLCHAIN_PREFIX=riscv-none
#TOOLCHAIN_PREFIX=riscv64
#TOOLCHAIN_PREFIX=riscv32

# ---- Test patterns for project raven ----

.SUFFIXES:

PATTERN = pokey

hex:  ${PATTERN:=.hex}

%.elf: %.c sections.lds crt0_vex.S printf.c
	$(TOOLCHAIN_PATH)$(TOOLCHAIN_PREFIX)-elf-gcc -I../ -I../generated/ -O0 -mabi=ilp32 -march=rv32i_zicsr -D__vexriscv__ -Wl,-Bstatic,-T,sections.lds,--strip-debug -nostartfiles -o $@ crt0_vex.S printf.c $<
	${TOOLCHAIN_PATH}$(TOOLCHAIN_PREFIX)-elf-objdump -D pokey.elf > pokey.lst

%.hex: %.elf
	$(TOOLCHAIN_PATH)$(TOOLCHAIN_PREFIX)-elf-objcopy -O verilog $< $@
	sed -i 's/@1000/@0000/g' $@

%.bin: %.elf
	$(TOOLCHAIN_PATH)$(TOOLCHAIN_PREFIX)-elf-objcopy -O binary $< $@

client: client.c
	gcc client.c -o client

flash: pokey.hex
	python3 ../util/caravel_hkflash.py pokey.hex

flash2: pokey.hex
	python3 ../util/caravel_flash.py pokey.hex

# ---- Clean ----

clean:
	rm -f *.elf *.hex *.bin *.vvp *.vcd

.PHONY: clean hex all flash
