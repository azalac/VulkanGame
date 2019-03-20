
#include "Window.hpp"

int GLFWManager::instances = 0;

std::map<GLFWwindow*, InputHandler*> InputHandler::handlers;
