struct HitRecord
{
    float3 p;
    float3 normal;
    float t;
    bool front_face;

    void set_face_normal(thread const Ray& r, thread const float3& outward_normal)
    {
        front_face = metal::dot(r.direction(), outward_normal) < 0;
        normal = front_face ? outward_normal : -outward_normal;
    }
};