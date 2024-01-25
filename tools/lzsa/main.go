package main

import (
	"flag"
	"io/fs"
	"log"
	"os"
)

var printHelp bool

func init() {
	flag.BoolVar(&printHelp, "help", false,
		"print help message and exit")
}

func readFile(path string) []byte {
	var f *os.File
	var fi fs.FileInfo
	var err error

	if f, err = os.Open(flag.Arg(0)); err != nil {
		log.Fatal("Open:", err)
	}

	if fi, err = f.Stat(); err != nil {
		log.Fatal("Stat:", err)
	}

	data := make([]byte, fi.Size())
	if _, err = f.Read(data); err != nil {
		log.Fatal("Read:", err)
	}

	if err = f.Close(); err != nil {
		log.Fatal("Close:", err)
	}

	return data
}

func main() {
	flag.Parse()

	if len(flag.Args()) < 1 || printHelp {
		flag.PrintDefaults()
		os.Exit(1)
	}

	input := readFile(flag.Arg(0))
	output := Decompress(input)

	file, err := os.OpenFile(flag.Arg(0)+".unpacked", os.O_WRONLY|os.O_CREATE|os.O_TRUNC, 0644)
	if err != nil {
		return
	}

	defer file.Close()

	file.Write(output)
}
