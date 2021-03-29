#pragma once

#include <stdio.h>
#include "application.h"
#include <glm\gtc\type_ptr.hpp>

using namespace Copperplate;

int main(int argc, char* argv[]) {
	// Setup
	Application::Init();

	
	// Main Loop
	Application::Run();
	
	// Shutdown
	return 0;
}