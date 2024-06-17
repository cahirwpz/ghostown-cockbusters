#ifndef __3D_H__
#define __3D_H__

#include <sort.h>
#include <2d.h>
#include <pixmap.h>

extern char SqrtTab8[256];

/* 3D transformations */

typedef struct {
  short x, y, z;
  /* one if vertex belongs to a face that is visible,
   * otherwise set to zero,
   * remember to set to 0 after use */
  char flags;
} Point3D;

typedef struct Edge {
  /* negative if edge belongs to a face that is not visible,
   * otherwise it's an edge color in range 0..15
   * remember to set to -1 after use */
  char flags;
  char pad;
  short point[2];
} EdgeT;

typedef struct {
  short m00, m01, m02, x;
  short m10, m11, m12, y;
  short m20, m21, m22, z;
} Matrix3D;

void LoadIdentity3D(Matrix3D *M);
void Translate3D(Matrix3D *M, short x, short y, short z);
void Scale3D(Matrix3D *M, short sx, short sy, short sz);
void LoadRotate3D(Matrix3D *M, short ax, short ay, short az);
void LoadReverseRotate3D(Matrix3D *M, short ax, short ay, short az);
void Compose3D(Matrix3D *md, Matrix3D *ma, Matrix3D *mb);
void Transform3D(Matrix3D *M, Point3D *out, Point3D *in, short n);

/*
 * 3D mesh representation
 *
 * Please read the output from obj2c to understand it.
 */
typedef struct Mesh3D {
  short vertices;
  short edges;
  short faces;
  short materials;

  /* these arrays are shared with Object3D */
  short *vertex;        /* [x y z flags/pad] */
  short *edge;          /* [flags point_0 point_1] */
  short *object;
} Mesh3D;

/* 3D object representation */

typedef struct FaceIndex {
  short vertex;
  short edge;
} FaceIndexT; 

typedef struct Face {
  /* Face normal - absent for points and lines. */
  short normal[3];
  /* Flags store negative value when face is not visible,
   * or a color of face normalized to 0..15 range. */
  char flags;
  char material;
  short count;
  FaceIndexT indices[0];
} FaceT;

typedef struct Object3D {
  /* copied from mesh */
  short vertices;
  short edges;

  Point3D *point;
  EdgeT *edge;
  short *group;

  void *objdat;

  /* private */
  Point3D rotate;
  Point3D scale;
  Point3D translate;

  Matrix3D objectToWorld; /* object -> world transformation */
  Matrix3D worldToObject; /* world -> object transformation */

  /* camera position in object space */
  Point3D camera;

  /* camera coordinates or screen coordinates + depth */
  Point3D *vertex;

  /* ends with guard element */
  SortItemT *visibleFace;
} Object3D;

/* The environment has to define `_data` and `_vertex`. */
#define POINT(i) ((Point3D *)(_objdat + (i)))
#define VERTEX(i) ((Point3D *)(_vertex + (i)))
#define EDGE(i) ((EdgeT *)(_objdat + (i)))
#define FACE(i) ((FaceT *)(_objdat + (i)))

Object3D *NewObject3D(Mesh3D *mesh);
void DeleteObject3D(Object3D *object);
void UpdateFaceNormals(Object3D *object);
void UpdateObjectTransformation(Object3D *object);
void UpdateFaceVisibility(Object3D *object);
void UpdateVertexVisibility(Object3D *object);
void SortFaces(Object3D *object);

#endif
