static MeshSurfaceT _{{ .Name }}_surf[1] = {
    [0] = { /* name = "default"  */
        .r = {{ .R }}, .g = {{ .G }}, .b = {{ .B }},
        .sideness = {{ .Sideness }}
        .texture = {{ .Texture }},
    },
};

static Point3D _{{ .Name }}_pnts[{{ .PointsCounter }}] = {
    {{ range .Points }}
    { .x = {{ .X }}, .y = {{ .Y }}, .z = {{ .Z }}, .pad = {{ .Pad }} },
    {{ end -}}
};

static IndexListT *_{{ .Name }}_face[{{ .FaceCounter }}] = {
    {{ range .Faces }}
    (IndexListT *)(short[7]{{ . }}),
    {{ end -}}
    NULL
};

static u_char _{{ .Name }}_face_surf[{{ .SurfaceCounter }}] = {
    {{ range .Surfaces }}
    {{- . -}},
    {{ end -}}
};

Mesh3D {{ .Name }} = {
    .vertices = {{ .VerticeCounter }},
    .faces = {{ .FaceCounter }},
    .edges = {{ .EdgeCounter }},
    .surfaces = {{ .SurfaceCounter }},
    .images = {{ .ImageCounter}},
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
