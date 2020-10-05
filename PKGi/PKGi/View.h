#ifndef VIEW_H
#define VIEW_H

#define HEADER_SIZE 200
#define FOOTER_SIZE 100
#define BORDER_X 100

#define MENU_BORDER 10
#define MENU_IN_IMAGE_BORDER 10
#define MENU_NBR_PER_PAGE 5

#define FOOTER_ICON_SIZE 35
#define FOOTER_BORDER_Y 25
#define FOOTER_TEXT_BORDER 10
#define FOOTER_TEXT_SIZE 20

class View
{
public:
	virtual int Update(void) = 0;
	virtual int Render(void) = 0;
};

#endif