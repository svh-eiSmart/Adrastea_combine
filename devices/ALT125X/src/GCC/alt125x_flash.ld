INCLUDE defs.ld

/* Linker script to configure memory regions. */

/* The base address for MCU firmware on flash */
__MCU_MTD_PARTITION_BASE__ = 0x08000000;

/* The header size for MCU firmware */
__HEADER_SIZE__ = 0x200;

/* An instruction cache is present on the I-CODE bus for
 * address 0x1000_0000 to 0x1FFF_FFFF whereas
 * address 0x0000_0000 to 0x0FFF_FFFF is used for direct instruction fetching. */


/* Reserve a memory region for the .uncached section (direct access) */
__UNCACHED_SIZE__ = 0x400;
__UNCACHED_ORIGIN__ = __MCU_MTD_PARTITION_BASE__ + __MCU_MTD_PARTITION_OFFSET__ + __HEADER_SIZE__;

/* To leverage instruction cache, use address 0x1000_0000 to 0x1FFF_FFFF */
__FLASH_ORIGIN__ = __UNCACHED_ORIGIN__ + __UNCACHED_SIZE__ + 0x10000000;


/* A part of the GPM memory is used as an extended heap.
 * By defining __RESERVED_GPM_RAM_SIZE__, one can adjust the size of the second HEAP (default to half of the GPM RAM).
 */
__RESERVED_GPM_RAM_SIZE__ = DEFINED(__RESERVED_GPM_RAM_SIZE__) ? __RESERVED_GPM_RAM_SIZE__ : __GPM_RAM_SIZE__ / 2;


/* The mermoy size for the mailbox. Internal use. */
__MBOX_MEM_LENGTH__ = 128;

MEMORY
{
  /* Memory region for direct code access (reserved for .uncached section) */
  UNCACHED (rx): ORIGIN = __UNCACHED_ORIGIN__ , LENGTH = __UNCACHED_SIZE__

  /* Memory region for code access through instruction cache */
  FLASH (rx): ORIGIN = __FLASH_ORIGIN__ , LENGTH = __MCU_MTD_PARTITION_SIZE__ - __UNCACHED_SIZE__ - __HEADER_SIZE__

  /* Retention RAM from GPM memory */
  GPM_RAM (rwx): ORIGIN = __GPM_RAM_BASE__, LENGTH = __GPM_RAM_SIZE__ - __RESERVED_GPM_RAM_SIZE__

  /* An extended heap from GPM memory */
  RESERVED (rwx): ORIGIN = __GPM_RAM_BASE__ + __GPM_RAM_SIZE__ - __RESERVED_GPM_RAM_SIZE__, LENGTH = __RESERVED_GPM_RAM_SIZE__

  /* Memory regions for the use of inter-processor communication */
  MBOX4_MEM (rw): ORIGIN = 0x0D02FA00, LENGTH = __MBOX_MEM_LENGTH__
  MBOX5_MEM (rw): ORIGIN = 0x0D02FA80, LENGTH = __MBOX_MEM_LENGTH__

  /* Internal SRAM */
  RAM (rwx): ORIGIN = 0x20000, LENGTH = 128K
}

/* Library configurations */
GROUP(libgcc.a libc.a libm.a libnosys.a)

/* Linker script to place sections and symbol values. Should be used together
 * with other linker script that defines memory regions FLASH and RAM.
 * It references following symbols, which must be defined in code:
 *   Reset_Handler : Entry of reset handler
 *
 * It defines following symbols, which code can use without definition:
 *   __exidx_start
 *   __exidx_end
 *   __copy_table_start__
 *   __copy_table_end__
 *   __zero_table_start__
 *   __zero_table_end__
 *   __etext
 *   __data_start__
 *   __preinit_array_start
 *   __preinit_array_end
 *   __init_array_start
 *   __init_array_end
 *   __fini_array_start
 *   __fini_array_end
 *   __data_end__
 *   __bss_start__
 *   __bss_end__
 *   __end__
 *   end
 *   __HeapBase
 *   __HeapLimit
 *   __StackLimit
 *   __StackTop
 *   __stack
 *   __Vectors_End
 *   __Vectors_Size
 */
ENTRY(Reset_Handler)

