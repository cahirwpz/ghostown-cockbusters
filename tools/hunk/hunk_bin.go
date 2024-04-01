package hunk

import (
	"bytes"
	"encoding/hex"
	"fmt"
	"io"
)

// This is private, non-standard extension!
type BinHunkFlag uint32

const (
	BHF_COMP      BinHunkFlag = 4 << 29
	BHF_ALGO_ZX0  BinHunkFlag = 0 << 29
	BHF_ALGO_LZSA BinHunkFlag = 1 << 29
	BHF_ALGO_MASK BinHunkFlag = 3 << 29

	BHF_COMP_MASK = uint32(BHF_COMP) | uint32(BHF_ALGO_MASK)
	BHF_SIZE_MASK = ^BHF_COMP_MASK
)

func (flag BinHunkFlag) String() string {
	if flag&BHF_COMP == 0 {
		return ""
	}

	switch flag & BHF_ALGO_MASK {
	case BHF_ALGO_ZX0:
		return "ZX0"
	case BHF_ALGO_LZSA:
		return "LZSA"
	default:
		return "???"
	}
}

func binHunkSpec(x uint32) (BinHunkFlag, uint32) {
	return BinHunkFlag(x & BHF_COMP_MASK), (x & BHF_SIZE_MASK) * 4
}

type HunkBin struct {
	htype HunkType
	Flags BinHunkFlag
	Data  *bytes.Buffer
}

func readHunkBin(r io.Reader, htype HunkType) *HunkBin {
	flags, size := binHunkSpec(readLong(r))
	return &HunkBin{htype, flags, bytes.NewBuffer(readData(r, size))}
}

func (h HunkBin) Type() HunkType {
	return h.htype
}

func (h HunkBin) Write(w io.Writer) {
	writeLong(w, uint32(h.htype))
	writeLong(w, uint32(h.Flags)|bytesSize(h.Data.Bytes()))
	writeData(w, h.Data.Bytes())
}

func (h HunkBin) String() string {
	var flags string
	if h.Flags != 0 {
		flags = h.Flags.String() + ", "
	} else {
		flags = ""
	}
	return fmt.Sprintf("%s [%s%d bytes]\n%s", h.Type().String(),
		flags, h.Data.Len(), hex.Dump(h.Data.Bytes()))
}
