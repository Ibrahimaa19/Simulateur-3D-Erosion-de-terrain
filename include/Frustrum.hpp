#ifndef FRUSTRUM_H
#define FRUSTRUM_H

#include <glm/glm.hpp>

struct Plan {
    glm::vec3 normal;
    float d;
};

struct Sphere
{
    glm::vec3 center;
    float radius;

    Sphere(glm::vec3 c,float r) : center(c), radius(r) {}
};

class Frustrum
{
private:
    Plan plans[6];
public:
    Frustrum();
    ~Frustrum();
    void updateFrustum(glm::mat4& projection, glm::mat4& view);
    bool patchDansFrustum(const glm::vec3& patchCentre,float radius);
};


#endif