#define _SCL_SECURE_NO_WARNINGS
#define _USE_MATH_DEFINES

#include "bitmap_image.hpp"
#include "Globals.h"
#include "Scene.h"
#include "Camera.h"
#include "Matrix44.h"
#include <vector>
#include <time.h>
#include <sstream>
#include <thread>

Color GetColorAt(Vector &position, Vector &sceneDirection, const std::vector<std::shared_ptr<Object>> &sceneObjects, int indexOfClosestObject, const std::vector<std::shared_ptr<Light>> &lightSources, int depth);
Color GetReflections(Vector &position, Vector &sceneDirection, const std::vector<std::shared_ptr<Object>> &sceneObjects,
					 int indexOfClosestObject, const std::vector<std::shared_ptr<Light>> &lightSources, int depth);


// Returns the closest object's index that the ray intersected with
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
			for(int intersectionIndex = 0; intersectionIndex < intersections.size(); intersectionIndex++)
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
			// All intersections were negative (didn't hit anything)
			return -1;
		}
	}
}

FPType clamp(const FPType &lo, const FPType &hi, const FPType &v)
{
	return std::max(lo, std::min(hi, v));
}

inline
FPType deg2rad(const FPType &deg)
{
	return deg * M_PI / 180;
}

FPType fresnel(Vector sceneDirection, Vector &closestObjectNormal, Material &closestObjectMaterial)
{
	FPType kr;
	Vector I = sceneDirection;
	Vector N = closestObjectNormal;
	FPType cosi = clamp(-1, 1, I.Dot(N));
	FPType etai = 1, etat = closestObjectMaterial.GetRefraction();
	if(cosi > 0)
	{
		std::swap(etai, etat);
	}
	// Compute sini using Snell's law
	FPType sint = etai / etat * sqrtf(std::fmax(0.f, 1 - cosi * cosi));
	// Total internal reflection
	if(sint >= 1)
	{
		kr = 1;
	}
	else
	{
		FPType cost = sqrtf(std::fmax(0.f, 1 - sint * sint));
		cosi = fabsf(cosi);
		FPType Rs = ((etat * cosi) - (etai * cost)) / ((etat * cosi) + (etai * cost));
		FPType Rp = ((etai * cosi) - (etat * cost)) / ((etai * cosi) + (etat * cost));
		kr = (Rs * Rs + Rp * Rp) / 2;
	}
	return kr;
	// As a consequence of the conservation of energy, transmittance is given by:
	// kt = 1 - kr;
}

Ray GetRefractionRay(Vector &position, Vector &sceneDirection, Vector &closestObjectNormal, Material &closestObjectMaterial)
{
	Vector normal = closestObjectNormal;
	FPType nDotI = normal.Dot(sceneDirection);

	FPType n1 = GLOBAL_REFRACTION;
	FPType n2 = closestObjectMaterial.GetRefraction();

	if(nDotI < 0) // Ray is outside the surface
		nDotI = -nDotI;
	else // Ray is inside refractive surface
	{
		normal = -normal;
		std::swap(n1, n2);
	}
	FPType cosI = clamp(-1, 1, sceneDirection.Dot(normal)); // Refraction angle
	FPType n = n1 / n2;
	FPType k = 1 - n * n * (1 - cosI * cosI);

	Vector refractionDir = (sceneDirection * n) + normal * (n * cosI - sqrtf(k));
	Vector offset = refractionDir * BIAS;
	Ray refractionRay(position + offset, refractionDir);
	return refractionRay;
}

//Vector Refract(const Vector &direction, const Vector &closestObjectNormal, Material &closestObjectMaterial)
//{
//	Vector I = direction;
//	Vector N = closestObjectNormal;
//	FPType ior = closestObjectMaterial.GetRefraction();
//
//	FPType cosi = clamp(-1, 1, I.Dot(N));
//	FPType etai = 1, etat = ior;
//	Vector n = N;
//	if(cosi < 0)
//	{
//		cosi = -cosi;
//	}
//	else
//	{
//		std::swap(etai, etat); n = -N;
//	}
//	FPType eta = etai / etat;
//	FPType k = 1 - eta * eta * (1 - cosi * cosi);
//	return k < 0 ? 0 : I * eta + n * (eta * cosi - sqrtf(k));
//}

