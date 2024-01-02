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
	writeLong(w, HUNK_NAME)
	writeString(w, h.Name)
}

func (h HunkName) String() string {
	return fmt.Sprintf("HUNK_NAME '%s'\n", h.Name)
}
