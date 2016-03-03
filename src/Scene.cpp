#include "Scene.h"

std::vector<std::shared_ptr<Object>> Scene::InitObjects()
{
	std::shared_ptr<Plane> floorPlane = std::make_shared<Plane>(Vector(0, -1, 0), Vector(0, 1, 0));
	floorPlane->SetMaterial(tileFloor);

	std::shared_ptr<Sphere> sphere1 = std::make_shared<Sphere>(0.5, Vector(1, -0.5, 2.5));
	sphere1->SetMaterial(maroonM);
	std::shared_ptr<Sphere> sphere2 = std::make_shared<Sphere>(0.5, Vector(-1.5, -0.5, 4));
	sphere2->SetMaterial(prettyGreen);
	std::shared_ptr<Sphere> sphere3 = std::make_shared<Sphere>(0.2, Vector(sphere1->GetPosition().x - 2, sphere1->GetPosition().y, sphere1->GetPosition().z - 2));
	sphere3->SetMaterial(blueM);
	std::shared_ptr<Sphere> sphere4 = std::make_shared<Sphere>(0.2, Vector(sphere1->GetPosition().x - 0.8, sphere1->GetPosition().y + 0.4, sphere1->GetPosition().z - 0.67));
	sphere4->SetMaterial(silver);
	std::shared_ptr<Sphere> sphere5 = std::make_shared<Sphere>(0.4, Vector(floorPlane->GetCenter().x - 3.5, -0.6, floorPlane->GetCenter().z + 2.9));
	sphere5->SetMaterial(orangeM);

	std::vector<std::shared_ptr<Object>> sceneObjects;
	sceneObjects.push_back(sphere1);
	sceneObjects.push_back(sphere2);
	sceneObjects.push_back(sphere3);
	sceneObjects.push_back(sphere4);
	sceneObjects.push_back(sphere5);
	sceneObjects.push_back(floorPlane);

	return sceneObjects;
}

std::vector<std::shared_ptr<Light>> Scene::InitLightSources()
{
	std::vector<std::shared_ptr<Light>> lightSources;
	Vector light1Position(- 2.5, 1, 0.6);
	std::shared_ptr<Light> light1 = std::make_shared<Light>(light1Position, white);
	Light light2(Vector(light1Position.x, light1Position.y, light1Position.z), white);
	lightSources.push_back(light1);
	//lightSources.push_back(&light2);

	return lightSources;
}
