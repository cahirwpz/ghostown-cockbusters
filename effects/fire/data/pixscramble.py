colors = [
  0x000,
  0x100,
  0x200,
  0x300,
  0x400,
  0x510,
  0x610,
  0x720,
  0x820,
  0x830,
  0x930,
  0xA40,
  0xB40,
  0xB50,
  0xC50,
  0xC60,
  0xC70,
  0xD70,
  0xE80,
  0xE90,
  0xF90,
  0xFA0,
  0xFB0,
  0xFC0,
  0xFD0,
  0xFE0,
  0xFF0,
  0xEF0,
  0xDF0,
  0xCF0,
  0xBF0,
  0xAF0,
  0x9F0,
  0x8F0,
  0x7F0,
  0x6F0,
  0x5F0,
  0x4F0,
  0x3F0,
  0x2F0,
  0x1F0,
  0x0F0,
]

def scramble(c):
  r0 = '1' if c & 0b000100000000 else '0' # RED
  r1 = '1' if c & 0b001000000000 else '0'
  r2 = '1' if c & 0b010000000000 else '0'
  r3 = '1' if c & 0b100000000000 else '0'
  g0 = '1' if c & 0b000000010000 else '0' # GREEN
  g1 = '1' if c & 0b000000100000 else '0'
  g2 = '1' if c & 0b000001000000 else '0'
  g3 = '1' if c & 0b000010000000 else '0'
  b0 = '1' if c & 0b000000000001 else '0' # BLUE
  b1 = '1' if c & 0b000000000010 else '0'
  b2 = '1' if c & 0b000000000100 else '0'
  b3 = '1' if c & 0b000000001000 else '0'
  out0 = r0+g0+b0+b0
  out1 = r1+g1+b1+b1
  out2 = r2+g2+b2+b2
  out3 = r3+g3+b3+b3
  return hex(int(out0+out1+out2+out3, 2))


if __name__ == "__main__":
  for c in colors:
    print(scramble(c))