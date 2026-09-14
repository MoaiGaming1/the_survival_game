#pragma once

#include "glad.h"
#include "glfw3.h"
#include "glm.hpp"
#include "gtc/matrix_transform.hpp"
#include "gtc/type_ptr.hpp"

#include "physicsImplementation.hpp"
#include "event.hpp"
#include "constants.hpp"

#include <vector>
#include <algorithm>

namespace object {
	namespace MaterialTypes {
		enum MaterialType {
			Wood,
			Stone,
			Iron,
			Gold,
			Copper,
			Uranium,
			Coal,
			MATERIAL_TYPE_COUNT
		};
	};

	namespace ObjectHitboxTypes {
		enum ObjectHitboxType {
			Box = 1
		};
	};

	class PointLight {
	public:
		glm::vec3 position = {0.0f, 0.0f, 0.0f};
		glm::vec3 color = {1.0f, 1.0f, 1.0f};
		float intensity = 1;
		float radius = 0;
	};

	class Object {
	public:
		uint VAO, VBO, EBO;

		std::vector<float> vertices;
		std::vector<uint> indices;

		glm::vec4 objColor = {0.0f, 0.0f, 0.0f, 1.0f};

		uint shaderProgram;

		uint dimensions = 0;

		Event<float> onPreUpdate;
		Event<float> onUpdate;

		bool doesUpdate = true;
		bool doesRender = true;
		bool visible = true;
		bool shadow = true;

		std::vector<float> getPureVertices() {
			std::vector<float> pure;
			for (int i = 0; i < vertices.size(); i += RENDER_VERTEX_STRIDE) {
				pure.push_back(vertices[i]);
				pure.push_back(vertices[i+1]);
				pure.push_back(vertices[i+2]);
			}
			return pure;
		}

		void setPureVertex(glm::vec3 vertex, int i) {
			vertices[i*RENDER_VERTEX_STRIDE] = vertex.x;
			vertices[i*RENDER_VERTEX_STRIDE+1] = vertex.y;
			vertices[i*RENDER_VERTEX_STRIDE+2] = vertex.z;
		}

		// dangerous function, dont change the number of vertices or instant crash on render
		void setPureVertices(std::vector<float> pure) {
			for (int i = 0; i < pure.size() / 3; i++) {
				setPureVertex({pure[i*3],pure[i*3+1],pure[i*3+2]}, i);
			}
		}

