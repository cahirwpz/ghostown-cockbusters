package obj

import (
	_ "embed"
	"strings"
	"text/template"
)

//go:embed template.tpl
var tpl string

func Convert(obj *WavefrontObj, cp ConverterParams) (string, error) {
	ps := TemplateParams{Name: obj.Name}

	s := cp.Scale * 16
	for _, v := range obj.Vertices {
		ov := []int{int(v[0] * s), int(v[1] * s), int(v[2] * s)}
		ps.Vertices = append(ps.Vertices, ov)
		ps.VertexCount += 1
	}

	faceOffset := 1
	for _, f := range obj.Faces {
		of := []int{len(f)}
		for _, fi := range f {
			of = append(of, fi.Vertex-1)
		}
		ps.Faces = append(ps.Faces, of)
		ps.FaceIndices = append(ps.FaceIndices, faceOffset)

		faceOffset += len(of)
		ps.FaceCount += 1
	}

	ps.FaceDataCount = faceOffset

	if cp.FaceNormals {
		faceNormals, err := CalculateFaceNormals(obj)
		if err != nil {
			return "", err
		}

		s := 4096.0
		for _, fn := range faceNormals {
			ofn := []int{int(fn[0] * s), int(fn[1] * s), int(fn[2] * s)}
			ps.FaceNormals = append(ps.FaceNormals, ofn)
		}
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
	Scale       float64
	FaceNormals bool
}

type TemplateParams struct {
	Name string

	VertexCount   int
	FaceCount     int
	FaceDataCount int

	Vertices    [][]int
	Faces       [][]int
	FaceIndices []int
	FaceNormals [][]int
}
