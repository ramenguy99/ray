#include "sbuf.cpp"
#include "arena.cpp"
#include "map.cpp"
#include "string_intern.cpp"
#include "lexer.cpp"
#include "xml.cpp"

#include "collada.h"

internal vec3*
FloatArrayElementToVec3Array(xml_element* FloatArray, u32 Count)
{
    if(Count == 0) return 0;
    
    Assert(Count % 3 == 0);
    vec3* Result = (vec3*)ZeroAlloc(sizeof(float) * Count);
    
    lexer Lexer;
    InitLexer(&Lexer, FloatArray->Text, LEXER_IGNORE_NEWLINES | LEXER_XML_COMMENTS);
    
    
    u32 Index = 0;
    while(!IsToken(&Lexer, TOKEN_EOF) && Index < (Count / 3))
    {
        float x = ParseFloatOrIntWithSign(&Lexer);
        float y = ParseFloatOrIntWithSign(&Lexer);
        float z = ParseFloatOrIntWithSign(&Lexer);
        Result[Index++] = vec3(x, y, z);
    }
    Assert(Index == Count / 3 && IsToken(&Lexer, TOKEN_EOF));
    FreeLexer(&Lexer);
    
    return Result;
}

internal vec2*
FloatArrayElementToVec2Array(xml_element* FloatArray, u32 Count)
{
    if(Count == 0) return 0;
    
    Assert(Count % 2 == 0);
    vec2* Result = (vec2*)ZeroAlloc(sizeof(float) * Count);
    
    lexer Lexer;
    InitLexer(&Lexer, FloatArray->Text, LEXER_IGNORE_NEWLINES | LEXER_XML_COMMENTS);
    
    u32 Index = 0;
    while(!IsToken(&Lexer, TOKEN_EOF) && Index < (Count / 2))
    {
        float x = ParseFloatOrIntWithSign(&Lexer);
        float y = ParseFloatOrIntWithSign(&Lexer);
        Result[Index++] = vec2(x, y);
    }
    Assert(Index == Count / 2 && IsToken(&Lexer, TOKEN_EOF));
    FreeLexer(&Lexer);
    
    return Result;
}

internal mat4*
FloatArrayElementToMat4Array(xml_element* FloatArray, u32 Count)
{
    if(Count == 0) return 0;
    
    Assert(Count % 16 == 0);
    u32 MatCount = Count / 16;
    mat4* Result = (mat4*)ZeroAlloc(sizeof(float) * Count);
    
    lexer Lexer;
    InitLexer(&Lexer, FloatArray->Text, LEXER_IGNORE_NEWLINES | LEXER_XML_COMMENTS);
    
    u32 Index = 0;
    while(!IsToken(&Lexer, TOKEN_EOF) && Index < MatCount)
    {
        Result[Index++] = ParseMat4RowMajor(&Lexer);
    }
    Assert(Index == MatCount && IsToken(&Lexer, TOKEN_EOF));
    FreeLexer(&Lexer);
    
    return Result;
}

internal u32*
IntArrayElementToU32Array(xml_element* ArrayElement, u32 Count)
{
    if(Count == 0) return 0;
    
    u32* Result = (u32*)ZeroAlloc(sizeof(u32) * Count);
    
    lexer Lexer;
    InitLexer(&Lexer, ArrayElement->Text, LEXER_IGNORE_NEWLINES | LEXER_XML_COMMENTS);
    
    u32 Index = 0;
    while(!IsToken(&Lexer, TOKEN_EOF) && Index < Count)
    {
        Result[Index++] = (u32)ParseInt(&Lexer);
    }
    Assert(Index == Count && IsToken(&Lexer, TOKEN_EOF));
    FreeLexer(&Lexer);
    
    return Result;
}

internal f32*
FloatArrayElementToFloatArray(xml_element* ArrayElement, u32 Count)
{
    if(Count == 0) return 0;
    
    f32* Result = (f32*)ZeroAlloc(sizeof(f32) * Count);
    
    lexer Lexer;
    InitLexer(&Lexer, ArrayElement->Text, LEXER_IGNORE_NEWLINES | LEXER_XML_COMMENTS);
    
    u32 Index = 0;
    while(!IsToken(&Lexer, TOKEN_EOF) && Index < Count)
    {
        Result[Index++] = (f32)ParseFloatOrIntWithSign(&Lexer);
    }
    Assert(Index == Count && IsToken(&Lexer, TOKEN_EOF));
    FreeLexer(&Lexer);
    
    return Result;
}

internal char**
NameArrayElementToStringArray(xml_element* ArrayElement, u32 Count)
{
    if(Count == 0) return 0;
    
    char** Result = (char**)ZeroAlloc(sizeof(char*) * Count);
    
    lexer Lexer;
    InitLexer(&Lexer, ArrayElement->Text, LEXER_IGNORE_NEWLINES | LEXER_XML_COMMENTS
              | LEXER_NO_NAME_INTERNING);
    
    u32 Index = 0;
    while(!IsToken(&Lexer, TOKEN_EOF) && Index < Count)
    {
        char* NameBegin = Lexer.Token.Start;
        char* NameEnd = Lexer.Token.End;
        ExpectToken(&Lexer, TOKEN_NAME);
        
        u32 NameLength = (u32)(NameEnd - NameBegin);
        char* String = (char*)ZeroAlloc(NameLength + 1);
        strncpy(String, NameBegin, NameLength);
        Result[Index++] = String;
    }
    Assert(Index == Count && IsToken(&Lexer, TOKEN_EOF));
    FreeLexer(&Lexer);
    
    return Result;
}

