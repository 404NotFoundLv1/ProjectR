// Copyright Epic Games, Inc. All Rights Reserved.

#include "Director/PRHttpDirectorProvider.h"

FName FPRHttpDirectorProvider::GetProviderId() const { return TEXT("HttpBoundary"); }
bool FPRHttpDirectorProvider::IsAvailable() const { return false; }
void FPRHttpDirectorProvider::RequestRule(const FPRDirectorRequest& Request, FPRDirectorProviderCompletion Completion) { Completion.ExecuteIfBound(FPRDirectorResponse()); }
void FPRHttpDirectorProvider::CancelRequest(FGuid RequestId) {}
