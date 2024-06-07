#include <strings.h>
#include <3d.h>

void UpdateVertexVisibility(Object3D *object) {
  char *vertexFlags = &object->vertex[0].flags;
  char *faceFlags = object->faceFlags;
  short **vertexIndexList = object->faceVertexIndexList;
  short n = object->faces;

  while (--n >= 0) {
    short *vertexIndex = *vertexIndexList++;

    if (*faceFlags++ >= 0) {
      short count = vertexIndex[-1];
      short i;

      /* Face has at least (and usually) three vertices. */
      switch (count) {
        case 6: i = *vertexIndex++ << 3; vertexFlags[i] = -1;
        case 5: i = *vertexIndex++ << 3; vertexFlags[i] = -1;
        case 4: i = *vertexIndex++ << 3; vertexFlags[i] = -1;
        case 3: i = *vertexIndex++ << 3; vertexFlags[i] = -1;
                i = *vertexIndex++ << 3; vertexFlags[i] = -1;
                i = *vertexIndex++ << 3; vertexFlags[i] = -1;
                i = *vertexIndex++ << 3; vertexFlags[i] = -1;
        default: break;
      }
    }
  }
}