internal char*
GetUrlNameFromString(char* String)
{
    char* Result = (char*)ZeroAlloc(strlen(String) + 2);
    Result[0] = '#';
    strcpy(Result + 1, String);
    return Result;
}

internal collada_vertex_data
AssembleVerticesFromColladaLoadingData(collada_loading_data* Data, u32 * Indices,
                                       u32 IndicesCount)
{
    collada_vertex_data Result = {};
    
    Assert(Data->Positions);
    //We use the indexing of the positions as a base, we then add other attributes
    //at the same index as long as they match the same vertex, otherwise we push a new vertex
    SbufPushN(Result.Positions, Data->PositionsCount);
    SbufPushN(Result.Normals, Data->PositionsCount);
    SbufPushN(Result.UVs, Data->PositionsCount);
    SbufPushN(Result.Tangents, Data->PositionsCount);
    
    //Those are matched with positions, so we index them with VertexIndex
    if(Data->Weights && Data->Joints)
    {
        SbufPushN(Result.Weights, Data->PositionsCount);
        SbufPushN(Result.Joints, Data->PositionsCount);
    }
    
    for(u32 Index = 0; Index < Data->PositionsCount; Index++)
    {
        Result.Positions[Index] = Data->Positions[Index];
        if(Data->Weights && Data->Joints)
        {
            Result.Weights[Index] = Data->Weights[Index];
            Result.Joints[Index] = Data->Joints[Index];
        }
        
        Result.Normals[Index] = Data->Normals ? vec3(NAN, NAN, NAN) : vec3(0);
        Result.Tangents[Index] = Data->Tangents ? vec3(NAN, NAN, NAN) : vec3(0);
        Result.UVs[Index] = Data->UVs ? vec2(NAN, NAN) : vec2(0);
    }
    
    for(u32 i = 0; i < IndicesCount; i += Data->AttributesCount)
    {
        //The position index becomes the index of the vertex
        u32 VertexIndex = Indices[i + Data->PositionsOffset];
        u32 NormalsIndex = Indices[i + Data->NormalsOffset];
        u32 UVsIndex = Indices[i + Data->UVsOffset];
        u32 TangentsIndex = Indices[i + Data->TangentsOffset];
        
        SbufPush(Result.Indices, VertexIndex);
        b32 NeedToPushVertex = false;
        
        
#define NEED_TO_PUSH(attribute) \
if(Data->attribute && \
!isnan(Result.attribute[VertexIndex].x) && \
Result.attribute[VertexIndex] != Data->attribute[attribute##Index]) \
{ NeedToPushVertex = true; }
        
        NEED_TO_PUSH(Normals);
        NEED_TO_PUSH(UVs);
        NEED_TO_PUSH(Tangents);
        
#define PUSH_ATTRIBUTE(attribute) \
if(Data->attribute) { SbufPush(Result.attribute, Data->attribute[attribute##Index]); } \
else { SbufPush(Result.attribute, {}); }
        
        if(NeedToPushVertex) {
            //Put the last vertex position as new index
            Result.Indices[i / Data->AttributesCount] = (u32)SbufLen(Result.Positions);
            //Push copies of these at the end of the array
            SbufPush(Result.Positions, Result.Positions[VertexIndex]);
            if(Data->Weights && Data->Joints)
            {
                SbufPush(Result.Weights, Result.Weights[VertexIndex]);
                SbufPush(Result.Joints, Result.Joints[VertexIndex]);
            }
            
            PUSH_ATTRIBUTE(Normals);
            PUSH_ATTRIBUTE(Tangents);
            PUSH_ATTRIBUTE(UVs);
        }
        else {
#define SET_ATTRIBUTE(attribute) if(Data->attribute) { Result.attribute[VertexIndex] = Data->attribute[attribute##Index]; }
            SET_ATTRIBUTE(Normals);
            SET_ATTRIBUTE(Tangents);
            SET_ATTRIBUTE(UVs);
        }
    }
    
    //Some models are made up of multiple meshes, we currently don't support this behaviour,
    //In case we will in the future it might be more clever to recreate the positions buffer
    //aswell instead of reusing it, because there might be some vertices that are not indexed
    //by the primitive node, this way their attributes remain NAN as no index references them.
#if 1
    u64 VertexCount = SbufLen(Result.Positions);
    u64 FinalIndicesCount = SbufLen(Result.Indices);
    Assert(VertexCount == SbufLen(Result.Normals) &&
           VertexCount == SbufLen(Result.Tangents) &&
           VertexCount == SbufLen(Result.UVs));
    
    for(u32 Index = 0; Index < VertexCount; Index++)
    {
        if(IsNan(Result.Positions[Index]))
            Result.Positions[Index] = vec3(0);
        if(IsNan(Result.Normals[Index]))
            Result.Normals[Index] = vec3(0);
        if(IsNan(Result.Tangents[Index]))
            Result.Tangents[Index] = vec3(0);
        if(IsNan(Result.UVs[Index]))
            Result.UVs[Index] = vec2(0);
    }
#endif
    return Result;
}
#undef NEED_TO_PUSH
#undef PUSH_ATTRIBUTE
#undef SET_ATTRIBUTE

internal void
FillColladaLoadingData(collada_loading_data* Data, char* Semantic, xml_element* FloatArray,
                       xml_element* Accessor, u32 Count, u32 Offset)
{
    Data->AttributesCount = MAX(Offset + 1, Data->AttributesCount);
    if(strcmp(Semantic, "POSITION") == 0)
    {
        Assert(Data->Positions == 0);
        Assert(SbufLen(Accessor->Children) == 3);
        Data->PositionsOffset = Offset;
        Data->PositionsCount = Count / 3;
        Data->Positions = FloatArrayElementToVec3Array(FloatArray, Count);
    }
    else if(strcmp(Semantic, "NORMAL") == 0)
    {
        Assert(Data->Normals == 0);
        Assert(SbufLen(Accessor->Children) == 3);
        Data->NormalsOffset = Offset;
        Data->NormalsCount = Count / 3;
        Data->Normals = FloatArrayElementToVec3Array(FloatArray, Count);
    }
    else if(strcmp(Semantic, "TEXCOORD") == 0)
    {
        Assert(Data->UVs == 0);
        Assert(SbufLen(Accessor->Children) == 2);
        Data->UVsOffset = Offset;
        Data->UVsCount = Count / 2;
        Data->UVs = FloatArrayElementToVec2Array(FloatArray, Count);
    }
    else if(strcmp(Semantic, "TANGENT") == 0)
    {
        Assert(Data->Tangents == 0);
        Assert(SbufLen(Accessor->Children) == 3);
        Data->TangentsOffset = Offset;
        Data->TangentsCount = Count / 3;
        Data->Tangents = FloatArrayElementToVec3Array(FloatArray, Count);
    }
    
}

internal int
JointWeightComp(const void *pa, const void *pb)
{
    joint_weight_pair A = *(joint_weight_pair*)pa;
    joint_weight_pair B = *(joint_weight_pair*)pb;
    
    //Sort them in decreasing order
    if(A.Weight < B.Weight) return 1;
    else if(A.Weight > B.Weight) return -1;
    else return 0;
}

internal void
GetBestJointWeightPairs(joint_weight_pair* Pairs, u32 NumPairs, vec4* WeightVec, ivec4 *JointVec)
{
    qsort(Pairs, NumPairs, sizeof(joint_weight_pair), JointWeightComp);
    u32 Index = 0;
    for(; Index < MIN(NumPairs, 4); Index++)
    {
        WeightVec->e[Index] = Pairs[Index].Weight;
        JointVec->e[Index] = Pairs[Index].Joint;
    }
    for(; Index < 4; Index++)
    {
        WeightVec->e[Index] = 0;
        JointVec->e[Index] = 0;
    }
    
    f32 VecSum = WeightVec->x + WeightVec->y + WeightVec->z + WeightVec->w;
    *WeightVec = *WeightVec / VecSum;
    
    //Math check
#if 0
    VecSum = WeightVec->x + WeightVec->y + WeightVec->z + WeightVec->w;
    Assert(1.01f > VecSum && VecSum > 0.99f);
    Assert(WeightVec->x >= WeightVec->y && WeightVec->y >= WeightVec->z &&
           WeightVec->z >= WeightVec->w);
#endif
}

internal u32
FindStringPositionInArray(char* String, char** Array, u32 Count)
{
    for(u32 Index = 0; Index < Count; Index++)
    {
        if(strcmp(String, Array[Index]) == 0)
        {
            return Index;
        }
    }
    
    return (u32)-1;
}

internal mat4
GetMatrixFromElement(xml_element* Element)
{
    lexer Lexer;
    InitLexer(&Lexer, Element->Text, LEXER_IGNORE_NEWLINES | LEXER_XML_COMMENTS
              | LEXER_NO_NAME_INTERNING);
    mat4 Result = ParseMat4RowMajor(&Lexer);
    Assert(IsToken(&Lexer, TOKEN_EOF));
    
    return Result;
}

internal u32
CountColladaJointTree(collada_joint_tree* Tree)
{
    u32 Count = 1;
    for(u32 Index = 0; Index < SbufLen(Tree->Children); Index++)
    {
        Count += CountColladaJointTree(Tree->Children[Index]);
    }
    
    return Count;
}

internal collada_joint_tree*
LoadColladaJointTree(xml_element* Element, char** JointsNames, u32 JointsNamesCount,
                     xml_element* Root, mat4 ParentTransform)
{
    //Sid or name here?
    char* JointSid = GetXmlAttributeValue(Element, Sid_Keyword);
    u32 JointIndex = FindStringPositionInArray(JointSid, JointsNames, JointsNamesCount);
    if(JointIndex == (u32)-1) return 0;
    
    xml_element* MatrixElement = FindXmlFirstChildByName(Element, Matrix_Keyword);
    mat4 JointTransform = GetMatrixFromElement(MatrixElement);
    mat4 BindTransform = ParentTransform * JointTransform;
    mat4 InverseBindTransform = Mat4Inverse(BindTransform);
    
    //Load the animation data for this joint
    char* MatrixSid = GetXmlAttributeValue(MatrixElement, Sid_Keyword);
    //The animation target is in the format "JointId/MatrixSid"
    char* JointId = GetXmlAttributeValue(Element, Id_Keyword);
    u32 JointIdLength = (u32)strlen(JointId);
    char* AnimationTarget = (char*)ZeroAlloc(JointIdLength + 1 + strlen(MatrixSid) + 1);
    strcpy(AnimationTarget, JointId);
    AnimationTarget[JointIdLength] = '/';
    strcpy(AnimationTarget + JointIdLength + 1, MatrixSid);
    
    xml_element* LibraryAnimations = FindXmlFirstChildByName(Root, LibraryAnimations_Keyword);
    
    xml_element* ChannelElement = FindXmlFirstChildByNameAndAttribute(LibraryAnimations, Channel_Keyword, Target_Keyword, AnimationTarget);
    Free(AnimationTarget);
    
    xml_element* AnimationElement = ChannelElement->Parent;
    
    char* SamplerId = GetXmlAttributeValue(ChannelElement, Source_Keyword);
    SamplerId++; //Skip # char
    xml_element* SamplerElement = FindXmlFirstChildByAttribute(AnimationElement, Id_Keyword, SamplerId);
    
    xml_element* InputElement = FindXmlFirstChildByAttribute(SamplerElement, Semantic_Keyword, "INPUT");
    char* InputSourceId = GetXmlAttributeValue(InputElement, Source_Keyword);
    InputSourceId++; //Skip #char
    xml_element* InputSource = FindXmlFirstChildByAttribute(AnimationElement, Id_Keyword, InputSourceId);
    xml_element* InputFloatArray = FindXmlFirstChildByName(InputSource, FloatArray_Keyword);
    u32 InputCount = GetXmlAttributeValueAsU32(InputFloatArray, Count_Keyword);
    float* KeyframeTimes = FloatArrayElementToFloatArray(InputFloatArray, InputCount);
    
    xml_element* OutputElement = FindXmlFirstChildByAttribute(SamplerElement, Semantic_Keyword, "OUTPUT");
    char* OutputSourceId = GetXmlAttributeValue(OutputElement, Source_Keyword);
    OutputSourceId++; //Skip #char
    xml_element* OutputSource = FindXmlFirstChildByAttribute(AnimationElement, Id_Keyword, OutputSourceId);
    xml_element* OutputFloatArray = FindXmlFirstChildByName(OutputSource, FloatArray_Keyword);
    u32 OutputCount = GetXmlAttributeValueAsU32(OutputFloatArray, Count_Keyword);
    mat4* KeyframeTransforms = FloatArrayElementToMat4Array(OutputFloatArray, OutputCount);
    Assert(OutputCount / 16 == InputCount);
    
    collada_joint_tree** JointChildren = 0;
    for(u32 Index = 0; Index < SbufLen(Element->Children); Index++)
    {
        xml_element* Child = Element->Children[Index];
        if(Child->Name == Node_Keyword)
        {
            collada_joint_tree* JointChild = LoadColladaJointTree(Child, JointsNames, JointsNamesCount, Root, BindTransform);
            if(JointChild) {
                SbufPush(JointChildren, JointChild);
            }
        }
    }
    
    collada_joint_tree* Result = (collada_joint_tree*)ZeroAlloc(sizeof(collada_joint_tree));
    Result->Name = JointSid;
    Result->Id = JointIndex;
    Result->Transform = JointTransform;
    Result->InverseBindTransform = InverseBindTransform;
    Result->Children = JointChildren;
    Result->KeyframeTransforms = KeyframeTransforms;
    Result->KeyframeTimes = KeyframeTimes;
    Result->KeyframeCount = InputCount;
    
    return Result;
}

internal mesh_joint
ColladaJointTreeToMeshJointTree(collada_joint_tree* Joint, mat4* InverseBindMatrices, u32 InverseBindMatricesCount)
{
    mesh_joint MeshJoint = {};
    MeshJoint.Name = (char*)ZeroAlloc(strlen(Joint->Name) + 1);
    strcpy(MeshJoint.Name, Joint->Name);
    MeshJoint.Id = Joint->Id;
    Assert(Joint->Id < InverseBindMatricesCount);
    //Instead of using the one given by them we use ours, because its better
    MeshJoint.InverseBindMatrix = InverseBindMatrices[Joint->Id];
    //    MeshJoint.InverseBindMatrix = Joint->InverseBindTransform;
    for(u32 Index = 0; Index < SbufLen(Joint->Children); Index++)
    {
        mesh_joint ChildJoint =  ColladaJointTreeToMeshJointTree(Joint->Children[Index], InverseBindMatrices, InverseBindMatricesCount);
        SbufPush(MeshJoint.Children, ChildJoint);
        MeshJoint.ChildrenCount++;
    }
    
    return MeshJoint;
}

internal void
FillAnimationFromColladaJointTree(collada_joint_tree* Joint, mesh_animation* Data, u32 JointsCount)
{
    Assert(Joint->Id < JointsCount);
    joint_animation* JointAnimation = &Data->Joints[Joint->Id];
    Assert(JointAnimation->Keyframes == 0);
    JointAnimation->Keyframes = (animation_keyframe*)ZeroAlloc(sizeof(animation_keyframe) * Joint->KeyframeCount);
    JointAnimation->KeyframesCount = Joint->KeyframeCount;
    for(u32 Index = 0; Index < Joint->KeyframeCount; Index++)
    {
        JointAnimation->Keyframes[Index].Time = Joint->KeyframeTimes[Index];
        vec3 Position;
        quaternion Rotation;
        Mat4ToPositionAndQuaternion(Joint->KeyframeTransforms[Index], &Position, &Rotation);
        JointAnimation->Keyframes[Index].Position = Position;
        JointAnimation->Keyframes[Index].Rotation = Rotation;
        //        JointAnimation->Keyframes[Index].Transform = Joint->KeyframeTransforms[Index];
    }
    
    for(u32 Index = 0; Index < SbufLen(Joint->Children); Index++)
    {
        FillAnimationFromColladaJointTree(Joint->Children[Index], Data, JointsCount);
    }
}

internal void
FreeColladaJointTree(collada_joint_tree* Joint)
{
    for(u32 Index = 0; Index < SbufLen(Joint->Children); Index++)
    {
        FreeColladaJointTree(Joint->Children[Index]);
    }
    
    Free(Joint->KeyframeTimes);
    Free(Joint->KeyframeTransforms);
    Free(Joint);
}


internal void
FreeColladaLoadingData(collada_loading_data* Data)
{
    Free(Data->Positions);
    Free(Data->Normals);
    Free(Data->Tangents);
    Free(Data->UVs);
    Free(Data->Weights);
    Free(Data->Joints);
    
    Data = {};
}

internal collada_scene
ReadColladaFromString(char* String)
{
    lexer Lexer;
    InitLexer(&Lexer, String, LEXER_IGNORE_NEWLINES | LEXER_XML_COMMENTS);
    xml_file XmlFile = ParseXmlFile(&Lexer);
    xml_element* Root = XmlFile.Root;
    
    xml_element* LibraryGeometries =
        FindXmlFirstChildByName(Root, InternString(&Lexer.Intern, "library_geometries"));
    
    Geometry_Keyword = InternString(&Lexer.Intern, "geometry");
    Mesh_Keyword = InternString(&Lexer.Intern, "mesh");
    Polylist_Keyword = InternString(&Lexer.Intern, "polylist");
    Source_Keyword = InternString(&Lexer.Intern, "source");
    Input_Keyword = InternString(&Lexer.Intern, "input");
    Id_Keyword = InternString(&Lexer.Intern, "id");
    Vertices_Keyword = InternString(&Lexer.Intern, "vertices");
    Semantic_Keyword = InternString(&Lexer.Intern, "semantic");
    FloatArray_Keyword = InternString(&Lexer.Intern, "float_array");
    Accessor_Keyword = InternString(&Lexer.Intern, "accessor");
    Count_Keyword = InternString(&Lexer.Intern, "count");
    Triangles_Keyword = InternString(&Lexer.Intern, "triangles");
    Offset_Keyword = InternString(&Lexer.Intern, "offset");
    VCount_Keyword = InternString(&Lexer.Intern, "vcount");
    P_Keyword = InternString(&Lexer.Intern, "p");
    V_Keyword = InternString(&Lexer.Intern, "v");
    Controller_Keyword = InternString(&Lexer.Intern, "controller");
    Skin_Keyword = InternString(&Lexer.Intern, "skin");
    BindShapeMatrix_Keyword = InternString(&Lexer.Intern, "bind_shape_matrix");
    Joints_Keyword = InternString(&Lexer.Intern, "joints");
    Stride_Keyword = InternString(&Lexer.Intern, "stride");
    VertexWeights_Keyword = InternString(&Lexer.Intern, "vertex_weights");
    NameArray_Keyword = InternString(&Lexer.Intern, "Name_array");
    InstanceController_Keyword = InternString(&Lexer.Intern, "instance_controller");
    Url_Keyword = InternString(&Lexer.Intern, "url");
    Skeleton_Keyword = InternString(&Lexer.Intern, "skeleton");
    LibraryVisualScenes_Keyword = InternString(&Lexer.Intern, "library_visual_scenes");
    Node_Keyword = InternString(&Lexer.Intern, "node");
    Name_Keyword = InternString(&Lexer.Intern, "name");
    Sid_Keyword = InternString(&Lexer.Intern, "sid");
    Matrix_Keyword = InternString(&Lexer.Intern, "matrix");
    Animation_Keyword = InternString(&Lexer.Intern, "animation");
    Channel_Keyword = InternString(&Lexer.Intern, "channel");
    LibraryAnimations_Keyword = InternString(&Lexer.Intern, "library_animations");
    Target_Keyword = InternString(&Lexer.Intern, "target");
    Type_Keyword = InternString(&Lexer.Intern, "type");
    
    char* Semantics[] = {
        "POSITION",
        "NORMAL",
        "TEXCOORD",
        "TANGENT",  //Do these ever come?
        "COLOR",    //We currently don't store this as we don't use it in our pipeline
    };
    
    collada_scene Result = {};
    _sbuf_ xml_element** Geometries = FindAllDirectXmlChildrenByName(LibraryGeometries, Geometry_Keyword);
    for(u32 GeometryIndex = 0; GeometryIndex < SbufLen(Geometries); GeometryIndex++)
    {
        collada_loading_data Data = {};
        mesh_joint* MeshRootJoint = 0;
        mesh_animation* MeshAnimations = 0;
        u32 MeshAnimationsCount = 0;
        
        //Gather vertex data arrays
        xml_element* GeometryElement = Geometries[GeometryIndex];
        xml_element* MeshElement = FindXmlFirstChildByName(GeometryElement, Mesh_Keyword);
        if(!MeshElement) continue;
        
        //If we have a mesh we first load the animation data, because we will need it
        //to assemble primitives.
        char* MeshId = GetXmlAttributeValue(GeometryElement, Id_Keyword);
        
        char* SkinSourceId = GetUrlNameFromString(MeshId);
        xml_element* SkinElement =
            FindXmlFirstChildByNameAndAttribute(Root, Skin_Keyword, Source_Keyword, SkinSourceId);
        Free(SkinSourceId);
        
        u32 JointsCount = 0;
        u32 VertexWeightsCount = 0;
        if(SkinElement)
        {
            //We have skin data
            mat4 BindShapeMatrix = Mat4Identity(); //We currently don't use this
            xml_element* BindShapeMatrixElement = FindXmlFirstChildByName(SkinElement, BindShapeMatrix_Keyword);
            xml_element* JointsElement = FindXmlFirstChildByName(SkinElement, Joints_Keyword);
            
            //Load InvBind Matrices
            xml_element* InvBindMatrixInput = FindXmlFirstChildByAttribute(JointsElement, Semantic_Keyword, "INV_BIND_MATRIX");
            char* InvBindSourceName = GetXmlAttributeValue(InvBindMatrixInput, Source_Keyword);
            InvBindSourceName++;
            xml_element* InvBindSource = FindXmlFirstChildByAttribute(SkinElement, Id_Keyword, InvBindSourceName);
            xml_element* InvBindFloatArray = FindXmlFirstChildByName(InvBindSource, FloatArray_Keyword);
            u32 InvBindFloatArrayCount = strtoul(GetXmlAttributeValue(InvBindFloatArray, Count_Keyword), 0, 10);
            xml_element* InvBindAccessor = FindXmlFirstChildByName(InvBindSource, Accessor_Keyword);
            u32 InvBindCount = GetXmlAttributeValueAsU32(InvBindAccessor, Count_Keyword);
            u32 InvBindStride = GetXmlAttributeValueAsU32(InvBindAccessor, Stride_Keyword);
            Assert(InvBindStride == 16);
            Assert(InvBindCount * 16 == InvBindFloatArrayCount);
            mat4* InvBindMatrices = FloatArrayElementToMat4Array(InvBindFloatArray, InvBindFloatArrayCount);
            
            //Load vertex weights and joint ids
            xml_element* VertexWeightsElement = FindXmlFirstChildByName(SkinElement, VertexWeights_Keyword);
            VertexWeightsCount = GetXmlAttributeValueAsU32(VertexWeightsElement, Count_Keyword);
            
            xml_element* WeightInput = FindXmlFirstChildByAttribute(VertexWeightsElement, Semantic_Keyword, "WEIGHT");
            u32 WeightOffset = GetXmlAttributeValueAsU32(WeightInput, Offset_Keyword);
            char* WeightSourceName = GetXmlAttributeValue(WeightInput, Source_Keyword);
            WeightSourceName++; //Skip #
            xml_element* WeightSource = FindXmlFirstChildByAttribute(SkinElement, Id_Keyword, WeightSourceName);
            xml_element* WeightFloatArray = FindXmlFirstChildByName(WeightSource, FloatArray_Keyword);
            u32 WeightCount = strtoul(GetXmlAttributeValue(WeightFloatArray, Count_Keyword), 0, 10);
            f32* WeightsArray = FloatArrayElementToFloatArray(WeightFloatArray, WeightCount);
            
            xml_element* JointInput = FindXmlFirstChildByAttribute(VertexWeightsElement, Semantic_Keyword, "JOINT");
            u32 JointOffset = GetXmlAttributeValueAsU32(JointInput, Offset_Keyword);
            char* JointSourceName = GetXmlAttributeValue(JointInput, Source_Keyword);
            JointSourceName++; //Skip #
            xml_element* JointSource =  FindXmlFirstChildByAttribute(SkinElement, Id_Keyword, JointSourceName);
            xml_element* JointNameArray = FindXmlFirstChildByName(JointSource, NameArray_Keyword);
            JointsCount = GetXmlAttributeValueAsU32(JointNameArray, Count_Keyword);
            char** JointsArray = NameArrayElementToStringArray(JointNameArray, JointsCount);
            
            xml_element* VCountElement = FindXmlFirstChildByName(VertexWeightsElement, VCount_Keyword);
            u32* VCountArray = IntArrayElementToU32Array(VCountElement, VertexWeightsCount);
            u32 TotalVCount = 0;
            u32 MaxWeightCount = 0; //We calculate the maximum number of weights affecting a vertex
            for(u32 Index = 0; Index < VertexWeightsCount; Index++)
            {
                MaxWeightCount = MAX(MaxWeightCount, VCountArray[Index]);
                TotalVCount += VCountArray[Index];
            }
            u32 WeightAttributeCount = 2;
            TotalVCount *= WeightAttributeCount;
            
            xml_element* VElement = FindXmlFirstChildByName(VertexWeightsElement, V_Keyword);
            u32* VArray = IntArrayElementToU32Array(VElement, TotalVCount);
            
            //Assemble vertex weights and joint ids
            joint_weight_pair* CurrentPairs = (joint_weight_pair*)ZeroAlloc(MaxWeightCount * sizeof(joint_weight_pair));
            
            vec4* VertexWeights = (vec4*)ZeroAlloc(sizeof(vec4) * VertexWeightsCount);
            ivec4* VertexJoints = (ivec4*)ZeroAlloc(sizeof(ivec4) * VertexWeightsCount);
            
            u32 TotalIndex = 0;
            for(u32 Index = 0; Index < VertexWeightsCount; Index++)
            {
                u32 NumPairs = VCountArray[Index];
                Assert(NumPairs <= MaxWeightCount);
                for(u32 PairIndex = 0; PairIndex < NumPairs; PairIndex++)
                {
                    u32 JointPos = TotalIndex + PairIndex * WeightAttributeCount + JointOffset;
                    u32 WeightPos = TotalIndex + PairIndex * WeightAttributeCount + WeightOffset;
                    
                    Assert(JointPos < TotalVCount);
                    Assert(WeightPos < TotalVCount);
                    
                    u32 JointIndex = VArray[JointPos];
                    u32 WeightIndex = VArray[WeightPos];
                    Assert(JointIndex < JointsCount);
                    Assert(WeightIndex < WeightCount);
                    CurrentPairs[PairIndex].Joint = JointIndex;
                    CurrentPairs[PairIndex].Weight = WeightsArray[WeightIndex];
                }
                vec4 WeightVec;
                ivec4 JointVec;
                GetBestJointWeightPairs(CurrentPairs, NumPairs, &WeightVec, &JointVec);
                
                VertexWeights[Index] = WeightVec;
                VertexJoints[Index] = JointVec;
                
                TotalIndex += NumPairs * WeightAttributeCount;
            }
            Free(CurrentPairs);
            Free(VCountArray);
            Free(VArray);
            Free(WeightsArray);
            
            Data.Weights = VertexWeights;
            Data.Joints = VertexJoints;
            
            //Load joint hierarchy
            xml_element* LibraryVisualScenes = FindXmlFirstChildByName(Root, LibraryVisualScenes_Keyword);
            char* ControllerId = GetXmlAttributeValue(SkinElement->Parent, Id_Keyword);
            char* InstanceControllerUrl = GetUrlNameFromString(ControllerId);
            xml_element* InstanceController = FindXmlFirstChildByNameAndAttribute(LibraryVisualScenes, InstanceController_Keyword, Url_Keyword, InstanceControllerUrl);
            //There can be multiple skeletons bound to the same instance of a controller,
            //we currently only look at the first one
            //If we don't find any we can 
            xml_element* RootJointElement = 0;
            xml_element* SkeletonElement = FindXmlFirstChildByName(InstanceController, Skeleton_Keyword);
            if(SkeletonElement)
            {
                //@Logging
                char* RootJointId = SkeletonElement->Text;
                RootJointId++; //Skip #
                RootJointElement = FindXmlFirstChildByNameAndAttribute(LibraryVisualScenes, Node_Keyword, Id_Keyword, RootJointId);
            } else {
                RootJointElement = FindXmlFirstChildByAttribute(LibraryVisualScenes, Type_Keyword, "JOINT");
            }
            
            if(RootJointElement)
            {
                collada_joint_tree* RootJoint = LoadColladaJointTree(RootJointElement, JointsArray, JointsCount, Root, Mat4Identity());
                u32 JointTreeCount = CountColladaJointTree(RootJoint);
                //Assert that the number of joints specified in the joints element is the same
                //as the one in the xml joint tree
                Assert(JointTreeCount == JointsCount);
                
                //For now we only support a single animation, we would need to process
                //animation clips to provide more than one (if the file provides them)
                MeshRootJoint = (mesh_joint*)ZeroAlloc(sizeof(mesh_joint));
                *MeshRootJoint = ColladaJointTreeToMeshJointTree(RootJoint, InvBindMatrices, InvBindCount);
                
                MeshAnimations = (mesh_animation*)ZeroAlloc(sizeof(mesh_animation));
                MeshAnimations->Joints = (joint_animation*)ZeroAlloc(sizeof(joint_animation) * JointsCount);
                FillAnimationFromColladaJointTree(RootJoint, MeshAnimations, JointsCount);
                MeshAnimations->Duration = MeshAnimations->Joints->Keyframes[MeshAnimations->Joints->KeyframesCount - 1].Time;
                MeshAnimationsCount = 1;
                
                FreeColladaJointTree(RootJoint);
            }
        }
        
        xml_element* PolylistElement = FindXmlFirstChildByName(MeshElement, Polylist_Keyword);
        xml_element* TrianglesElement = FindXmlFirstChildByName(MeshElement, Triangles_Keyword);
        
        xml_element* PrimitiveElement = 0;
        if(PolylistElement)
        {
            PrimitiveElement = PolylistElement;
        }
        else if(TrianglesElement)
        {
            PrimitiveElement = TrianglesElement;
        }
        else
        {
            //Currently only support polylist and triangles as primitive
            Assert(0);
        }
        
        xml_element** Inputs = FindAllDirectXmlChildrenByName(PrimitiveElement, Input_Keyword);
        for(u32 InputIndex = 0; InputIndex < SbufLen(Inputs); InputIndex++)
        {
            xml_element* Input = Inputs[InputIndex];
            char* InputSource = GetXmlAttributeValue(Input, Source_Keyword);
            InputSource++; //Skip # char
            xml_element* Source = FindXmlFirstChildByAttribute(MeshElement, Id_Keyword, InputSource);
            u32 Offset = GetXmlAttributeValueAsU32(Input, Offset_Keyword);
            //Now the fun begins, if this is VERTEX we go look for the <vertices>
            //thing and then use that to find the right source, otherwise we go direct
            //to source, people thinking this must have been drunk af
            if(Source->Name == Vertices_Keyword)
            {
                for(u32 SemanticIndex = 0; SemanticIndex < ArrayCount(Semantics); SemanticIndex++)
                {
                    char* Semantic = Semantics[SemanticIndex];
                    xml_element* VertInput = FindXmlFirstChildByAttribute(Source, Semantic_Keyword, Semantic);
                    if(VertInput)
                    {
                        char* ActualInputSource = GetXmlAttributeValue(VertInput, Source_Keyword);
                        ActualInputSource++; //Skip # char
                        xml_element* ActualSource = FindXmlFirstChildByAttribute(MeshElement, Id_Keyword, ActualInputSource);
                        
                        xml_element* FloatArray = FindXmlFirstChildByName(ActualSource, FloatArray_Keyword);
                        u32 Count = GetXmlAttributeValueAsU32(FloatArray, Count_Keyword);
                        xml_element* Accessor = FindXmlFirstChildByName(ActualSource, Accessor_Keyword);
                        
                        FillColladaLoadingData(&Data, Semantic, FloatArray, Accessor, Count, Offset);
                    }
                }
            }
            else if(Source->Name == Source_Keyword)
            {
                char* Semantic = GetXmlAttributeValue(Input, Semantic_Keyword);
                xml_element* FloatArray = FindXmlFirstChildByName(Source, FloatArray_Keyword);
                xml_element* Accessor = FindXmlFirstChildByName(Source, Accessor_Keyword);
                u32 Count = GetXmlAttributeValueAsU32(FloatArray, Count_Keyword);
                //Can POSITION even happen here without the indirection to vertices?
                FillColladaLoadingData(&Data, Semantic, FloatArray, Accessor, Count, Offset);
            }
        }
        SbufFree(Inputs);
        
        //Now analyze the primitives to gather indices and form the result arrays
        u32 IndicesCount = 0;
        u32* Indices = 0;
        if(PolylistElement)
        {
            //Number of polygons, basically the number of elements in the vcount element text
            u32 PolyCount = GetXmlAttributeValueAsU32(PolylistElement, Count_Keyword);
            xml_element* VCountElement = FindXmlFirstChildByName(PolylistElement, VCount_Keyword);
            u32* VCountArray = IntArrayElementToU32Array(VCountElement, PolyCount);
            //Only support triangles
            for(u32 Index = 0; Index < PolyCount; Index++)
            {
                Assert(VCountArray[Index] == 3);
            }
            
            xml_element* P = FindXmlFirstChildByName(PolylistElement, P_Keyword);
            IndicesCount = PolyCount * 3 * Data.AttributesCount;
            Indices = IntArrayElementToU32Array(P, IndicesCount);
        }
        else if(TrianglesElement)
        {
            u32 TrianglesCount = GetXmlAttributeValueAsU32(TrianglesElement, Count_Keyword);
            xml_element* P = FindXmlFirstChildByName(TrianglesElement, P_Keyword);
            IndicesCount = TrianglesCount * 3 * Data.AttributesCount;
            Indices = IntArrayElementToU32Array(P, IndicesCount);
        }
        else
        {
            Assert(0);
        }
        
        if(Data.Joints && Data.Weights) {
            Assert(VertexWeightsCount == Data.PositionsCount);
        }
        collada_vertex_data VertexData = AssembleVerticesFromColladaLoadingData(&Data, Indices, IndicesCount);
        
        mesh_data Mesh = {};
        Mesh.Positions = VertexData.Positions;
        Mesh.Joints = VertexData.Joints;
        Mesh.Weights = VertexData.Weights;
        Mesh.Normals = VertexData.Normals;
        Mesh.Tangents = VertexData.Tangents;
        Mesh.UVs = VertexData.UVs;
        Mesh.VerticesCount = (u32)SbufLen(VertexData.Positions);
        Mesh.Indices = VertexData.Indices;
        Mesh.IndicesCount = (u32)SbufLen(VertexData.Indices);
        Mesh.RootJoint = MeshRootJoint;
        Mesh.JointsCount = JointsCount;
        Mesh.Animations = MeshAnimations;
        Mesh.AnimationsCount = MeshAnimationsCount;
        Mesh.Flags |= Mesh.Animations ? MESH_HAS_ANIMATION : 0;
        //TODO: we should check if this is needed, for now this is the default because
        //we usually don't have them, and those seem pretty accurate anyways
        ComputeMeshTangents(&Mesh);
        
        SbufPush(Result.Meshes, Mesh);
        FreeColladaLoadingData(&Data);
    }
    SbufFree(Geometries);
    FreeLexer(&Lexer);
    FreeXmlFile(&XmlFile);
    
    Result.MeshesCount = (u32)SbufLen(Result.Meshes);
    return Result;
}

internal collada_scene
ReadColladaFromFile(char* Path)
{
    char* FileString = (char*)ReadFileAsString(Path);
    if(!FileString) {
        return {};
    }
    
    collada_scene Result = ReadColladaFromString(FileString);
    FreeFileMemory(FileString);
    
    return Result;
}
