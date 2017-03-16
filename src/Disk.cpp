#include "Disk.hpp"

Disk::Disk()
{
	radius = 1;
	position = Vector3d(0, 1, 8);
	normal = Vector3d(0, 0, 1);
}

Disk::Disk(FPType radius_, Vector3d position_, Vector3d normal_)
	:radius {radius_}, position{position_}, normal{normal_}
{}

FPType Disk::GetIntersection(Ray ray)
{
	if(Plane::GetIntersectionDisk(ray, normal, position))
	{
		FPType t = Plane::GetIntersectionDisk(ray, normal, position);
		Vector3d intersectionPoint = ray.GetOrigin() + ray.GetDirection() * t;
		Vector3d intersectionToMidDist = intersectionPoint - position;
		FPType d2 = intersectionToMidDist.Dot(intersectionToMidDist);
		FPType radius2 = radius * radius;
		return (d2 <= radius2);
	}
	else
		return false;
}

Vector3d Disk::GetNormalAt(const Vector3d point)
{
	return normal;
}

Vector3d Disk::GetPosition() const
{
	return position;
}
