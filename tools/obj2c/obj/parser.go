package obj

import (
	"fmt"
	"strconv"
	"strings"
)

const (
	PRE_VERTEX_NORMAL   = "vn"
	PRE_TEXTURE_VERTICE = "vt"
	PRE_VERTICE         = "v"
	PRE_POINT           = "p"
	PRE_LINE            = "l"
	PRE_FACE            = "f"
	PRE_GROUP_NAME      = "g"
	PRE_OBJECT_NAME     = "o"
	PRE_MATERIAL_NAME   = "usemtl"
	PRE_MATERIAL_LIB    = "mtllib"
)

func Parse(input string) (*Obj, error) {
	out := &Obj{}
	lines := strings.Split(input, "\n")
	for _, line := range lines {
		switch {
		case strings.HasPrefix(line, PRE_VERTEX_NORMAL):
			{
				items := strings.Split(line, " ")
				vec, err := toVector(items, 3)
				if err != nil {
					return nil, err
				}
				out.NVertices = append(out.NVertices, vec)
			}
		case strings.HasPrefix(line, PRE_TEXTURE_VERTICE):
			{
				items := strings.Split(line, " ")
				vec, err := toVector(items, 2)
				if err != nil {
					return nil, err
				}
				out.TVertices = append(out.TVertices, vec)
			}
		case strings.HasPrefix(line, PRE_VERTICE):
			{
				items := strings.Split(line, " ")
				vec, err := toVector(items, 3)
				if err != nil {
					return nil, err
				}
				out.GVertices = append(out.GVertices, vec)
			}
		}
	}
	return nil, nil
}

func toVector(items []string, length int) (vec Vector, err error) {
	for _, s := range items {
		if v, err := strconv.ParseFloat(s, 64); err == nil {
			vec = append(vec, v)
		}
	}
	if len(vec) != length {
		return nil, fmt.Errorf("expected vector with len %v, got %v", length, len(vec))
	}
	return vec, nil
}
