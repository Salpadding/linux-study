ENTRY(_start)
SECTIONS
{

	. = 0x100000;
	.text :
	{
		_text = .;
        *(.entry)
		*(.text)
		_etext = .;
	}

	.rodata : 
	{
		_rodata = .;	
		*(.rodata)
		_erodata = .;
	}

	. = ALIGN(4096);

	.data :
	{
		_data = .;
		*(.data)		
		_edata = .;
	}


    _edata = . ;

	. = ALIGN(4096);

	.bss :
	{
        . = ALIGN(8);
		_bss = .;
		*(.bss)
        . = ALIGN(8);
		_ebss = .;
	}

    . = ALIGN(4096); 
	_end = .;
}


