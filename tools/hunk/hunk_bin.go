package hunk

import (
	"encoding/hex"
	"fmt"
	"io"
)

type HunkBin struct {
	htype HunkType
	Bytes []byte
}

func readHunkCode(r io.Reader) HunkBin {
	return HunkBin{HUNK_CODE, readData(r, readLong(r)*4)}
}

func readHunkData(r io.Reader) HunkBin {
	return HunkBin{HUNK_DATA, readData(r, readLong(r)*4)}
}

func (h HunkBin) Type() HunkType {
	return h.htype
}

func (h HunkBin) Write(w io.Writer) {
	writeLong(w, uint32(h.htype))
	writeData(w, h.Bytes)
}

func (h HunkBin) String() string {
	return fmt.Sprintf(
		"%s [%d bytes]\n%s", HunkNameMap[h.Type()], len(h.Bytes), hex.Dump(h.Bytes))
}
