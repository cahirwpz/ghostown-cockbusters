#include <3d.h>
#include <fx.h>
#include <system/memory.h>

Object3D *NewObject3D(Mesh3D *mesh) {
  Object3D *object = MemAlloc(sizeof(Object3D), MEMF_PUBLIC|MEMF_CLEAR);

  object->vertices = mesh->vertices;
  object->edges = mesh->edges;

  object->objdat = (void *)mesh->vertex;
  object->point = (Point3D *)mesh->vertex;
  object->edge = (EdgeT *)mesh->edge;
  object->group = mesh->object;

  object->vertex = MemAlloc(sizeof(Point3D) * mesh->vertices, MEMF_PUBLIC);
  object->visibleFace =
    MemAlloc(sizeof(SortItemT) * (mesh->faces + 1), MEMF_PUBLIC);

  object->scale.x = fx12f(1.0);
  object->scale.y = fx12f(1.0);
  object->scale.z = fx12f(1.0); 

  return object;
}
