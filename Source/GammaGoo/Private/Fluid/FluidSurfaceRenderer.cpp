// Copyright 2026 Bret Wright. All Rights Reserved.

#include "Fluid/FluidSurfaceRenderer.h"
#include "Fluid/FluidSubsystem.h"
#include "Fluid/FluidTypes.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"

AFluidSurfaceRenderer::AFluidSurfaceRenderer()
{
	PrimaryActorTick.bCanEverTick = false;

	FluidPlaneMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FluidPlaneMesh"));
	RootComponent = FluidPlaneMesh;

	// Mesh assigned in Blueprint (128x128 subdivided plane)
	FluidPlaneMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FluidPlaneMesh->SetCastShadow(false);
}

void AFluidSurfaceRenderer::BeginPlay()
{
	Super::BeginPlay();

	CreateRenderTargets();

	// Register render targets with subsystem
	if (UFluidSubsystem* Subsystem = GetWorld()->GetSubsystem<UFluidSubsystem>())
	{
		Subsystem->SetRenderTargets(HeightRenderTarget, FlowRenderTarget);
	}

	// Create dynamic material instance and bind textures
	if (FluidMaterial)
	{
		DynamicMaterial = UMaterialInstanceDynamic::Create(FluidMaterial, this);
		FluidPlaneMesh->SetMaterial(0, DynamicMaterial);

		if (HeightRenderTarget)
		{
			DynamicMaterial->SetTextureParameterValue(TEXT("HeightTexture"), HeightRenderTarget);
		}
		if (FlowRenderTarget)
		{
			DynamicMaterial->SetTextureParameterValue(TEXT("FlowTexture"), FlowRenderTarget);
		}
	}
}

void AFluidSurfaceRenderer::CreateRenderTargets()
{
	const int32 Size = FluidConstants::GridSize; // 128

	if (!HeightRenderTarget)
	{
		HeightRenderTarget = NewObject<UTextureRenderTarget2D>(this, TEXT("RT_FluidHeight"));
		HeightRenderTarget->InitAutoFormat(Size, Size);
		HeightRenderTarget->RenderTargetFormat = ETextureRenderTargetFormat::RTF_R16f;
		HeightRenderTarget->Filter = TF_Bilinear;
		HeightRenderTarget->AddressX = TA_Clamp;
		HeightRenderTarget->AddressY = TA_Clamp;
		HeightRenderTarget->UpdateResourceImmediate(true);
	}

	if (!FlowRenderTarget)
	{
		FlowRenderTarget = NewObject<UTextureRenderTarget2D>(this, TEXT("RT_FluidFlow"));
		FlowRenderTarget->InitAutoFormat(Size, Size);
		FlowRenderTarget->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA16f;
		FlowRenderTarget->Filter = TF_Bilinear;
		FlowRenderTarget->AddressX = TA_Clamp;
		FlowRenderTarget->AddressY = TA_Clamp;
		FlowRenderTarget->UpdateResourceImmediate(true);
	}
}
