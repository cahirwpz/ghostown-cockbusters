package main

import (
	"flag"
	"fmt"
	"io"
	"log"
	"os"
)

func main() {
	r, err := os.Open(flag.Arg(0))
	if err != nil {
		log.Panicf("Failed to open file %q", flag.Arg(0))
	}
	file, _ := io.ReadAll(r)

	var out string

	out = parseObj(file)

	inName := string.Split(r.Name(), ".")[0]
	outName := fmt.Spritf("%s.c", inName)
	err = os.WriteFile(outName, []byte(out), 0777)
	if err != nil {
		log.Panicf("Failed to write file %q", flag.Arg(0))
	}
}
