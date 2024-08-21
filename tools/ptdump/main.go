package main

import (
	"bufio"
	"bytes"
	"flag"
	"fmt"
	"html/template"
	"log"
	"os"

	"ghostown.pl/protracker"
)

var (
	printHelp bool
	timing    bool
)

var moduleReportTemplate = `Module Name: {{.Name}}
Patterns Order: [{{- range .Song}}{{.}} {{end}}]

 ---------=[ Samples ]=---------
{{range $idx, $sam := .Samples}}
{{- printf "%3d %-22s %5d" $idx $sam.Name (len $sam.Data)}}
{{end}}
{{- range $idx, $pat := .Patterns}}
 ---------------=[ Pattern {{printf "%2d" $idx}} ]=---------------
{{range $pat}}
{{- range .}}{{.}}{{- end}}
{{end}}
{{- end}}
`

func dumpModule(m protracker.Module) string {
	t, err := template.New("export").Parse(moduleReportTemplate)
	if err != nil {
		log.Fatal(err)
	}
	var b bytes.Buffer
	err = t.Execute(&b, m)
	if err != nil {
		log.Fatal(err)
	}
	return b.String()
}

func init() {
	flag.BoolVar(&printHelp, "help", false,
		"print help message and exit")
	flag.BoolVar(&timing, "timing", false,
		"print timing for each row in the song")
}

func main() {
	flag.Parse()

	if len(flag.Args()) == 0 || printHelp {
		flag.PrintDefaults()
		os.Exit(1)
	}

	file, err := os.Open(flag.Arg(0))
	if err != nil {
		log.Fatal(err)
	}

	writer := bufio.NewWriter(os.Stdout)
	mod := protracker.ReadModule(file)

	if timing {
		framesPerRow := 6
		time := 0.0
		for _, pat := range mod.Song {
			for i := 0; i < 64; i++ {
				row := mod.Patterns[pat][i]
				ts := fmt.Sprintf("%.2f", time)
				fmt.Printf("%7ss | %02x/%02x %s\n", ts, pat, i, row)
				for _, ch := range row {
					if ch.Effect == 0xf {
						if ch.EffectParams < 0x20 {
							// F-speed
							framesPerRow = int(ch.EffectParams)
						} else {
							// F-speed
							if ch.EffectParams != 0x7D {
								log.Fatal("only default CIA tempo is supported")
							}
						}
					}
				}
				time += float64(framesPerRow) / 50.0
			}
		}
	} else {
		_, err = writer.WriteString(dumpModule(mod))
		if err != nil {
			log.Fatal(err)
			return
		}
	}

	writer.Flush()
}
