#pragma once

#include "object.hpp"
#include "glm.hpp"

#include <vector>

namespace deform {
	std::vector<float> deformMath(std::vector<float> vertices, glm::vec3 pos, glm::vec3 point, float intensity, float resist, float epsilon = 0.0001f) {
		std::vector<float> newVertices;
		std::vector<glm::vec3> vVec;

		vVec.resize(vertices.size() / 3);
		newVertices.resize(vertices.size());
		
		for (int i = 0; i < vertices.size(); i++) {
			int cI = i % 3;
			int vI = glm::floor(i / 3);
			float val = vertices[i];
			if (cI == 0) {
				vVec[vI].x = val;
			} else if (cI == 1) {
				vVec[vI].y = val;
			} else if (cI == 2) {
				vVec[vI].z = val;
			}
		}

		int i = 0;
		for (glm::vec3 vec : vVec) {
			glm::vec3 Pv = vec + pos;
			glm::vec3 d = glm::vec3(point.x - Pv.x, point.y - Pv.y, point.z - Pv.z);
			float lenSqr = (float) (d.x * d.x + d.y * d.y + d.z * d.z) + epsilon;
			glm::vec3 Pn = Pv - d / lenSqr * intensity / resist;
			glm::vec3 PnGL = Pn - pos;
			newVertices[i*3] = PnGL.x;
			newVertices[i*3+1] = PnGL.y;
			newVertices[i*3+2] = PnGL.z;
			i++;
		}

		return newVertices;
	}

	void deformPoint(object::Object3D* obj, glm::vec3 point, float intensity) {
		std::vector<float> nV = deformMath(obj->getPureVertices(), obj->position, point, intensity, obj->deformResistance);
		obj->setPureVertices(nV);
		obj->updateVAOVBOEBO();
	}
}
