#define _SCL_SECURE_NO_WARNINGS

#include "bitmap_image.hpp"
#include "Globals.h"
#include "Sphere.h"
#include "Plane.h"
#include "Light.h"
#include "Material.h"
#include "Scene.h"
#include "Camera.h"
#include <vector>
#include <iostream>
#include <time.h>
#include <sstream>

int ClosestObjectIndex(const std::vector<FPType> &intersections)
{
	int minValueIndex = 0;

	// Prevent unnecessary calculations (only check the ones that intersect)
	if(intersections.size() == 0) // No intersections
		return -1;

	else if(intersections.size() == 1)
	{
		if(intersections[0] > 0) // If intersection is greater than 0, then it's our index of minimum value (0th element in vector)
			return 0;
		else // Otherwise the only intersection value is negative (ray missed everything)
			return -1;
	}
	else // If there's more than 1 intersection, find the MAX value
	{
		FPType max = 0;
		for(FPType i : intersections)
		{
			if(max < i)
				max = i;
		}
		if(max > 0)
		{
			// Only searh for positive intersections
			// Find the minimum POSITIVE value
			for(unsigned intersectionIndex = 0; intersectionIndex < intersections.size(); intersectionIndex++)
			{
				// If intersection is positive and is lower or equal to the max intersection
				if(intersections[intersectionIndex] > 0 && intersections[intersectionIndex] <= max)
				{
					max = intersections[intersectionIndex];
					minValueIndex = intersectionIndex;
				}
			}
			return minValueIndex;
		}
		else
		{
			// All intersections were negative (didn't hit shit)
			return -1;
		}
	}
}

Color GetColorAt(Vector intersectionRayPos, Vector intersectingRayDir, const std::vector<std::shared_ptr<Object>> &sceneObjects, int indexOfClosestObject, const std::vector<std::shared_ptr<Light>> &lightSources)
{
	Material closestObjectMaterial = sceneObjects[indexOfClosestObject]->GetMaterial();
	Vector closestObjectNormal = sceneObjects[indexOfClosestObject]->GetNormalAt(intersectionRayPos);

	if(closestObjectMaterial.GetSpecial() == 2) // Checkerboard pattern floor
	{
		int square = int(floor(intersectionRayPos.x)) + int(floor(intersectionRayPos.z)); // (floor() rounds down)
		if(square % 2 == 0) // black tile
			closestObjectMaterial.SetColor(Color(0, 0, 0));
		else // white tile
			closestObjectMaterial.SetColor(Color(255, 255, 255));
	}

	bool shadowed = false;
	Color finalColor = closestObjectMaterial.GetColor() * AMBIENT_LIGHT; // Add ambient light to the calculation
														 
	if(REFLECTIONS_ON)
	{
		if(closestObjectMaterial.GetSpecular() > 0 && closestObjectMaterial.GetSpecular() <= 1)
		{
			Vector scalar = closestObjectNormal * (closestObjectNormal.Dot(-intersectingRayDir));
			Vector resultantReflection = -intersectingRayDir + ((scalar + (intersectingRayDir)) * 2);
			Vector reflectionDirection = resultantReflection.Normalize();

			Vector offset = reflectionDirection * 0.001; // The rays that start from reflecting object A are considered hitting itself, since it's the nearest object from the ray start position

			Ray reflectionRay(intersectionRayPos + offset, resultantReflection);

			// determine what the ray intersects with first
			std::vector<FPType> reflectionIntersections;
			for(auto sceneObject : sceneObjects)
			{
				reflectionIntersections.push_back(sceneObject->GetIntersection(reflectionRay));
			}

			int closestObjectWithReflection = ClosestObjectIndex(reflectionIntersections);

			if(closestObjectWithReflection != -1 && closestObjectWithReflection != indexOfClosestObject) // Depth checking
			{
				// reflection ray missed everthing else
				if(reflectionIntersections[closestObjectWithReflection] > TOLERANCE)
				{
					// determine the position and direction at the point of intersection with the reflection ray
					// the ray only affects the color if it reflected off something
					Vector reflectionIntersectionPosition = intersectionRayPos + (resultantReflection * (reflectionIntersections[closestObjectWithReflection]));
					Vector reflectionIntersectionRayDirection = resultantReflection;
					Color reflectionIntersectionColor = GetColorAt(reflectionIntersectionPosition, reflectionIntersectionRayDirection, sceneObjects, closestObjectWithReflection, lightSources);
					finalColor += (reflectionIntersectionColor * closestObjectMaterial.GetReflection());
				}
			}
		}
	}

	for(const auto &lightSource : lightSources)
	{
		Vector lightDir = (lightSource->GetPosition() - intersectionRayPos).Normalize(); // Calculate the directional vector towards the lightSource
		FPType cosineAngle = closestObjectNormal.Dot(lightDir);

		// Diffuse shading
		if(DIFFUSE_ON)
		{
			finalColor *= cosineAngle * AMBIENT_LIGHT;
		}
		if(cosineAngle > 0)
		{
			// Shadows
			if(SHADOWS_ON)
			{
				Ray shadowRay(intersectionRayPos, (lightSource->GetPosition() - intersectionRayPos).Normalize()); // Cast a ray from the first intersection to the light

				std::vector<FPType> secondaryIntersections;
				for(const auto &sceneObject : sceneObjects)
				{
					secondaryIntersections.push_back(sceneObject->GetIntersection(shadowRay));
				}

				for(const auto &secondaryIntersection : secondaryIntersections)
				{
					if(secondaryIntersection > TOLERANCE)
					{
						// Shadows
						shadowed = true;
						finalColor *= closestObjectMaterial.GetDiffuse() * AMBIENT_LIGHT;
						//finalColor = Color(0,0,0);
						
						break;
					}

					/*
					Surface.shade(ray, point, normal, light) {
					 shadRay = (point, light.pos � point);
					 if (shadRay not blocked) {
					 v = �normalize(ray.direction);
					 l = normalize(light.pos � point);
					 // compute shading
					 }
					 return */
				}
			}
			// Specular intensity / Phong illumination
			if(shadowed == false && SPECULAR_ON)
			{
				if(closestObjectMaterial.GetSpecular() > 0 && closestObjectMaterial.GetSpecular() <= 1)
				{
					Vector scalar1 = closestObjectNormal * (closestObjectNormal.Dot(-intersectingRayDir));
					Vector resultantReflection = -intersectingRayDir + ((scalar1 + (intersectingRayDir)) * 2);

					FPType specular = resultantReflection.Dot(lightDir);
					if(specular > 0)
					{
						// pow(specular, shininess factor (higher shine = more condensed phong light)) 
						specular = pow(specular, 120) * closestObjectMaterial.GetSpecular();
						finalColor += (lightSource->GetColor() * AMBIENT_LIGHT) * (specular * closestObjectMaterial.GetSpecular());
					}
				}
			}
		}
	}
	return finalColor.Clip();
}