Ray GetReflectionRay(Vector &closestObjectNormal, Vector &sceneDirection, Vector &position)
{
	FPType nDotV = closestObjectNormal.Dot(-sceneDirection);

	Vector resultantReflection = (closestObjectNormal * nDotV * 2) + sceneDirection;
	Vector reflectionDirection = resultantReflection.Normalize();

	// To not hit itself
	Vector offset = reflectionDirection * BIAS;

	Ray reflectionRay(position + offset, resultantReflection);
	return reflectionRay;
}

Color GetRefractions(Vector &position, Vector &dir, const std::vector<std::shared_ptr<Object>> &sceneObjects,
					 int indexOfClosestObject, const std::vector<std::shared_ptr<Light>> &lightSources, int depth)
{
	if(indexOfClosestObject != -1)
	{
		Material closestObjectMaterial = sceneObjects[indexOfClosestObject]->GetMaterial();
		Vector closestObjectNormal = sceneObjects[indexOfClosestObject]->GetNormalAt(position);
		if(closestObjectMaterial.GetRefraction() > 0 && closestObjectMaterial.GetReflection() > 0)
		{
			Ray refractionRay = GetRefractionRay(position, dir, closestObjectNormal, closestObjectMaterial);

			std::vector<FPType> refractionIntersections;
			for(auto sceneObject : sceneObjects)
			{
				refractionIntersections.push_back(sceneObject->GetIntersection(refractionRay));
			}
			if(refractionIntersections.size() > 0)
			{
				int closestObjectWithRefraction = ClosestObjectIndex(refractionIntersections);
				if(closestObjectWithRefraction != -1)
				{
					Color refractionColor = Color(0, 0, 0);
					// compute fresnel
					//FPType kr = 1;
					FPType kr = fresnel(dir, closestObjectNormal, closestObjectMaterial);
					//std::cout << "KR: " << kr;
					bool outside = dir.Dot(closestObjectNormal) < 0;
					Vector bias = closestObjectNormal * BIAS;

					// compute refraction if it is not a case of total internal reflection
					if(kr < 1)
					{
						Vector refractionDirection = refractionRay.GetDirection().Normalize();
						Vector refractionIntersectionPosition = refractionRay.GetOrigin() + (refractionRay.GetDirection() * (refractionIntersections[closestObjectWithRefraction]));
						Vector refractionRayOrig = outside ? refractionIntersectionPosition - bias : refractionIntersectionPosition + bias;

						refractionColor += GetColorAt(refractionRayOrig, refractionDirection, sceneObjects, closestObjectWithRefraction, lightSources, +1);
					}
					else
					{
						refractionColor = Color(0, 0, 0);
						//std::cout << "TIR";
					}

					// uncommenting reflectionColor crashes
					//Color reflectionColor = GetReflections(position, dir, sceneObjects, indexOfClosestObject, lightSources, +1);
					// mix the two
					Color refraReflColor = /*reflectionColor * kr + */refractionColor * (1 - kr);
					return refraReflColor;
				}
			}
			else
				return Color(0, 0, 0);
		}
		else
			return Color(0, 0, 0);
	}

	else
		return Color(0, 0, 0);
}

