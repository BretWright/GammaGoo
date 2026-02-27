// Copyright 2026 Bret Wright. All Rights Reserved.
// Player character with fluid depth interaction, heat lance, and build component.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "FluidDefenseCharacter.generated.h"

class UFluidSubsystem;
class UBuildComponent;
class UFluidSiegeHUD;
class UInputAction;
class UInputMappingContext;

UENUM(BlueprintType)
enum class EFluidDepthTier : uint8
{
	Dry,        // No fluid
	Ankle,      // 0-30cm: 80% speed, no damage
	Knee,       // 30-70cm: 50% speed, 5 HP/s
	Waist,      // 70-120cm: 30% speed, 15 HP/s
	Chest       // 120cm+: 15% speed, 30 HP/s
};

UCLASS(BlueprintType, Blueprintable)
class GAMMAGOO_API AFluidDefenseCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AFluidDefenseCharacter();

	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UFUNCTION(BlueprintPure, Category = "Player|Fluid")
	EFluidDepthTier GetCurrentDepthTier() const { return CurrentDepthTier; }

	UFUNCTION(BlueprintPure, Category = "Player|Fluid")
	float GetCurrentFluidDepth() const { return CurrentFluidDepth; }

	UFUNCTION(BlueprintPure, Category = "Player|HeatLance")
	float GetEnergyPercent() const { return MaxEnergy > 0.f ? Energy / MaxEnergy : 0.f; }

protected:
	// --- Build Component ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player|Build")
	TObjectPtr<UBuildComponent> BuildComponent;

	// --- HUD ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|UI")
	TSubclassOf<UFluidSiegeHUD> HUDWidgetClass;

	UPROPERTY(BlueprintReadOnly, Category = "Player|UI")
	TObjectPtr<UFluidSiegeHUD> HUDWidget;

	// --- Enhanced Input ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Input")
	TObjectPtr<UInputAction> FireAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Input")
	TObjectPtr<UInputAction> BuildToggleAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Input")
	TObjectPtr<UInputMappingContext> FluidSiegeMappingContext;

	// --- Fluid Interaction ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Fluid")
	float FluidCheckInterval = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Health")
	float PlayerHealth = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Health")
	float MaxPlayerHealth = 100.f;

	// --- Heat Lance ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|HeatLance")
	float LanceRange = 1500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|HeatLance")
	float LanceRadius = 150.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|HeatLance")
	float LanceEvaporateRate = 15.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|HeatLance")
	float Energy = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|HeatLance")
	float MaxEnergy = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|HeatLance")
	float EnergyDrainRate = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|HeatLance")
	float EnergyRechargeRate = 5.f;

private:
	// --- Fluid depth ---
	void CheckFluidDepth();
	EFluidDepthTier ComputeDepthTier(float Depth) const;
	void ApplyDepthEffects(float DeltaTime);

	FTimerHandle FluidCheckTimerHandle;
	EFluidDepthTier CurrentDepthTier = EFluidDepthTier::Dry;
	float CurrentFluidDepth = 0.f;

	// --- Heat lance ---
	void StartFiring();
	void StopFiring();
	void FireLanceTick();

	bool bFiring = false;
	FTimerHandle LanceTickHandle;

	// --- Input ---
	void OnBuildToggle();

	UPROPERTY()
	TObjectPtr<UFluidSubsystem> FluidSubsystem = nullptr;

	/** Cached default walk speed from CharacterMovement. */
	float DefaultWalkSpeed = 600.f;
};
