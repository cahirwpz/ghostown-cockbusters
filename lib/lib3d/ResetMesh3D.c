#include <3d.h>
#include <system/memory.h>

void ResetMesh3D(Mesh3D *mesh) {
  MemFree(mesh->edge);
  MemFree(mesh->faceEdge);

  mesh->edges = 0;
  mesh->edge = NULL;
  mesh->faceEdge = NULL;
}
