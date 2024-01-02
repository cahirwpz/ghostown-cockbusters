package hunk

import (
	"fmt"
	"io"
)

type HunkBss struct {
	Memory HunkMemory
	Size   uint32
}

func readHunkBss(r io.Reader) HunkBss {
	mem, size := hunkSpec(readLong(r))
	return HunkBss{mem, size}
}

func (h HunkBss) Type() HunkType {
	return HUNK_BSS
}

func (h HunkBss) Write(w io.Writer) {
	writeLong(w, HUNK_BSS)
	writeLong(w, h.Size/4)
}

func (h HunkBss) String() string {
	return fmt.Sprintf("%s [%d bytes]\n", HunkNameMap[h.Type()], h.Size)
}
