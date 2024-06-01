#include <3d.h>
#include <fx.h>
#include <system/memory.h>

Object3D *NewObject3D(Mesh3D *mesh) {
  Object3D *object = MemAlloc(sizeof(Object3D), MEMF_PUBLIC|MEMF_CLEAR);
  short vertices = mesh->vertices;
  short faces = mesh->faces;
  short edges = mesh->edges;

  object->vertices = vertices;
  object->faces = faces;
  object->edges = edges;

  object->point = mesh->vertex;
  object->edge = mesh->edge;
  object->faceNormal = mesh->faceNormal;

  object->face = MemAlloc((sizeof(short *) + 1) * faces, MEMF_PUBLIC);
  {
    short **facePtr = object->face;
    short *face = mesh->face;
    short n;

    while ((n = *face++)) {
      *facePtr++ = face;
      face += n;
    }

    *facePtr = NULL;
  }

  object->faceEdge = MemAlloc((sizeof(short *) + 1) * faces, MEMF_PUBLIC);
  {
    short **faceEdgePtr = object->faceEdge;
    short *faceEdge = mesh->faceEdge;
    short n;

    while ((n = *faceEdge++)) {
      *faceEdgePtr++ = faceEdge;
      faceEdge += n;
    }

    *faceEdgePtr = NULL;
  }

  object->vertex = MemAlloc(sizeof(Point3D) * vertices, MEMF_PUBLIC);
  object->vertexFlags = MemAlloc(vertices, MEMF_PUBLIC);
  object->faceFlags = MemAlloc(faces, MEMF_PUBLIC);
  if (edges)
    object->edgeFlags = MemAlloc(edges, MEMF_PUBLIC);
  object->visibleFace = MemAlloc(sizeof(SortItemT) * faces, MEMF_PUBLIC);

  object->scale.x = fx12f(1.0);
  object->scale.y = fx12f(1.0);
  object->scale.z = fx12f(1.0); 

  return object;
}
