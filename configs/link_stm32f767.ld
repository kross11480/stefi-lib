/* STM32F767ZI: 2 MB flash, 512 KB SRAM (DTCM 128 KB + SRAM1 368 KB +
 * SRAM2 16 KB, contiguous from 0x20000000). Caches stay off, so DMA buffers
 * may live anywhere in RAM without cache maintenance. */
ENTRY(Reset_Handler)

MEMORY
{
  RAM      (xrw) : ORIGIN = 0x20000000, LENGTH = 512K
  FLASH    (rx)  : ORIGIN = 0x8000000,  LENGTH = 2048K
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
