package hunk

import (
	"encoding/binary"
	"fmt"
	"io"
)

/* http://sourceware.org/gdb/current/onlinedocs/stabs/Stab-Types.html */

type StabType uint8

const (
	UNDF       StabType = 0x00
	EXT        StabType = 0x01
	ABS        StabType = 0x02
	TEXT       StabType = 0x04
	DATA       StabType = 0x06
	BSS        StabType = 0x08
	INDR       StabType = 0x0a
	SIZE       StabType = 0x0c
	COMM       StabType = 0x12
	SETA       StabType = 0x14
	SETT       StabType = 0x16
	SETD       StabType = 0x18
	SETB       StabType = 0x1a
	SETV       StabType = 0x1c
	WARNING    StabType = 0x1e
	FN         StabType = 0x1f
	GSYM       StabType = 0x20
	FNAME      StabType = 0x22
	FUN        StabType = 0x24
	STSYM      StabType = 0x26
	LCSYM      StabType = 0x28
	MAIN       StabType = 0x2a
	ROSYM      StabType = 0x2c
	PC         StabType = 0x30
	NSYMS      StabType = 0x32
	NOMAP      StabType = 0x34
	MAC_DEFINE StabType = 0x36
	OBJ        StabType = 0x38
	MAC_UNDEF  StabType = 0x3a
	OPT        StabType = 0x3c
	RSYM       StabType = 0x40
	SLINE      StabType = 0x44
	DSLINE     StabType = 0x46
	BSLINE     StabType = 0x48
	FLINE      StabType = 0x4c
	EHDECL     StabType = 0x50
	CATCH      StabType = 0x54
	SSYM       StabType = 0x60
	ENDM       StabType = 0x62
	SO         StabType = 0x64
	LSYM       StabType = 0x80
	BINCL      StabType = 0x82
	SOL        StabType = 0x84
	PSYM       StabType = 0xa0
	EINCL      StabType = 0xa2
	ENTRY      StabType = 0xa4
	LBRAC      StabType = 0xc0
	EXCL       StabType = 0xc2
	SCOPE      StabType = 0xc4
	RBRAC      StabType = 0xe0
	BCOMM      StabType = 0xe2
	ECOMM      StabType = 0xe4
	ECOML      StabType = 0xe8
	WITH       StabType = 0xea
	NBTEXT     StabType = 0xf0
	NBDATA     StabType = 0xf2
	NBBSS      StabType = 0xf4
	NBSTS      StabType = 0xf6
	NBLCS      StabType = 0xf8
)

var StabTypeMap map[StabType]string

func init() {
	StabTypeMap = map[StabType]string{
		UNDF:       "UNDF",
		EXT:        "EXT",
		ABS:        "ABS",
		TEXT:       "TEXT",
		DATA:       "DATA",
		BSS:        "BSS",
		INDR:       "INDR",
		SIZE:       "SIZE",
		COMM:       "COMM",
		SETA:       "SETA",
		SETT:       "SETT",
		SETD:       "SETD",
		SETB:       "SETB",
		SETV:       "SETV",
		WARNING:    "WARNING",
		FN:         "FN",
		GSYM:       "GSYM",
		FNAME:      "FNAME",
		FUN:        "FUN",
		STSYM:      "STSYM",
		LCSYM:      "LCSYM",
		MAIN:       "MAIN",
		ROSYM:      "ROSYM",
		PC:         "PC",
		NSYMS:      "NSYMS",
		NOMAP:      "NOMAP",
		MAC_DEFINE: "MAC_DEFINE",
		OBJ:        "OBJ",
		MAC_UNDEF:  "MAC_UNDEF",
		OPT:        "OPT",
		RSYM:       "RSYM",
		SLINE:      "SLINE",
		DSLINE:     "DSLINE",
		BSLINE:     "BSLINE",
		FLINE:      "FLINE",
		EHDECL:     "EHDECL",
		CATCH:      "CATCH",
		SSYM:       "SSYM",
		ENDM:       "ENDM",
		SO:         "SO",
		LSYM:       "LSYM",
		BINCL:      "BINCL",
		SOL:        "SOL",
		PSYM:       "PSYM",
		EINCL:      "EINCL",
		ENTRY:      "ENTRY",
		LBRAC:      "LBRAC",
		EXCL:       "EXCL",
		SCOPE:      "SCOPE",
		RBRAC:      "RBRAC",
		BCOMM:      "BCOMM",
		ECOMM:      "ECOMM",
		ECOML:      "ECOML",
		WITH:       "WITH",
		NBTEXT:     "NBTEXT",
		NBDATA:     "NBDATA",
		NBBSS:      "NBBSS",
		NBSTS:      "NBSTS",
		NBLCS:      "NBLCS",
	}
}

type Stab struct {
	StrOff  int32
	BinType uint8
	Other   int8
	Desc    int16
	Value   uint32
}

func (s Stab) External() bool {
	return bool(s.BinType&1 == 1)
}

func (s Stab) Type() StabType {
	return StabType(s.BinType & 254)
}

func readStab(r io.Reader) (s Stab) {
	if binary.Read(r, binary.BigEndian, &s.StrOff) != nil {
		panic("no data")
	}
	if binary.Read(r, binary.BigEndian, &s.BinType) != nil {
		panic("no data")
	}
	if binary.Read(r, binary.BigEndian, &s.Other) != nil {
		panic("no data")
	}
	if binary.Read(r, binary.BigEndian, &s.Desc) != nil {
		panic("no data")
	}
	if binary.Read(r, binary.BigEndian, &s.Value) != nil {
		panic("no data")
	}
	return
}

func (s Stab) Write(w io.Writer) {
	if binary.Write(w, binary.BigEndian, s.StrOff) != nil {
		panic("failed to write Stab.StrOff")
	}
	if binary.Write(w, binary.BigEndian, s.BinType) != nil {
		panic("failed to write Stab.BinType")
	}
	if binary.Write(w, binary.BigEndian, s.Other) != nil {
		panic("failed to write Stab.Other")
	}
	if binary.Write(w, binary.BigEndian, s.Desc) != nil {
		panic("failed to write Stab.Desc")
	}
	if binary.Write(w, binary.BigEndian, s.Value) != nil {
		panic("failed to write Stab.Value")
	}
}

func (s Stab) String(strtab map[int]string) string {
	var visibility rune

	if s.External() {
		visibility = 'g'
	} else {
		visibility = 'l'
	}

	if strtab != nil {
		return fmt.Sprintf("%08x %c %6s %04x %02x %s", s.Value, visibility,
			StabTypeMap[s.Type()], s.Other, s.Desc, strtab[int(s.StrOff)])
	} else {
		return fmt.Sprintf("%08x %c %6s %04x %02x %d", s.Value, visibility,
			StabTypeMap[s.Type()], s.Other, s.Desc, s.StrOff)
	}
}
