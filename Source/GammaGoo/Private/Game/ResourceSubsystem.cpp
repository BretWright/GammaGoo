// Copyright 2026 Bret Wright. All Rights Reserved.

#include "Game/ResourceSubsystem.h"

void UResourceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Currency = StartingCurrency;
}

void UResourceSubsystem::AddCurrency(float Amount)
{
	if (Amount <= 0.f) { return; }
	Currency += Amount;
	OnResourceChanged.Broadcast(Currency);
}

bool UResourceSubsystem::SpendCurrency(float Amount)
{
	if (Amount <= 0.f || Currency < Amount) { return false; }
	Currency -= Amount;
	OnResourceChanged.Broadcast(Currency);
	return true;
}
