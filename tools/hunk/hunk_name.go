package hunk

import (
	"fmt"
	"io"
)

type HunkName struct {
	Name string
}

func readHunkName(r io.Reader) HunkName {
	return HunkName{readString(r)}
}

func (h HunkName) Type() HunkType {
	return HUNK_NAME
}

func (h HunkName) Write(w io.Writer) {
	writeLong(w, uint32(HUNK_NAME))
	writeStringWithSize(w, h.Name)
}

func (h HunkName) String() string {
	return fmt.Sprintf("HUNK_NAME '%s'\n", h.Name)
}