// Calculate reflection colors
Color GetReflections(Vector &position, Vector &sceneDirection, const std::vector<std::shared_ptr<Object>> &sceneObjects,
					 int indexOfClosestObject, const std::vector<std::shared_ptr<Light>> &lightSources, int depth)
{
	if(indexOfClosestObject != -1)
	{
		Material closestObjectMaterial = sceneObjects[indexOfClosestObject]->GetMaterial();
		Vector closestObjectNormal = sceneObjects[indexOfClosestObject]->GetNormalAt(position);

		if(closestObjectMaterial.GetReflection() > 0)
		{
			if(closestObjectMaterial.GetSpecular() > 0 && closestObjectMaterial.GetSpecular() <= 1)
			{
				Ray reflectionRay = GetReflectionRay(closestObjectNormal, sceneDirection, position);

				// determine what the ray intersects with first
				std::vector<FPType> reflectionIntersections;
				for(auto sceneObject : sceneObjects)
				{
					if(sceneObject->GetIntersection(reflectionRay))
						reflectionIntersections.push_back(sceneObject->GetIntersection(reflectionRay));
					else
						break;
				}

				int closestObjectWithReflection = ClosestObjectIndex(reflectionIntersections);

				if(closestObjectWithReflection != -1)
				{
					// reflection ray missed everthing else
					if(reflectionIntersections[closestObjectWithReflection] > BIAS)
					{
						// determine the position and sceneDirectionection at the position of intersection with the reflection ray
						// the ray only affects the color if it reflected off something
						Vector reflectionIntersectionPosition = reflectionRay.GetOrigin() + (reflectionRay.GetDirection() * (reflectionIntersections[closestObjectWithReflection]));
						Color reflectionIntersectionColor = GetColorAt(reflectionIntersectionPosition, reflectionRay.GetDirection(), sceneObjects, closestObjectWithReflection, lightSources, depth + 1);
						return reflectionIntersectionColor;
					}
				}
			}
			else
				return Color(0, 0, 0);
		}
		else
			return Color(0, 0, 0);
	}
}

// Get the color of the pixel at the ray-object intersection position
Color GetColorAt(Vector &origin, Vector &direction, const std::vector<std::shared_ptr<Object>> &sceneObjects, int indexOfClosestObject,
				 const std::vector<std::shared_ptr<Light>> &lightSources, int depth)
{
	if(indexOfClosestObject != -1)
	{
		Color finalColor;
		if(depth > DEPTH)
			return Color(0, 0, 0);

		Material closestObjectMaterial = sceneObjects[indexOfClosestObject]->GetMaterial();
		Vector closestObjectNormal = sceneObjects[indexOfClosestObject]->GetNormalAt(origin);

		// Checkerboard pattern floor
		if(closestObjectMaterial.GetSpecial() == 2)
		{
			unsigned square = int(floor(origin.x)) + int(floor(origin.z)); // (floor() rounds down)
			if(square % 2 == 0) // black tile
				closestObjectMaterial.SetColor(Color(0, 0, 0));
			else // white tile
				closestObjectMaterial.SetColor(Color(255, 255, 255));
		}

		Color ambient;
		Color diffuse;
		FPType lambertian;
		FPType phong;
		Color specular;

		// Ambient
		if(AMBIENT_ON)
		{
			ambient = closestObjectMaterial.GetColor() * AMBIENT_LIGHT * closestObjectMaterial.GetAmbient();
			finalColor += ambient;
		}

		// Shadows, Diffuse, Specular
		for(const auto &lightSource : lightSources)
		{
			bool shadowed = false;
			Vector lightDir;
			if(lightSource->POINT)
				lightDir = (lightSource->GetPosition() - origin); // Calculate the sceneDirectionectional vector towards the lightSource

																  /*if(lightSource->AREA)
																  {

																  }*/

			FPType distance = lightDir.Magnitude();
			lightDir = lightDir.Normalize();
			lambertian = closestObjectNormal.Dot(lightDir.Normalize());

			// Shadows
			if(SHADOWS_ON && lambertian > 0)
			{
				Ray shadowRay(origin, lightDir); // Cast a ray from the first intersection to the light

				std::vector<FPType> secondaryIntersections;
				for(const auto &sceneObject : sceneObjects)
				{
					secondaryIntersections.push_back(sceneObject->GetIntersection(shadowRay));
				}

				for(const auto &secondaryIntersection : secondaryIntersections)
				{
					if(secondaryIntersection > BIAS) // If shadow ray intersects with some object along the way
					{
						if(secondaryIntersection <= distance)
						{
							shadowed = true;
							break;
						}
					}
				}
			}

			// Diffuse
			if(DIFFUSE_ON && shadowed == false)
			{
				//diffuse = closestObjectMaterial.GetColor().Average(lightSource->GetColor()) * 0.18 * M_PI * lightSource->GetIntensity() * std::fmax(0, lambertian) / distance;
				diffuse = closestObjectMaterial.GetColor().Average(lightSource->GetColor()) * closestObjectMaterial.GetDiffuse() * lightSource->GetIntensity() * std::fmax(lambertian, 0) / distance;
				finalColor += diffuse;
			}

			// Specular
			if(shadowed == false && SPECULAR_ON)
			{
				if(closestObjectMaterial.GetSpecular() > 0 && closestObjectMaterial.GetSpecular() <= 1)
				{
					Vector V = -direction;
					// Blinn-Phong
					Vector H = (lightDir + V).Normalize();
					FPType NdotH = closestObjectNormal.Dot(H);

					phong = pow(NdotH, 100);
					specular = lightSource->GetColor() * std::fmax(0, phong) * lightSource->GetIntensity(); // closestObjectMaterial.GetSpecular(); add or no?
					finalColor += specular;
				}
			}
		}

		// perfect mirrors
		if(REFLECTIONS_ON && closestObjectMaterial.GetRefraction() == 0 && closestObjectMaterial.GetReflection() > 0)
		{
			Color reflections = GetReflections(origin, direction, sceneObjects, indexOfClosestObject, lightSources, depth + 1);
			finalColor += reflections;
		}

		//Reflections & Refractions
		if(REFRACTIONS_ON && closestObjectMaterial.GetRefraction() > 0 && closestObjectMaterial.GetReflection() > 0)
		{
			Color refractions = GetRefractions(origin, direction, sceneObjects, indexOfClosestObject, lightSources, depth + 1);
			finalColor = refractions;
		}

		finalColor.Clip();
		return finalColor;
	}

	else
		return Color(0, 0, 0);
}

