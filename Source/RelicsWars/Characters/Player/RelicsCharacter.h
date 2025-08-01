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
    // Tick pour la rotation et la cam�ra dynamique
    virtual void Tick(float DeltaTime) override;
    // Setup des inputs
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
    virtual void BeginPlay() override;

    // Sprint
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Movement)
    float WalkSpeed = 300.f; // Vitesse de marche r�aliste Uncharted 2
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Movement)
    float SprintSpeed = 450.f; // Vitesse de sprint r�aliste Uncharted 2
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
    // UAnimMontage* JumpForwardMontage; // D�sactiv�, animation pilot�e via Animation Blueprint

    // Saut Idle
    UPROPERTY(BlueprintReadOnly, Category = Movement)
    bool bIsIdleJump = false;
    UFUNCTION(BlueprintCallable, Category = Movement)
    bool IsIdleJumping() const;

    // --- Syst�me de roulade ---
    UPROPERTY(BlueprintReadOnly, Category = Movement)
    bool bIsRolling = false; // Statut de roulade
    UPROPERTY(BlueprintReadOnly, Category = Movement)
    bool bIsRecovering = false; // Statut de r�cup�ration post-roulade
    UFUNCTION(BlueprintCallable, Category = Movement)
    void StartRoll(); // D�marre la roulade
    UFUNCTION(BlueprintCallable, Category = Movement)
    void EndRoll(); // Termine la roulade
    UFUNCTION(BlueprintCallable, Category = Movement)
    bool IsRolling() const;
    UFUNCTION(BlueprintCallable, Category = Movement)
    bool IsRecovering() const { return bIsRecovering; }
    FTimerHandle RollTimerHandle; // Handle du timer de roulade
    FTimerHandle RecoverTimerHandle; // Handle du timer de r�cup�ration
    float SavedBrakingFrictionFactor = 2.0f; // Stocke la valeur originale
    UPROPERTY(BlueprintReadOnly, Category = Movement)
    bool bCanRoll = true; // Contr�le anti-spam pour la roulade
    UPROPERTY(BlueprintReadOnly, Category = Movement)
    bool bCanJump = true; // Contr�le anti-spam pour le saut
    UPROPERTY(BlueprintReadOnly, Category = Movement)
    float LastJumpTime = -100.f; // Temps du dernier saut
    UPROPERTY(BlueprintReadOnly, Category = Movement)
    float JumpCooldown = 2.0f; // Cooldown en secondes
    UPROPERTY(BlueprintReadOnly, Category = Movement)
    bool bWantsToJump = false; // Indique si le joueur veut sauter
    UPROPERTY(BlueprintReadOnly, Category = Movement)
    bool bIsJumping = false; // True uniquement quand le saut d�marre
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

    // Vis�e TPS
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

    // --- Cam�ra TPS/Vis�e Uncharted 4 ---
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
    // SpringArm pour la cam�ra third-person �paule gauche
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
    USpringArmComponent* CameraBoom;
    // Cam�ra third-person
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
    UCameraComponent* FollowCamera;

    // Direction d'entr�e du joueur (MoveForward/MoveRight combin�s)
    FVector InputDirection;
    // Rotation cible du personnage
    FRotator TargetRotation;
    // Vitesse d'interpolation de rotation du personnage
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Movement)
    float CharacterRotationInterpSpeed = 8.0f;
    // Vitesse d'interpolation du SpringArm pour effet cin�matique
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera)
    float CameraArmInterpSpeed = 6.0f;
    // Offset de rotation dynamique du SpringArm
    FRotator SpringArmTargetOffset;
    // Offset serr� du SpringArm quand immobile
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera)
    FVector TightSocketOffset = FVector(0.f, 40.f, 90.f);
    // Offset normal du SpringArm pour mouvement
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera)
    FVector DefaultSocketOffset = FVector(0.f, 60.f, 70.f);
    // Pour la vis�e (� impl�menter plus tard)
    FVector CameraSocketOffset = FVector(0.f, 75.f, 65.f);
    // Offset cible pour effets dynamiques (ex : vis�e, focus)
    FVector TargetSocketOffset = FVector(0.f, 75.f, 65.f);

    // Champs pour la vis�e
    float DefaultFOV = 90.f;
    float AimFOV = 70.f;
    // Offset cam�ra vis�e �paule droite (Uncharted style)
    FVector AimSocketOffset = FVector(0.f, 40.f, 90.f); // X=avant/arri�re, Y=lat�ral (�paule droite), Z=hauteur
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

    // Ajoute la variable pour stocker l'�tat du bouton de vis�e
    UPROPERTY(BlueprintReadOnly, Category = "Aim")
    bool bIsAimInputPressed = false;

    // Ajoute les handlers d'input
    UFUNCTION(BlueprintCallable, Category = "Aim")
    void OnAimPressed();
    UFUNCTION(BlueprintCallable, Category = "Aim")
    void OnAimReleased();

    // Ajout pour gestion avanc�e de la vis�e pendant le saut
    UPROPERTY(BlueprintReadOnly, Category = "Aim")
    bool bWantsToAimAfterJump = false;
};
