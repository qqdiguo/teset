// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "KismetNodes/SGraphNodeK2Default.h"

class SGraphNodeSpawnActorFromClass : public SGraphNodeK2Default
{
public:

	// SGraphNode interface
	virtual void CreatePinWidgets() override;
	// End of SGraphNode interface
};
