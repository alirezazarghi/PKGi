// Based on https://github.com/psxdev/liborbis/blob/master/liborbisPad/source/orbisPad.c

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "Controller.h"

// Initialize the controller
Controller::Controller() {
	// Initialize the Pad library
	if (scePadInit() != 0)
	{
		printf("[ERROR] Failed to initialize pad library!\n");
		return;
	}

	// Get the user ID
	OrbisUserServiceInitializeParams param;
	param.priority = ORBIS_KERNEL_PRIO_FIFO_LOWEST;
	sceUserServiceInitialize(&param);
	sceUserServiceGetInitialUser(&userID);

	// Open a handle for the controller
	padID = scePadOpen(userID, 0, 0, NULL);

	if (padID < 0)
	{
		printf("[ERROR] Failed to open pad!\n");
		return;
	}
}

// Uninitialize controller
Controller::~Controller() {
	scePadClose(padID);
}

// Controller : Get current button pressed
unsigned int Controller::GetCurrentButtonsPressed()
{
    return buttonsPressed;
}

// Controller : Set current pressed button
void Controller::SetCurrentButtonsPressed(unsigned int buttons)
{
    buttonsPressed = buttons;
}

// Controller : Get current released button
unsigned int Controller::GetCurrentButtonsReleased()
{
    return buttonsReleased;
}

// Controller : Set released button
void Controller::SetCurrentButtonsReleased(unsigned int buttons)
{
    buttonsReleased = buttons;
}

// Controller : Hold button
bool Controller::GetButtonHold(unsigned int filter)
{
    if ((buttonsHold & filter) == filter)
    {
        return 1;
    }
    return 0;
}

// Controller : Pressed button
bool Controller::GetButtonPressed(unsigned int filter)
{
    if ((buttonsPressed & filter) == filter)
    {
        return 1;
    }
    return 0;
}

// Controller : Released button
bool Controller::GetButtonReleased(unsigned int filter)
{
    if ((buttonsReleased & filter) == filter)
    {
        if (~(padData.buttons) & filter)
        {
            return 0;
        }
        return 1;
    }
    return 0;
}

// Update controllers value
void Controller::Update() {

    unsigned int actualButtons = 0;
    unsigned int lastButtons = 0;

    lastButtons = padData.buttons;
    scePadReadState(padID, &padData);
    actualButtons = padData.buttons;

    buttonsPressed = (actualButtons) & (~lastButtons);
    if (actualButtons != lastButtons)
    {
        buttonsReleased = (~actualButtons) & (lastButtons);
    }
    else
    {
        buttonsReleased = 0;
    }

    curButtons = padData.buttons;
    buttonsHold = actualButtons & lastButtons;
}