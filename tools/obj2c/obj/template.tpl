/* all indices are offsets in bytes from the beginning of data array */

static short _{{ .Name }}_data[] = {
  /* vertices: [x y z pad] */
  /* offset: 0 */
  {{- range .Vertices }}
  {{ range . }}{{ . }}, {{ end -}}
{{ end }} 

  /* edges: [flags vertex-index-0 vertex-index-1] */
  /* offset: {{ .EdgeOffset }} */
  {{- range .Edges }}
  {{ range . }}{{ . }}, {{ end -}}
{{- end }}

  /* faces: [face-normal{x y z} {flags:8 material:8} #indices | {vertex-index edge-index}...] */
  /* lines/points: [{flags:8 material:8} #indices | vertex-index...] */
  /* offset: {{ .FaceDataOffset }} */
  {{- range .FaceData }}
  {{range . }}{{ . }}, {{ end -}}
{{- end}}

  /* groups: [#faces face-index...] */
  /* offset: {{ .GroupDataOffset }} */
  {{- range .Groups }}
  /* {{ .Name }} */
  {{ len .Indices }}, {{range .Indices }}{{ . }}, {{ end -}}
{{- end}}
  /* end */
  0
};
{{ range .Groups }}
#define {{ $.Name }}_grp_{{ .Name }} {{ .Offset }}
{{- end}}
{{ range .Materials }}
#define {{ $.Name }}_mtl_{{ .Name }} {{ .Index }}
{{- end}}

Mesh3D {{ .Name }} = {
  .vertices = {{ len .Vertices }},
  .faces = {{ len .FaceData }},
  .edges = {{ len .Edges }},
  .groups = {{ len .Groups }},
  .materials = {{ len .Materials }},
  .vertex = _{{ .Name }}_data,
  .edge = (void *)_{{ .Name }}_data + {{ .EdgeOffset }},
  .group = (void *)_{{ .Name }}_data + {{ .GroupDataOffset }},
};
