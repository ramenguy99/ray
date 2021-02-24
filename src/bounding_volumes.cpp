#include "bounding_volumes.h"

//Return the index of the minimum of 3 elements
inline u32
IndexOfMinOf3(f32 a, f32 b, f32 c)
{
    if(a < b)
    {
        if(a < c)
        {
            return 0;
        }
        else
        {
            return 2;
        }
    }
    else
    {
        if(b < c)
        {
            return 1;
        }
        else
        {
            return 2;
        }
    }
}

//Divides indices in two partitions and returns index of beginning of second partition
internal u32
PartitionIndexedTriangles(vec3* Positions, u32* Indices, u32 IndicesCount)
{
    //Compute mean of all the vertices
    vec3 Mean = vec3(0.0f);
    float Contribution = 1.0f / IndicesCount;
    For(i, IndicesCount)
    {
        vec3 P = Positions[Indices[i]];
        Mean = Mean + P * Contribution;
    }
    
    //x = 0, y = 1, z = 2
    vec3 Axis[] = {
        vec3(1.0f, 0.0f, 0.0f),
        vec3(0.0f, 1.0f, 0.0f),
        vec3(0.0f, 0.0f, 1.0f),
    };
    
    aabb InitAABB;
    InitAABB.Max = vec3(-FLT_MAX);
    InitAABB.Min = vec3(FLT_MAX);
    
    //AABBs of partitions given the 3 axis vectors and the mean of all points
    aabb Above[3] = {
        InitAABB,
        InitAABB,
        InitAABB,
    };
    aabb Below[3] = {
        InitAABB,
        InitAABB,
        InitAABB,
    };
    
    //Number of triangles below the mean for each axis
    u32 BelowTriangleCounts[3] = {};
    
    //Compute aabbs splitting the parent at the mean for each axis
    for(u32 i = 0; i < IndicesCount; i += 3)
    {
        vec3 P[3];
        P[0] = Positions[Indices[i + 0]];
        P[1] = Positions[Indices[i + 1]];
        P[2] = Positions[Indices[i + 2]];
        
        vec3 C = (P[0] + P[1] + P[2]) * (1.0f / 3.0f);
        
        For(AxisIndex, 3)
        {
            vec3 n = Axis[AxisIndex];
            float d = Mean.e[AxisIndex];
            b32 IsAbove = Dot(n, C) >= d ? true : false;
            
            u32 Min = IndexOfMinOf3(P[0].e[AxisIndex], P[1].e[AxisIndex], P[2].e[AxisIndex]);
            if(IsAbove)
            {
                UpdateAABB(&Above[AxisIndex], P[Min]);
            }
            else
            {
                //Count triangles below the mean
                BelowTriangleCounts[AxisIndex]++;
                UpdateAABB(&Below[AxisIndex], P[Min]);
            }
        }
    }
    
    //Find the best splitting by comparing aabb area of each split axis
    u32 BestAxis = 0;
    f32 BestArea = FLT_MAX;
    For(i, 3)
    {
        f32 TotalArea = AABBArea(Above[i]) + AABBArea(Below[i]);
        if(TotalArea < BestArea)
        {
            BestArea = TotalArea;
            BestAxis = i;
        }
    }
    
    u32 BeginningOfAbove = BelowTriangleCounts[BestAxis] * 3; //3 indices per triangle
    
    u32 BelowIt = 0;
    u32 AboveIt = BeginningOfAbove;
    
    //Partition vertices moving the vertices above the mean at the end of the array
    while(BelowIt != BeginningOfAbove)
    {
        vec3 P[3];
        P[0] = Positions[Indices[BelowIt + 0]];
        P[1] = Positions[Indices[BelowIt + 1]];
        P[2] = Positions[Indices[BelowIt + 2]];
        
        vec3 C = (P[0] + P[1] + P[2]) * (1.0f / 3.0f);
        
        vec3 n = Axis[BestAxis];
        float d = Mean.e[BestAxis];
        b32 IsAbove = Dot(n, C) >= d ? true : false;
        
        if(IsAbove)
        {
            //Swap 3 indices from AboveIt to BelowIt
            For(SwapIndex, 3)
            {
                u32 Temp = Indices[AboveIt + SwapIndex];
                Indices[AboveIt + SwapIndex] = Indices[BelowIt + SwapIndex];
                Indices[BelowIt + SwapIndex] = Temp;
            }
            
            AboveIt += 3;
        }
        else
        {
            BelowIt += 3;
        }
    }
    
    return BeginningOfAbove;
}

