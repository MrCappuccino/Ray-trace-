#pragma once
#include "Vector3.h"
#include "Ray.h"
#include "Color.h"
#include "Material.h"
#include "BBox.h"

class Object
{
public:
	Object() noexcept;
	virtual ~Object();

	virtual FPType GetIntersection(const Ray &ray);
	virtual Vector3d GetNormalAt(const Vector3d &intersectionPosition);
	virtual Vector3d GetTexCoords(Vector3d &normal, const Vector3d &hitPoint);

	Material material;
	BBox bbox;
};