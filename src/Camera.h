#pragma once
#include "Vector.h"
#include "Ray.h"

class Camera
{
public:
	Camera();
	Camera(const Vector &origin, const Vector &direction);
	Vector GetOrigin();
	Vector GetSceneDirection();
	Vector GetCameraDirection();
	Vector GetCamX();
	Vector GetCamY();
	void SetSceneDirection(const Vector &dir);
	
private:
	// CAMERA COORDINATE SYSTEM
	Vector origin; // replace with from
	Vector sceneDirection; // Where the camera looks at in the scene, replace with to
	// replace V with Vector forward = (from - to).Normalize(); 
	Vector camDirection = (GetSceneDirection() - GetOrigin()).Normalize(); // Where camera's view is centered
	
	// Vector tmp(0, 1, 0);
	// replace camX with: Vector right = (tmp.Normalize()).Cross(forward);
	// replace camY with: Vector up = forward.Cross(right);
	// camX and Y represent the 2D coordinate system on the IMAGE plane
	Vector camX = Vector(0, 1, 0).Cross(GetCameraDirection()).Normalize();
	Vector camY = camX.Cross(GetCameraDirection());
};

Camera::Camera()
	:origin {0, 0, 0}, sceneDirection {0, 0, 1}
{}

inline Camera::Camera(const Vector &origin, const Vector &sceneDirection)
	: origin(origin), sceneDirection(sceneDirection)
{}

inline Vector Camera::GetOrigin()
{
	return origin;
}

inline Vector Camera::GetSceneDirection()
{
	return sceneDirection;
}

inline Vector Camera::GetCameraDirection()
{
	return camDirection;
}

inline Vector Camera::GetCamX()
{
	return camX;
}

inline Vector Camera::GetCamY()
{
	return camY;
}

void Camera::SetSceneDirection(const Vector &dir)
{
	sceneDirection = dir;
}
