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
    float WalkSpeed = 300.f; // Vitesse de marche réaliste Uncharted 2
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Movement)
    float SprintSpeed = 450.f; // Vitesse de sprint réaliste Uncharted 2
    UPROPERTY(BlueprintReadOnly, Category = Movement)
    bool bIsSprinting = false;
    void StartSprint();
    void StopSprint();

    // Saut Uncharted
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Movement)
    float JumpForwardSpeed = 600.f;
    virtual void Jump() override;
    virtual void Landed(const FHitResult& Hit) override;

    // Animation jumpforward (Uncharted jog)
    // UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Animation)
    // UAnimMontage* JumpForwardMontage; // Désactivé, animation pilotée via Animation Blueprint

    // Saut Idle
    UPROPERTY(BlueprintReadOnly, Category = Movement)
    bool bIsIdleJump = false;
    UFUNCTION(BlueprintCallable, Category = Movement)
    bool IsIdleJumping() const;

    // --- Système de roulade ---
    UPROPERTY(BlueprintReadOnly, Category = Movement)
    bool bIsRolling = false; // Statut de roulade
    UPROPERTY(BlueprintReadOnly, Category = Movement)
    bool bIsRecovering = false; // Statut de récupération post-roulade
    UFUNCTION(BlueprintCallable, Category = Movement)
    void StartRoll(); // Démarre la roulade
    UFUNCTION(BlueprintCallable, Category = Movement)
    void EndRoll(); // Termine la roulade
    UFUNCTION(BlueprintCallable, Category = Movement)
    bool IsRolling() const;
    UFUNCTION(BlueprintCallable, Category = Movement)
    bool IsRecovering() const { return bIsRecovering; }
    FTimerHandle RollTimerHandle; // Handle du timer de roulade
    FTimerHandle RecoverTimerHandle; // Handle du timer de récupération
    float SavedBrakingFrictionFactor = 2.0f; // Stocke la valeur originale
    UPROPERTY(BlueprintReadOnly, Category = Movement)
    bool bCanRoll = true; // Contrôle anti-spam pour la roulade
    UPROPERTY(BlueprintReadOnly, Category = Movement)
    bool bCanJump = true; // Contrôle anti-spam pour le saut
    UPROPERTY(BlueprintReadOnly, Category = Movement)
    float LastJumpTime = -100.f; // Temps du dernier saut
    UPROPERTY(BlueprintReadOnly, Category = Movement)
    float JumpCooldown = 2.0f; // Cooldown en secondes
    UPROPERTY(BlueprintReadOnly, Category = Movement)
    bool bWantsToJump = false; // Indique si le joueur veut sauter
    UPROPERTY(BlueprintReadOnly, Category = Movement)
    bool bIsJumping = false; // True uniquement quand le saut démarre
    UPROPERTY(BlueprintReadOnly, Category = Movement)
    bool IsInAir = false; // True si le perso est en l'air

    UFUNCTION(BlueprintImplementableEvent, Category = "Animation")
    void OnJumpStarted();
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    UAnimMontage* RollMontage;
    UFUNCTION(BlueprintImplementableEvent, Category = "Animation")
    void OnRollStarted();

    // Sprint moderne
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprint")
    float SprintDurationMax = 4.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprint")
    float SprintCooldown = 2.0f;
    UPROPERTY(BlueprintReadOnly, Category = "Sprint")
    float SprintTimeLeft = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category = "Sprint")
    bool bIsSprintOnCooldown = false;
    FTimerHandle SprintCooldownTimerHandle;
    UFUNCTION(BlueprintImplementableEvent, Category = "Sprint")
    void OnSprintBlockedFeedback();
    void ResetSprintCooldown();

    // Visée TPS
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aim")
    bool bIsAiming = false;
    UPROPERTY(BlueprintReadOnly, Category = "Aim")
    float AimForward = 0.f;
    UPROPERTY(BlueprintReadOnly, Category = "Aim")
    float AimRight = 0.f;
    UFUNCTION(BlueprintCallable, Category = "Aim")
    void StartAim();
    UFUNCTION(BlueprintCallable, Category = "Aim")
    void StopAim();

    // --- Caméra TPS/Visée Uncharted 4 ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera)
    FVector TPS_SocketOffset = FVector(0, 60, 50);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera)
    FVector Aim_SocketOffset = FVector(10, 32, 60);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera)
    float TPS_FOV = 88.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera)
    float Aim_FOV = 58.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera)
    float CameraBlendAlpha = 0.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera)
    float CameraBlendSpeed = 8.0f;

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
    FVector CameraSocketOffset = FVector(0.f, 75.f, 65.f);
    // Offset cible pour effets dynamiques (ex : visée, focus)
    FVector TargetSocketOffset = FVector(0.f, 75.f, 65.f);

    // Champs pour la visée
    float DefaultFOV = 90.f;
    float AimFOV = 70.f;
    // Offset caméra visée épaule droite (Uncharted style)
    FVector AimSocketOffset = FVector(0.f, 40.f, 90.f); // X=avant/arrière, Y=latéral (épaule droite), Z=hauteur
    float DefaultWalkSpeed = 400.f;
    float AimWalkSpeed = 200.f;

    // Fonctions d'input
    void MoveForward(float Value);
    void MoveRight(float Value);
    void Turn(float Value);
    void LookUp(float Value);
    // Rotation fluide du personnage vers la direction du mouvement
    void RotateCharacterToMovement(float DeltaTime);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    UAnimMontage* JumpMontage;

    UFUNCTION(BlueprintCallable, Category = "Animation")
    void OnJumpLanded();

    // Saut directionnel Uncharted
    UPROPERTY(BlueprintReadOnly, Category = Movement)
    bool bLockRotationDuringJump = false;
    // Stocke la direction du saut
    FVector JumpDirection;

    // Ajoute la variable pour stocker l'état du bouton de visée
    UPROPERTY(BlueprintReadOnly, Category = "Aim")
    bool bIsAimInputPressed = false;

    // Ajoute les handlers d'input
    UFUNCTION(BlueprintCallable, Category = "Aim")
    void OnAimPressed();
    UFUNCTION(BlueprintCallable, Category = "Aim")
    void OnAimReleased();
};
