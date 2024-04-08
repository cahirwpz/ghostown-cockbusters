package obj

type Vector []float64

type Obj struct {
	GVertices  []Vector  // "v Geometric Vertices"
	NVertices  []Vector  // "vn Vertex Normals"
	TVertices  []Vector  // "vt Texture Vertices"
	PSVertices []float32 // "vp"
	Faces      []int32   // "f"
}

type Model struct {
	Name     string
	Surfaces []*Surface
	Points   []*Point
	Mesh     Mesh
}

type Surface struct {
	R        int16
	G        int16
	B        int16
	Sideness int32
	Texture  int32
}

type Point struct {
	X   int16
	Y   int16
	Z   int16
	Pad int16
}

type Mesh struct {
	VerticeCounter int16
	FaceCounter    int16
	EdgeCounter    int16
	SurfaceCounter int16
	ImageCounter   int16
}
