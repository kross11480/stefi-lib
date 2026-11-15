/* Protected volatile data segment of FIREWALL_VolatileData_Executable:
 * code + data in SRAM1 at 0x2000F100 (3840 B), loaded from flash by main().
 * The load address follows .data: no MEMORY region is named because this
 * script is read before the board script. */
SECTIONS
{
    .fw_vdata 0x2000F100 : AT(LOADADDR(.data) + SIZEOF(.data))
    {
        _fw_vdata_start = .;
        KEEP(*(.fw_vdata.0_pad))     /* first word */
        KEEP(*(.fw_vdata.1_gate))    /* start + 4: the call gate */
        KEEP(*(.fw_vdata*))
        . = ALIGN(4);
        _fw_vdata_end = .;
    }
    _fw_vdata_load = LOADADDR(.fw_vdata);
}
INSERT AFTER .data;
