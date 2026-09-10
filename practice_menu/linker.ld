ENTRY(GameTaskWrapper)

MEMORY {
    ROM (rx)    : ORIGIN = 0x09020000, LENGTH = 256K
    EWRAM (rw)  : ORIGIN = 0x0203D000, LENGTH = 8K
    MODAL (rw)  : ORIGIN = 0x0203B000, LENGTH = 8K
}

SECTIONS {
    .text : ALIGN(4) {
        KEEP(*(.entry))
        KEEP(*(.text*))
        KEEP(*(.rodata*))
        . = ALIGN(4);
    } > ROM

    .bss (NOLOAD) : ALIGN(4) {
        __bss_start__ = .;
        *(.bss.gPracticeState)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        __bss_end__ = .;
    } > EWRAM

    .modal (NOLOAD) : ALIGN(4) {
        __modal_start__ = .;
        KEEP(*(.modal))
        __modal_end__ = .;
    } > MODAL

    /DISCARD/ : {
        *(.comment*)
        *(.ARM.attributes*)
        *(.eh_frame*)
    }
}
