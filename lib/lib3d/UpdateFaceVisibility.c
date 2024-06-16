#include <3d.h>

void UpdateFaceVisibility(Object3D *object) {
  short *camera = (short *)&object->camera;
  char *sqrt = SqrtTab8;

  void *_objdat = object->objdat;
  short *group = object->group;
  short f;

  while (*group++) {
    while ((f = *group++)) {
      short px, py, pz;
      short l;
      int v;

      {
        short i = FACE(f)->indices[0].vertex;
        short *p = (short *)POINT(i);
        short *c = camera;
        px = *c++ - *p++;
        py = *c++ - *p++;
        pz = *c++ - *p++;
      }

      {
        short *fn = FACE(f)->normal;
        int x = *fn++ * px;
        int y = *fn++ * py;
        int z = *fn++ * pz;
        v = x + y + z;
      }

      if (v >= 0) {
        /* normalize dot product */
#if 0
        int s = px * px + py * py + pz * pz;
        s = swap16(s); /* s >>= 16, ignore upper word */
#else
        short s;
        asm("mulsw %0,%0\n"
            "mulsw %1,%1\n"
            "mulsw %2,%2\n"
            "addl  %1,%0\n"
            "addl  %2,%0\n"
            "swap  %0\n"
            : "+d" (px), "+d" (py), "+d" (pz));
        s = px;
#endif
        v = swap16(v); /* f >>= 16, ignore upper word */
        l = div16((short)f * (short)f, s);
        if (l >= 256)
          l = 15;
        else
          l = sqrt[l];
      } else {
        l = -1;
      }

      FACE(f)->flags = l;
    }
  }
}
