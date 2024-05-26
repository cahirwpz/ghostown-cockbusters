package obj

import (
	"bufio"
	"fmt"
	"io"
	"strconv"
	"strings"
)

func ParseObj(file io.Reader) (*WavefrontObj, error) {
	scanner := bufio.NewScanner(file)

	var vs []Vector
	var vts []Vector
	var vns []Vector

	lc := 1

	for scanner.Scan() {
		line := strings.Trim(scanner.Text(), " ")
		fields := strings.Split(line, " ")
		cmd := fields[0]
		fields = fields[1:]
		switch cmd {
		case "":
		case "#":
			/* empty line or comment */
		case "v":
			/* Geometric vertex, with (x, y, z, [w]) coordinates
			 * w is optional and defaults to 1.0 */
			v, err := parseVector(fields, 3)
			if err != nil {
				return nil, err
			}
			vs = append(vs, v)
		case "vt":
			/* Texture coordinates, in (u, [v, w]) coordinates, these will vary between 0 and 1
			 * v, w are optional and default to 0.0 */
			vt, err := parseVector(fields, 1)
			if err != nil {
				return nil, err
			}
			vts = append(vts, vt)
		case "vn":
			/* Vertex normal in (x,y,z) form; normals might not be unit vectors */
			vn, err := parseVector(fields, 3)
			if err != nil {
				return nil, err
			}
			vns = append(vns, vn)
		case "f":
			/* Faces are defined using lists of vertex, texture and normal
			 * indices in the format vertex_index/texture_index/normal_index for
			 * which each index starts at 1 and increases corresponding to the
			 * order in which the referenced element was defined. */
			indices := strings.Split(fields[0], "/")
			face := Face{}
			for i, index := range indices {
				if index == "" {
					continue
				}

				v, err := strconv.ParseInt(index, 10, 32)
				if err != nil {
					return nil, err
				}
				switch i {
				case 0:
					face.Vertex = v
				case 1:
					face.TexCoord = v
				case 2:
					face.Normal = v
				default:
				}
			}
		case "mtllib":
		case "p":
		case "l":
		case "g":
		case "o":
		case "usemtl":
			/* not implemented, skipped */
		default:
			return nil, fmt.Errorf("unknown command '%s' in line %d", cmd, lc)
		}

		lc++
	}
	if err := scanner.Err(); err != nil {
		return nil, fmt.Errorf("error while reading object: %s", err)
	}
	return &WavefrontObj{Vertices: vs, TexCoords: vts, Normals: vns}, nil
}

func parseVector(fields []string, length int) (vec Vector, err error) {
	if len(fields) < length {
		return nil, fmt.Errorf("expected vector with len %v, got %v", length, len(vec))
	}
	for _, field := range fields {
		v, err := strconv.ParseFloat(field, 64)
		if err != nil {
			return nil, err
		}
		vec = append(vec, v)
	}
	return vec, nil
}
