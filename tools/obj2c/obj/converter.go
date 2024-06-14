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

	faceNormals, err := CalculateFaceNormals(obj)
	if err != nil {
		return "", err
	}

	edges := CalculateEdges(obj)

	ps := TemplateParams{
		Name:  obj.Name,
		Edges: edges,
	}

	/* vertices */
	s = cp.Scale * 16
	for _, v := range obj.Vertices {
		ov := []int{int(v[0] * s), int(v[1] * s), int(v[2] * s), 0}
		ps.Vertices = append(ps.Vertices, ov)
	}

	/* edges */
	ps.EdgeOffset = len(ps.Vertices) * cp.VertexSize

	for i := 0; i < len(edges); i++ {
		for j := 0; j < len(edges[i]); j++ {
			/* pre-compute vertex indices */
			edges[i][j] *= cp.VertexSize
		}
		edges[i] = append([]int{0}, edges[i]...)
	}

	/* faces */
	ps.FaceDataOffset = ps.EdgeOffset + len(ps.Edges)*cp.EdgeSize

	var faceIndices []int

	for i, f := range obj.Faces {
		poly := bool(len(f.Indices) >= 3)
		of := []int{}
		if poly {
			fn := faceNormals[i]
			x, y, z := fn[0]*4096, fn[1]*4096, fn[2]*4096
			of = append(of, int(x), int(y), int(z))
		}
		of = append(of, f.Material, len(f.Indices))
		faceIndices = append(faceIndices, ps.FaceDataCount+len(of)*cp.IndexSize+ps.FaceDataOffset)
		for _, fi := range f.Indices {
			/* precompute vertex / edge indices */
			of = append(of, (fi.Vertex-1)*cp.VertexSize)
			if poly {
				of = append(of, fi.Edge*cp.EdgeSize+ps.EdgeOffset)
			}
		}
		ps.FaceData = append(ps.FaceData, of)
		ps.FaceDataCount += len(of) * cp.IndexSize
	}

	/* groups */
	ps.GroupDataOffset = ps.FaceDataOffset + ps.FaceDataCount*cp.IndexSize
	ps.GroupDataCount = 0

	for _, grp := range obj.Groups {
		og := []int{}
		for _, fi := range grp.FaceIndices {
			og = append(og, faceIndices[fi])
		}
		ps.Groups = append(ps.Groups,
			FaceGroup{
				Name:    grp.Name,
				Offset:  ps.GroupDataCount,
				Indices: og,
			})
		ps.GroupDataCount += len(og) + 1
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
	IndexSize  int
}
type FaceGroup struct {
	Name    string
	Offset  int
	Indices []int
}

type FaceMaterial struct {
	Name  string
	Index int
}

type TemplateParams struct {
	Name string

	FaceDataCount  int
	GroupDataCount int

	EdgeOffset      int
	FaceDataOffset  int
	GroupDataOffset int

	Vertices  [][]int
	Edges     []Edge
	FaceData  [][]int
	Groups    []FaceGroup
	Materials []FaceMaterial
}
