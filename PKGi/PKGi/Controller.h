#include <stdint.h>
#include <orbis/libkernel.h>
#include <orbis/Pad.h>
#include <orbis/UserService.h>

#ifndef CONTROLLER_H
#define CONTROLLER_H

class Controller
{
public:
	Controller();
	~Controller();
	void Update();
	unsigned int GetCurrentButtonsPressed();
	void SetCurrentButtonsPressed(unsigned int buttons);
	unsigned int GetCurrentButtonsReleased();
	void SetCurrentButtonsReleased(unsigned int buttons);
	bool GetButtonHold(unsigned int filter);
	bool GetButtonPressed(unsigned int filter);
	bool GetButtonReleased(unsigned int filter);
private:
	int userID;
	int padID;
	unsigned int buttonsPressed;
	unsigned int buttonsReleased;
	unsigned int curButtons;
	unsigned int buttonsHold;
	OrbisPadData padData;
};

#endif