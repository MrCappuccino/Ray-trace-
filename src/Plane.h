#pragma once

#include "Vector.h"
#include "Object.h"
#include "Ray.h"

class Plane : public Object
{
public:
    Vector normal, center;

    Plane();
    Plane(Vector center_, Vector normal_);
    
    virtual Vector GetNormalAt(Vector point);
    virtual FPType GetIntersection(Ray ray);
};
