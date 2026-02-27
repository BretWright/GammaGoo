// Copyright 2026 Bret Wright. All Rights Reserved.
// AFluidSurfaceRenderer owns the fluid plane mesh and render targets.
// Purely visual — all simulation state lives in UFluidSubsystem.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FluidSurfaceRenderer.generated.h"

class UStaticMeshComponent;
class UTextureRenderTarget2D;
class UFluidSubsystem;
class UMaterialInstanceDynamic;

UCLASS(BlueprintType, Blueprintable)
class GAMMAGOO_API AFluidSurfaceRenderer : public AActor
{
	GENERATED_BODY()

public:
	AFluidSurfaceRenderer();

	virtual void BeginPlay() override;

protected:
	/** The plane mesh that gets displaced by the material. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Fluid|Rendering")
	TObjectPtr<UStaticMeshComponent> FluidPlaneMesh;

	/** 128x128 R16F — encodes fluid surface height per cell. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid|Rendering")
	TObjectPtr<UTextureRenderTarget2D> HeightRenderTarget;

	/** 128x128 RGBA16F — RG = FlowVelocity.XY, B = bFrozen. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid|Rendering")
	TObjectPtr<UTextureRenderTarget2D> FlowRenderTarget;

	/** Base material to create dynamic instance from. Set in Blueprint. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid|Rendering")
	TObjectPtr<UMaterialInterface> FluidMaterial;

private:
	void CreateRenderTargets();

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> DynamicMaterial;
};
