/* STM32L476RG: 1 MB flash, 128 KB SRAM — SRAM1 (96 KB at 0x20000000) and
 * SRAM2 (32 KB, aliased at 0x20018000), contiguous. The default configs/
 * link.ld only uses 32 KB (it is shared with the G431). */
ENTRY(Reset_Handler)

MEMORY
{
  RAM      (xrw) : ORIGIN = 0x20000000, LENGTH = 128K
  FLASH    (rx)  : ORIGIN = 0x8000000,  LENGTH = 1024K
}

_estack = ORIGIN(RAM) + LENGTH(RAM); /* ram end */

SECTIONS
{

	.isr_vector     : { KEEP(*(.isr_vector)) }  > FLASH

	.text :
        {
            *(.text*)   /* All .text and .text.function_name, no keep */
            *(.rodata*) /* Read-only data */
        } > FLASH

	.ARM.exidx : { *(.ARM.exidx*) } > FLASH

	.data :
	{
		. = ALIGN(4);
		_sdata = .;
		*(.data*)
		. = ALIGN(4);
		_edata = .;
	} > RAM AT > FLASH
	_sidata = LOADADDR(.data);

	.bss (NOLOAD):
	{
		. = ALIGN(4);
		_sbss = .;
		*(.bss*)
		*(COMMON)
		. = ALIGN(4);
		_ebss = .;
	} > RAM

	. = ALIGN(8);
    _end = .;
}
