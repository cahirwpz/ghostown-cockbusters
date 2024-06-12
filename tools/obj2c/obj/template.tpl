static short _{{ .Name }}_vertex[{{ .VertexCount }} * 4] = {
  /* x y z flags/pad */
  {{- range .Vertices }}
  {{ range . }}{{ . }}, {{ end -}} 0,
{{- end }} 
};

static short _{{ .Name }}_edge[{{ .EdgeCount }} * 3] = {
  /* flags point_0 point_1 */
  {{- range .Edges }}
  0, {{ range . }}{{ . }}, {{ end -}}
{{- end }}
};

static short _{{ .Name }}_face_normal[{{ .FaceCount }} * 4] = {
  /* x y z flags/pad */
  {{- range .FaceNormals }}
  {{ range . }}{{ . }}, {{ end -}} 0,
{{- end }}
};

static short _{{ .Name }}_face_data[{{ .FaceDataCount }}] = {
  /* #indices material [vertex-index edge-index]... */
  {{- range .FaceData }}
  {{range . }}{{ . }}, {{ end -}}
{{- end}}
  0
};

static short _{{ .Name }}_group_data[{{ .GroupDataCount }}] = {
  /* #faces [flags face-index]... */
  {{- range .GroupData }}
  {{ len . }}, {{range . }}0, {{ . }}, {{ end -}}
{{- end}}
  0,
};

{{ range .Groups }}
#define {{ $.Name }}_grp_{{ .Name }} {{ .Index }}
{{- end}}

{{ range .Materials }}
#define {{ $.Name }}_mtl_{{ .Name }} {{ .Index }}
{{- end}}

Mesh3D {{ .Name }} = {
  .vertices = {{ .VertexCount }},
  .faces = {{ .FaceCount }},
  .edges = {{ .EdgeCount }},
  .groups = {{ .GroupCount }},
  .materials = {{ .MaterialCount }},
  .vertex = _{{ .Name }}_vertex,
  .faceNormal = _{{ .Name }}_face_normal,
  .edge = _{{ .Name }}_edge,
  .faceData = _{{ .Name }}_face_data,
  .groupData = _{{ .Name }}_group_data,
};
