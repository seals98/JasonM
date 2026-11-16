/* Cortex-M7 bootloader, STM32H7-style memory map. Keep in sync with src/config.h. */
ENTRY(Reset_Handler)

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 128K
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 2M
}

_estack = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS
{
    .isr_vector : { KEEP(*(.isr_vector)) } > FLASH
    .text : { *(.text*) *(.rodata*) . = ALIGN(4); } > FLASH

    _sidata = LOADADDR(.data);
    .data : { _sdata = .; *(.data*) . = ALIGN(4); _edata = .; } > RAM AT > FLASH

    .bss  : { _sbss = .; *(.bss*) *(COMMON) . = ALIGN(4); _ebss = .; } > RAM

    /* Survives reset (not zeroed by startup) so the app can leave a boot request. */
    .noinit (NOLOAD) : { . = ALIGN(4); *(.noinit*) } > RAM

    /DISCARD/ : { *(.ARM.exidx*) *(.comment) }
}
