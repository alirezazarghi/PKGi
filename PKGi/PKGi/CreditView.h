#include "View.h"
#include "Graphics.h"

#ifndef CREDIT_VIEW_H
#define CREDIT_VIEW_H

class Application;

class CreditView : public View
{
public:
	CreditView(Application* app);
	~CreditView();
	int Update(void);
	int Render(void);
private:
	Application* App;

	// Background and foreground colors
	Color bgColor;
	Color fgColor;
};
#endif