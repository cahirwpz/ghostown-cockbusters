package main

import (
	"flag"
	"fmt"
	"log"
	"os"
	"os/exec"
	"slices"

	"ghostown.pl/hunk"
)

var printHelp bool
var compression bool

func init() {
	flag.BoolVar(&compression, "compression", false,
		"use ZX0 compression on CODE & DATA hunks")
	flag.BoolVar(&printHelp, "help", false,
		"print help message and exit")
}

type Loadable struct {
	Hunks   []hunk.Hunk
	Indices []uint32
}

func zx0(path string, data []byte) []byte {
	var f *os.File
	var fi os.FileInfo
	var err error
	var dir string

	if dir, err = os.Getwd(); err != nil {
		log.Fatal("Getwd:", err)
	}

	if f, err = os.CreateTemp(dir, path); err != nil {
		log.Fatal("CreateTemp:", err)
	}

	name := f.Name()

	defer os.Remove(name)

	if _, err := f.Write(data); err != nil {
		log.Fatal("Write:", err)
	}
	if err = f.Close(); err != nil {
		log.Fatal("Close:", err)
	}

	cmd := exec.Command("salvador", name, name+".zx0")
	if err := cmd.Run(); err != nil {
		log.Fatalf("%s: %v", cmd.String(), err)
	}

	if f, err = os.Open(name + ".zx0"); err != nil {
		log.Fatal("Open:", err)
	}
	if fi, err = f.Stat(); err != nil {
		log.Fatal("Stat:", err)
	}

	output := make([]byte, fi.Size())
	if _, err = f.Read(output); err != nil {
		log.Fatal("Read:", err)
	}
	if err = f.Close(); err != nil {
		log.Fatal("Close:", err)
	}

	defer os.Remove(name + ".zx0")

	return output
}

func splitExe(hunks []hunk.Hunk) []*Loadable {
	var loadables []*Loadable

	header := hunks[0].(*hunk.HunkHeader)

	var loadableHunk uint32 = 0
	seenCodeHunk := false

	var hs []hunk.Hunk
	var is []uint32
	var hdr *hunk.HunkHeader

	for _, h := range hunks[1:] {
		ht := h.Type()

		if ht == hunk.HUNK_CODE {
			if seenCodeHunk {
				loadables = append(loadables, &Loadable{hs, is})
			}

			seenCodeHunk = true
			hdr = &hunk.HunkHeader{}
			hs = []hunk.Hunk{hdr}
			is = []uint32{}
		}

		if ht == hunk.HUNK_CODE || ht == hunk.HUNK_DATA || ht == hunk.HUNK_BSS {
			hs = append(hs, h)
			is = append(is, loadableHunk)

			hdr.Specifiers = append(hdr.Specifiers, header.Specifiers[loadableHunk])
			hdr.Hunks += 1
			hdr.Last = hdr.Hunks - 1

			loadableHunk += 1
		}

		if ht == hunk.HUNK_RELOC32 {
			hs = append(hs, h)
		}

		if ht == hunk.HUNK_END && hs[len(hs)-1].Type() != hunk.HUNK_END {
			hs = append(hs, h)
		}
	}

	return append(loadables, &Loadable{hs, is})
}

func writeExe(exeName string, l *Loadable) {
	header := l.Hunks[0].(*hunk.HunkHeader)

	loadableNum := 0
	for _, h := range l.Hunks {
		switch h.Type() {
		case hunk.HUNK_CODE, hunk.HUNK_DATA:
			if compression {
				hd := h.(*hunk.HunkBin)
				fmt.Printf("Compressing hunk %d: %d", loadableNum, len(hd.Bytes))
				compressed := zx0(exeName, hd.Bytes)
				fmt.Printf(" -> %d\n", len(compressed))
				hd.Bytes = compressed
				header.Specifiers[loadableNum] |= uint32(hunk.HUNKF_ADVISORY)
				header.Specifiers[loadableNum] += 1
			}
			loadableNum += 1
		case hunk.HUNK_BSS:
			loadableNum += 1
		case hunk.HUNK_RELOC32:
			hr := h.(*hunk.HunkReloc32)
			for i, r := range hr.Relocs {
				newIndex := slices.Index(l.Indices, r.HunkRef)
				if newIndex >= 0 {
					hr.Relocs[i].HunkRef = uint32(newIndex)
				} else {
					hr.Relocs[i].HunkRef += uint32(len(l.Indices))
				}
			}
			hr.Sort()
		}
	}

	if compression {
		println()
	}
	println(header.String())

	if err := hunk.WriteFile(exeName, l.Hunks, 0755); err != nil {
		panic("failed to write Amiga Hunk file")
	}
}

func main() {
	flag.Parse()

	if len(flag.Args()) < 1 || printHelp {
		flag.PrintDefaults()
		os.Exit(1)
	}

	hunks, err := hunk.ReadFile(flag.Arg(0))
	if err != nil {
		panic("failed to read Amiga Hunk file")
	}

	if hunks[0].Type() != hunk.HUNK_HEADER {
		panic("Not an executable file")
	}

	loadables := splitExe(hunks)
	n := flag.NArg() - 1

	if len(loadables) != n {
		fmt.Printf("Please provide %d executable file names!\n", len(loadables))
		os.Exit(1)
	}

	for i, loadable := range loadables {
		writeExe(flag.Arg(i+1), loadable)
	}
}
