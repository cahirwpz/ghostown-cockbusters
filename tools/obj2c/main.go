package main

import (
	"flag"
	"log"
	"os"

	"ghostown.pl/obj2c/obj"
)

var printHelp bool
var scaleFactor float64

func init() {
	flag.BoolVar(&printHelp, "help", false,
		"print help message and exit")
	flag.Float64Var(&scaleFactor, "scale", 1.0,
		"the object will be scaled by this factor")
}

func main() {
	flag.Parse()

	if len(flag.Args()) < 1 || printHelp {
		flag.PrintDefaults()
		os.Exit(1)
	}

	file, err := os.Open(flag.Arg(0))
	if err != nil {
		log.Panicf("Failed to open file %q", flag.Arg(0))
	}

	_, err = obj.ParseObj(file)
	if err != nil {
		log.Fatalf("failed to parse Wavefront object file: %v", err)
	}

	// inName := strings.Split(r.Name(), ".")[0]
	// outName := fmt.Sprintf("%s.c", inName)
	// err = os.WriteFile(outName, []byte(out), 0777)
	// if err != nil {
	// 	log.Panicf("Failed to write file %q", flag.Arg(0))
	// }
}
