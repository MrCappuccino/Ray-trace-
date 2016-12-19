#pragma once
#include "Vector.h"
#include "Ray.h"
#include "Color.h"
#include "Material.h"
#include <iostream>

class Object: public Material
{
public:
	Object() noexcept;
	virtual ~Object();

	virtual FPType GetIntersection(Ray ray);

	virtual Vector GetNormalAt(Vector intersectionPosition);

	virtual Color GetColor();

	Material GetMaterial();

	void SetMaterial(const Material &material_);

private:
	Material material;
};