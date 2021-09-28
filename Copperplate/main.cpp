#pragma once

#include "application.h"

#include <glm\gtc\type_ptr.hpp>

#include <stdio.h>

using namespace Copperplate;

int main(int argc, char* argv[]) {
	// Setup
	Application::Init();

	
	// Main Loop
	Application::Run();
	
	// Shutdown
	return 0;
}
