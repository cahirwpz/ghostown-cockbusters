package obj

import (
	_ "embed"
	"strings"
	"text/template"
)

//go:embed template.tpl
var tpl string

func Convert(obj *WavefrontObj, cp ConverterParams) (string, error) {
	var s float64

	ps := TemplateParams{Name: obj.Name, MaterialCount: len(obj.Materials)}

	faceNormals, err := CalculateFaceNormals(obj)
	if err != nil {
		return "", err
	}

	edges := CalculateEdges(obj)
	ps.EdgeCount = len(edges)
	ps.Edges = edges

	for i := 0; i < len(edges); i++ {
		for j := 0; j < len(edges[i]); j++ {
			/* pre-compute vertex indices */
			edges[i][j] *= cp.VertexSize
		}
	}

	s = cp.Scale * 16
	for _, v := range obj.Vertices {
		ov := []int{int(v[0] * s), int(v[1] * s), int(v[2] * s)}
		ps.Vertices = append(ps.Vertices, ov)
		ps.VertexCount += 1
	}

	var faceIndices []int

	for i, f := range obj.Faces {
		poly := bool(len(f.Indices) >= 3)
		of := []int{len(f.Indices), f.Material}
		if poly {
			fn := faceNormals[i]
			x, y, z := fn[0]*4096, fn[1]*4096, fn[2]*4096
			of = append(of, int(x), int(y), int(z))
		}
		faceIndices = append(faceIndices, ps.FaceDataCount)
		for _, fi := range f.Indices {
			/* precompute vertex / edge indices */
			of = append(of, (fi.Vertex-1)*cp.VertexSize)
			if poly {
				of = append(of, fi.Edge*cp.EdgeSize)
			}
		}
		ps.FaceData = append(ps.FaceData, of)
		ps.FaceDataCount += len(of) * 2
	}

	ps.FaceCount = len(obj.Faces)
	ps.MaterialCount = len(obj.Materials)
	ps.GroupCount = len(obj.Groups)

	ps.GroupDataCount = 1
	for _, grp := range obj.Groups {
		og := []int{}
		for _, fi := range grp.FaceIndices {
			og = append(og, faceIndices[fi])
		}
		ps.Groups = append(ps.Groups,
			FaceGroup{
				Name:  grp.Name,
				Index: ps.GroupDataCount,
			})
		ps.GroupDataCount += len(og)*2 + 1
		ps.GroupData = append(ps.GroupData, og)
	}

	for _, mtl := range obj.Materials {
		ps.Materials = append(ps.Materials,
			FaceMaterial{
				Name:  mtl.Name,
				Index: mtl.Index,
			})
	}

	tmpl, err := template.New("template").Parse(tpl)
	if err != nil {
		return "", err
	}

	var buf strings.Builder
	err = tmpl.Execute(&buf, ps)
	if err != nil {
		return "", err
	}

	return buf.String(), nil
}

type ConverterParams struct {
	Scale      float64
	VertexSize int
	EdgeSize   int
}
type FaceGroup struct {
	Name  string
	Index int
}

type FaceMaterial struct {
	Name  string
	Index int
}

type TemplateParams struct {
	Name string

	VertexCount    int
	FaceCount      int
	FaceDataCount  int
	EdgeCount      int
	MaterialCount  int
	GroupCount     int
	GroupDataCount int

	Vertices    [][]int
	FaceData    [][]int
	GroupData   [][]int
	Groups      []FaceGroup
	FaceNormals [][]int
	Edges       []Edge
	Materials   []FaceMaterial
}
