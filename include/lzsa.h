#ifndef __LZSA_H__
#define __LZSA_H__

void lzsa_depack_stream(const void *input asm("a0"), void *output asm("a1"));

#endif /* !__LZSA_H__ */