void Render(bitmap_image *image, unsigned x, unsigned y, Color tempColor[])
{
	Color totalColor = Color(0, 0, 0);

	for(int col = 0; col < SUPERSAMPLING*SUPERSAMPLING; col++)
	{
		totalColor += tempColor[col];
	}

	Color avgColor = totalColor / (SUPERSAMPLING * SUPERSAMPLING);
	image->set_pixel(x, y, avgColor.GetRed(), avgColor.GetGreen(), avgColor.GetBlue());
}

// Camera pos, sceneDirection here
void EvaluateIntersections(FPType xCamOffset, FPType yCamOffset, unsigned aaIndex, Color tempColor[], Matrix44f &cameraToWorld, Vector &orig)
{
	Camera camera(Vector(-1.5, 0.5, 2), Vector(0, 0, -1));

	// Set up scene
	Scene scene;
	std::vector<std::shared_ptr<Object>> sceneObjects = scene.InitObjects();
	std::vector<std::shared_ptr<Light>> lightSources = scene.InitLightSources();

	// Camera sceneDirectionection for every ray shot through each pixel
	//Vector camRayDir = (camera.GetForward() + camera.GetRight() * (xCamOffset - 0.5) + camera.GetUp() * (yCamOffset - 0.5)).Normalize();
	Vector camRayDir;
	cameraToWorld.MultDirMatrix(Vector(xCamOffset, yCamOffset, -1), camRayDir);
	camRayDir.Normalize();
	camera.SetTo(camRayDir);
	// Shoot ray into evey pixel of the image
	Ray camRay(camera.GetFrom(), camera.GetTo());

	std::vector<FPType> intersections;
	intersections.reserve(1024);

	// Check if ray intersects with any scene sceneObjects
	for(const auto &sceneObject : sceneObjects)
	{
		intersections.push_back(sceneObject->GetIntersection(camRay));
	}
	int indexOfClosestObject = ClosestObjectIndex(intersections);
	// If it doesn't register a ray trace set that pixel to be black (ray missed everything)
	if(indexOfClosestObject == -1)
	{
		tempColor[aaIndex] = Color(0, 0, 0);
	}
	else // Ray hit an object
	{
		if(intersections[indexOfClosestObject] > BIAS) // If intersection at that position > accuracy, get color of object
		{
			// If ray hit something, set position position to ray-object intersection
			Vector position((camera.GetFrom() + (camera.GetTo() * intersections[indexOfClosestObject])));
			Color intersectionColor = GetColorAt(position, camera.GetTo(), sceneObjects, indexOfClosestObject, lightSources, 0);
			tempColor[aaIndex] = Color(intersectionColor.GetRed(), intersectionColor.GetGreen(), intersectionColor.GetBlue());
		}
	}
}

