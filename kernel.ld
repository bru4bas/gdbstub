
SECTIONS {
  .init 0x2000 : {
    load_addr = .;
    *(.init)
  }
  .text : { *(.text) }

  . = ALIGN(0x8);
  rodata_begin = .;
  .rodata : { *(.rodata) }
  rodata_end = .;
  
  . = ALIGN(0x8);
  data_begin = .;
  .data : { *(.data) }

  . = ALIGN(0x8);
  bss_begin = .;
  .bss : { *(.bss*) }
  bss_end = .;

  . = ALIGN(8);
  . = . + 8K;
  stack_irq = .;
  . = . + 8K;
  stack_svr = .;
}
