MEMORY
{
    RAM (rwx) : ORIGIN = 0x10000, LENGTH = 0x2f000 /* Code module area from 0x10000 right to the start of the boot ROM service page */
}

/* produce a link error if there is not this amount of RAM for these sections */
_minimum_stack_size = 8K;
_minimum_heap_size = 16K;

SECTIONS
{
    . = 0x10000;

    .text ALIGN(0x10):
    {
        . = ALIGN(4);
        *(.text*);
        *(.rodata*);
        . = ALIGN(4);
        KEEP(*(.init))
        . = ALIGN(4);
        __preinit_array_start = .;
        KEEP (*(.preinit_array))
        __preinit_array_end = .;
        __init_array_start = .;
        KEEP (*(SORT(.init_array.*)))
        KEEP (*(.init_array))
        __init_array_end = .;
        _etext = .;
        _sidata = _etext;
    } > RAM

    .data : AT ( _sidata )
    {
        . = ALIGN(4);
        _sdata = .;
        _ram_start = .;
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > RAM

    .bss ALIGN(0x10):
    {
        . = ALIGN(4);
        _sbss = .;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > RAM

    /* this is to define the start of the heap, and make sure we have a minimum size */
    .heap :
        {
        . = ALIGN(4);
        _heap_start = .;    /* define a global symbol at heap start */
        . = . + _minimum_heap_size;
        _heap_end = .;
    } >RAM

    /* this just checks there is enough RAM for the stack */
    .stack :
        {
        . = ALIGN(4);
        _sstack = .;
        . = . + _minimum_stack_size;
        . = ALIGN(4);
    } >RAM

    _estack = ORIGIN(RAM) + LENGTH(RAM);
    _ram_end = ORIGIN(RAM) + LENGTH(RAM);
}

