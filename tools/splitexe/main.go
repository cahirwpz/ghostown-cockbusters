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

func splitExe(hunks []hunk.Hunk) [][]hunk.Hunk {
	var loadables [][]hunk.Hunk

	header := hunks[0].(*hunk.HunkHeader)

	loadableHunk := 0
	seenCodeHunk := false

	var hs []hunk.Hunk
	var hdr *hunk.HunkHeader

	for _, h := range hunks[1:] {
		ht := h.Type()

		if ht == hunk.HUNK_CODE {
			if seenCodeHunk {
				loadables = append(loadables, hs)
			}

			seenCodeHunk = true
			hdr = &hunk.HunkHeader{}
			hs = []hunk.Hunk{hdr}
		}

		if ht == hunk.HUNK_CODE || ht == hunk.HUNK_DATA || ht == hunk.HUNK_BSS {
			hs = append(hs, h)

			hdr.Specifiers = append(hdr.Specifiers, header.Specifiers[loadableHunk])
			hdr.Hunks += 1
			hdr.Last = hdr.Hunks - 1

			loadableHunk += 1
		}

		if ht == hunk.HUNK_RELOC32 {
			hs = append(hs, h)
		}
	}

	return append(loadables, hs)
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

	loadables := splitExe(hunks)
	for i, loadable := range loadables {
		writeExe(flag.Arg(0), i, loadable)
	}
}
