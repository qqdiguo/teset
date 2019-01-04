// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IPDisplayClusterRenderManager.h"

class FDisplayClusterDeviceBase;
class FDisplayClusterNativePresentHandler;

/**
 * Render manager. Responsible for anything related to a visual part.
 */
class FDisplayClusterRenderManager
	: public IPDisplayClusterRenderManager
{
public:
	FDisplayClusterRenderManager();
	virtual ~FDisplayClusterRenderManager();

public:
	//////////////////////////////////////////////////////////////////////////////////////////////
	// IPDisplayClusterManager
	//////////////////////////////////////////////////////////////////////////////////////////////
	virtual bool Init(EDisplayClusterOperationMode OperationMode) override;
	virtual void Release() override;
	virtual bool StartSession(const FString& configPath, const FString& nodeId) override;
	virtual void EndSession() override;
	virtual void PreTick(float DeltaSeconds) override;

public:
	//////////////////////////////////////////////////////////////////////////////////////////////
	// IDisplayClusterRenderManager
	//////////////////////////////////////////////////////////////////////////////////////////////
	virtual IDisplayClusterStereoDevice* GetStereoDevice() const override;

public:
	//////////////////////////////////////////////////////////////////////////////////////////////
	// IPDisplayClusterRenderManager
	//////////////////////////////////////////////////////////////////////////////////////////////

private:
	FDisplayClusterDeviceBase* CreateStereoDevice();
	void ResizeWindow(int32 WinX, int32 WinY, int32 ResX, int32 ResY);
	void OnViewportCreatedHandler();
	void OnBeginDrawHandler();

private:
	EDisplayClusterOperationMode CurrentOperationMode;
	FString ConfigPath;
	FString ClusterNodeId;

	// Interface pointer to eliminate type casting
	IDisplayClusterStereoDevice* Device = nullptr;
	FDisplayClusterNativePresentHandler* NativePresentHandler;
	bool bWindowAdjusted = false;
};

