#include "Frustrum.hpp"

Frustrum::Frustrum()
{
}

Frustrum::~Frustrum()
{
}

void Frustrum::updateFrustum(glm::mat4 &projection, glm::mat4 &view)
{
    glm::mat4 mat = projection * view;

    // Plan gauche (left)
    mPlans[0].normal = glm::vec3(mat[0][3] + mat[0][0], mat[1][3] + mat[1][0], mat[2][3] + mat[2][0]);
    mPlans[0].d = mat[3][3] + mat[3][0];

    // Plan droit (right)
    mPlans[1].normal = glm::vec3(mat[0][3] - mat[0][0], mat[1][3] - mat[1][0], mat[2][3] - mat[2][0]);
    mPlans[1].d = mat[3][3] - mat[3][0];

    // Plan bas (bottom)
    mPlans[2].normal = glm::vec3(mat[0][3] + mat[0][1], mat[1][3] + mat[1][1], mat[2][3] + mat[2][1]);
    mPlans[2].d = mat[3][3] + mat[3][1];

    // Plan haut (top)
    mPlans[3].normal = glm::vec3(mat[0][3] - mat[0][1], mat[1][3] - mat[1][1], mat[2][3] - mat[2][1]);
    mPlans[3].d = mat[3][3] - mat[3][1];

    // Plan proche (near)
    mPlans[4].normal = glm::vec3(mat[0][3] + mat[0][2], mat[1][3] + mat[1][2], mat[2][3] + mat[2][2]);
    mPlans[4].d = mat[3][3] + mat[3][2];

    // Plan lointain (far)
    mPlans[5].normal = glm::vec3(mat[0][3] - mat[0][2], mat[1][3] - mat[1][2], mat[2][3] - mat[2][2]);
    mPlans[5].d = mat[3][3] - mat[3][2];
}

bool Frustrum::isPatchInFrustum(const glm::vec3 &patchCentre, float radius)
{
    Sphere patch(patchCentre, radius);

    for (int i = 0; i < 6; i++)
    {
        float distance = glm::dot(mPlans[i].normal, patch.center) + mPlans[i].d;

        if (distance < -patch.radius)
            return false;
    }

    return true;
}