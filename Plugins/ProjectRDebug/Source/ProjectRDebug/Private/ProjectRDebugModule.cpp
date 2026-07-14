// Copyright Epic Games, Inc. All Rights Reserved.

#include "Modules/ModuleManager.h"

#if UE_BUILD_SHIPPING
#error ProjectRDebug must never be compiled into a Shipping target.
#endif

IMPLEMENT_MODULE(FDefaultModuleImpl, ProjectRDebug)
