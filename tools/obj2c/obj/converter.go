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

	s = cp.Scale * 16
	for _, v := range obj.Vertices {
		ov := []int{int(v[0] * s), int(v[1] * s), int(v[2] * s)}
		ps.Vertices = append(ps.Vertices, ov)
		ps.VertexCount += 1
	}

	ps.FaceDataCount = 2
	for _, f := range obj.Faces {
		of := []int{f.Material, len(f.Indices)}
		for _, fi := range f.Indices {
			of = append(of, fi.Vertex-1)
			of = append(of, fi.Edge)
		}
		ps.FaceData = append(ps.FaceData, of)
		ps.FaceDataCount += len(f.Indices)*2 + 2
		ps.FaceCount += 1
	}

	/* append guard element */
	ps.FaceData = append(ps.FaceData, []int{-1, 0})

	s = 4096.0
	for _, fn := range faceNormals {
		ofn := []int{int(fn[0] * s), int(fn[1] * s), int(fn[2] * s)}
		ps.FaceNormals = append(ps.FaceNormals, ofn)
	}

	ps.MaterialCount = len(obj.Materials)
	ps.GroupCount = len(obj.Groups)

	ps.GroupDataCount = 1
	for _, grp := range obj.Groups {
		og := append([]int{len(grp.FaceIndices)}, grp.FaceIndices...)
		ps.GroupDataCount += len(og)
		ps.GroupData = append(ps.GroupData, og)
	}

	/* append guard element */
	ps.GroupData = append(ps.GroupData, []int{0})

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
	Scale float64
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
	FaceNormals [][]int
	Edges       []Edge
}
