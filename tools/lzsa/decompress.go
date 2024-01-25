package main

import (
	"log"
)

type Format int

const (
	V1 Format = 0
	V2 Format = 1
)

type decompressor struct {
	input     []uint8
	output    []uint8
	lastIndex int
}

func (d *decompressor) getByte() int {
	b := int(d.input[d.lastIndex])
	d.lastIndex++
	return b
}

func (d *decompressor) getWord() int {
	w := d.getByte()
	w |= d.getByte() << 8
	return w
}

func (d *decompressor) getByteArray(n int) []uint8 {
	bs := d.input[d.lastIndex : d.lastIndex+n]
	d.lastIndex += n
	return bs
}

func (d *decompressor) lzsa1_block(untilIndex int) {
	var literal int
	var offset int
	var match int

	for {
		/* token: <O|LLL|MMMM> */
		token := d.getByte()

		/* optional extra literal length */
		literal = (token >> 4) & 7
		if literal == 7 {
			literal += d.getByte()
			if literal == 256 {
				/* a second and third byte follow */
				literal = d.getWord()
			} else if literal > 256 {
				/* a second byte follows */
				literal = 256 + d.getByte()
			}
		}

		/* copy literal */
		d.output = append(d.output, d.getByteArray(literal)...)

		/* end of block in a stream */
		if d.lastIndex == untilIndex {
			return
		}

		/* match offset low */
		if token&0x80 != 0 {
			offset = d.getWord()
			offset |= -65536
		} else {
			offset = d.getByte()
			offset |= -256
		}

		/* optional extra encoded match length */
		match = 3 + (token & 15)
		if match == 18 {
			match += d.getByte()
			if match == 256 {
				/* a second and third byte follow */
				match = d.getWord()
				/* end of block */
				if match == 0 {
					return
				}
			} else if match > 256 {
				/* a second byte follows */
				match = 256 + d.getByte()
			}
		}

		/* copy match */
		offset += len(d.output)
		for k := 0; k < match; k++ {
			d.output = append(d.output, d.output[offset+k])
		}
	}
}

func (d *decompressor) lzsa2_block(frameSize int) {
	// var literal int
	// var offset int
	// var match int

	for {
		/* token: <XYZ|LL|MMM> */
		// token := d.getByte()
	}
}

func Decompress(input []uint8) []uint8 {
	d := &decompressor{input, make([]uint8, 0), 0}

	if d.getByte() != 0x7b || d.getByte() != 0x9e {
		log.Fatal("Invalid header")
	}

	var format Format

	if d.getByte()&0x20 != 0 {
		format = V2
		println("LZSAv2")
	} else {
		format = V1
		println("LZSAv1")
	}

	for {
		compressed := true

		lo := d.getByte()
		mid := d.getByte()
		hi := d.getByte()

		if lo == 0 && mid == 0 && hi == 0 {
			break
		}

		if hi&0x80 != 0 {
			compressed = false
		}

		length := (hi&1)<<16 | mid<<8 | lo

		println("compressed:", compressed)
		println("length:", length)

		if compressed {
			if format == V1 {
				d.lzsa1_block(length + d.lastIndex)
			} else {
				d.lzsa2_block(length + d.lastIndex)
			}
		} else {

		}
	}

	return d.output
}
