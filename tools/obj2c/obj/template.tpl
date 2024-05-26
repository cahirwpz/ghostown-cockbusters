static MeshSurfaceT _{{ .Name }}_surf[1] = {
    [0] = { /* name = "default"  */
        .r = {{ .R }}, .g = {{ .G }}, .b = {{ .B }},
        .sideness = {{ .Sideness }}
        .texture = {{ .Texture }},
    },
};

static Point3D _{{ .Name }}_pnts[{{ .PointsCount }}] = {
    {{ range .Points }}
    { .x = {{ .X }}, .y = {{ .Y }}, .z = {{ .Z }}, .pad = {{ .Pad }} },
    {{ end -}}
};

static IndexListT *_{{ .Name }}_face[{{ .FaceCount }}] = {
    {{ range .Faces }}
    (IndexListT *)(short[7]{{ . }}),
    {{ end -}}
    NULL
};

static u_char _{{ .Name }}_face_surf[{{ .SurfaceCount }}] = {
    {{ range .Surfaces }}
    {{- . -}},
    {{ end -}}
};

Mesh3D {{ .Name }} = {
    .vertices = {{ .VerticeCount }},
    .faces = {{ .FaceCount }},
    .edges = {{ .EdgeCount }},
    .surfaces = {{ .SurfaceCount }},
    .images = {{ .ImageCount}},
    .vertex = _{{ .Name }}_pnts,
    .uv = NULL,
    .faceNormal = NULL, 
    .edge = NULL,
    .face = _{{ .Name }}_face,
    .faceEdge = NULL, 
    .faceUV = NULL,
    .vertexFace = NULL,
    .image = NULL,
    .surface = _{{ .Name }}_surf,

};
