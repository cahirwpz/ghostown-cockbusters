package obj

import (
	"fmt"
	"math"
)

/*
 * For given triangle T with vertices A, B and C, surface normal N is a cross
 * product between vectors AB and BC.
 *
 * Ordering of vertices in polygon description is meaningful - depending on
 * that the normal vector will be directed inwards or outwards.
 *
 * Clockwise convention is used.
 */
func CalculateFaceNormals(obj *WavefrontObj) ([]Vector, error) {
	var ns []Vector

	for i, face := range obj.Faces {
		p1 := obj.Vertices[face[0].Vertex-1]
		p2 := obj.Vertices[face[1].Vertex-1]
		p3 := obj.Vertices[face[2].Vertex-1]

		ax := p1[0] - p2[0]
		ay := p1[1] - p2[1]
		az := p1[2] - p2[2]
		bx := p2[0] - p3[0]
		by := p2[1] - p3[1]
		bz := p2[2] - p3[2]

		x := ay*bz - by*az
		y := az*bx - bz*ax
		z := ax*by - bx*ay

		l := math.Sqrt(x*x + y*y + z*z)

		if l == 0 {
			return nil, fmt.Errorf("face #%d normal vector has zero length", i)
		}

		// Normal vector has a unit length.
		ns = append(ns, Vector{x / l, y / l, z / l})
	}

	return ns, nil
}
