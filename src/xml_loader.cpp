#include <math.h>
#include <stdio.h>
#include <string.h>
#include <glm/gtx/euler_angles.hpp>
#include "xml_loader.h"

// Example of how to use tinyxml2
void iterate_xml(XMLElement* root, int depth) {
	for (int i = 0; i < depth; i++)
		printf("    ");
	printf("<%s", root->Name());
	for (const XMLAttribute* a = root->FirstAttribute(); a != NULL; a = a->Next())
		printf(" %s=\"%s\"", a->Name(), a->Value());
	if (root->NoChildren()) {
		printf("/>\n");
		return;
	}
	printf(">\n");
	for (XMLElement* e = root->FirstChildElement(); e != NULL; e = e->NextSiblingElement())
		iterate_xml(e, depth + 1);
	for (int i = 0; i < depth; i++)
		printf("    ");
	printf("</%s>\n", root->Name());
}

void Scene::RecursiveLoad(XMLElement* element, vec3 parent_pos, quat parent_rot) {
	vec3 position = vec3(0, 0, 0);
	const char* pos = element->Attribute("pos");
	if (pos != NULL) {
		float x, y, z;
		sscanf(pos, "%f %f %f", &x, &y, &z);
		position = vec3(x, y, z);
	}
	quat rotation = quat(1, 0, 0, 0);
	const char* rot = element->Attribute("rot");
	if (rot != NULL) {
		float x, y, z;
		sscanf(rot, "%f %f %f", &x, &y, &z);
		rotation = quat_cast(eulerAngleYZ(radians(y), radians(z)) * eulerAngleX(radians(x)));
		//vec3 euler = eulerAngles(rotation);
		//if (abs(x - degrees(euler.x)) > 1 || abs(y - degrees(euler.y)) > 1 || abs(z - degrees(euler.z)) > 1)
		//	printf("[ERROR] Euler angles don't match: %d %d %d != %d %d %d\n", (int)x, (int)y, (int)z, (int)degrees(euler.x), (int)degrees(euler.y), (int)degrees(euler.z));
	}
	position = parent_rot * position + parent_pos;
	rotation = parent_rot * rotation;

	if (strcmp(element->Name(), "vox") == 0) {
		scene_t vox = { "", "", position, rotation, 1.0f };
		const char* file = element->Attribute("file");
		if (file == NULL) {
			printf("[ERROR] No file specified for vox\n");
			exit(EXIT_FAILURE);
		}
		if (strncmp(file, "MOD/", 4) == 0)
			vox.file = parent_folder + string(file + 4);
		else
			vox.file = file;
		if (models.find(vox.file) == models.end()) {
			VoxLoader* model = new VoxLoader(vox.file.c_str());
			models[vox.file] = model;
		}
		const char* object = element->Attribute("object");
		if (object != NULL) vox.object = object;
		const char* scale = element->Attribute("scale");
		if (scale != NULL) vox.scale = atof(scale);
		shapes.push_back(vox);
		//printf("%13s\tpos=[%5.2f %5.2f %5.2f ]\n", object, vox.position.x, vox.position.y, vox.position.z);
	} else if (strcmp(element->Name(), "voxbox") == 0) {
		vec3 size = vec3(10, 10, 10);
		const char* size_str = element->Attribute("size");
		if (size_str != NULL) {
			float x, y, z;
			sscanf(size_str, "%f %f %f", &x, &y, &z);
			size = vec3(x, y, z);
		}
		vec3 color = vec3(1, 1, 1);
		const char* color_str = element->Attribute("color");
		if (color_str != NULL) {
			float r, g, b;
			sscanf(color_str, "%f %f %f", &r, &g, &b);
			color = vec3(r, g, b);
		}
		VoxboxRender* voxbox = new VoxboxRender(size, color);
		//printf("Voxbox [%g %g %g] color=(%g %g %g)\n", size.x, size.y, size.z, color.x, color.y, color.z);
		voxbox->setWorldTransform(position, rotation);
		voxboxes.push_back(voxbox);
	} else if (strcmp(element->Name(), "water") == 0) {
		vector<vec2> water_verts;
		for (XMLElement* e = element->FirstChildElement(); e != NULL; e = e->NextSiblingElement()) {
			if (strcmp(e->Name(), "vertex") == 0) {
				const char* pos = e->Attribute("pos");
				if (pos != NULL) {
					float x, y;
					sscanf(pos, "%f %f", &x, &y);
					water_verts.push_back(vec2(x, y));
				}
			}
		}
		if (water_verts.size() > 2) {
			WaterRender* water = new WaterRender(water_verts);
			water->setWorldTransform(position);
			if (waters.size() > 1)
				printf("[Warning] Too much water! be responsible and reduces water use during drought\n");
			else
				waters.push_back(water);
		}
	} else if (strcmp(element->Name(), "rope") == 0) {
		vector<vec3> rope_verts;
		for (XMLElement* e = element->FirstChildElement(); e != NULL; e = e->NextSiblingElement()) {
			if (strcmp(e->Name(), "location") == 0) {
				const char* pos = e->Attribute("pos");
				vec3 vert_pos = vec3(0, 0, 0);
				if (pos != NULL) {
					float x, y, z;
					sscanf(pos, "%f %f %f", &x, &y, &z);
					vert_pos = vec3(x, y, z);
				}
				rope_verts.push_back(vert_pos);
			}
		}
		vec3 color = vec3(0, 0, 0);
		const char* color_str = element->Attribute("color");
		if (color_str != NULL) {
			float r, g, b;
			sscanf(color_str, "%f %f %f", &r, &g, &b);
			color = vec3(r, g, b);
		}
		if (rope_verts.size() > 1) {
			RopeRender* rope = new RopeRender(rope_verts, color);
			rope->setWorldTransform(position, rotation);
			ropes.push_back(rope);
		}
	}
	for (XMLElement* child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
		RecursiveLoad(child, position, rotation);
}

Scene::Scene(string path) {
	XMLDocument xml_file;
	if (xml_file.LoadFile(path.c_str()) != XML_SUCCESS) {
		printf("[Warning] XML file %s not found or corrupted.\n", path.c_str());
		return;
	}
	parent_folder = path.substr(0, path.find_last_of("/\\") + 1);
	XMLElement* root = xml_file.RootElement();
	//iterate_xml(root, 0);
	vec3 position = vec3(0, 0, 0);
	quat rotation = quat(1, 0, 0, 0);
	RecursiveLoad(root, position, rotation);
	printf("Loaded %d objects, %d voxbox, %d water, %d rope\n", (int)shapes.size(), (int)voxboxes.size(), (int)waters.size(), (int)ropes.size());
}

void Scene::draw(Shader& shader, Camera& camera, vec4 clip_plane) {
	for (vector<scene_t>::iterator it = shapes.begin(); it != shapes.end(); it++) {
		VoxLoader* model = models[it->file];
		if (it->object == "")
			model->draw(shader, camera, clip_plane, it->position, it->rotation, it->scale);
		else
			model->draw(shader, camera, clip_plane, it->object, it->position, it->rotation, it->scale);
	}
}

void Scene::drawRope(Shader& shader, Camera& camera) {
	for (vector<RopeRender*>::iterator it = ropes.begin(); it != ropes.end(); it++)
		(*it)->draw(shader, camera);
}

void Scene::drawWater(Shader& shader, Camera& camera) {
	for (vector<WaterRender*>::iterator it = waters.begin(); it != waters.end(); it++)
		(*it)->draw(shader, camera);
}

void Scene::drawVoxbox(Shader& shader, Camera& camera) {
	for (vector<VoxboxRender*>::iterator it = voxboxes.begin(); it != voxboxes.end(); it++)
		(*it)->draw(shader, camera);
}

Scene::~Scene() {
	for (map<string, VoxLoader*>::iterator it = models.begin(); it != models.end(); it++)
		delete it->second;
	for (vector<VoxboxRender*>::iterator it = voxboxes.begin(); it != voxboxes.end(); it++)
		delete *it;
}
