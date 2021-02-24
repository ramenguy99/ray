#include "mesh.h"

internal void
ComputeTangents(vec3* Positions, vec3* Normals, vec2* UVs, u32 VerticesCount,
                u32* Indices, u32 IndicesCount, vec3* Tangents, b32 IsStrip = false)
{
    if(IndicesCount == 0 || VerticesCount == 0) return;
    
    Assert(IsStrip || IndicesCount % 3 == 0);
    
    vec3* Tan1 = (vec3*)ZeroAlloc(sizeof(vec3) * VerticesCount * 2);
    vec3* Tan2 = Tan1 + VerticesCount;
    
    for(u32 Index = 0;;)
    {
        u32 i0;
        u32 i1;
        u32 i2;
        if(!IsStrip)
        {
            i0 = Indices[Index + 0];
            i1 = Indices[Index + 1];
            i2 = Indices[Index + 2];
        }
        else
        {
            //If odd
            if(Index & 1)
            {
                i0 = Indices[Index + 0];
                i1 = Indices[Index + 1];
                i2 = Indices[Index + 2];
            }
            else
            {
                i0 = Indices[Index + 1];
                i1 = Indices[Index + 0];
                i2 = Indices[Index + 2];
            }
        }
        
        vec3 Pos0 = Positions[i0];
        vec3 Pos1 = Positions[i1];
        vec3 Pos2 = Positions[i2];
        
        vec2 UV0 = UVs[i0];
        vec2 UV1 = UVs[i1];
        vec2 UV2 = UVs[i2];
        
        f32 x1 = Pos1.x - Pos0.x;
        f32 x2 = Pos2.x - Pos0.x;
        f32 y1 = Pos1.y - Pos0.y;
        f32 y2 = Pos2.y - Pos0.y;
        f32 z1 = Pos1.z - Pos0.z;
        f32 z2 = Pos2.z - Pos0.z;
        
        f32 s1 = UV1.x - UV0.x;
        f32 s2 = UV2.x - UV0.x;
        f32 t1 = UV1.y - UV0.y;
        f32 t2 = UV2.y - UV0.y;
        
        float epsilon = 0.0000001f;
        float den = s1 * t2 - s2 * t1;
        
        //@Robustness better way to handle this?
        if(den < epsilon && den > -epsilon) {
            den = epsilon;
        }
        
        float r = 1.0f / den;
        vec3 sdir = vec3((t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r, (t2 * z1 - t1 * z2) * r);
        vec3 tdir = vec3((s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r, (s1 * z2 - s2 * z1) * r);
        
        Tan1[i0] = Tan1[i0] + sdir;
        Tan1[i1] = Tan1[i1] + sdir;
        Tan1[i2] = Tan1[i2] + sdir;
        
        Tan2[i0] = Tan2[i0] + tdir;
        Tan2[i1] = Tan2[i1] + tdir;
        Tan2[i2] = Tan2[i2] + tdir;
        
        if(Index + 3 == IndicesCount) break;
        
        Index += IsStrip ? 1 : 3;
    }
    
    for(u32 Index = 0; Index < VerticesCount; Index++)
    {
        vec3 n = Normals[Index];
        vec3 t = Tan1[Index];
        
        // Gram-Schmidt orthogonalize
        t = t - n * Dot(n, t);
        if(t == vec3(0.0f))
        {
            t = vec3(1.0f, 0.0f, 0.0f);
        }
        else
        {
            t = Normalize(t);
        }
        
        Tangents[Index] = t;
        if(Dot(Cross(n, t), Tan2[Index]) < 0.0f)
        {
            Tangents[Index] = Negate(Tangents[Index]);
        }
        
        Assert(!isnan(Tangents[Index].x) &&
               !isnan(Tangents[Index].y) &&
               !isnan(Tangents[Index].z) );
    }
    
    Free(Tan1);
}

internal void
ComputeMeshTangents(mesh_data* Mesh)
{
    if(!Mesh->Tangents) {
        Mesh->Tangents = (vec3*)ZeroAlloc(sizeof(vec3) * Mesh->VerticesCount);
    }
    
    ComputeTangents(Mesh->Positions, Mesh->Normals, Mesh->UVs, Mesh->VerticesCount,
                    Mesh->Indices, Mesh->IndicesCount, Mesh->Tangents);
}

internal void
TransformMeshVertices(mesh_data* Mesh, mat4& Transform)
{
    if(Mesh->Positions)
    {
        for(u32 Index = 0; Index < Mesh->VerticesCount; Index++)
        {
            Mesh->Positions[Index] = vec3(Transform * vec4(Mesh->Positions[Index], 1.0f));
        }
    }
    
    mat4 NormalMatrix = Mat4NormalMatrix(Transform);
    if(Mesh->Normals)
    {
        for(u32 Index = 0; Index < Mesh->VerticesCount; Index++)
        {
            Mesh->Normals[Index] = Normalize(vec3(NormalMatrix * vec4(Mesh->Normals[Index], 1.0f)));
        }
        
    }
    if(Mesh->Tangents)
    {
        for(u32 Index = 0; Index < Mesh->VerticesCount; Index++)
        {
            Mesh->Tangents[Index] = Normalize(vec3(NormalMatrix * vec4(Mesh->Tangents[Index], 1.0f)));
        }
    }
}

internal void
UpdateJointStateRecursively(mesh_joint* Joint, mesh_animation* Animation, mat4 ParentTransform,
                            mat4* Joints, u32 Count, float Time)
{
    joint_animation* JointAnimation = &Animation->Joints[Joint->Id];
    if(JointAnimation->KeyframesCount == 0) return;
    
    Assert(Time >= 0.0f && Time <= JointAnimation->Keyframes[JointAnimation->KeyframesCount - 1].Time);
    u32 BeginIndex = 0;
    u32 EndIndex = 0;
    for(u32 Index = 1; Index < JointAnimation->KeyframesCount; Index++)
    {
        if(JointAnimation->Keyframes[Index].Time > Time)
        {
            EndIndex = Index;
            BeginIndex = Index - 1;
            break;
        }
    }
    
    animation_keyframe BeginKeyframe = JointAnimation->Keyframes[BeginIndex];
    animation_keyframe EndKeyframe = JointAnimation->Keyframes[EndIndex];
    
    float Progress = (Time - BeginKeyframe.Time) / (EndKeyframe.Time - BeginKeyframe.Time);
    vec3 Position = Lerp(BeginKeyframe.Position, EndKeyframe.Position, Progress);
    quaternion Rotation = Interpolate(BeginKeyframe.Rotation, EndKeyframe.Rotation, Progress);
    
    mat4 LocalTransform = Mat4Translation(Position) * QuaternionToMat4(Rotation);
    mat4 JointTransform = ParentTransform * LocalTransform;
    
    for(u32 Index = 0; Index < Joint->ChildrenCount; Index++)
    {
        UpdateJointStateRecursively(&Joint->Children[Index], Animation, JointTransform,
                                    Joints, Count, Time);
    }
    
    mat4 OutputTransform = JointTransform * Joint->InverseBindMatrix;
    Assert(Joint->Id <= Count);
    Joints[Joint->Id] = OutputTransform;
}

internal void
GetJointsFromAnimator(mesh_animator* Animator, mat4* Joints, u32 Count, u32 Pose)
{
    Assert(Count >= Animator->JointsCount);
    UpdateJointStateRecursively(Animator->RootJoint, &Animator->Animations[0], Mat4Identity(),
                                Joints, Count, Animator->Time);
}

internal void
UpdateAnimator(mesh_animator* Animator, float Delta)
{
    Animator->Time += Delta;
    mesh_animation* Animation = Animator->Animations;
    
    if(Animator->Time > Animation->Duration)
    {
        Animator->Time -= Animation->Duration;
    }
}
