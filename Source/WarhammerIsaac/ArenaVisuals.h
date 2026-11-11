#pragma once
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace ArenaVisuals
{
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
