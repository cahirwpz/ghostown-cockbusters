#include <3d.h>

void SortFaces(Object3D *object) {
  short *item = (short *)object->visibleFace;
  short count = 0;

  void *_objdat = object->objdat;
  short *group = object->group;
  short f;

  while (*group++) {
    while ((f = *group++)) {
      if (FACE(f)->flags >= 0) {
        short z;
        short i;

        i = FACE(f)->indices[0].vertex;
        z = POINT(i)->z;
        i = FACE(f)->indices[1].vertex;
        z += POINT(i)->z;
        i = FACE(f)->indices[2].vertex;
        z += POINT(i)->z;

        *item++ = z;
        *item++ = f;
        count++;
      }
    }
  }

  /* guard element */
  *item++ = 0;
  *item++ = -1;

  SortItemArray(object->visibleFace, count);
}
