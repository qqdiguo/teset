// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "HTML5/HTML5PlatformInput.h"

THIRD_PARTY_INCLUDES_START
	#include <SDL.h>
THIRD_PARTY_INCLUDES_END

uint32 FHTML5PlatformInput::GetKeyMap(uint32* KeyCodes, FString* KeyNames, uint32 MaxMappings)
{
#define ADDKEYMAP(KeyCode, KeyName)		if (NumMappings<MaxMappings) { KeyCodes[NumMappings]=KeyCode; KeyNames[NumMappings]=KeyName; ++NumMappings; };

	uint32 NumMappings = 0;

	if (KeyCodes && KeyNames && (MaxMappings > 0))
	{
		ADDKEYMAP(SDL_SCANCODE_BACKSPACE, TEXT("BackSpace"));
		ADDKEYMAP(SDL_SCANCODE_TAB, TEXT("Tab"));
		ADDKEYMAP(SDL_SCANCODE_RETURN, TEXT("Enter"));
		ADDKEYMAP(SDL_SCANCODE_RETURN2, TEXT("Enter"));
		ADDKEYMAP(SDL_SCANCODE_KP_ENTER, TEXT("Enter"));
		ADDKEYMAP(SDL_SCANCODE_PAUSE, TEXT("Pause"));

		ADDKEYMAP(SDL_SCANCODE_ESCAPE, TEXT("Escape"));
		ADDKEYMAP(SDL_SCANCODE_SPACE, TEXT("SpaceBar"));
		ADDKEYMAP(SDL_SCANCODE_PAGEUP, TEXT("PageUp"));
		ADDKEYMAP(SDL_SCANCODE_PAGEDOWN, TEXT("PageDown"));
		ADDKEYMAP(SDL_SCANCODE_END, TEXT("End"));
		ADDKEYMAP(SDL_SCANCODE_HOME, TEXT("Home"));

		ADDKEYMAP(SDL_SCANCODE_LEFT, TEXT("Left"));
		ADDKEYMAP(SDL_SCANCODE_UP, TEXT("Up"));
		ADDKEYMAP(SDL_SCANCODE_RIGHT, TEXT("Right"));
		ADDKEYMAP(SDL_SCANCODE_DOWN, TEXT("Down"));

		ADDKEYMAP(SDL_SCANCODE_INSERT, TEXT("Insert"));
		ADDKEYMAP(SDL_SCANCODE_DELETE, TEXT("Delete"));

		ADDKEYMAP(SDL_SCANCODE_F1, TEXT("F1"));
		ADDKEYMAP(SDL_SCANCODE_F2, TEXT("F2"));
		ADDKEYMAP(SDL_SCANCODE_F3, TEXT("F3"));
		ADDKEYMAP(SDL_SCANCODE_F4, TEXT("F4"));
		ADDKEYMAP(SDL_SCANCODE_F5, TEXT("F5"));
		ADDKEYMAP(SDL_SCANCODE_F6, TEXT("F6"));
		ADDKEYMAP(SDL_SCANCODE_F7, TEXT("F7"));
		ADDKEYMAP(SDL_SCANCODE_F8, TEXT("F8"));
		ADDKEYMAP(SDL_SCANCODE_F9, TEXT("F9"));
		ADDKEYMAP(SDL_SCANCODE_F10, TEXT("F10"));
		ADDKEYMAP(SDL_SCANCODE_F11, TEXT("F11"));
		ADDKEYMAP(SDL_SCANCODE_F12, TEXT("F12"));

		ADDKEYMAP(SDL_SCANCODE_LCTRL, TEXT("LeftControl"));
		ADDKEYMAP(SDL_SCANCODE_LSHIFT, TEXT("LeftShift"));
		ADDKEYMAP(SDL_SCANCODE_LALT, TEXT("LeftAlt"));
		ADDKEYMAP(SDL_SCANCODE_LGUI, TEXT("LeftCommand"));
		ADDKEYMAP(SDL_SCANCODE_RCTRL, TEXT("RightControl"));
		ADDKEYMAP(SDL_SCANCODE_RSHIFT, TEXT("RightShift"));
		ADDKEYMAP(SDL_SCANCODE_RALT, TEXT("RightAlt"));
		ADDKEYMAP(SDL_SCANCODE_RGUI, TEXT("RightCommand"));

		ADDKEYMAP(SDL_SCANCODE_KP_0, TEXT("NumPadZero"));
		ADDKEYMAP(SDL_SCANCODE_KP_1, TEXT("NumPadOne"));
		ADDKEYMAP(SDL_SCANCODE_KP_2, TEXT("NumPadTwo"));
		ADDKEYMAP(SDL_SCANCODE_KP_3, TEXT("NumPadThree"));
		ADDKEYMAP(SDL_SCANCODE_KP_4, TEXT("NumPadFour"));
		ADDKEYMAP(SDL_SCANCODE_KP_5, TEXT("NumPadFive"));
		ADDKEYMAP(SDL_SCANCODE_KP_6, TEXT("NumPadSix"));
		ADDKEYMAP(SDL_SCANCODE_KP_7, TEXT("NumPadSeven"));
		ADDKEYMAP(SDL_SCANCODE_KP_8, TEXT("NumPadEight"));
		ADDKEYMAP(SDL_SCANCODE_KP_9, TEXT("NumPadNine"));
		ADDKEYMAP(SDL_SCANCODE_KP_MULTIPLY, TEXT("Multiply"));
		ADDKEYMAP(SDL_SCANCODE_KP_PLUS, TEXT("Add"));
		ADDKEYMAP(SDL_SCANCODE_KP_MINUS, TEXT("Subtract"));
		ADDKEYMAP(SDL_SCANCODE_KP_DECIMAL, TEXT("Decimal"));
		ADDKEYMAP(SDL_SCANCODE_KP_DIVIDE, TEXT("Divide"));
		ADDKEYMAP(SDL_SCANCODE_KP_PERIOD, TEXT("Period"));

		ADDKEYMAP(SDL_SCANCODE_CAPSLOCK, TEXT("CapsLock"));
		ADDKEYMAP(SDL_SCANCODE_NUMLOCKCLEAR, TEXT("NumLock"));
		ADDKEYMAP(SDL_SCANCODE_SCROLLLOCK, TEXT("ScrollLock"));
		ADDKEYMAP(SDL_SCANCODE_CANCEL, TEXT("Cancel")); // UE-58441 -- CTRL+Pause/Break and CTRL+ScrollLock
		ADDKEYMAP(SDL_SCANCODE_APPLICATION, TEXT("Menu")); // UE-58441 -- contextual menu
	}
#undef ADDKEYMAP

	check(NumMappings < MaxMappings);
	return NumMappings;
}

uint32 FHTML5PlatformInput::GetCharKeyMap(uint32* KeyCodes, FString* KeyNames, uint32 MaxMappings)
{
	return FGenericPlatformInput::GetStandardPrintableKeyMap(KeyCodes, KeyNames, MaxMappings, false, true);
}
