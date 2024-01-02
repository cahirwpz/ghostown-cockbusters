package hunk

import (
	"encoding/hex"
	"fmt"
	"io"
)

type HunkBin struct {
	htype  HunkType
	Memory MemoryType
	Bytes  []byte
}

func readHunkBin(r io.Reader, htype HunkType) HunkBin {
	mem, size := hunkSpec(readLong(r))
	return HunkBin{htype, mem, readData(r, size)}
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
