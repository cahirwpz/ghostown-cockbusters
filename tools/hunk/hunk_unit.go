package hunk

import (
	"fmt"
	"io"
)

type HunkUnit struct {
	Name string
}

func readHunkUnit(r io.Reader) HunkUnit {
	return HunkUnit{readString(r)}
}

func (h HunkUnit) Type() HunkType {
	return HUNK_UNIT
}

func (h HunkUnit) Write(w io.Writer) {
	writeLong(w, HUNK_UNIT)
	writeString(w, h.Name)
}

func (h HunkUnit) String() string {
	return fmt.Sprintf("HUNK_UNIT '%s'\n", h.Name)
}
