package main

import (
	"flag"
	"ghostown.pl/hunk"
	"os"
)

var printHelp bool

func init() {
	flag.BoolVar(&printHelp, "help", false,
		"print help message and exit")
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

	if err := hunk.WriteFile(flag.Arg(0)+".out", hunks); err != nil {
		panic("failed to write Amiga Hunk file")
	}
}
