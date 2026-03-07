#include "Frustrum.hpp"

Frustrum::Frustrum()
{
}

Frustrum::~Frustrum()
{
}

void Frustrum::updateFrustum(glm::mat4& projection, glm::mat4& view){

    glm::mat4 mat = projection * view;

    plans[0].normal = glm::vec3(mat[0][3] + mat[0][0],
                                  mat[1][3] + mat[1][0],
                                  mat[2][3] + mat[2][0]);
    plans[0].d = mat[3][3] + mat[3][0];
    
    // Plan droit (right)
    plans[1].normal = glm::vec3(mat[0][3] - mat[0][0],
                                  mat[1][3] - mat[1][0],
                                  mat[2][3] - mat[2][0]);
    plans[1].d = mat[3][3] - mat[3][0];
    
    // Plan bas (bottom)
    plans[2].normal = glm::vec3(mat[0][3] + mat[0][1],
                                  mat[1][3] + mat[1][1],
                                  mat[2][3] + mat[2][1]);
    plans[2].d = mat[3][3] + mat[3][1];
    
    // Plan haut (top)
    plans[3].normal = glm::vec3(mat[0][3] - mat[0][1],
                                  mat[1][3] - mat[1][1],
                                  mat[2][3] - mat[2][1]);
    plans[3].d = mat[3][3] - mat[3][1];
    
    // Plan proche (near)
    plans[4].normal = glm::vec3(mat[0][3] + mat[0][2],
                                  mat[1][3] + mat[1][2],
                                  mat[2][3] + mat[2][2]);
    plans[4].d = mat[3][3] + mat[3][2];
    
    // Plan lointain (far)
    plans[5].normal = glm::vec3(mat[0][3] - mat[0][2],
                                  mat[1][3] - mat[1][2],
                                  mat[2][3] - mat[2][2]);
    plans[5].d = mat[3][3] - mat[3][2];

    for (int i = 0; i < 6; i++) {
        float length = glm::length(plans[i].normal);
        if (length > 0.0001f) {
            plans[i].normal /= length;
            plans[i].d /= length;
        }
    }

}

#include <iostream>

bool Frustrum::patchDansFrustum(const glm::vec3& patchCentre,float radius) {
    Sphere patch(patchCentre,radius);

    for(int i = 0; i < 6; i++)
        {
            float distance = glm::dot(plans[i].normal, patch.center) + plans[i].d;

            if(distance < -patch.radius)
                return false;
    }

    return true;
}