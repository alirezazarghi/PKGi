#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <math.h>

#include "App.h"

int main()
{
	// Start the Application
	Application* mainApp = new Application();
	mainApp->Run();
	delete mainApp;

    return 0;
}
