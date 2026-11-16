MEMORY { ROM (rx) : ORIGIN = 0x00000000, LENGTH = 4M }
SECTIONS { .text : { KEEP(*(.vectors)) *(.text*) *(.rodata*) } > ROM }
