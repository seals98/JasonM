/* Cortex-M7 bootloader executing in place from QSPI NOR (Boot ROM starts it).
 * Keep addresses in sync with src/config.h. */
ENTRY(Reset_Handler)

MEMORY
{
    QSPI (rx)  : ORIGIN = 0x60000000, LENGTH = 128K
    RAM  (rwx) : ORIGIN = 0x20000000, LENGTH = 2M
}

_estack = ORIGIN(RAM) + LENGTH(RAM);
BOOT_ROM_HDR_SIZE = 0x1000;

SECTIONS
{
    /* Image header/FCB/IVT the Boot ROM parses: filled in by the SoC port (platform/<name>). */
    .boot_header : {
        KEEP(*(.boot_header))
        . = ORIGIN(QSPI) + BOOT_ROM_HDR_SIZE;
    } > QSPI

    .isr_vector : { . = ALIGN(256); KEEP(*(.isr_vector)) } > QSPI
    .text : { *(.text*) *(.rodata*) . = ALIGN(4); } > QSPI

    /* .ramfunc runs from RAM so flash can be erased/programmed with XIP switched off. */
    _sidata = LOADADDR(.data);
    .data : { _sdata = .; *(.data*) *(.ramfunc*) . = ALIGN(4); _edata = .; } > RAM AT > QSPI

    .bss  : { _sbss = .; *(.bss*) *(COMMON) . = ALIGN(4); _ebss = .; } > RAM

    /* Survives reset (not zeroed by startup) so the app can leave a boot request. */
    .noinit (NOLOAD) : { . = ALIGN(4); *(.noinit*) } > RAM

    /DISCARD/ : { *(.ARM.exidx*) *(.comment) }
}
