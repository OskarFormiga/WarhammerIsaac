#pragma once
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace ArenaVisuals
{
    inline void PlaceModel(UStaticMeshComponent* Mesh, UStaticMesh* Model, double Scale,
        const FRotator& Rotation, const FVector& Center)
    {
        Mesh->SetStaticMesh(Model);
        Mesh->SetRelativeScale3D(FVector(Scale));
        Mesh->SetRelativeRotation(Rotation);
        Mesh->SetRelativeLocation(Center - Rotation.RotateVector(Model->GetBounds().Origin * Scale));
    }

    inline void FitCharacter(UStaticMeshComponent* Mesh, UStaticMesh* Model, double Height, double FacingYaw)
    {
        const double Scale = Height / FMath::Max(1.0, Model->GetBounds().BoxExtent.Z * 2.0);
        PlaceModel(Mesh, Model, Scale, FRotator(0, FacingYaw, 0), FVector(0, 0, Height * 0.5 - 84.0));
    }

    inline void Tint(UStaticMeshComponent* Mesh, const FLinearColor& Color)
    {
        UMaterialInstanceDynamic* Material = Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(0));
        if (!Material)
        {
            UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr,
                TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
            Material = Mesh->CreateAndSetMaterialInstanceDynamicFromMaterial(0, Base);
        }
        if (Material)
        {
            Material->SetVectorParameterValue(TEXT("Color"), Color);
            Material->SetVectorParameterValue(TEXT("BaseColor"), Color);
        }
    }
}
