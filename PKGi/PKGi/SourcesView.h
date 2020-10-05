#include "View.h"
#include "Graphics.h"

#ifndef SOURCES_VIEW_H
#define SOURCES_VIEW_H

class Application;
class SourcesView;

typedef struct  {
	char name[100];
	char api_url[100];
} Source;

class SourcesView : public View
{
public:
	SourcesView(Application* app);
	~SourcesView();
	int Update(void);
	int Render(void);

private:
	Application* App;

	// Background and foreground colors
	Color bgColor;
	Color fgColor;

	// Package list system
	OrbisPthreadMutex sources_mtx;

	Source* sources;
	int sourceNbr;
	int sourceSelected;

	char errorMessage[255];
	bool onError;

	// Fetch animation
	int fetchStepAnim;
	int fetchTimeStep;

	void ShowError(const char* error);
};
#endif