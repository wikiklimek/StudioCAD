#pragma once
#include <MG1Math/Vect3.h>
#include "quaternion.h"
#include "matrixesModelViewProjection.h"

class Camera {
public:
    Vect3 position;
    Vect3 target;
    Vect3 up;

    // Tymczasowe transformacje
    Quaternion tempOrbit;
    float tempZoom;
    Vect3 tempPan = Vect3(0.0f);

    bool hasTemporaryChanges = false;

    Camera(Vect3 pos, Vect3 tgt, Vect3 upVec)
            : position(pos), target(tgt), up(upVec), tempZoom(1.0f), tempOrbit(1.0f, 0.0f, 0.0f, 0.0f) {}

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
};