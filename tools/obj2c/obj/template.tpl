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
  .edges = 0,
  .vertex = (Point3D *)&_{{ .Name }}_pnts,
{{- if .FaceNormals }}
  .faceNormal = (Point3D *)&_{{ .Name }}_face_normals,
{{- else }}
  .faceNormal = NULL,{{ end }}
  .edge = NULL,
  .face = _{{ .Name }}_face,
  .faceEdge = NULL,
};
