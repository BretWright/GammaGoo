// Copyright 2026 Bret Wright. All Rights Reserved.

#include "Player/FluidDefenseCharacter.h"
#include "Player/BuildComponent.h"
#include "Fluid/FluidSubsystem.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Engine/World.h"
#include "TimerManager.h"

AFluidDefenseCharacter::AFluidDefenseCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	BuildComponent = CreateDefaultSubobject<UBuildComponent>(TEXT("BuildComponent"));
}

void AFluidDefenseCharacter::BeginPlay()
{
	Super::BeginPlay();

	FluidSubsystem = GetWorld()->GetSubsystem<UFluidSubsystem>();
	DefaultWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;

	// Add mapping context
	if (FluidSiegeMappingContext)
	{
		if (const APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			if (UEnhancedInputLocalPlayerSubsystem* InputSub =
				ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
			{
				InputSub->AddMappingContext(FluidSiegeMappingContext, 1);
			}
		}
	}

	// Fluid depth check timer
	GetWorldTimerManager().SetTimer(
		FluidCheckTimerHandle,
		FTimerDelegate::CreateUObject(this, &AFluidDefenseCharacter::CheckFluidDepth),
		FluidCheckInterval,
		/*bLoop=*/true
	);
}

void AFluidDefenseCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (FireAction)
		{
			EIC->BindAction(FireAction, ETriggerEvent::Started, this, &AFluidDefenseCharacter::StartFiring);
			EIC->BindAction(FireAction, ETriggerEvent::Completed, this, &AFluidDefenseCharacter::StopFiring);
		}
		if (BuildToggleAction)
		{
			EIC->BindAction(BuildToggleAction, ETriggerEvent::Started, this, &AFluidDefenseCharacter::OnBuildToggle);
		}
	}
}

// ---------------------------------------------------------------------------
// Fluid Depth Interaction
// ---------------------------------------------------------------------------

void AFluidDefenseCharacter::CheckFluidDepth()
{
	if (!FluidSubsystem) { return; }

	const FVector Loc = GetActorLocation();
	const float FluidSurface = FluidSubsystem->GetFluidHeightAtWorldPos(Loc);
	const float FeetZ = Loc.Z; // Character origin at feet

	CurrentFluidDepth = FMath::Max(0.f, FluidSurface - FeetZ);
	CurrentDepthTier = ComputeDepthTier(CurrentFluidDepth);

	// Apply speed modifier
	float SpeedMult = 1.f;
	switch (CurrentDepthTier)
	{
	case EFluidDepthTier::Dry:    SpeedMult = 1.f;   break;
	case EFluidDepthTier::Ankle:  SpeedMult = 0.8f;  break;
	case EFluidDepthTier::Knee:   SpeedMult = 0.5f;  break;
	case EFluidDepthTier::Waist:  SpeedMult = 0.3f;  break;
	case EFluidDepthTier::Chest:  SpeedMult = 0.15f; break;
	}
	GetCharacterMovement()->MaxWalkSpeed = DefaultWalkSpeed * SpeedMult;

	// Apply damage over time
	ApplyDepthEffects(FluidCheckInterval);

	// Recharge energy when not firing
	if (!bFiring && Energy < MaxEnergy)
	{
		Energy = FMath::Min(MaxEnergy, Energy + EnergyRechargeRate * FluidCheckInterval);
	}
}

EFluidDepthTier AFluidDefenseCharacter::ComputeDepthTier(float Depth) const
{
	if (Depth <= 0.f)   return EFluidDepthTier::Dry;
	if (Depth <= 30.f)  return EFluidDepthTier::Ankle;
	if (Depth <= 70.f)  return EFluidDepthTier::Knee;
	if (Depth <= 120.f) return EFluidDepthTier::Waist;
	return EFluidDepthTier::Chest;
}

void AFluidDefenseCharacter::ApplyDepthEffects(float DeltaTime)
{
	float DPS = 0.f;
	switch (CurrentDepthTier)
	{
	case EFluidDepthTier::Dry:
	case EFluidDepthTier::Ankle:
		DPS = 0.f;
		break;
	case EFluidDepthTier::Knee:
		DPS = 5.f;
		break;
	case EFluidDepthTier::Waist:
		DPS = 15.f;
		break;
	case EFluidDepthTier::Chest:
		DPS = 30.f;
		break;
	}

	if (DPS > 0.f)
	{
		PlayerHealth = FMath::Max(0.f, PlayerHealth - DPS * DeltaTime);
	}
}

// ---------------------------------------------------------------------------
// Heat Lance
// ---------------------------------------------------------------------------

void AFluidDefenseCharacter::StartFiring()
{
	if (bFiring) { return; }
	bFiring = true;

	// Fire at 30Hz for smooth evaporation
	GetWorldTimerManager().SetTimer(
		LanceTickHandle,
		FTimerDelegate::CreateUObject(this, &AFluidDefenseCharacter::FireLanceTick),
		1.f / 30.f,
		/*bLoop=*/true,
		/*InFirstDelay=*/0.f
	);
}

void AFluidDefenseCharacter::StopFiring()
{
	bFiring = false;
	GetWorldTimerManager().ClearTimer(LanceTickHandle);
}

void AFluidDefenseCharacter::FireLanceTick()
{
	if (!FluidSubsystem || Energy <= 0.f)
	{
		StopFiring();
		return;
	}

	const float DeltaTime = 1.f / 30.f;

	// Drain energy
	Energy = FMath::Max(0.f, Energy - EnergyDrainRate * DeltaTime);

	// Line trace from camera
	const APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) { return; }

	FVector CamLoc;
	FRotator CamRot;
	PC->GetPlayerViewPoint(CamLoc, CamRot);

	const FVector TraceEnd = CamLoc + CamRot.Vector() * LanceRange;
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(HeatLance), false, this);

	if (GetWorld()->LineTraceSingleByChannel(Hit, CamLoc, TraceEnd, ECC_WorldStatic, Params))
	{
		FluidSubsystem->RemoveFluidInRadius(Hit.ImpactPoint, LanceRadius, LanceEvaporateRate * DeltaTime);
	}
}

// ---------------------------------------------------------------------------
// Build Mode Toggle
// ---------------------------------------------------------------------------

void AFluidDefenseCharacter::OnBuildToggle()
{
	if (BuildComponent)
	{
		BuildComponent->ToggleBuildMode();
	}
}
