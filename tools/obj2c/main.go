package main

import (
	"flag"
	"io"
	"log"
	"os"

	"ghostown.pl/obj2c/obj"
)

func main() {
	r, err := os.Open("test.obj")
	if err != nil {
		log.Panicf("Failed to open file %q", flag.Arg(0))
	}
	file, _ := io.ReadAll(r)

	_, err = obj.Parse(string(file))
	if err != nil {
		log.Fatalf("failed to parse the .obj file: %v", err)
	}

	// inName := strings.Split(r.Name(), ".")[0]
	// outName := fmt.Sprintf("%s.c", inName)
	// err = os.WriteFile(outName, []byte(out), 0777)
	// if err != nil {
	// 	log.Panicf("Failed to write file %q", flag.Arg(0))
	// }
}
