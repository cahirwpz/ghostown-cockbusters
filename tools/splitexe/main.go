package main

import (
	"flag"
	"fmt"
	"os"

	"ghostown.pl/hunk"
)

var printHelp bool

func init() {
	flag.BoolVar(&printHelp, "help", false,
		"print help message and exit")
}

func writeExe(baseName string, num int, hunks []hunk.Hunk) {
	println(hunks[0].String())

	hunks = append(hunks, &hunk.HunkEnd{})
	exeName := fmt.Sprintf("%s.%d", baseName, num)

	if err := hunk.WriteFile(exeName, hunks); err != nil {
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

	var exeHunks []hunk.Hunk
	var exeHeader *hunk.HunkHeader

	header := hunks[0].(*hunk.HunkHeader)
	println(header.String())
	hunkNum := 0
	exeNum := 0

	for _, h := range hunks[1:] {
		ht := h.Type()
		if ht == hunk.HUNK_CODE {
			if exeNum > 0 {
				writeExe(flag.Arg(0), exeNum, exeHunks)
			}
			exeHeader = &hunk.HunkHeader{}
			exeHunks = []hunk.Hunk{exeHeader}
			exeNum += 1
		}

		if ht == hunk.HUNK_CODE || ht == hunk.HUNK_DATA ||
			ht == hunk.HUNK_BSS || ht == hunk.HUNK_RELOC32 {

			exeHunks = append(exeHunks, h)

			fmt.Printf("%d: %s\n", exeNum, h.Type().String())

			if ht != hunk.HUNK_RELOC32 {
				exeHeader.Specifiers = append(exeHeader.Specifiers, header.Specifiers[hunkNum])
				exeHeader.Hunks += 1
				exeHeader.Last = exeHeader.Hunks - 1
				hunkNum += 1
			}
		}
	}

	writeExe(flag.Arg(0), exeNum, exeHunks)
}
