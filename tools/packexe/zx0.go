package main

type State int

const (
	END                   State = 0
	COPY_LITERALS         State = 1
	COPY_FROM_LAST_OFFSET State = 2
	COPY_FROM_NEW_OFFSET  State = 3
)

const (
	INITIAL_OFFSET = 1
)

type decompressor struct {
	lastOffset int
	input      []byte
	output     []byte
	inputIndex int
	bitMask    byte
	bitValue   byte
	backtrack  bool
	lastByte   byte
}

func (d *decompressor) readByte() byte {
	d.lastByte = d.input[d.inputIndex]
	d.inputIndex++
	return d.lastByte
}

func (d *decompressor) readBit() byte {
	if d.backtrack {
		d.backtrack = false
		return d.input[d.inputIndex-1] & 1
	}
	d.bitMask >>= 1
	if d.bitMask == 0 {
		d.bitMask = 128
		d.bitValue = d.readByte()
	}
	if d.bitValue&d.bitMask != 0 {
		return 1
	}
	return 0
}

func (d *decompressor) readInterlacedEliasGamma(msb bool) int {
	value := 1
	bit := byte(0)
	if msb {
		bit = 1
	}
	for d.readBit() == 0 {
		value <<= 1
		value |= int(d.readBit() ^ bit)
	}
	return value
}

func (d *decompressor) writeByte(value byte) {
	d.output = append(d.output, value)
}

func (d *decompressor) copyBytes(length int) {
	for i := 0; i < length; i++ {
		d.output = append(d.output, d.output[len(d.output)-d.lastOffset])
	}
}

func (d *decompressor) decompress() []byte {
	state := COPY_LITERALS

	for {
		switch state {
		case COPY_LITERALS:
			length := d.readInterlacedEliasGamma(false)
			for i := 0; i < length; i++ {
				d.writeByte(d.readByte())
			}
			if d.readBit() == 0 {
				state = COPY_FROM_LAST_OFFSET
			} else {
				state = COPY_FROM_NEW_OFFSET
			}
		case COPY_FROM_LAST_OFFSET:
			length := d.readInterlacedEliasGamma(false)
			d.copyBytes(length)
			if d.readBit() == 0 {
				state = COPY_LITERALS
			} else {
				state = COPY_FROM_NEW_OFFSET
			}
		case COPY_FROM_NEW_OFFSET:
			msb := d.readInterlacedEliasGamma(true)
			if msb == 256 {
				return d.output
			}
			lsb := d.readByte() >> 1
			d.lastOffset = int(msb)*128 - int(lsb)
			d.backtrack = true
			length := d.readInterlacedEliasGamma(false) + 1
			d.copyBytes(length)
			if d.readBit() == 0 {
				state = COPY_LITERALS
			} else {
				state = COPY_FROM_NEW_OFFSET
			}
		}
	}
}

func UnZX0(data []byte) []byte {
	d := &decompressor{input: data, lastOffset: INITIAL_OFFSET}
	return d.decompress()
}
