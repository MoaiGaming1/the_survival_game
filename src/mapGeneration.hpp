#include <vector>

#include "glm.hpp"
#include "noise/FastNoiseLite.h"

#include "constants.hpp"
#include "physicsImplementation.hpp"
#include "object.hpp"

namespace mapGen {
	struct Map {
		std::vector<float> heightmap;
		object::Object3D* terrain;
		JPH::BodyID terrainBodyID;
		std::vector<object::Object3D*> trees;
	};

	std::vector<float> generateHeightmap(int seed) {
		std::vector<float> hs;
		hs.reserve((size_t)glm::pow(TERRAIN_GRID_COUNT, 2));

		FastNoiseLite terrainNoise;
		terrainNoise.SetSeed(seed);
		terrainNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
		terrainNoise.SetFrequency(TERRAIN_NOISE_FREQUENCY);

		FastNoiseLite mountainNoise;
		mountainNoise.SetSeed(seed);
		mountainNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
		mountainNoise.SetFrequency(TERRAIN_MOUNTAIN_NOISE_FREQUENCY);

		for (int z = 0; z < TERRAIN_GRID_COUNT; z++) {
			for (int x = 0; x < TERRAIN_GRID_COUNT; x++) {
				float h = terrainNoise.GetNoise((float)x, (float)z) * TERRAIN_HEIGHT_SCALE + glm::max(mountainNoise.GetNoise((float)x, (float)z) * TERRAIN_MOUNTAIN_HEIGHT_SCALE, 0.0f);
				hs.push_back(h);
			}
		}

		return hs;
	}

	Map* generateMap(uint shaderProg, object::Camera* camera, int seed) {
		std::vector<float> heightmap = generateHeightmap(seed);
		
		std::vector<glm::vec3> vertices;
		std::vector<uint> indices;
		std::vector<glm::vec3> normals;

		// vertices and normals calculation
		for (int z = 0; z < TERRAIN_GRID_COUNT; z++) {
			for (int x = 0; x < TERRAIN_GRID_COUNT; x++) {
				int vI = z * TERRAIN_GRID_COUNT + x;
				glm::vec3 v = glm::vec3(x * TERRAIN_GRID_SIZE, heightmap[vI], z * TERRAIN_GRID_SIZE);
				vertices.push_back(v);

				auto getHeight = [&](int xi, int zi) -> float {
					xi = glm::clamp(xi, 0, TERRAIN_GRID_COUNT - 1);
					zi = glm::clamp(zi, 0, TERRAIN_GRID_COUNT - 1);
					return heightmap[zi * TERRAIN_GRID_COUNT + xi];
				};

				float hL = getHeight(x - 1, z);
				float hR = getHeight(x + 1, z);
				float hD = getHeight(x, z - 1);
				float hU = getHeight(x, z + 1);

				glm::vec3 normal = glm::normalize(glm::vec3(
					(hL - hR) / (2.0f * TERRAIN_GRID_SIZE),
					1.0f,
					(hD - hU) / (2.0f * TERRAIN_GRID_SIZE)
				));

				normals.push_back(normal);
			}
		}

		// indices calculation
		for (int z = 0; z < TERRAIN_GRID_COUNT - 1; z++) {
			for (int x = 0; x < TERRAIN_GRID_COUNT - 1; x++) {
				uint topLeft = z * TERRAIN_GRID_COUNT + x;
				uint topRight = topLeft + 1;
				uint bottomLeft = (z + 1) * TERRAIN_GRID_COUNT + x;
				uint bottomRight = bottomLeft + 1;

				indices.push_back(topLeft);
				indices.push_back(bottomLeft);
				indices.push_back(topRight);

				indices.push_back(topRight);
				indices.push_back(bottomLeft);
				indices.push_back(bottomRight);
			}
		}
		
		std::vector<float> allVertices;
		for (int i = 0; i < vertices.size(); i++) {
			glm::vec3 v = vertices[i];
			glm::vec3 n = normals[i];

			allVertices.push_back(v.x);
			allVertices.push_back(v.y);
			allVertices.push_back(v.z);

			allVertices.push_back(n.x);
			allVertices.push_back(n.y);
			allVertices.push_back(n.z);
		}

		object::Object3D* terrain = new object::Object3D(shaderProg, camera, allVertices, indices);

		// tree gen
		std::vector<object::Object3D*> trees;
		{
			FastNoiseLite treeNoise;

			treeNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
			treeNoise.SetSeed(seed);
			treeNoise.SetFrequency(MAP_TREE_FREQUENCY);

			for (float x = 0; x < TERRAIN_GRID_COUNT * TERRAIN_GRID_SIZE; x += MAP_TREE_GRID_SIZE) {
				for (float z = 0; z < TERRAIN_GRID_COUNT * TERRAIN_GRID_SIZE; z += MAP_TREE_GRID_SIZE) {
					if (treeNoise.GetNoise(x, z) >= MAP_TREE_SPAWN_MIN_VAL) {
						// spawn tree
						object::Object3D* tree = new object::Object3D(shaderProg, camera);
						tree->setModel("assets/models/Tree.json");
						tree->position = {x, heightmap[glm::floor(z/TERRAIN_GRID_SIZE)*TERRAIN_GRID_COUNT + glm::floor(x/TERRAIN_GRID_SIZE)]-1, z};
						trees.push_back(tree);
					}
				}
			}
		}

		Map* map = new Map();

		// terrain collision body setup
		{
			JPH::HeightFieldShapeSettings hfSettings(
				heightmap.data(),
				JPH::Vec3(0.0f, 0.0f, 0.0f),
				JPH::Vec3(TERRAIN_GRID_SIZE, 1.0f, TERRAIN_GRID_SIZE),
				TERRAIN_GRID_COUNT
			);

			JPH::ShapeRefC hfShape = hfSettings.Create().Get();

			JPH::BodyCreationSettings bSettings(
				hfShape,
				JPH::RVec3(0.0f, 0.0f, 0.0f),
				JPH::Quat::sIdentity(),
				JPH::EMotionType::Static,
				physics::Layers::NON_MOVING
			);

			bSettings.mFriction = 0.5f;
			bSettings.mRestitution = 0.0f;

			JPH::Body* b = physics::physicsSystem.GetBodyInterface().CreateBody(bSettings);
			physics::physicsSystem.GetBodyInterface().AddBody(b->GetID(), JPH::EActivation::DontActivate);

			map->terrainBodyID = b->GetID();
		}
		
		map->heightmap = heightmap;
		map->terrain = terrain;
		map->trees = trees;

		return map;
	}
}
