package main

import (
	"flag"
	"fmt"
	"log"
	"os"
	"os/exec"

	"ghostown.pl/hunk"
)

var unpack bool
var printHelp bool

func init() {
	flag.BoolVar(&unpack, "unpack", false,
		"unpack hunks instead of packing them")
	flag.BoolVar(&printHelp, "help", false,
		"print help message and exit")
}

type Action int

const (
	Pack   Action = 0
	Unpack Action = 1
)

func zx0(data []byte, action Action) []byte {
	var f *os.File
	var fi os.FileInfo
	var err error
	var dir string

	if dir, err = os.Getwd(); err != nil {
		log.Fatal("Getwd:", err)
	}

	if f, err = os.CreateTemp(dir, flag.Arg(0)); err != nil {
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

	var cmd *exec.Cmd
	if action == Pack {
		cmd = exec.Command("salvador", name, name+".zx0")
	} else {
		cmd = exec.Command("salvador", "-d", name, name+".zx0")
	}

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

func packExe(hs []hunk.Hunk) {
	header, ok := hs[0].(*hunk.HunkHeader)
	if !ok {
		panic("Not an executable file")
	}

	loadableNum := 0
	for _, h := range hs {
		switch h.Type() {
		case hunk.HUNK_CODE, hunk.HUNK_DATA:
			if header.Specifiers[loadableNum]&uint32(hunk.HUNKF_ADVISORY) != 0 {
				fmt.Printf("Hunk %d (%s): already packed\n", loadableNum,
					h.Type().String())
			} else {
				hd := h.(*hunk.HunkBin)
				fmt.Printf("Compressing hunk %d (%s): %d", loadableNum,
					h.Type().String(), len(hd.Bytes))
				packed := zx0(hd.Bytes, Pack)
				fmt.Printf(" -> %d\n", len(packed))
				if len(packed) >= len(hd.Bytes) {
					println("Skipping compression...")
				} else {
					hd.Bytes = packed
					header.Specifiers[loadableNum] |= uint32(hunk.HUNKF_ADVISORY)
					header.Specifiers[loadableNum] += 1
				}
			}
			loadableNum += 1
		case hunk.HUNK_BSS:
			loadableNum += 1
		}
	}
}

func unpackExe(hs []hunk.Hunk) {
	header, ok := hs[0].(*hunk.HunkHeader)
	if !ok {
		panic("Not an executable file")
	}

	loadableNum := 0
	for _, h := range hs {
		switch h.Type() {
		case hunk.HUNK_CODE, hunk.HUNK_DATA:
			if header.Specifiers[loadableNum]&uint32(hunk.HUNKF_ADVISORY) == 0 {
				fmt.Printf("Hunk %d (%s): already unpacked\n", loadableNum,
					h.Type().String())
			} else {
				hd := h.(*hunk.HunkBin)
				fmt.Printf("Decompressing hunk %d (%s): %d", loadableNum,
					h.Type().String(), len(hd.Bytes))
				hd.Bytes = zx0(hd.Bytes, Unpack)
				fmt.Printf(" -> %d\n", len(hd.Bytes))
				header.Specifiers[loadableNum] &= ^uint32(hunk.HUNKF_ADVISORY)
				header.Specifiers[loadableNum] -= 1
			}
			loadableNum += 1
		case hunk.HUNK_BSS:
			loadableNum += 1
		}
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

	if unpack {
		unpackExe(hunks)
	} else {
		packExe(hunks)
	}

	println()
	println(hunks[0].String())

	outName := flag.Arg(0)

	if len(flag.Args()) > 1 {
		outName = flag.Arg(1)
	}

	if err := hunk.WriteFile(outName, hunks, 0755); err != nil {
		panic("failed to write Amiga Hunk file")
	}
}