void launchThread(unsigned start, unsigned end, bitmap_image *image)
{
	unsigned width = WIDTH;
	unsigned height = HEIGHT;

	Color tempColor[SUPERSAMPLING*SUPERSAMPLING];
	unsigned aaIndex;
	FPType xCamOffset, yCamOffset; // Offset position of rays from the sceneDirectionection where camera is pointed (x & y positions)

	FPType scale = tan(deg2rad(FOV * 0.5));

	Matrix44f cameraToWorld;
	Vector orig;
	cameraToWorld.MultVecMatrix(Vector(0, 0, 0), orig);

	for(unsigned z = start; z < end; z++)
	{
		unsigned x = z % width;
		unsigned y = z / width;
		FPType aspectRatio = WIDTH / FPType(HEIGHT);

		for(unsigned i = 0; i < SUPERSAMPLING; i++)
		{
			for(unsigned j = 0; j < SUPERSAMPLING; j++)
			{
				aaIndex = j*SUPERSAMPLING + i;
				// No Anti-aliasing
				if(SUPERSAMPLING == 1) // Heigh cannot be bigger than width
				{
					xCamOffset = (2 * (x + 0.5) / FPType(WIDTH) - 1) * aspectRatio * scale;
					yCamOffset = (1 - 2 * (y + 0.5) / FPType(HEIGHT)) * scale;
				}
				// Supersampling anti-aliasing
				else // Heigh cannot be bigger than width
				{
					xCamOffset = (2 * (x + (0.5 + i) / (SUPERSAMPLING - 1)) / FPType(WIDTH) - 1) * aspectRatio * scale;
					yCamOffset = (1 - 2 * (y + (j + 0.5) / SUPERSAMPLING) / FPType(HEIGHT)) * scale;
				}
				EvaluateIntersections(xCamOffset, yCamOffset, aaIndex, tempColor, cameraToWorld, orig);
			}
		}
		Render(image, x, y, tempColor);
	}
}

void CalcIntersections()
{
	clock_t end, start = clock();
	bitmap_image *image = new bitmap_image(WIDTH, HEIGHT);

	unsigned nThreads = std::thread::hardware_concurrency();
	std::cout << "Threads: " << nThreads << std::endl;

	std::thread* tt = new std::thread[nThreads];

	unsigned size = WIDTH*HEIGHT;

	unsigned chunk = size / nThreads;
	unsigned rem = size % nThreads;

	//launch threads
	for(unsigned i = 0; i < nThreads - 1; i++)
	{
		tt[i] = std::thread(launchThread, i*chunk, (i + 1)*chunk, image);
	}

	launchThread((nThreads - 1)*chunk, (nThreads) *chunk + rem, image);

	for(unsigned int i = 0; i < nThreads - 1; i++)
		tt[i].join();

	end = clock();
	FPType diff = ((FPType) end - (FPType) start) / CLOCKS_PER_SEC;
	std::cout << "Resolution: " << WIDTH << "x" << HEIGHT << std::endl;
	std::cout << "Depth: " << DEPTH << std::endl;
	std::cout << "Time: " << diff << " seconds" << std::endl;

	std::string saveString = std::to_string(int(WIDTH)) + "x" + std::to_string(int(HEIGHT)) + " render, " + std::to_string(SUPERSAMPLING) + "x SS, " + std::to_string(DEPTH) + " depth.bmp";
	image->save_image(saveString);
	std::cout << "Output filename: " << saveString << std::endl;
}


int main()
{
	CalcIntersections();

	std::cout << "\nPress enter to exit...";
	std::cin.ignore();

	return 0;
}