		void updateVAOVBOEBO() {
			glBindVertexArray(VAO);

			glBindBuffer(GL_ARRAY_BUFFER, VBO);
			if (vertices.size() > 0) glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
			if (indices.size() > 0) glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint), indices.data(), GL_STATIC_DRAW);

			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, RENDER_VERTEX_STRIDE * sizeof(float), (void*)0);
			glEnableVertexAttribArray(0);

			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, RENDER_VERTEX_STRIDE * sizeof(float), (void*)(3*sizeof(float)));
			glEnableVertexAttribArray(1);

			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindVertexArray(0);
		}

		void initVAOVBOEBO() {
			glGenVertexArrays(1, &VAO);
			glGenBuffers(1, &VBO);
			glGenBuffers(1, &EBO);

			updateVAOVBOEBO();
		}

		Object(uint prog, std::vector<float> v = {}, std::vector<uint> i = {}) : shaderProgram(prog), vertices(v), indices(i) {
			initVAOVBOEBO();
		}

		virtual ~Object() {
			glDeleteVertexArrays(1, &VAO);
			glDeleteBuffers(1, &VBO);
			glDeleteBuffers(1, &EBO);
		}

		virtual void draw() {}
	};

	class Camera {
	public:
		glm::vec3 position = {0, 0, 0};
		glm::vec2 rotation = {0, 0};

		glm::vec3 forward;
		glm::vec3 right;
		glm::vec3 up;
		glm::vec3 worldUp = {0, 1, 0};

		float fov = 60;

		float nearDistance = RENDER_CAMERA_NEAR;
		float farDistance = RENDER_CAMERA_FAR;

		glm::mat4 viewMatrix;
		glm::mat4 projectionMatrix;

		GLFWwindow* boundWindow;

		bool isOrbit = false;
		float orbitDistance = 10.0f;
		glm::vec3 targetPosition = {0, 0, 0};

		void setMeaningfulPosition(glm::vec3 pos) {
			if (isOrbit) {
				targetPosition = pos;
			} else {
				position = pos;
			}
		}

		void update() {
			int w, h;
			glfwGetFramebufferSize(boundWindow, &w, &h);

			if ((w == 0) || (h == 0)) return;

			forward = glm::normalize(glm::vec3(
				cos(glm::radians(rotation.x)) * cos(glm::radians(rotation.y)),
				sin(glm::radians(rotation.y)),
				sin(glm::radians(rotation.x)) * cos(glm::radians(rotation.y))
			));
			right = glm::normalize(glm::cross(forward, worldUp));
			up = glm::normalize(glm::cross(right, forward));

			if (isOrbit) {
				position = targetPosition - forward * orbitDistance;
			}

			viewMatrix = glm::lookAt(position, (isOrbit ? targetPosition : position + forward), worldUp);
			projectionMatrix = glm::perspective(glm::radians(fov), ((float)w)/((float)h), nearDistance, farDistance);
		}

		Camera(GLFWwindow* win) : boundWindow(win) {

		}
	};

	class Object2D : public Object {
	public:
		uint objColorLoc;

		void draw() override {
			glUseProgram(shaderProgram);

			glBindVertexArray(VAO);

			glUniform4fv(objColorLoc, 1, glm::value_ptr(objColor));

			glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
		}

		Object2D(uint prog, std::vector<float> v = {}, std::vector<uint> i = {}) : Object(prog, v, i) {
			dimensions = 2;
			objColorLoc = glGetUniformLocation(shaderProgram, "objColor");
		}

		~Object2D() {

		}
	};

	class Object3D : public Object {
	public:
		glm::vec3 position = {0.0f, 0.0f, 0.0f};
		glm::vec3 rotation = {0.0f, 0.0f, 0.0f};
		glm::vec3 scale = {1.0f, 1.0f, 1.0f};

		Camera* boundCamera;
		uint mvpLoc, modelLoc, normalMatLoc, objColorLoc;

		JPH::BodyID bodyID;
		bool hasBody = false;

		float deformResistance = 1;

		bool hasMaterial = false;
		enum MaterialTypes::MaterialType materialType;
		float materialAmount = 1.0f;

		bool canBeBroken = true;

		void syncPhysics() {
			if (!hasBody) return;

			JPH::BodyInterface& bodyInterface = physics::physicsSystem.GetBodyInterface();

			JPH::RVec3 joltPos = bodyInterface.GetPosition(bodyID);
			position = glm::vec3(joltPos.GetX(), joltPos.GetY(), joltPos.GetZ());

			JPH::Quat joltRot = bodyInterface.GetRotation(bodyID);
			glm::quat q = physics::glmQuat(joltRot);
			rotation = glm::degrees(glm::eulerAngles(q));
		}

		void draw() override {
			glUseProgram(shaderProgram);

			glBindVertexArray(VAO);

			glm::mat4 model = glm::mat4(1.0f);

			model = glm::translate(model, position);

			model = glm::rotate(model, glm::radians(rotation.x), {1.0f, 0.0f, 0.0f});
			model = glm::rotate(model, glm::radians(rotation.y), {0.0f, 1.0f, 0.0f});
			model = glm::rotate(model, glm::radians(rotation.z), {0.0f, 0.0f, 1.0f});

			model = glm::scale(model, scale);

			glm::mat4 mvp = boundCamera->projectionMatrix * boundCamera->viewMatrix * model;
			glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(model)));

			glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));
			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    	glUniformMatrix3fv(normalMatLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
			glUniform4fv(objColorLoc, 1, glm::value_ptr(objColor));

			glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
		}

		Object3D(uint prog, Camera* cam, std::vector<float> v = {}, std::vector<uint> i = {}) : Object(prog, v, i), boundCamera(cam) {
			mvpLoc = glGetUniformLocation(shaderProgram, "mvp");
			modelLoc = glGetUniformLocation(shaderProgram, "model");
			normalMatLoc = glGetUniformLocation(shaderProgram, "normalMatrix");
			objColorLoc = glGetUniformLocation(shaderProgram, "objColor");
			dimensions = 3;
		}

		~Object3D() {
			if (hasBody) removePhysics();
		}

		void setModel(const char* modelJsonFile) {
			assetBridge::ModelData model = assetBridge::importJsonModel(modelJsonFile);
			vertices = model.vertices;
			indices = model.indices;
			updateVAOVBOEBO();
		}

		void addPhysics(enum ObjectHitboxTypes::ObjectHitboxType hitboxType = ObjectHitboxTypes::Box, enum JPH::EMotionType motionType = JPH::EMotionType::Dynamic, JPH::Vec3 hitboxSize = JPH::Vec3(1.0f, 1.0f, 1.0f)) {
			if (hasBody) return;
			JPH::ShapeRefC shape;

			switch (hitboxType) {
				case (ObjectHitboxTypes::Box):
					JPH::ShapeSettings* boxShapeSettings = new JPH::BoxShapeSettings(hitboxSize);
					shape = boxShapeSettings->Create().Get();
			}

			JPH::BodyCreationSettings bodySettings(
				shape,
				physics::joltVec3(position),
				physics::joltQuat(glm::quat(rotation)),
				motionType,
				(motionType == JPH::EMotionType::Static ? physics::Layers::NON_MOVING : physics::Layers::MOVING)
			);

			JPH::Body* body = physics::physicsSystem.GetBodyInterface().CreateBody(bodySettings);
			physics::physicsSystem.GetBodyInterface().AddBody(body->GetID(), JPH::EActivation::Activate);

			bodyID = body->GetID();
			hasBody = true;
		}

		void removePhysics() {
			if (!hasBody) return;
 			physics::physicsSystem.GetBodyInterface().RemoveBody(bodyID);
			physics::physicsSystem.GetBodyInterface().DestroyBody(bodyID);
			hasBody = false;
		}

		static Object3D* getObjectFromBodyID(JPH::BodyID body, std::vector<Object3D*> objs) {
			for (Object3D* obj : objs) {
				if (obj->bodyID == body) return obj;
			}
			return nullptr;
		}
	};

	struct Menu2D {
		std::vector<Object2D*> objects;

		bool wasAdded = false;
		bool needsMouse = false;

		void toggle(GLFWwindow* win, std::vector<Object2D*>& a) {
			if (wasAdded) {
				removeFromArray(win, a);
			} else {
				addToArray(win, a);
			}
		}

		void addToArray(GLFWwindow* win, std::vector<Object2D*>& a) {
			for (Object2D* obj : objects) {
				if (std::find(a.begin(), a.end(), obj) == a.end()) {
					a.push_back(obj);
				}
			}
			wasAdded = true;
			if (needsMouse) glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}

		void removeFromArray(GLFWwindow* win, std::vector<Object2D*>& a) {
			for (Object2D* obj : objects) {
				auto it = std::find(a.begin(), a.end(), obj);
				if (it != a.end()) {
					a.erase(it);
				}
			}
			wasAdded = false;
			if (needsMouse) glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		}
	};

	namespace MenuFactory {
		Menu2D placeToolMenu(uint shaderProg) {
			Menu2D menu;
			menu.needsMouse = true;

			object::Object2D* x = new object::Object2D(shaderProg, {
				-0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 0.0f,
				0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 0.0f,
				0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f,
				-0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f
			}, {
				0, 1, 2,
				0, 3, 2
			});
			menu.objects.push_back(x);

			return menu;
		}
	}
}
