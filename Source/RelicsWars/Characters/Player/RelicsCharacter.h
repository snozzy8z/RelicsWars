// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "RelicsCharacter.generated.h"

UCLASS(Blueprintable)
class RELICSWARS_API ARelicsCharacter : public ACharacter
{
    GENERATED_BODY()
public:
    // Constructeur
    ARelicsCharacter();
    // Tick pour la rotation et la caméra dynamique
    virtual void Tick(float DeltaTime) override;
    // Setup des inputs
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
    virtual void BeginPlay() override;

    // Sprint
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Movement)
    float WalkSpeed = 400.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Movement)
    float SprintSpeed = 800.f;
    UPROPERTY(BlueprintReadOnly, Category = Movement)
    bool bIsSprinting = false;
    void StartSprint();
    void StopSprint();

protected:
    // SpringArm pour la caméra third-person épaule gauche
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
    USpringArmComponent* CameraBoom;
    // Caméra third-person
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
    UCameraComponent* FollowCamera;

    // Direction d'entrée du joueur (MoveForward/MoveRight combinés)
    FVector InputDirection;
    // Rotation cible du personnage
    FRotator TargetRotation;
    // Vitesse d'interpolation de rotation du personnage
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Movement)
    float CharacterRotationInterpSpeed = 8.0f;
    // Vitesse d'interpolation du SpringArm pour effet cinématique
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera)
    float CameraArmInterpSpeed = 6.0f;
    // Offset de rotation dynamique du SpringArm
    FRotator SpringArmTargetOffset;
    // Offset serré du SpringArm quand immobile
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera)
    FVector TightSocketOffset = FVector(0.f, 40.f, 90.f);
    // Offset normal du SpringArm pour mouvement
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera)
    FVector DefaultSocketOffset = FVector(0.f, 60.f, 70.f);
    // Pour la visée (à implémenter plus tard)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera)
    bool bIsAiming = false;
    FVector CameraSocketOffset = FVector(0.f, 75.f, 65.f);
    // Offset cible pour effets dynamiques (ex : visée, focus)
    FVector TargetSocketOffset = FVector(0.f, 75.f, 65.f);

    // Fonctions d'input
    void MoveForward(float Value);
    void MoveRight(float Value);
    void Turn(float Value);
    void LookUp(float Value);
    // Rotation fluide du personnage vers la direction du mouvement
    void RotateCharacterToMovement(float DeltaTime);
};
