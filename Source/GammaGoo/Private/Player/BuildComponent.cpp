// Copyright 2026 Bret Wright. All Rights Reserved.

#include "Player/BuildComponent.h"
#include "Towers/FluidTowerBase.h"
#include "Fluid/FluidSubsystem.h"
#include "Fluid/FluidTypes.h"
#include "Game/ResourceSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

UBuildComponent::UBuildComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false; // Only tick in build mode
}

void UBuildComponent::BeginPlay()
{
	Super::BeginPlay();

	FluidSubsystem = GetWorld()->GetSubsystem<UFluidSubsystem>();
	if (const UGameInstance* GI = GetWorld()->GetGameInstance())
	{
		ResourceSubsystem = GI->GetSubsystem<UResourceSubsystem>();
	}

	// Create ghost mesh component (invisible until build mode)
	GhostMesh = NewObject<UStaticMeshComponent>(GetOwner(), TEXT("BuildGhost"));
	GhostMesh->SetupAttachment(GetOwner()->GetRootComponent());
	GhostMesh->RegisterComponent();
	GhostMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GhostMesh->SetCastShadow(false);
	GhostMesh->SetVisibility(false);
}

void UBuildComponent::ToggleBuildMode()
{
	bBuildModeActive = !bBuildModeActive;
	GhostMesh->SetVisibility(bBuildModeActive);
	SetComponentTickEnabled(bBuildModeActive);

	if (!bBuildModeActive)
	{
		GhostMesh->SetVisibility(false);
	}
}

void UBuildComponent::SelectTowerType(int32 Index)
{
	if (Index >= 0 && Index < TowerClasses.Num())
	{
		SelectedTowerIndex = Index;
	}
}

void UBuildComponent::ConfirmPlacement()
{
	if (!bBuildModeActive || !bPlacementValid) { return; }
	if (!TowerClasses.IsValidIndex(SelectedTowerIndex)) { return; }

	const TSubclassOf<AFluidTowerBase>& TowerClass = TowerClasses[SelectedTowerIndex];
	const AFluidTowerBase* CDO = TowerClass.GetDefaultObject();
	if (!CDO) { return; }

	// Check affordability and deduct
	if (!ResourceSubsystem || !ResourceSubsystem->SpendCurrency(CDO->GetBuildCost()))
	{
		return;
	}

	// Spawn the tower
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	GetWorld()->SpawnActor<AFluidTowerBase>(
		TowerClass,
		PlacementLocation,
		PlacementRotation,
		SpawnParams
	);
}

void UBuildComponent::CancelBuild()
{
	if (bBuildModeActive)
	{
		ToggleBuildMode();
	}
}

void UBuildComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bBuildModeActive)
	{
		UpdateGhostPosition();
	}
}

void UBuildComponent::UpdateGhostPosition()
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn) { return; }

	const APlayerController* PC = Cast<APlayerController>(OwnerPawn->GetController());
	if (!PC) { return; }

	// Line trace from camera center
	FVector CamLoc;
	FRotator CamRot;
	PC->GetPlayerViewPoint(CamLoc, CamRot);

	const FVector TraceEnd = CamLoc + CamRot.Vector() * 3000.f;
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(BuildTrace), false, GetOwner());

	if (!GetWorld()->LineTraceSingleByChannel(Hit, CamLoc, TraceEnd, ECC_WorldStatic, Params))
	{
		bPlacementValid = false;
		SetGhostColor(false);
		return;
	}

	// Snap to grid
	if (!FluidSubsystem) { return; }
	const FIntPoint Cell = FluidSubsystem->WorldToCell(Hit.ImpactPoint);
	if (!FluidSubsystem->IsValidCell(Cell.X, Cell.Y))
	{
		bPlacementValid = false;
		SetGhostColor(false);
		return;
	}

	const FVector SnappedPos = FluidSubsystem->CellToWorld(Cell.X, Cell.Y);
	PlacementLocation = FVector(SnappedPos.X, SnappedPos.Y, Hit.ImpactPoint.Z);
	PlacementRotation = FRotator::ZeroRotator;

	GhostMesh->SetWorldLocation(PlacementLocation);

	// Validate
	bPlacementValid = ValidatePlacement(PlacementLocation, Hit);
	SetGhostColor(bPlacementValid);
}

bool UBuildComponent::ValidatePlacement(const FVector& Location, const FHitResult& Hit) const
{
	if (!FluidSubsystem || !ResourceSubsystem) { return false; }
	if (!TowerClasses.IsValidIndex(SelectedTowerIndex)) { return false; }

	const AFluidTowerBase* CDO = TowerClasses[SelectedTowerIndex].GetDefaultObject();
	if (!CDO) { return false; }

	// Check affordability
	if (!ResourceSubsystem->CanAfford(CDO->GetBuildCost())) { return false; }

	// Check slope
	if (FVector::DotProduct(Hit.ImpactNormal, FVector::UpVector) < MaxSlopeDot) { return false; }

	// Check fluid depth at cell
	const FIntPoint Cell = FluidSubsystem->WorldToCell(Location);
	if (!FluidSubsystem->IsValidCell(Cell.X, Cell.Y)) { return false; }

	const auto& Grid = FluidSubsystem->GetGrid();
	const int32 Idx = FluidSubsystem->GetCellIndex(Cell.X, Cell.Y);
	if (Grid[Idx].FluidVolume > MaxFluidForPlacement) { return false; }
	if (Grid[Idx].bBlocked) { return false; }

	return true;
}

void UBuildComponent::SetGhostColor(bool bValid)
{
	if (UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(GhostMesh->GetMaterial(0)))
	{
		const FLinearColor Color = bValid ? FLinearColor::Green : FLinearColor::Red;
		MID->SetVectorParameterValue(TEXT("GhostColor"), Color);
	}
}
