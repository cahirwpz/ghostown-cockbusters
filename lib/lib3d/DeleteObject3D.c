#include <3d.h>
#include <system/memory.h>

void DeleteObject3D(Object3D *object) {
  if (object) {
    MemFree(object->edge);
    MemFree(object->face);
    MemFree(object->faceEdge);
    MemFree(object->visibleFace);
    MemFree(object->edgeFlags);
    MemFree(object->faceFlags);
    MemFree(object->vertexFlags);
    MemFree(object->vertex);
    MemFree(object);
  }
}
