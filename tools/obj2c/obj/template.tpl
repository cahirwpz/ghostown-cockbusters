static short _{{ .Name }}_vertex[{{ .VertexCount }} * 4] = {
  /* x, y, z, pad */
  {{- range .Vertices }}
  {{ range . }}{{ . }}, {{ end -}} 0,
{{- end }}
};

static short _{{ .Name }}_edges[{{ .EdgeCount }} * 2] = {
  {{- range .Edges }}
  {{ range . }}{{ . }}, {{ end -}}
{{- end }}
};

static short _{{ .Name }}_face_normals[{{ .FaceCount }} * 4] = {
  /* x, y, z, pad */
  {{- range .FaceNormals }}
  {{ range . }}{{ . }}, {{ end -}} 0,
{{- end }}
};

static short _{{ .Name }}_face_data[{{ .FaceDataCount }}] = {
  /* material, #indices, [vertex-index edge-index]... */
  {{- range .FaceData }}
  {{range . }}{{ . }}, {{ end -}}
{{- end}}
};

static short _{{ .Name }}_group_data[{{ .GroupDataCount }}] = {
  {{- range .GroupData}}
  {{range . }}{{ . }}, {{ end -}}
{{- end}}
};

Mesh3D {{ .Name }} = {
  .vertices = {{ .VertexCount }},
  .faces = {{ .FaceCount }},
  .edges = {{ .EdgeCount }},
  .groups = {{ .GroupCount }},
  .materials = {{ .MaterialCount }},
  .vertex = _{{ .Name }}_vertex,
  .faceNormal = _{{ .Name }}_face_normals,
  .edge = _{{ .Name }}_edges,
  .faceData = _{{ .Name }}_face_data,
  .groupData = _{{ .Name }}_group_data,
};
