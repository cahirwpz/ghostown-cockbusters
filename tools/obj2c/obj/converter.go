package obj

import (
	_ "embed"
	"strings"
	"text/template"
)

//go:embed template.tpl
var tpl string

func Convert(data *WavefrontData, cp ConverterParams) (string, error) {
	var s float64

	geom := Geometry{Name: cp.Name}

	/* dump geometry data from all the objects */
	for _, obj := range data.Objects {
		vertexBegin := len(geom.Vertices)
		edgeBegin := len(geom.Edges)

		/* vertices */
		s = cp.Scale * 16
		for _, v := range obj.Vertices {
			geom.Vertices = append(geom.Vertices,
				Vector{int(v[0] * s), int(v[1] * s), int(v[2] * s), 0})
		}

		/* edges */
		edges := CalculateEdges(obj)
		for _, e := range edges {
			/* pre-compute vertex indices */
			e.Point[0] = (e.Point[0] + edgeBegin) * cp.VertexSize
			e.Point[1] = (e.Point[1] + edgeBegin) * cp.VertexSize
			geom.Edges = append(geom.Edges, e)
		}

		/* faces */
		var faceIndices []int

		for i, f := range obj.Faces {
			poly := bool(len(f.Indices) >= 3)
			of := Face{Material: f.Material, Count: len(f.Indices)}
			sz := 2
			if poly {
				fn := CalculateFaceNormal(obj, i)
				of.Normal = Vector{int(fn[0] * 4096), int(fn[1] * 4096), int(fn[2] * 4096)}
				sz += 3
			}
			faceIndices = append(faceIndices, (geom.FaceDataCount+sz)*cp.IndexSize)
			for _, fi := range f.Indices {
				/* precompute vertex / edge indices */
				of.Indices = append(of.Indices, (fi.Vertex-1+vertexBegin)*cp.VertexSize)
				if poly {
					of.Indices = append(of.Indices, (fi.Edge+edgeBegin)*cp.EdgeSize)
				}
			}
			geom.Faces = append(geom.Faces, of)
			geom.FaceDataCount += sz + len(f.Indices)
		}

		/* objects & groups */
		object := Object{Name: obj.Name, Offset: geom.ObjectDataCount}
		for _, g := range obj.Groups {
			geom.ObjectDataCount += 1
			og := FaceGroup{Name: g.Name, Offset: geom.ObjectDataCount}
			for _, fi := range g.Indices {
				og.Faces = append(og.Faces, faceIndices[fi])
			}
			object.Groups = append(object.Groups, og)
			geom.ObjectDataCount += len(og.Faces) + 1
		}
		geom.Objects = append(geom.Objects, object)
	}

	/* terminate objects */
	geom.Objects = append(geom.Objects, Object{Name: "end", Offset: geom.ObjectDataCount})
	geom.ObjectDataCount += 1

	/* determine subarrays position after merging into single array */
	geom.EdgeOffset = len(geom.Vertices) * cp.VertexSize
	geom.FaceDataOffset = geom.EdgeOffset + len(geom.Edges)*cp.EdgeSize
	geom.ObjectDataOffset = geom.FaceDataOffset + geom.FaceDataCount*cp.IndexSize

	/* relocate edge indices in faces */
	for i, face := range geom.Faces {
		if len(face.Indices) >= 3 {
			for j := 1; j < len(face.Indices); j += 2 {
				geom.Faces[i].Indices[j] += geom.EdgeOffset
			}
		}
	}

	/* relocate face indices in groups */
	for i, object := range geom.Objects {
		for j, group := range object.Groups {
			for k := 0; k < len(group.Faces); k++ {
				geom.Objects[i].Groups[j].Faces[k] += geom.FaceDataOffset
			}
		}
	}

	for i, mtl := range data.Materials {
		geom.Materials = append(geom.Materials, FaceMaterial{Name: mtl.Name, Index: i})
	}

	tmpl, err := template.New("template").Parse(tpl)
	if err != nil {
		return "", err
	}

	var buf strings.Builder
	err = tmpl.Execute(&buf, geom)
	if err != nil {
		return "", err
	}

	return buf.String(), nil
}

type ConverterParams struct {
	Name       string
	Scale      float64
	VertexSize int
	EdgeSize   int
	IndexSize  int
}

type Vector []int

type Edge struct {
	Flags int
	Point [2]int
}

type Face struct {
	Normal   Vector
	Material int
	Count    int
	Indices  []int
}

type FaceGroup struct {
	Name   string
	Offset int
	Faces  []int
}

type Object struct {
	Name   string
	Offset int
	Groups []FaceGroup
}

type FaceMaterial struct {
	Name  string
	Index int
}

type Geometry struct {
	Name string

	FaceDataCount   int
	ObjectDataCount int

	EdgeOffset       int
	FaceDataOffset   int
	ObjectDataOffset int

	Vertices  []Vector
	Edges     []Edge
	Faces     []Face
	Objects   []Object
	Materials []FaceMaterial
}
