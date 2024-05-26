package obj

import (
	_ "embed"
	"fmt"
)

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

//go:embed template.tpl
var tpl string

func Convert(obj *WavefrontObj) (output string, err error) {
	// var model Model
	// out := util.CompileTemplate(tpl, model)
	return "", fmt.Errorf("not implemented")
}
