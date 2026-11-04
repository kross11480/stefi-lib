/* Protected segments of FIREWALL_VolatileData_Shared, added to the board's
 * linker script (INSERT). Code: call gate at segment start + 4. */
SECTIONS
{
    .fw_code 0x08010000 :
    {
        KEEP(*(.fw_code.0_pad))      /* first word: not executable entry */
        KEEP(*(.fw_code.1_gate))     /* start + 4: the call gate */
        KEEP(*(.fw_code*))
    }
    .fw_nvdata 0x080FF000 :
    {
        KEEP(*(.fw_nvdata*))
    }
    .fw_vdata 0x20017000 (NOLOAD) :
    {
        *(.fw_vdata*)
    }
}
INSERT AFTER .bss;
