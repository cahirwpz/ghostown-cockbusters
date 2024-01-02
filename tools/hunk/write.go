package hunk

import (
	"encoding/binary"
	"io"
	"os"
)

func writeWord(w io.Writer, x uint16) {
	if err := binary.Write(w, binary.BigEndian, x); err != nil {
		panic("failed to write word")
	}
}

func writeLong(w io.Writer, x uint32) {
	if err := binary.Write(w, binary.BigEndian, x); err != nil {
		panic("failed to write long")
	}
}

/*
func writeArrayOfLong(w io.Writer, xs []uint32) {
	writeLong(w, uint32(len(xs)))
	for _, v := range xs {
		writeLong(w, v)
	}
}
*/

func writeData(w io.Writer, xs []byte) {
	n := len(xs)
	writeLong(w, uint32((n+3)/4))
	binary.Write(w, binary.BigEndian, xs)
	k := n % 4
	if k > 0 {
		for i := 0; i < 4-k; i++ {
			binary.Write(w, binary.BigEndian, byte(0))
		}
	}
}

func writeString(w io.Writer, s string) {
	writeData(w, []byte(s))
}

func writeArrayOfString(w io.Writer, ss []string) {
	for _, s := range ss {
		writeString(w, s)
	}
	writeLong(w, 0)
}

func WriteFile(path string, hunks []Hunk) (err error) {
	file, err := os.OpenFile(path, os.O_WRONLY|os.O_CREATE, 0644)
	defer file.Close()

	if err != nil {
		return
	}

	for _, hunk := range hunks {
		hunk.Write(file)
	}

	return nil
}
