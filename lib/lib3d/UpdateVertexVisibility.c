#include <strings.h>
#include <3d.h>

void UpdateVertexVisibility(Object3D *object) {
  char *vertexFlags = &object->vertex[0].flags;
  short **vertexIndexList = object->faceVertexIndexList;
  short n = object->faces;

  register char s asm("d7") = 1;

  while (--n >= 0) {
    short *vertexIndex = *vertexIndexList++;

    if (vertexIndex[FV_FLAGS] >= 0) {
      short n = vertexIndex[FV_COUNT] - 3;
      short i;

      /* Face has at least (and usually) three vertices / edges. */
      i = *vertexIndex++; vertexFlags[i] = s;
      i = *vertexIndex++; vertexFlags[i] = s;

      do {
        i = *vertexIndex++; vertexFlags[i] = s;
      } while (--n != -1);
    }
  }
}