void Draw(bitmap_image &image, int &x, int &y, Camera &camera, int &indexOfClosestObject, 
		  std::vector<FPType> &intersections, std::vector<std::shared_ptr<Object>> &sceneObjects, std::vector<std::shared_ptr<Light>> &lightSources)
{
	// If it doesn't register a ray trace set that pixel to be black
	if(indexOfClosestObject == -1)
		image.set_pixel(x, y, 255, 0, 0);
	else
	{
		if(intersections[indexOfClosestObject] > TOLERANCE) // If intersection at that point > accuracy, get color of object
		{
			// If registers a ray trace, set pixel color to traced pixel color (the object color)
			Ray intersectionRay;
			intersectionRay.SetOrigin(camera.GetOrigin() + (camera.GetSceneDirection() * intersections[indexOfClosestObject]));
			intersectionRay.SetDirection(camera.GetSceneDirection());

			Color intersectionColor = GetColorAt(intersectionRay.GetOrigin(), camera.GetSceneDirection(), sceneObjects, indexOfClosestObject, lightSources);

			image.set_pixel(x, y, unsigned char(intersectionColor.GetRed()), unsigned char(intersectionColor.GetGreen()), unsigned char(intersectionColor.GetBlue()));
		}
	}
}

void CalcIntersections()
{
	clock_t end, start = clock();
	bitmap_image image(WIDTH, HEIGHT);

	Camera camera(Vector(0, 3, -3), Vector(0, -1, 6));
	
	int columnsCompleted = 0;
	FPType percentage;
	
	FPType xCamOffset, yCamOffset; // Offset position of rays from the direction where camera is pointed (x & y positions)
	for(int x = 0; x < WIDTH; x++)
	{
		// Calculates % of render completed
		columnsCompleted++;
		percentage = columnsCompleted / (FPType) WIDTH * 100;
		std::cout << '\r' << "Completion: " << (int)percentage << '%';
		fflush(stdout);

		for(int y = 0; y < HEIGHT; y++)
		{
			// No Anti-aliasing
			if(WIDTH > HEIGHT)
			{
				xCamOffset = (((x + 0.5) / WIDTH) * ASPECT_RATIO) - ((WIDTH - HEIGHT) / HEIGHT) / 2;
				yCamOffset = (y + 0.5) / HEIGHT;
			}
			else if(HEIGHT > WIDTH)
			{
				xCamOffset = (x + 0.5) / WIDTH;
				yCamOffset = ((y + 0.5) / HEIGHT) / ASPECT_RATIO - ((HEIGHT - WIDTH) / (WIDTH / 2));
			}
			else
			{
				// Image is square
				xCamOffset = (x + 0.5) / WIDTH, HEIGHT;
				yCamOffset = (y + 0.5) / WIDTH, HEIGHT;
			}

			// Set up scene
			Scene scene;
			std::vector<std::shared_ptr<Object>> sceneObjects = scene.InitObjects();
			std::vector<std::shared_ptr<Light>> lightSources = scene.InitLightSources();

			std::vector<FPType> intersections;
			intersections.reserve(1024);
			//intersections.clear();

			// Camera direction for every ray shot through each pixel
			Vector camRayDir = (camera.GetCameraDirection() + camera.GetCamX() * (xCamOffset - 0.5) + camera.GetCamY() * (yCamOffset - 0.5)).Normalize();
			camera.SetSceneDirection(camRayDir);

			Ray camRay(camera.GetOrigin(), camera.GetSceneDirection());

			// Check if ray intersects with any scene objects
			for(const auto &sceneObject : sceneObjects)
			{
				intersections.push_back(sceneObject->GetIntersection(camRay));
			}

			// Check which object is closest to the camera
			int indexOfClosestObject = ClosestObjectIndex(intersections);
			Draw(image, x, y, camera, indexOfClosestObject, intersections, sceneObjects, lightSources);
		}
	}

	end = clock();
	FPType diff = ((FPType) end - (FPType) start) / CLOCKS_PER_SEC;
	std::cout << "\n\nResolution: " << WIDTH << "x" << HEIGHT << std::endl;
	std::cout << "Time: " << diff << " seconds" << std::endl;

	std::string saveString = "render.bmp";
	image.save_image(saveString);
	std::cout << "Output filename: " << saveString << std::endl;
}

int main()
{
	CalcIntersections();

	std::cout << "\nPress enter to exit...";
	std::cin.ignore();

	return 0;
}