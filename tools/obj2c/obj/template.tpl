/* all indices are offsets in bytes from the beginning of data array */

static short _{{ .Name }}_data[] = {
  /* vertices: [x y z pad] */
  /* offset: 0, count: {{ len .Vertices }} */
  {{- range .Vertices }}
  {{ range . }}{{ . }}, {{ end -}}
{{ end }} 

  /* edges: [flags vertex-index-0 vertex-index-1] */
  /* offset: {{ .EdgeOffset }}, count: {{ len .Edges }} */
  {{- range .Edges }}
  {{ .Flags }}, {{ range .Point }}{{ . }}, {{ end -}}
{{- end }}

  /* faces: [face-normal{x y z} {flags:8 material:8} #indices | {vertex-index edge-index}...] */
  /* lines/points: [{flags:8 material:8} #indices | vertex-index...] */
  /* offset: {{ .FaceDataOffset }}, count: {{ .FaceDataCount }} */
  {{- range .Faces }}
  {{range .Normal }}{{ . }}, {{ end -}}{{.Material}}, {{.Count}}, {{ range .Indices }}{{ . }}, {{ end -}}
{{- end}}

  /* face-groups: [face-index... 0] 0 */
  /* offset: {{ .ObjectDataOffset }}, count: {{ .ObjectDataCount }} */
  {{- range $obj := .Objects }}

  {{- range .FaceGroups }}
  /* {{ $obj.Name }}:{{ .Name }} */
  {{ range .Indices }}{{ . }}, {{ end -}}0, 
{{- end}}
{{- end}}
  /* end */
  0
};
{{ range $obj := .Objects }}
#define {{ $.Name }}_obj_{{ $obj.Name }} {{ $obj.Offset }}
{{- range $grp := $obj.FaceGroups }}
#define {{ $obj.Name }}_grp_{{ $grp.Name }} {{ $grp.Offset }}
{{- end}}
{{- end}}
{{ range .Materials }}
#define {{ $.Name }}_mtl_{{ .Name }} {{ .Index }}
{{- end}}

Mesh3D {{ .Name }} = {
  .vertices = {{ len .Vertices }},
  .edges = {{ len .Edges }},
  .faces = {{ len .Faces }},
  .materials = {{ len .Materials }},
  .vertex = _{{ .Name }}_data,
  .edge = (void *)_{{ .Name }}_data + {{ .EdgeOffset }},
  .object = (void *)_{{ .Name }}_data + {{ .ObjectDataOffset }},
};