internal aabb_tree*
ComputeAABBTree(vec3* Positions, u32* Indices, u32 IndicesCount)
{
    aabb_tree* Tree = (aabb_tree*)ZeroAlloc(sizeof(aabb_tree));
    
    
    Tree->AABB = ComputeAABBIndexed(Positions, Indices, IndicesCount);
    Assert(IndicesCount % 3 == 0);
    
    //Stop if we have too few triangles left
    if(IndicesCount / 3 < MIN_TRIANGLES_PER_LEAF)
    {
        Tree->Indices = Indices;
        Tree->IndicesCount = IndicesCount;
    }
    else
    {
        u32 k = PartitionIndexedTriangles(Positions, Indices, IndicesCount);
        u32 LeftCount = k;
        u32 RightCount = IndicesCount - k;
        
        u32 IndicesDifference = MIN_TRIANGLE_DIFFERENCE * 3;
        
        // Stop if we have almost the same amount of primitives in one of the child as the parent
        if(LeftCount < IndicesDifference ||
           RightCount < IndicesDifference)
        {
            Tree->Indices = Indices;
            Tree->IndicesCount = IndicesCount;
        }
        else
        {
            // Recursively construct left and right subtree from subarrays and
            // point the left and right fields of the current node at the subtrees
            Tree->Left = ComputeAABBTree(Positions, &Indices[0], LeftCount);
            Tree->Right = ComputeAABBTree(Positions, &Indices[k], RightCount);
        }
    }
    
    return Tree;
}

internal void
GetAABBTreeInfoRec(bounding_tree_info* Info, aabb_tree* Tree, u32 CurrentDepth)
{
    if(!Tree) return;
    
    //Nodes info
    Info->Count++;
    Info->TotalDepthOfNodes += CurrentDepth;
    
    if(!Tree->Right && !Tree->Left)
    {
        //Leves info
        Info->LongestPathToLeaf = MAX(Info->LongestPathToLeaf, 1 + CurrentDepth);
        Info->ShortestPathToLeaf = MIN(Info->ShortestPathToLeaf, 1 + CurrentDepth);
        Info->LeavesCount++;
        Info->TotalAreaOfLeaves += AABBArea(Tree->AABB);
        Info->TotalVolumeOfLeaves += AABBVolume(Tree->AABB);
        Info->TotalDepthOfPathsToLeaves += CurrentDepth;
        Info->TotalPrimitivesPerLeaf += Tree->IndicesCount / 3;
    }
    
    //Traverse the tree recursively
    GetAABBTreeInfoRec(Info, Tree->Left, CurrentDepth + 1);
    GetAABBTreeInfoRec(Info, Tree->Right, CurrentDepth + 1);
}

internal bounding_tree_info
GetAABBTreeInfo(aabb_tree* Tree)
{
    bounding_tree_info Result = {};
    Result.ShortestPathToLeaf = UINT_MAX;
    GetAABBTreeInfoRec(&Result, Tree, 0);
    return Result;
}

internal void
PrintAABBInfo(aabb_tree* Tree)
{
    bounding_tree_info Info = GetAABBTreeInfo(Tree);
    u32 FullNodes = (1 << Info.LongestPathToLeaf) - 1;
    printf(" %u nodes of which %u are leaves (%ukb)\n", Info.Count, Info.LeavesCount, (u32)((sizeof(aabb_tree) * Info.Count) / 1024));
    printf(" Total triangles per leaf: %u (%.2f average)\n", Info.TotalPrimitivesPerLeaf, (f32)Info.TotalPrimitivesPerLeaf / Info.LeavesCount);
    printf(" Shortest path to leaf: %u\n", Info.ShortestPathToLeaf);
    printf(" Longest path to leaf: %u\n", Info.LongestPathToLeaf);
    printf(" Average depth of path to leaf: %.2f\n", (f32)Info.TotalDepthOfPathsToLeaves / Info.LeavesCount);
    printf(" Average depth of node: %.2f\n", (f32)Info.TotalDepthOfNodes / Info.Count);
    printf(" Saturation : %.2f\n", (f32)Info.Count / (f32)FullNodes);
    printf(" Total area of leaves: %.2f (%.2f average)\n", Info.TotalAreaOfLeaves, Info.TotalAreaOfLeaves / Info.LeavesCount);
    printf(" Total volume of leaves: %.2f (%.2f%% of total)\n", Info.TotalVolumeOfLeaves, Info.TotalVolumeOfLeaves / AABBVolume(Tree->AABB) * 100.0f);
    printf("\n");
}
