#pragma once

#include <algorithm>
#include <vector>

#include "glad.h"
#include "glfw3.h"
#include "glm.hpp"

#include "event.hpp"
#include "constants.hpp"

namespace input {
	class InputHandler {
	public:
		std::vector<int> keysToTrack;
		std::vector<int> pressedKeys;
		std::vector<int> mouseButtonsToTrack;
		std::vector<int> pressedMouseButtons;

		GLFWwindow* boundWindow;

		Event<int> onKeyStartPress;
		Event<int> onMouseButtonStartPress;
		Event<int> whileKeyHold;
		Event<int> whileMouseButtonHold;
		Event<glm::vec2> onMouseMovement;

		glm::vec2 lastMousePos = {0, 0};

		bool isKeyPressed(int key) {
			return std::find(pressedKeys.begin(), pressedKeys.end(), key) != pressedKeys.end();
		}

		bool isMouseButtonPressed(int button) {
			return std::find(pressedMouseButtons.begin(), pressedMouseButtons.end(), button) != pressedMouseButtons.end();
		}

		void update() {
			double sX, sY;
			glfwGetCursorPos(boundWindow, &sX, &sY);

			int w, h;
			glfwGetFramebufferSize(boundWindow, &w, &h);

			glm::vec2 mousePos = {sX / w, 1 - sY / h};
			glm::vec2 mouseDelta = mousePos - lastMousePos;

			lastMousePos = mousePos;


			if (mouseDelta.length() > 0) {
				onMouseMovement.broadcast(mouseDelta);
			}


			for (int k : keysToTrack) {
				int state = glfwGetKey(boundWindow, k);
				if (state == GLFW_PRESS) {
					if (!isKeyPressed(k)) {
						onKeyStartPress.broadcast(k);
						pressedKeys.push_back(k);
					}
					whileKeyHold.broadcast(k);
				} else {
					if (isKeyPressed(k)) {
						pressedKeys.erase(std::find(pressedKeys.begin(), pressedKeys.end(), k));
					}
				}
			}

			for (int b : mouseButtonsToTrack) {
				int state = glfwGetMouseButton(boundWindow, b);
				if (state == GLFW_PRESS) {
					if (!isMouseButtonPressed(b)) {
						onMouseButtonStartPress.broadcast(b);
						pressedMouseButtons.push_back(b);
					}
					whileMouseButtonHold.broadcast(b);
				} else {
					if (isMouseButtonPressed(b)) {
						pressedMouseButtons.erase(std::find(pressedMouseButtons.begin(), pressedMouseButtons.end(), b));
					}
				}
			}
		}

		InputHandler(GLFWwindow* win) : boundWindow(win) {
			glfwSetInputMode(boundWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			if (glfwRawMouseMotionSupported()) {
				glfwSetInputMode(boundWindow, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
			}
		};
	};
}
