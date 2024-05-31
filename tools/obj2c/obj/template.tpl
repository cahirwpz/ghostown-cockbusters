static short _{{ .Name }}_pnts[{{ .VertexCount }} * 4] = {
  /* x, y, z, pad */
  {{- range .Vertices }}
  {{ range . }}{{ . }}, {{ end -}} 0,
{{- end }}
};

static short _{{ .Name }}_face_data[{{ .FaceDataCount }}] = {
  /* #vertices, vertices... */
  {{- range .Faces }}
  {{range . }}{{ . }}, {{ end -}}
{{- end}}
  0 
};

static short *_{{ .Name }}_face[{{ .FaceCount }} + 1] = {
  {{- range .FaceIndices }}
  &_{{ $.Name }}_face_data[{{ . }}],
{{- end }}
  NULL
};

{{- if .Edges }}

static short _{{ .Name }}_edge_data[{{ .EdgeCount }} * 2] = {
  {{- range .Edges }}
  {{ range . }}{{ . }} * 8, {{ end -}}
{{- end }}
};

static short _{{ .Name }}_face_edge_data[{{ .FaceDataCount }}] = {
  /* #edge, edges... */
  {{- range .FaceEdges }}
  {{range . }}{{ . }}, {{ end -}}
{{- end}}
  0 
};

static short *_{{ .Name }}_face_edge[{{ .FaceCount }} + 1] = {
  {{- range .FaceIndices }}
  &_{{ $.Name }}_face_edge_data[{{ . }}],
{{- end }}
  NULL
};
{{- end }}

{{- if .FaceNormals }}

static short _{{ .Name }}_face_normals[{{ .FaceCount }} * 4] = {
  /* x, y, z, pad */
  {{- range .FaceNormals }}
  {{ range . }}{{ . }}, {{ end -}} 0,
{{- end }}
};
{{- end }}

Mesh3D {{ .Name }} = {
  .vertices = {{ .VertexCount }},
  .faces = {{ .FaceCount }},
  .edges = {{ .EdgeCount }},
  .vertex = (Point3D *)&_{{ .Name }}_pnts,
{{- if .FaceNormals }}
  .faceNormal = (Point3D *)&_{{ .Name }}_face_normals,
{{- else }}
  .faceNormal = NULL,{{ end }}
{{- if .Edges }}
  .edge = (EdgeT *)&_{{ .Name }}_edge_data,
{{- else }}
  .edge = NULL,{{ end }}
  .face = _{{ .Name }}_face,
{{- if .Edges }}
  .faceEdge = _{{ .Name }}_face_edge,
{{- else }}
  .faceEdge = NULL,{{ end }}
};
