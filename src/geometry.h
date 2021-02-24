struct plane
{
    vec3 Normal;
    float d; //Distance from the origin along the normal
};

struct sphere
{
    vec3 Center;
    float Radius;
};

struct aabb
{
    vec3 Min;
    vec3 Max;
};
