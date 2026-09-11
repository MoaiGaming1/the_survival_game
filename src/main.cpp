#define GLM_ENABLE_EXPERIMENTAL

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

#include "glad.h"
#include "glfw3.h"
#include "glm.hpp"
#include "gtc/matrix_transform.hpp"
#include "gtc/type_ptr.hpp"
#include "gtx/norm.hpp"

#include "constants.hpp"
#include "assetBridge.hpp"
#include "object.hpp"
#include "physicsImplementation.hpp"
#include "inputHandler.hpp"
#include "deform.hpp"
#include "mapGeneration.hpp"

std::vector<object::Object3D*> objects3D;
std::vector<object::PointLight*> pointLights;
object::Camera* camera;
input::InputHandler* inputHandler;

void framebufferSizeCallback(GLFWwindow* win, int x, int y) {
	glViewport(0, 0, x, y);
}

void checkShaderErrors(uint shader, const char* shaderErrorAlias) {
	int success;
	char infoLog[512];

	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(shader, 512, NULL, infoLog);
		std::cerr << "ERROR ON \"" << shaderErrorAlias << "\": " << infoLog << "\n";
	}
}

int main() {
	std::cout << "initializing window\n";

	{
		glfwInit();
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		glfwWindowHint(GLFW_SAMPLES, RENDER_MULTISAMPLING_SAMPLE_COUNT);
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	}

	GLFWwindow* window = glfwCreateWindow(WINDOW_DEFAULT_SIZE.x, WINDOW_DEFAULT_SIZE.y, WINDOW_TITLE, NULL, NULL);
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cerr << "GLAD LOAD FAIL\n";
		return EXIT_FAILURE;
	}

	
	std::cout << "creating shaders\n";

	uint shaderProg3D;
	uint depthMapFBO, depthMap, depthShaderProg;
	uint numPointLightsLoc;
	// shaders and shadows
	{
		std::string _vertSource3D = assetBridge::readFile("assets/shaders/vert3d.vert");
		std::string _fragSource3D = assetBridge::readFile("assets/shaders/frag3d.frag");

		const char* vertSource3D = _vertSource3D.c_str();
		const char* fragSource3D = _fragSource3D.c_str();

		uint v3d = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(v3d, 1, &vertSource3D, NULL);
		glCompileShader(v3d);
		checkShaderErrors(v3d, "vertex 3d");

		uint f3d = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(f3d, 1, &fragSource3D, NULL);
		glCompileShader(f3d);
		checkShaderErrors(f3d, "fragment 3d");

		shaderProg3D = glCreateProgram();
		glAttachShader(shaderProg3D, v3d);
		glAttachShader(shaderProg3D, f3d);
		glLinkProgram(shaderProg3D);

		glDeleteShader(v3d);
		glDeleteShader(f3d);

		glUseProgram(shaderProg3D);

		numPointLightsLoc = glGetUniformLocation(shaderProg3D, "numPointLights");

		// shadow depth map and stuff
		glGenFramebuffers(1, &depthMapFBO);

		glGenTextures(1, &depthMap);
		glBindTexture(GL_TEXTURE_2D, depthMap);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, LIGHTING_SHADOW_RES.x * LIGHTING_SHADOW_RANGE, LIGHTING_SHADOW_RES.y * LIGHTING_SHADOW_RANGE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

		glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		std::string _depthVertSource = assetBridge::readFile("assets/shaders/depth.vert");
		std::string _depthFragSource = assetBridge::readFile("assets/shaders/depth.frag");

		const char* depthVertSource = _depthVertSource.c_str();
		const char* depthFragSource = _depthFragSource.c_str();

		uint depthVert = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(depthVert, 1, &depthVertSource, NULL);
		glCompileShader(depthVert);
		checkShaderErrors(depthVert, "depth vert");

		uint depthFrag = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(depthFrag, 1, &depthFragSource, NULL);
		glCompileShader(depthFrag);
		checkShaderErrors(depthFrag, "depth frag");

		depthShaderProg = glCreateProgram();
		glAttachShader(depthShaderProg, depthVert);
		glAttachShader(depthShaderProg, depthFrag);
		glLinkProgram(depthShaderProg);

		glDeleteShader(depthVert);
		glDeleteShader(depthFrag);
	}

	std::cout << "setting up extra stuff\n";

	camera = new object::Camera(window);
	inputHandler = new input::InputHandler(window);
	inputHandler->keysToTrack = {
		GLFW_KEY_W,
		GLFW_KEY_A,
		GLFW_KEY_S,
		GLFW_KEY_D,
		GLFW_KEY_L
	};

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glEnable(GL_MULTISAMPLE);

	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	physics::init();

	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	object::Object3D* character = new object::Object3D(shaderProg3D, camera);
	character->setModel("assets/models/Cube.json");
	character->addPhysics(object::ObjectHitboxType::Box, JPH::EMotionType::Dynamic);
	character->objColor = {0.7f, 0.8f, 0.9f, 1.0f};
	character->visible = false;
	objects3D.push_back(character);

	object::Object3D* suzanne = new object::Object3D(shaderProg3D, camera);
	suzanne->setModel("assets/models/Suzanne.json");
	suzanne->addPhysics(object::ObjectHitboxType::Box, JPH::EMotionType::Dynamic);
	suzanne->objColor = {1.0f, 0.5f, 0.25f, 1.0f};
	objects3D.push_back(suzanne);

  object::Object3D* floorObj = new object::Object3D(shaderProg3D, camera);
	floorObj->setModel("assets/models/Cube.json");
  floorObj->scale = glm::vec3(20.0f, 0.5f, 20.0f);
  floorObj->position = glm::vec3(0.0f, -10.0f, 0.0f);
	floorObj->objColor = {0.8f, 0.8f, 0.8f, 1.0f};
	floorObj->addPhysics(object::ObjectHitboxType::Box, JPH::EMotionType::Static, {20.0f, 0.5f, 20.0f});
  objects3D.push_back(floorObj);

	std::cout << "starting the render loop\n";

	float renderDT = 0;

	bool debugFlyMode = false;
	character->onPreUpdate.addListener([&](float dt) -> void {
		glm::vec3 flatForward = glm::normalize(glm::vec3(camera->forward.x, 0.0f, camera->forward.z));
		glm::vec3 flatRight = glm::normalize(glm::vec3(camera->right.x, 0.0f, camera->right.z));
		glm::vec2 moveDir = {0,0};

		if (inputHandler->isKeyPressed(GLFW_KEY_W)) moveDir[0] += 1;
		if (inputHandler->isKeyPressed(GLFW_KEY_S)) moveDir[0] -= 1;
		if (inputHandler->isKeyPressed(GLFW_KEY_D)) moveDir[1] += 1;
		if (inputHandler->isKeyPressed(GLFW_KEY_A)) moveDir[1] -= 1;

		if (glm::length(moveDir) > 0.0f) moveDir = glm::normalize(moveDir);

		if (!debugFlyMode) {
			JPH::Vec3 newPosInDt = physics::joltVec3(character->position + (moveDir[0] * flatForward + moveDir[1] * flatRight) * CHARACTER_SPEED) + JPH::Vec3(0.0f, physics::physicsSystem.GetBodyInterface().GetLinearVelocity(character->bodyID).GetY(), 0.0f);
			physics::physicsSystem.GetBodyInterface().MoveKinematic(character->bodyID, newPosInDt, physics::joltQuat(character->rotation), 1);
		} else {
			character->position += (camera->forward * moveDir[0] + camera->right * moveDir[1]) * CHARACTER_DEBUG_FLY_SPEED * dt;
		}
	});
	inputHandler->onKeyStartPress.addListener([&](int k) -> void {
		if (k == GLFW_KEY_L) {
			//std::cout << "key pressed\n";
			debugFlyMode = !debugFlyMode;
			if (debugFlyMode) {
				//std::cout << "started fly\n";
				character->removePhysics();
			} else {
				//std::cout << "stopped fly\n";
				character->addPhysics(object::ObjectHitboxType::Box, JPH::EMotionType::Dynamic);
			}
		}
	});

	inputHandler->onMouseMovement.addListener([&](glm::vec2 delta) -> void {
		camera->rotation += delta * CHARACTER_MOUSE_SENSITIVITY;
	});

	mapGen::Map* map = mapGen::generateMap(shaderProg3D, camera, 10);
	objects3D.push_back(map->terrain);
	map->terrain->objColor = {0.0f, 1.0f, 1.0f, 1.0f};
	for (object::Object3D* tree : map->trees) {
		tree->objColor = {1.0f, 0.9f, 0.5f, 1.0f};
		objects3D.push_back(tree);
	}

	// here i set sun uniforms, setting once for optimization since they dont change
	{
		glUniform3f(glGetUniformLocation(shaderProg3D, "sunDirection"), LIGHTING_SUN_DIRECTION.x, LIGHTING_SUN_DIRECTION.y, LIGHTING_SUN_DIRECTION.z);
		glUniform1f(glGetUniformLocation(shaderProg3D, "sunIntensity"), LIGHTING_SUN_INTENSITY);
		glUniform4f(glGetUniformLocation(shaderProg3D, "sunColor"), LIGHTING_SUN_COLOR.r, LIGHTING_SUN_COLOR.g, LIGHTING_SUN_COLOR.b, LIGHTING_SUN_COLOR.a);
		glUniform1f(glGetUniformLocation(shaderProg3D, "ambientStrength"), LIGHTING_AMBIENT_STRENGTH);
	}

	while (!glfwWindowShouldClose(window)) {
		double renderStartTime = glfwGetTime();

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		inputHandler->update();
		
		camera->rotation.x = std::fmod(camera->rotation.x, 360.0f);
		camera->rotation.y = glm::clamp(camera->rotation.y, -89.9f, 89.9f);
		camera->position = glm::mix(camera->position, character->position, renderDT * CHARACTER_CAMERA_LERP_SPEED);
		camera->update();
		character->rotation = glm::vec3(360 - camera->rotation.y, 360 - camera->rotation.x, 0.0f);
		glm::vec3 camMeaningfulPos = camera->position;

		physics::update((float)glfwGetTime());
		
		// shadow pass
		{
			glm::vec3 sunDir = glm::normalize(LIGHTING_SUN_DIRECTION);
			glm::mat4 lightProjection = glm::ortho(-LIGHTING_SHADOW_RANGE, LIGHTING_SHADOW_RANGE, -LIGHTING_SHADOW_RANGE, LIGHTING_SHADOW_RANGE, 1.0f, 100.0f);
			glm::mat4 lightView = glm::lookAt(
				camMeaningfulPos -sunDir * 50.0f,
				camMeaningfulPos,
				camera->worldUp
			);
			glm::mat4 lightSpaceMatrix = lightProjection * lightView;

			glViewport(0, 0, LIGHTING_SHADOW_RES.x * LIGHTING_SHADOW_RANGE, LIGHTING_SHADOW_RES.y * LIGHTING_SHADOW_RANGE);
			glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
			glClear(GL_DEPTH_BUFFER_BIT);
			
			glUseProgram(depthShaderProg);
			glUniformMatrix4fv(glGetUniformLocation(depthShaderProg, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));

			for (object::Object3D* obj : objects3D) {
				if (!obj->doesRender || !obj->shadow) continue;
				glm::mat4 model = glm::mat4(1.0f);
				model = glm::translate(model, obj->position);
				model = glm::rotate(model, glm::radians(obj->rotation.x), {1.0f, 0.0f, 0.0f});
				model = glm::rotate(model, glm::radians(obj->rotation.y), {0.0f, 1.0f, 0.0f});
				model = glm::rotate(model, glm::radians(obj->rotation.z), {0.0f, 0.0f, 1.0f});
				model = glm::scale(model, obj->scale);

				glUniformMatrix4fv(glGetUniformLocation(depthShaderProg, "model"), 1, GL_FALSE, glm::value_ptr(model));

				glBindVertexArray(obj->VAO);
				glDrawElements(GL_TRIANGLES, obj->indices.size(), GL_UNSIGNED_INT, 0);
			}

			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			int winWidth, winHeight;
			glfwGetFramebufferSize(window, &winWidth, &winHeight);
			glViewport(0, 0, winWidth, winHeight);

			glUseProgram(shaderProg3D);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, depthMap);
			glUniform1i(glGetUniformLocation(shaderProg3D, "shadowMap"), 0);
			glUniformMatrix4fv(glGetUniformLocation(shaderProg3D, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
		}

		// point light calculations
		{
			std::vector<object::PointLight*> sorted;

			for (object::PointLight* light : pointLights) {
				float distSqr = glm::distance2(light->position, camera->position);
				float maxRelevantDist = (light->radius > 0.0f) ? light->radius : LIGHTING_MAX_POINT_LIGHT_FALLBACK_RANGE;

				if (distSqr <= maxRelevantDist * maxRelevantDist) {
					sorted.push_back(light);
				}
			}

			std::sort(sorted.begin(), sorted.end(), [&](object::PointLight* a, object::PointLight* b) -> bool {
				float dA = glm::distance2(a->position, camera->position);
				float dB = glm::distance2(b->position, camera->position);
				return dA < dB;
			});

			int numPointLights = std::min((int)sorted.size(), LIGHTING_MAX_POINT_LIGHTS);
			glUniform1i(numPointLightsLoc, numPointLights);

			for (int i = 0; i < numPointLights; i++) {
				std::string base = "pointLights[" + std::to_string(i) + "]";

				glUniform3f(glGetUniformLocation(shaderProg3D, (base + ".position").c_str()),
					sorted[i]->position.x, sorted[i]->position.y, sorted[i]->position.z
				);

				glUniform3f(glGetUniformLocation(shaderProg3D, (base + ".color").c_str()),
					sorted[i]->color.r, sorted[i]->color.g, sorted[i]->color.b
				);

				glUniform1f(glGetUniformLocation(shaderProg3D, (base + ".intensity").c_str()), sorted[i]->intensity);

				glUniform1f(glGetUniformLocation(shaderProg3D, (base + ".radius").c_str()), sorted[i]->radius);
			}
		}

		// render pass
		{
			for (object::Object3D* obj : objects3D) {
				if (!obj->doesUpdate) continue;
				if (renderDT != 0) obj->onPreUpdate.broadcast(renderDT);
				obj->syncPhysics();
				if (obj->doesRender && obj->visible) {
					obj->draw();
				}
				if (renderDT != 0) obj->onUpdate.broadcast(renderDT);
			}

			glfwSwapBuffers(window);
			glfwPollEvents();
		}

		renderDT = (float)(glfwGetTime() - renderStartTime);
	}

	delete camera;

	physics::shutdown();

	return EXIT_SUCCESS;
}
