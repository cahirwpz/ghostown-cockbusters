package hunk

import (
	"fmt"
	"io"
	"strings"
)

type HunkHeader struct {
	Residents  []string
	Hunks      uint32
	First      uint32
	Last       uint32
	Specifiers []uint32
}

func readHunkHeader(r io.Reader) (h HunkHeader) {
	h.Residents = readArrayOfString(r)
	h.Hunks = readLong(r)
	h.First = readLong(r)
	h.Last = readLong(r)
	n := h.Last - h.First + 1
	h.Specifiers = make([]uint32, n)
	for i := 0; i < int(n); i++ {
		h.Specifiers[i] = readLong(r)
	}
	return
}

func (h HunkHeader) Type() HunkType {
	return HUNK_HEADER
}

func (h HunkHeader) Write(w io.Writer) {
	writeLong(w, uint32(HUNK_HEADER))
	writeArrayOfString(w, h.Residents)
	writeLong(w, h.Hunks)
	writeLong(w, h.First)
	writeLong(w, h.Last)
	for _, v := range h.Specifiers {
		writeLong(w, v)
	}
}

func (h HunkHeader) String() string {
	var sb strings.Builder
	sb.WriteString("HUNK_HEADER\n")
	fmt.Fprintf(&sb, "  Residents: %s\n", h.Residents)
	for i, spec := range h.Specifiers {
		mem, size := hunkSpec(spec)
		fmt.Fprintf(&sb, "  Hunk %d: %6d bytes", i+int(h.First), size)
		if mem == HUNKF_CHIP {
			sb.WriteString(" [MEMF_CHIP]\n")
		} else if mem == HUNKF_FAST {
			sb.WriteString(" [MEMF_FAST]\n")
		} else {
			sb.WriteString(" [MEMF_PUBLIC]\n")
		}
	}
	return sb.String()
}
