/* OUTPUT_FORMAT("elf64-x86-64") */
/* OUTPUT_ARCH(i386:x86-64) */
ENTRY(_start)

PHDRS {
 text PT_LOAD FILEHDR FLAGS(7);
 note PT_NOTE FLAGS(4);
 stack PT_GNU_STACK FLAGS(7);
}

SECTIONS { 
    PROVIDE (__executable_start = SEGMENT_START("text-segment", 0x400000));
	. = SEGMENT_START("text-segment", 0x400000) + SIZEOF_HEADERS;
	.text :	{
		_text = .; 	/* Text */
		*(.text)
		*(.text.*)
		*(.noinstr.text)
		_etext = . ;
	}:text
	.rodata : {
		_rodata = . ;
		*(.rodata)	 /* read-only data */
		*(.rodata.*)
		_erodata = . ;
	}
	.data :	{
		_data = . ;
		*(.data)
		*(.data.*)
		*(.bss.efistub)
		_edata = . ;
	}
	. = ALIGN(64);
	.bss : {
		_bss = . ;
		*(.bss)
		*(.bss.*)
		*(COMMON)
		. = ALIGN(8);	/* For convenience during zeroing */
		_ebss = .;
	}

	/DISCARD/ : {
		*(.note.gnu.property)
	}
}

