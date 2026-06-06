# Rock model assets

Place irregular, rounded, volumetric rock models at:

```text
assets/models/rocks/rock01.obj
assets/models/rocks/rock02.obj
assets/models/rocks/rock03.obj
```

The application loads every available file through Assimp and distributes the
models across foreground-left, mid-right, and far-background clusters.

OBJ models may reference sibling `.mtl` and texture files. Keep those files in
this directory beside the matching model.

