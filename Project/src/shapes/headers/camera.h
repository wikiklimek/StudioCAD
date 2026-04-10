#pragma once
#include <MG1Math/Vect3.h>
#include "quaternion.h"
#include "matrixesModelViewProjection.h"
#include "enums.h"

class Camera {
public:
    Vect3 position;
    Vect3 target;
    Vect3 up;
    float fov;

    float nearPlane = 0.1f;
    float farPlane = 5000.0f;

    // Gimbal Lock protection
    float minAngleZ = 0.05f;
    float maxAngleZ = (float)M_PI - 0.05f;

    Quaternion tempOrbit;
    float tempZoom;
    Vect3 tempPan = Vect3(0.0f);

    bool hasTemporaryChanges = false;

    Camera(Vect3 pos, Vect3 tgt, Vect3 upVec, float fieldOfView)
            : position(pos), target(tgt), up(upVec), fov(fieldOfView), tempZoom(1.0f), tempOrbit(1.0f, 0.0f, 0.0f, 0.0f) {}


    Mat4 getProjectionMatrix(float aspectRatio) const {
        return createProjectionMatrix(fov, aspectRatio, nearPlane, farPlane);
    }


    void processMouseDrag(float dx_world, float dy_world, CameraDragMode mode)
    {
        hasTemporaryChanges = true;

        if (mode == CAM_ORBIT)
        {
            Quaternion qZ = Quaternion::fromAxisAngle(0.0f, 0.0f, 1.0f, -dx_world * 2.0f);
            Vect3 dir = getDirectionNotBaked().normalize();
            Vect3 right = Vect3::cross(up, dir).normalize();

            float currentAngleZ = std::acos(dir.z);
            float deltaAngleZ = -dy_world * 2.0f;

            if (currentAngleZ + deltaAngleZ < minAngleZ) deltaAngleZ = minAngleZ - currentAngleZ;
            else if (currentAngleZ + deltaAngleZ > maxAngleZ) deltaAngleZ = maxAngleZ - currentAngleZ;

            Quaternion qRight = Quaternion::fromAxisAngle(right.x, right.y, right.z, deltaAngleZ);
            tempOrbit = qRight * qZ;
            tempOrbit.normalize();
        }
        else if (mode == CAM_ZOOM)
        {
            Vect3 dir = getDirectionNotBaked();
            float dist = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
            float minZoom = 0.1f / dist;
            tempZoom = std::max(minZoom, 1.0f - dy_world * 2.0f);
        }
        else if (mode == CAM_PAN)
        {
            Vect3 forward(0.0f), right(0.0f), localUp(0.0f);
            getCameraVectors(forward, right, localUp);

            float distance = std::sqrt(forward.x * forward.x + forward.y * forward.y + forward.z * forward.z);
            Vect3 panRight = Vect3(right.x * -dx_world * distance, right.y * -dx_world * distance, right.z * -dx_world * distance);
            Vect3 panUp = Vect3(localUp.x * dy_world * distance, localUp.y * dy_world * distance, localUp.z * dy_world * distance);

            tempPan = panRight + panUp;
        }
    }

    void getCameraVectors(Vect3& outForward, Vect3& outRight, Vect3& outUp) const
    {
        Vect3 activePos(0.0), activeTarget(0.0);
        getActiveState(activePos, activeTarget);

        outForward = (activePos - activeTarget).normalize();
        outRight = Vect3::cross(up, outForward).normalize();
        outUp = Vect3::cross(outForward, outRight).normalize();
    }


    Vect3 getDirectionNotBaked() const {
        return position - target;
    }

    void getActiveState(Vect3& outPos, Vect3& outTarget) const
    {
        if (!hasTemporaryChanges)
        {
            outPos = position;
            outTarget = target;
            return;
        }

        Vect3 dir = getDirectionNotBaked();

        Mat4 rotMat = tempOrbit.toMat4();
        Vect4 dir4(dir.x, dir.y, dir.z, 1.0f);
        Vect4 rotatedDir4 = rotMat * dir4;
        Vect3 rotatedDir(rotatedDir4.x, rotatedDir4.y, rotatedDir4.z);

        rotatedDir = Vect3(rotatedDir.x * tempZoom, rotatedDir.y * tempZoom, rotatedDir.z * tempZoom);

        outTarget = target + tempPan;
        outPos = outTarget + rotatedDir;
    }

    Mat4 getViewMatrix() const
    {
        Vect3 activePos(0.0), activeTarget(0.0);
        getActiveState(activePos, activeTarget);
        return createViewMatrix(activePos, activeTarget, Vect3(0.0f, 0.0f, 1.0f));
    }

    void bake() {

        if (!hasTemporaryChanges) return;

        Vect3 activePos(0.0), activeTarget(0.0);
        getActiveState(activePos, activeTarget);

        position = activePos;
        target = activeTarget;

        tempOrbit = Quaternion(1.0f, 0.0f, 0.0f, 0.0f);
        tempZoom = 1.0f;
        tempPan = Vect3(0.0f, 0.0f, 0.0f);
        hasTemporaryChanges = false; // Zerujemy flagę
    }



    void enforceConstraints()
    {
        Vect3 dir = position - target;
        float dist = std::sqrt(dir.x*dir.x + dir.y*dir.y + dir.z*dir.z);


        if (dist < 0.1f)
        {
            if (dist < 0.0001f)
                dir = Vect3(0.0f, -1.0f, 0.0f);
            else
            {
                dir.x /= dist;
                dir.y /= dist;
                dir.z /= dist;
            }

            position = target + Vect3(dir.x * 0.1f, dir.y * 0.1f, dir.z * 0.1f);
            dir = position - target;
            dist = 0.1f;
        }


        dir.x /= dist;
        dir.y /= dist;
        dir.z /= dist;

        float dot = dir.z; // Zakładamy up = (0,0,1)
        if (std::abs(dot) > 0.999f)
        {
            position.x += 0.05f;
            position.y += 0.05f;
        }
    }
};