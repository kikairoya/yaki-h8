OUTPUT_ARCH(h8300:h8300hn)
OUTPUT_FORMAT(elf32-h8300)
ENTRY("_start")

MEMORY {
  ram0(rwx) : o = 0xF780, l = 1024
  ram1(rw)  : o = 0xFB80, l = 1024 - 2
  stack(rw) : o = 0xFF80 - 2, l = 2
}

SECTIONS {
  .text 0xF780 : {
    *(.text)
	*(.rodata)
  } > ram0
  .data : {
	*(.data)
  } > ram0
  .bss : {
    *(.bss)
    *(COMMON)
  } > ram1
  .stack : {
    _end = ABSOLUTE(.);
  } > stack
  .stab 0 (NOLOAD) : {
    [ .stab ]
  }
  .stabstr 0 (NOLOAD) : {
    [ .stabstr ]
  }
  /DISCARD/ : {
    *(.comment)
  }
}
