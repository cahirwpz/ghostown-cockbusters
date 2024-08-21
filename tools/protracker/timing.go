package protracker

import "log"
import "fmt"

type Timings [][64]int 

func CalculateTimings(mod Module) Timings {
	timings := make(Timings, len(mod.Song))

	fmt.Println(len(mod.Song), mod.Song)
	fmt.Println(len(mod.Patterns))

	framesPerRow := 6
	frame := 0


	for j, pat := range mod.Song {
		println(j, pat)

		for i := 0; i < 64; i++ {
			timings[j][i] = frame

			/* update speed */
			for _, ch := range mod.Patterns[pat][i] {
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

			frame += framesPerRow
		}
	}

	return timings
}
