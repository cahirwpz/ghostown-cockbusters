/* Linker scripts are documented at:
 * https://sourceware.org/binutils/docs/ld/Scripts.html */
OUTPUT_FORMAT("amiga")
OUTPUT_ARCH(m68k)
PROVIDE(_SysBase = 0x4);
PROVIDE(__ciaa = 0xbfe001);
PROVIDE(__ciab = 0xbfd000);
PROVIDE(__custom = 0xdff000);
SECTIONS
{
  . = 0x0;
  .text :
  {
    __text = .;
    EXCLUDE_FILE(uvmap-rgb.o bumpmap-rgb.o) *(.text)
    *(.text.*)
  }
  __text_size = SIZEOF(.text);
  . = ALIGN(0x0);
  .data :
  {
    __data = .;
    CONSTRUCTORS
    EXCLUDE_FILE(uvmap-rgb.o bumpmap-rgb.o) *(.data)
  }
  __data_size = SIZEOF(.data);
  .bss :
  {
    __bss = .;
    EXCLUDE_FILE(uvmap-rgb.o bumpmap-rgb.o) *(.bss)
    *(COMMON)
  }
  __bss_size = SIZEOF(.bss);
  .datachip :
  {
    __data_chip = .;
    EXCLUDE_FILE(uvmap-rgb.o bumpmap-rgb.o) *(.datachip)
  }
  __data_chip_size = SIZEOF(.datachip);
  .bsschip :
  {
    __bss_chip = .;
    EXCLUDE_FILE(uvmap-rgb.o bumpmap-rgb.o) *(.bsschip)
  }
  __bss_chip_size = SIZEOF(.bsschip);

  .text.uvmap-rgb :
  {
    uvmap-rgb.o(.text .text.*)
  }

  .data.uvmap-rgb :
  {
    uvmap-rgb.o(.data)
  }

  .bss.uvmap-rgb :
  {
    uvmap-rgb.o(.bss)
  }

  .datachip.uvmap-rgb :
  {
    uvmap-rgb.o(.datachip)
  }

  .bsschip.uvmap-rgb :
  {
    uvmap-rgb.o(.bsschip)
  }

  .text.bumpmap-rgb :
  {
    bumpmap-rgb.o(.text .text.*)
  }

  .data.bumpmap-rgb :
  {
    bumpmap-rgb.o(.data)
  }

  .bss.bumpmap-rgb :
  {
    bumpmap-rgb.o(.bss)
  }

  .datachip.bumpmap-rgb :
  {
    bumpmap-rgb.o(.datachip)
  }

  .bsschip.bumpmap-rgb :
  {
    bumpmap-rgb.o(.bsschip)
  }
}
