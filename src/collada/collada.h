struct collada_loading_data
{
    vec3* Positions;
    vec3* Normals;
    vec3* Tangents;
    vec2* UVs;
    
    vec4* Weights;
    ivec4* Joints;
    
    u32 PositionsCount;
    u32 NormalsCount;
    u32 TangentsCount;
    u32 UVsCount;
    
    //Offset into a single <p> face
    u32 PositionsOffset;
    u32 NormalsOffset;
    u32 TangentsOffset;
    u32 UVsOffset;
    
    //Stride in the indices buffer (basically max{offsets} + 1), as it appears in the file
    //so it may count buffers that we are not using too.
    u32 AttributesCount;
};

struct collada_vertex_data
{
    _sbuf_ vec3* Positions;
    _sbuf_ vec3* Normals;
    _sbuf_ vec3* Tangents;
    _sbuf_ vec2* UVs;
    
    _sbuf_ vec4* Weights;
    _sbuf_ ivec4* Joints;
    
    _sbuf_ u32* Indices;
};

struct collada_joint_tree
{
    char* Name; //Owned by the xml node it refers to
    u32 Id;
    
    //Local bind transform
    mat4 Transform; //Used if we have no inverse bind transforms already
    //Inverse bind transform, inverse of ParentTransform * Transform
    mat4 InverseBindTransform;
    
    _sbuf_ collada_joint_tree** Children;
    
    f32* KeyframeTimes;
    mat4* KeyframeTransforms;
    u32 KeyframeCount;
};

struct collada_scene
{
    _sbuf_ mesh_data* Meshes;
    u32 MeshesCount;
};

struct joint_weight_pair
{
    u32 Joint;
    f32 Weight;
};

//Keywords (for now those are interned every time we read a file
char* Geometry_Keyword = 0;
char* Mesh_Keyword = 0;
char* Polylist_Keyword = 0;
char* Source_Keyword = 0;
char* Input_Keyword = 0;
char* Id_Keyword = 0;
char* Vertices_Keyword = 0;
char* Semantic_Keyword = 0;
char* FloatArray_Keyword = 0;
char* Accessor_Keyword = 0;
char* Count_Keyword = 0;
char* Triangles_Keyword = 0;
char* Offset_Keyword = 0;
char* VCount_Keyword = 0;
char* P_Keyword = 0;
char* V_Keyword = 0;
char* Controller_Keyword = 0;
char* Skin_Keyword = 0;
char* BindShapeMatrix_Keyword = 0;
char* Joints_Keyword = 0;
char* Stride_Keyword = 0;
char* VertexWeights_Keyword = 0;
char* NameArray_Keyword = 0;
char* InstanceController_Keyword = 0;
char* Url_Keyword = 0;
char* Skeleton_Keyword = 0;
char* LibraryVisualScenes_Keyword = 0;
char* Node_Keyword = 0;
char* Name_Keyword = 0;
char* Sid_Keyword = 0;
char* Matrix_Keyword = 0;
char* Animation_Keyword = 0;
char* Channel_Keyword = 0;
char* LibraryAnimations_Keyword = 0;
char* Target_Keyword = 0;
char* Type_Keyword = 0;