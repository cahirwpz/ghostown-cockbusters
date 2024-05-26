package obj

type Vector []float64

type Face struct {
	Vertex   int64
	TexCoord int64
	Normal   int64
}

type WavefrontObj struct {
	Vertices  []Vector
	TexCoords []Vector
	Normals   []Vector
	Faces     []Face
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
	VerticeCount int16
	FaceCount    int16
	EdgeCount    int16
	SurfaceCount int16
	ImageCount   int16
}
