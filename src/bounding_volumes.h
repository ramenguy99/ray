//Binary AABB tree
struct aabb_tree
{
    aabb AABB;
    
    //Section of the indices included in the bounding volume
    u32* Indices;
    u32 IndicesCount;
    
    aabb_tree* Left;
    aabb_tree* Right;
};

//Tree info, used for printing stats about the tree
struct bounding_tree_info
{
    u32 Count;
    u32 LeavesCount;
    u32 TotalDepthOfNodes;
    u32 TotalDepthOfPathsToLeaves;
    f32 TotalAreaOfLeaves;
    f32 TotalVolumeOfLeaves;
    u32 TotalPrimitivesPerLeaf;
    u32 LongestPathToLeaf;
    u32 ShortestPathToLeaf;
};
