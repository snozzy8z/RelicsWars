// Copyright Epic Games, Inc. All Rights Reserved.
#include "RelicsWars/GameModes/RelicsGameModeBase.h"
#include "RelicsWars/Characters/Player/RelicsCharacter.h"
#include "RelicsWars/Controllers/RelicsPlayerController.h"
#include "UObject/ConstructorHelpers.h"

ARelicsGameModeBase::ARelicsGameModeBase()
{
    static ConstructorHelpers::FClassFinder<APawn> PawnBPClass(TEXT("/Game/Characters/Player/BP_RelicsCharacter"));
    if (PawnBPClass.Class != nullptr)
    {
        DefaultPawnClass = PawnBPClass.Class;
    }
    else
    {
        DefaultPawnClass = ARelicsCharacter::StaticClass();
    }
    PlayerControllerClass = ARelicsPlayerController::StaticClass();
}
