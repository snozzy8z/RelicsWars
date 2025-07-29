// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RelicsWars/Characters/Player/RelicsCharacter.h"
#include "RelicsWars/Controllers/RelicsPlayerController.h"
#include "RelicsGameModeBase.generated.h"

UCLASS(Blueprintable)
class RELICSWARS_API ARelicsGameModeBase : public AGameModeBase
{
    GENERATED_BODY()
public:
    ARelicsGameModeBase();
};
