#pragma once
#include "Globals.h"
#include "Vector.h"
#include "Ray.h"
#include "Plane.h"

class Disk : public Plane
{
public:
	Disk();
	Disk(FPType radius_, Vector position_, Vector normal_);
	FPType GetIntersection(Ray ray);

	Vector GetNormalAt(Vector point);
	Vector GetPosition() const;

private:
	Vector position;
	Vector normal;
	FPType radius;
};