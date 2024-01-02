package hunk

import (
	"io"
)

/* Refer to The AmigaDOS Manual (3rd Edition), chapter 10. */

type HunkType uint32

const (
	HUNK_NONE         HunkType = 0
	HUNK_UNIT         HunkType = 999
	HUNK_NAME         HunkType = 1000
	HUNK_CODE         HunkType = 1001
	HUNK_DATA         HunkType = 1002
	HUNK_BSS          HunkType = 1003
	HUNK_RELOC32      HunkType = 1004
	HUNK_RELOC16      HunkType = 1005
	HUNK_RELOC8       HunkType = 1006
	HUNK_EXT          HunkType = 1007
	HUNK_SYMBOL       HunkType = 1008
	HUNK_DEBUG        HunkType = 1009
	HUNK_END          HunkType = 1010
	HUNK_HEADER       HunkType = 1011
	HUNK_OVERLAY      HunkType = 1013
	HUNK_BREAK        HunkType = 1014
	HUNK_DREL32       HunkType = 1015
	HUNK_DREL16       HunkType = 1016
	HUNK_DREL8        HunkType = 1017
	HUNK_LIB          HunkType = 1018
	HUNK_INDEX        HunkType = 1019
	HUNK_RELOC32SHORT HunkType = 1020
	HUNK_RELRELOC32   HunkType = 1021
	HUNK_ABSRELOC16   HunkType = 1022
)

type ExtType uint8

const (
	EXT_NONE      ExtType = 255
	EXT_SYMB      ExtType = 0   // symbol table
	EXT_DEF       ExtType = 1   // relocatable definition
	EXT_ABS       ExtType = 2   // Absolute definition
	EXT_RES       ExtType = 3   // no longer supported
	EXT_GNU_LOCAL ExtType = 33  // GNU local symbol definition
	EXT_REF32     ExtType = 129 // 32 bit absolute reference to symbol
	EXT_COMMON    ExtType = 130 // 32 bit absolute reference to COMMON block
	EXT_REF16     ExtType = 131 // 16 bit PC-relative reference to symbol
	EXT_REF8      ExtType = 132 // 8  bit PC-relative reference to symbol
	EXT_DEXT32    ExtType = 133 // 32 bit data relative reference
	EXT_DEXT16    ExtType = 134 // 16 bit data relative reference
	EXT_DEXT8     ExtType = 135 // 8  bit data relative reference
	EXT_RELREF32  ExtType = 136 // 32 bit PC-relative reference to symbol
	EXT_RELCOMMON ExtType = 137 // 32 bit PC-relative reference to COMMON block
	EXT_ABSREF16  ExtType = 138 // 16 bit absolute reference to symbol
	EXT_ABSREF8   ExtType = 139 // 8 bit absolute reference to symbol
)

type HunkFlag uint32

const (
	/* Any hunks that have the HUNKB_ADVISORY bit set will be ignored if they
	 * aren't understood.  When ignored, they're treated like HUNK_DEBUG hunks.
	 * NOTE: this handling of HUNKB_ADVISORY started as of V39 dos.library!  If
	 * loading such executables is attempted under <V39 dos, it will fail with a
	 * bad hunk type. */
	HUNKB_ADVISORY = 29
	HUNKB_CHIP     = 30
	HUNKB_FAST     = 31

	HUNKF_PUBLIC   HunkFlag = 0
	HUNKF_ADVISORY HunkFlag = 1 << HUNKB_ADVISORY
	HUNKF_CHIP     HunkFlag = 1 << HUNKB_CHIP
	HUNKF_FAST     HunkFlag = 1 << HUNKB_FAST

	HUNKF_MEMORY_MASK = uint32(HUNKF_ADVISORY) | uint32(HUNKF_CHIP) | uint32(HUNKF_FAST)
	HUNKF_SIZE_MASK   = ^HUNKF_MEMORY_MASK
)

func hunkSpec(x uint32) (HunkFlag, uint32) {
	return HunkFlag(x & HUNKF_MEMORY_MASK), (x & HUNKF_SIZE_MASK) * 4
}

type DebugType uint32

const (
	DEBUG_HCLN   DebugType = 0x48434c4e
	DEBUG_HEAD   DebugType = 0x48454144
	DEBUG_LINE   DebugType = 0x4c494e45
	DEBUG_ODEF   DebugType = 0x4f444546
	DEBUG_OPTS   DebugType = 0x4f505453
	DEBUG_ZMAGIC DebugType = 267
)

var HunkNameMap map[HunkType]string
var HunkExtNameMap map[ExtType]string
var HunkFlagMap map[HunkFlag]string

func init() {
	HunkNameMap = map[HunkType]string{
		HUNK_UNIT:         "HUNK_UNIT",
		HUNK_NAME:         "HUNK_NAME",
		HUNK_CODE:         "HUNK_CODE",
		HUNK_DATA:         "HUNK_DATA",
		HUNK_BSS:          "HUNK_BSS",
		HUNK_RELOC32:      "HUNK_RELOC32",
		HUNK_RELOC16:      "HUNK_RELOC16",
		HUNK_RELOC8:       "HUNK_RELOC8",
		HUNK_EXT:          "HUNK_EXT",
		HUNK_SYMBOL:       "HUNK_SYMBOL",
		HUNK_DEBUG:        "HUNK_DEBUG",
		HUNK_END:          "HUNK_END",
		HUNK_HEADER:       "HUNK_HEADER",
		HUNK_OVERLAY:      "HUNK_OVERLAY",
		HUNK_BREAK:        "HUNK_BREAK",
		HUNK_DREL32:       "HUNK_DREL32",
		HUNK_DREL16:       "HUNK_DREL16",
		HUNK_DREL8:        "HUNK_DREL8",
		HUNK_LIB:          "HUNK_LIB",
		HUNK_INDEX:        "HUNK_INDEX",
		HUNK_RELOC32SHORT: "HUNK_RELOC32SHORT",
		HUNK_RELRELOC32:   "HUNK_RELRELOC32",
		HUNK_ABSRELOC16:   "HUNK_ABSRELOC16",
	}

	HunkExtNameMap = map[ExtType]string{
		EXT_SYMB:      "EXT_SYMB",
		EXT_DEF:       "EXT_DEF",
		EXT_ABS:       "EXT_ABS",
		EXT_RES:       "EXT_RES",
		EXT_GNU_LOCAL: "EXT_GNU_LOCAL",
		EXT_REF32:     "EXT_REF32",
		EXT_COMMON:    "EXT_COMMON",
		EXT_REF16:     "EXT_REF16",
		EXT_REF8:      "EXT_REF8",
		EXT_DEXT32:    "EXT_DEXT32",
		EXT_DEXT16:    "EXT_DEXT16",
		EXT_DEXT8:     "EXT_DEXT8",
		EXT_RELREF32:  "EXT_RELREF32",
		EXT_RELCOMMON: "EXT_RELCOMMON",
		EXT_ABSREF16:  "EXT_ABSREF16",
		EXT_ABSREF8:   "EXT_ABSREF8",
	}

	HunkFlagMap = map[HunkFlag]string{
		HUNKF_PUBLIC:   "MEMF_PUBLIC",
		HUNKF_FAST:     "MEMF_FAST",
		HUNKF_CHIP:     "MEMF_CHIP",
		HUNKF_ADVISORY: "ADVISORY",
	}
}

type Hunk interface {
	String() string
	Type() HunkType
	Write(w io.Writer)
}
