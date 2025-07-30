// Copyright (c) 2024. Uncharted 2 Multiplayer-inspired Third Person Character for Unreal Engine 5
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "TimerManager.h"
#include "RelicsCharacter.generated.h"

UCLASS(Blueprintable)
class RELICSWARS_API ARelicsCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    // --- Construction & Setup ---
    ARelicsCharacter();
    virtual void Tick(float DeltaTime) override;
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
    virtual void BeginPlay() override;
    virtual void Landed(const FHitResult& Hit) override;

    // --- Sprint ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float WalkSpeed = 350.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float SprintSpeed = 500.f;
    UPROPERTY(BlueprintReadOnly, Category = "Movement")
    bool bIsSprinting = false;
    void StartSprint();
    void StopSprint();

    // --- Saut ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float JumpForwardImpulse = 320.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float JumpUpImpulse = 320.f;
    UPROPERTY(BlueprintReadOnly, Category = "Movement")
    bool bIsJumping = false;
    UPROPERTY(BlueprintReadOnly, Category = "Movement")
    bool bIsFalling = false;
    UPROPERTY(BlueprintReadOnly, Category = "Movement")
    bool bJumpOnCooldown = false;
    FTimerHandle JumpCooldownHandle;
    FTimerHandle JumpBlendOutTimerHandle;
    float JumpBlendOutTime = 0.18f;
    float JumpCooldown = 0.9f;
    virtual void Jump() override;
    void EndJumpBlendOut();
    void EndJumpCooldown();

    // --- Roulade Uncharted 2 Multiplayer ---
    UPROPERTY(BlueprintReadOnly, Category = "Movement")
    bool bIsRolling = false; // Exposé à l'AnimBP
    UPROPERTY(BlueprintReadOnly, Category = "Movement")
    bool bRollOnCooldown = false; // Cooldown strict
    UPROPERTY(BlueprintReadOnly, Category = "Movement")
    bool bRollBlendOut = false; // Pour blending AnimBP
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float RollDuration = 0.7f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float RollBlendOutTime = 0.18f; // Blending encore plus long pour une transition très douce
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float RollCooldown = 0.7f; // Cooldown roulade réduit
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float RollImpulseSpeed = 600.f; // Vitesse minimale de la roulade
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float RollFriction = 0.5f; // Friction temporaire pendant la roulade
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float RollBrakingDeceleration = 32.f; // Décélération temporaire pendant la roulade
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float RollSteerInfluence = 0.15f; // Influence max du stick pendant la roulade
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float RollSteerMaxAngle = 20.f; // Angle max de courbe (en degrés)
    float SavedFriction = 8.0f; // Friction normale sauvegardée
    float SavedBrakingDeceleration = 2048.f; // Décélération normale sauvegardée
    FTimerHandle RollTimerHandle;
    FTimerHandle RollBlendOutTimerHandle;
    FTimerHandle RollCooldownHandle;
    void StartRoll();
    void EndRoll();
    void EndRollBlendOut();
    void EndRollCooldown();

    // --- Caméra épaule dynamique ---
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    USpringArmComponent* CameraBoom;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    UCameraComponent* FollowCamera;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    FVector TargetSocketOffset = FVector(0.f, 75.f, 65.f);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    float CameraArmInterpSpeed = 6.0f;

    // --- Direction & Rotation ---
    UPROPERTY(BlueprintReadOnly, Category = "Movement")
    FVector InputDirection;
    UPROPERTY(BlueprintReadOnly, Category = "Movement")
    FRotator TargetRotation;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float CharacterRotationInterpSpeed = 8.0f;

    // --- COVER SYSTEM ---
    UPROPERTY(BlueprintReadOnly, Category = "Cover")
    bool bIsInCover = false;
    void EnterCover();
    void ExitCover();

    // --- Steering subtil global, exposé à l'AnimBP et modifiable dans l'éditeur ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float SteeringRatio = 0.12f; // Influence max du stick pour steering subtil

protected:
    // --- Input ---
    void MoveForward(float Value);
    void MoveRight(float Value);
    void Turn(float Value);
    void LookUp(float Value);
    // --- Rotation fluide ---
    void RotateCharacterToMovement(float DeltaTime);

    // --- Roulade : direction et momentum ---
    FVector RollDirection;

private:
    // Interpolation friction roulade
    float RollFrictionLerpElapsed = 0.f;
    float RollFrictionLerpDuration = 0.3f;
    float RollFrictionLerpStart = 0.f;
    float RollFrictionLerpEnd = 0.f;
    float RollBrakingLerpStart = 0.f;
    float RollBrakingLerpEnd = 0.f;
    FTimerHandle RollFrictionLerpTimerHandle;
    void InterpRollFrictionTick();

    // --- Roulade : protection anti-spam et restauration friction/momentum ---

    // Ajoute une variable pour vérifier si le blend-out est en cours pour cette roulade
    bool bBlendOutActive = false; // interne, non exposé

    // --- Saut dynamique Uncharted 2 ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float JumpImpulse = 850.f; // Force horizontale du saut (X/Y)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float JumpHeight = 350.f; // Hauteur du saut (Z)
};