SECTIONS
{
	.uncached :
	{
		__uncached_start__ = .;
		/* Place vector table in UNCACHED region */
		KEEP(*(.vectors))
		__Vectors_End = .;
		__Vectors_Size = __Vectors_End - __Vectors;
		__end__ = .;
		   *(.uncached*)
		__uncached_end__ = .;

	} > UNCACHED

	/* Check if UNCACHED usage exceeds UNCACHED size */
	ASSERT(LENGTH(UNCACHED) >= (__uncached_end__ - __uncached_start__), "UNCACHED memory overflowed")

	.text : AT(ADDR(.uncached) + __UNCACHED_SIZE__)
	{
		*(.text*)

		KEEP(*(.init))
		KEEP(*(.fini))

		/* .ctors */
		*crtbegin.o(.ctors)
		*crtbegin?.o(.ctors)
		*(EXCLUDE_FILE(*crtend?.o *crtend.o) .ctors)
		*(SORT(.ctors.*))
		*(.ctors)

		/* .dtors */
		*crtbegin.o(.dtors)
		*crtbegin?.o(.dtors)
		*(EXCLUDE_FILE(*crtend?.o *crtend.o) .dtors)
		*(SORT(.dtors.*))
		*(.dtors)

		*(.rodata*)

		KEEP(*(.eh_frame*))
	} > FLASH

	.ARM.extab : AT(LOADADDR(.text) + SIZEOF(.text))
	{
		*(.ARM.extab* .gnu.linkonce.armextab.*)
	} > FLASH

	__exidx_start = .;
	.ARM.exidx : AT(LOADADDR(.ARM.extab) + SIZEOF(.ARM.extab))
	{
		*(.ARM.exidx* .gnu.linkonce.armexidx.*)
	} > FLASH
	__exidx_end = .;

	/* To copy multiple ROM to RAM sections,
	 * uncomment .copy.table section and,
	 * define __STARTUP_COPY_MULTIPLE in startup_ARMCMx.S */
	/*
	.copy.table :
	{
		. = ALIGN(4);
		__copy_table_start__ = .;
		LONG (__etext)
		LONG (__data_start__)
		LONG (__data_end__ - __data_start__)
		LONG (__etext2)
		LONG (__data2_start__)
		LONG (__data2_end__ - __data2_start__)
		__copy_table_end__ = .;
	} > FLASH
	*/

	/* To clear multiple BSS sections,
	 * uncomment .zero.table section and,
	 * define __STARTUP_CLEAR_BSS_MULTIPLE in startup_ARMCMx.S */
	/*
	.zero.table :
	{
		. = ALIGN(4);
		__zero_table_start__ = .;
		LONG (__bss_start__)
		LONG (__bss_end__ - __bss_start__)
		LONG (__bss2_start__)
		LONG (__bss2_end__ - __bss2_start__)
		__zero_table_end__ = .;
	} > FLASH
	*/

	__etext = .;

	.data : AT (LOADADDR(.ARM.exidx) + SIZEOF(.ARM.exidx))
	{
		__data_start__ = .;
		*(vtable)
		*(.data*)

		. = ALIGN(4);
		/* preinit data */
		PROVIDE_HIDDEN (__preinit_array_start = .);
		KEEP(*(.preinit_array))
		PROVIDE_HIDDEN (__preinit_array_end = .);

		. = ALIGN(4);
		/* init data */
		PROVIDE_HIDDEN (__init_array_start = .);
		KEEP(*(SORT(.init_array.*)))
		KEEP(*(.init_array))
		PROVIDE_HIDDEN (__init_array_end = .);


		. = ALIGN(4);
		/* finit data */
		PROVIDE_HIDDEN (__fini_array_start = .);
		KEEP(*(SORT(.fini_array.*)))
		KEEP(*(.fini_array))
		PROVIDE_HIDDEN (__fini_array_end = .);

		KEEP(*(.jcr*))
		. = ALIGN(4);
		/* All data end */
		__data_end__ = .;

	} > RAM

	/* Check if data exceeds RAM */
	ASSERT(__data_end__ <= (ORIGIN(RAM) + LENGTH(RAM)), ".data is too big for RAM")

	.bss :
	{
		. = ALIGN(4);
		__bss_start__ = .;
		*(.bss*)
		*(COMMON)
		. = ALIGN(4);
		__bss_end__ = .;

		__end__ = .;
		end = __end__;
	} > RAM

	/* Check if bss exceeds RAM */
	ASSERT(__bss_end__ <= (ORIGIN(RAM) + LENGTH(RAM)), ".bss is too big for RAM")

	/* .stack_dummy section doesn't contains any symbols. It is only
	 * used for linker to calculate size of stack sections, and assign
	 * values to stack symbols later */
	.stack_dummy (COPY):
	{
		KEEP(*(.stack*))
	} > RAM

	/* Set stack top to end of RAM, and stack limit move down by
	* size of stack_dummy section */
	__StackTop = ADDR(.stack_dummy) + SIZEOF(.stack_dummy);
	__StackLimit = ADDR(.stack_dummy);
	PROVIDE(__stack = __StackTop);

	/* Place holder for additional heap */
	.heap_0 (COPY):
	{
		__HeapBase0 = .;
		. += (ORIGIN(RAM) + LENGTH(RAM) - .);
		__HeapLimit0 = .;
	} > RAM

	/* Check if heap exceeds RAM */
	ASSERT(__HeapLimit0 <= (ORIGIN(RAM) + LENGTH(RAM)), ".heap_0 is too big for RAM")

	/* Placeholder for default single heap */
	.heap (COPY):
	{
		__end__ = .;
		end = __end__;
		__HeapBase = .;
		*(.heap*)
		. += (ORIGIN(RESERVED) + LENGTH(RESERVED) - .);
		__HeapLimit = .;
	} > RESERVED

	/* Check if heap exceeds RESERVED */
	ASSERT(__HeapLimit <= (ORIGIN(RESERVED) + LENGTH(RESERVED)), ".heap is too big for RESERVED")

	PROVIDE(HEAP_SIZE = (__HeapLimit - __HeapBase) + (__HeapLimit0 - __HeapBase0));

	/*
	 * GPMdata section. Data put in GPMdata section are retained even when
	 * MCU operates in standby mode.
	 */
	GPMdata (NOLOAD):
	{
		*(GPMdata)
	} > GPM_RAM

	/*
	 * Mailbox section for Internal-processor communication. Internal use.
	 */
	SecMbox4 (NOLOAD):
	{
		*(SecMbox4)
	} > MBOX4_MEM

	/*
	 * Mailbox section for Internal-processor communication. Internal use.
	 */
	SecMbox5 (NOLOAD):
	{
		*(SecMbox5)
	} > MBOX5_MEM

}
