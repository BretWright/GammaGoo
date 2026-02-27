// Copyright 2026 Bret Wright. All Rights Reserved.
// UResourceSubsystem tracks player currency. All spending and earning goes through this class.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ResourceSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnResourceChanged, float, NewAmount);

UCLASS()
class GAMMAGOO_API UResourceSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintCallable, Category = "Resource")
	void AddCurrency(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Resource")
	bool SpendCurrency(float Amount);

	UFUNCTION(BlueprintPure, Category = "Resource")
	float GetCurrency() const { return Currency; }

	UFUNCTION(BlueprintPure, Category = "Resource")
	bool CanAfford(float Amount) const { return Currency >= Amount; }

	UPROPERTY(BlueprintAssignable, Category = "Resource")
	FOnResourceChanged OnResourceChanged;

private:
	UPROPERTY()
	float Currency = 0.f;

	static constexpr float StartingCurrency = 500.f;
};
