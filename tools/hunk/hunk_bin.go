package hunk

import (
	"encoding/hex"
	"fmt"
	"io"
)

type HunkBin struct {
	htype  HunkType
	Memory HunkMemory
	Bytes  []byte
}

func readHunkBin(r io.Reader, htype HunkType) HunkBin {
	h := readLong(r)
	return HunkBin{htype, HunkMemory(h & HUNKF_MASK), readData(r, h*4)}
}

func (h HunkBin) Type() HunkType {
	return h.htype
}

func (h HunkBin) Write(w io.Writer) {
	writeLong(w, uint32(h.htype))
	writeLong(w, uint32(h.Memory)|bytesSize(h.Bytes))
	writeData(w, h.Bytes)
}

func (h HunkBin) String() string {
	return fmt.Sprintf(
		"%s [%d bytes]\n%s", HunkNameMap[h.Type()], len(h.Bytes), hex.Dump(h.Bytes))
}
