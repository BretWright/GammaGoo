// Copyright 2026 Bret Wright. All Rights Reserved.
// UBuildComponent handles ghost placement, grid snapping, and tower spawning.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BuildComponent.generated.h"

class AFluidTowerBase;
class UFluidSubsystem;
class UResourceSubsystem;
class UStaticMeshComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GAMMAGOO_API UBuildComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBuildComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Build")
	void ToggleBuildMode();

	UFUNCTION(BlueprintCallable, Category = "Build")
	void SelectTowerType(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "Build")
	void ConfirmPlacement();

	UFUNCTION(BlueprintCallable, Category = "Build")
	void CancelBuild();

	UFUNCTION(BlueprintPure, Category = "Build")
	bool IsInBuildMode() const { return bBuildModeActive; }

protected:
	/** Tower classes the player can build. Order matches UI radial menu. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Build")
	TArray<TSubclassOf<AFluidTowerBase>> TowerClasses;

	/** Max terrain slope for placement (dot product of surface normal vs up). 0.87 ~ 30 degrees. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Build")
	float MaxSlopeDot = 0.87f;

	/** Max fluid volume at placement cell for it to count as "dry." */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Build")
	float MaxFluidForPlacement = 5.f;

private:
	bool bBuildModeActive = false;
	int32 SelectedTowerIndex = 0;
	bool bPlacementValid = false;
	FVector PlacementLocation = FVector::ZeroVector;
	FRotator PlacementRotation = FRotator::ZeroRotator;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> GhostMesh;

	UPROPERTY()
	TObjectPtr<UFluidSubsystem> FluidSubsystem = nullptr;

	UPROPERTY()
	TObjectPtr<UResourceSubsystem> ResourceSubsystem = nullptr;

	void UpdateGhostPosition();
	bool ValidatePlacement(const FVector& Location, const FHitResult& Hit) const;
	void SetGhostColor(bool bValid);
};
