// Copyright Epic Games, Inc. All Rights Reserved.
#include "RelicsWars/Characters/Player/RelicsCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/InputComponent.h"
#include "Kismet/KismetMathLibrary.h"

// Constructeur : initialise la caméra third-person, le spring arm et les paramètres de rotation
ARelicsCharacter::ARelicsCharacter()
    : InputDirection(FVector::ZeroVector)
    , TargetRotation(FRotator::ZeroRotator)
    , CharacterRotationInterpSpeed(8.0f)
    , CameraSocketOffset(0.f, 60.f, 70.f) // décalage épaule droite, hauteur réaliste
    , TargetSocketOffset(0.f, 60.f, 70.f) // vue normale
    , WalkSpeed(400.f)
    , SprintSpeed(500.f)
    , AimWalkSpeed(300.f)
    , bIsSprinting(false)
    , bIsRolling(false)
    , bIsAiming(false)
    , bCanRoll(true)
    , bCanJump(true)
    , LastJumpTime(-100.f)
    , JumpCooldown(2.0f)
    , bWantsToJump(false)
{
    PrimaryActorTick.bCanEverTick = true;
    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 350.0f; // distance caméra réaliste TPS
    CameraBoom->bUsePawnControlRotation = true;
    CameraBoom->SocketOffset = CameraSocketOffset; // décalage épaule droite
    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;
    // Correction : désactive la rotation auto
    bUseControllerRotationYaw = false;
    GetCharacterMovement()->bOrientRotationToMovement = false;
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed; // Initialise la vitesse de marche par dfaut
    GetCharacterMovement()->JumpZVelocity = 300.f; // Hauteur de saut rDUITE
}

void ARelicsCharacter::BeginPlay()
{
    Super::BeginPlay();
    // S'assure que la caméra est bien positionnée au démarrage
    if (CameraBoom)
    {
        CameraBoom->SocketOffset = DefaultSocketOffset;
    }
    if (FollowCamera)
    {
        FollowCamera->SetFieldOfView(DefaultFOV);
    }
    TargetSocketOffset = DefaultSocketOffset;
}

// Tick : gère la rotation fluide du personnage et l'interpolation dynamique de la caméra
void ARelicsCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    // Blend fluide entre TPS et visée
    CameraBlendAlpha = FMath::FInterpTo(CameraBlendAlpha, bIsAiming ? 1.0f : 0.0f, DeltaTime, CameraBlendSpeed);
    if (CameraBoom)
    {
        CameraBoom->SocketOffset = FMath::Lerp(TPS_SocketOffset, Aim_SocketOffset, CameraBlendAlpha);
    }
    if (FollowCamera)
    {
        FollowCamera->SetFieldOfView(FMath::Lerp(TPS_FOV, Aim_FOV, CameraBlendAlpha));
    }
    // Option Pro : tourner le mesh en même temps que la caméra en visée
    if (bIsAiming && Controller)
    {
        FRotator ControlRot = Controller->GetControlRotation();
        FRotator YawRot(0, ControlRot.Yaw, 0);
        SetActorRotation(YawRot);
    }
    else if (!bLockRotationDuringJump)
    {
        RotateCharacterToMovement(DeltaTime);
    }
    IsInAir = GetCharacterMovement()->IsFalling();
    // Saut différé
    if (bWantsToJump && GetCharacterMovement()->IsMovingOnGround())
    {
        FTimerDelegate DelayedJumpDelegate;
        DelayedJumpDelegate.BindLambda([this]()
        {
            if (bWantsToJump && GetCharacterMovement()->IsMovingOnGround())
            {
                Jump();
                bWantsToJump = false;
            }
        });
        GetWorldTimerManager().SetTimer(RecoverTimerHandle, DelayedJumpDelegate, 0.05f, false);
    }
    // Gestion du sprint
    if (bIsSprinting)
    {
        SprintTimeLeft -= DeltaTime;
        if (SprintTimeLeft <= 0.0f)
        {
            bIsSprinting = false;
            bIsSprintOnCooldown = true;
            GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
            SprintTimeLeft = 0.0f;
            GetWorldTimerManager().SetTimer(
                SprintCooldownTimerHandle, this, &ARelicsCharacter::ResetSprintCooldown, SprintCooldown, false
            );
        }
    }
    else
    {
        if (!bIsSprintOnCooldown && SprintTimeLeft < SprintDurationMax)
        {
            SprintTimeLeft = FMath::Min(SprintDurationMax, SprintTimeLeft + DeltaTime);
        }
    }
}

void ARelicsCharacter::RotateCharacterToMovement(float DeltaTime)
{
    FVector MoveDir = InputDirection;
    if (MoveDir.SizeSquared() > 0.1f)
    {
        AddMovementInput(MoveDir.GetSafeNormal(), MoveDir.Size());
        FRotator CurrentRotation = GetActorRotation();
        FRotator DesiredRotation = MoveDir.Rotation();
        FRotator NewRotation = FMath::RInterpTo(CurrentRotation, DesiredRotation, DeltaTime, CharacterRotationInterpSpeed);
        SetActorRotation(NewRotation);
    }
    // Reset InputDirection après traitement pour éviter la rotation à l'arrêt
    InputDirection = FVector::ZeroVector;
}

// Setup des inputs : bind les axes et actions
void ARelicsCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    check(PlayerInputComponent);
    PlayerInputComponent->BindAxis("MoveForward", this, &ARelicsCharacter::MoveForward);
    PlayerInputComponent->BindAxis("MoveRight", this, &ARelicsCharacter::MoveRight);
    PlayerInputComponent->BindAxis("Turn", this, &ARelicsCharacter::Turn);
    PlayerInputComponent->BindAxis("LookUp", this, &ARelicsCharacter::LookUp);
    PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
    PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);
    PlayerInputComponent->BindAction("Sprint", IE_Pressed, this, &ARelicsCharacter::StartSprint);
    PlayerInputComponent->BindAction("Sprint", IE_Released, this, &ARelicsCharacter::StopSprint);
    PlayerInputComponent->BindAction("Roll", IE_Pressed, this, &ARelicsCharacter::StartRoll);
    PlayerInputComponent->BindAction("Aim", IE_Pressed, this, &ARelicsCharacter::OnAimPressed);
    PlayerInputComponent->BindAction("Aim", IE_Released, this, &ARelicsCharacter::OnAimReleased);
}

// --- Système de roulade avant fluide ---
void ARelicsCharacter::StartRoll()
{
    // Empêche la relance si une roulade est déjà en cours, si le cooldown est actif, ou si le personnage n'est pas au sol
    if (bIsRolling || !bCanRoll || GetCharacterMovement()->IsFalling())
        return;

    // Bloque la roulade en idle (vitesse horizontale quasi nulle)
    if (GetVelocity().Size2D() < 10.f)
        return;

    // Désactive la visée au début de la roulade
    bIsAiming = false;
    bIsRolling = true;
    bCanRoll = false;
    bCanJump = false;
    SavedBrakingFrictionFactor = GetCharacterMovement()->BrakingFrictionFactor;
    GetCharacterMovement()->BrakingFrictionFactor = 0.f;

    FVector LaunchDir = GetActorForwardVector() * 600.f;
    LaunchCharacter(LaunchDir, true, true);

    OnRollStarted();

    if (RollMontage && GetMesh() && GetMesh()->GetAnimInstance())
    {
        GetMesh()->GetAnimInstance()->Montage_Play(RollMontage, 1.0f);
    }

    FTimerDelegate RollEndDelegate;
    RollEndDelegate.BindLambda([this]()
    {
        EndRoll();
        FTimerDelegate CanRollDelegate;
        CanRollDelegate.BindLambda([this]()
        {
            bCanRoll = true;
        });
        GetWorldTimerManager().SetTimer(RecoverTimerHandle, CanRollDelegate, 0.2f, false);
    });
    GetWorldTimerManager().SetTimer(RollTimerHandle, RollEndDelegate, 0.8f, false);
}

void ARelicsCharacter::EndRoll()
{
    GetCharacterMovement()->BrakingFrictionFactor = SavedBrakingFrictionFactor;
    bIsRolling = false;
    bCanJump = true;
    if (RollMontage && GetMesh() && GetMesh()->GetAnimInstance())
    {
        GetMesh()->GetAnimInstance()->Montage_Stop(0.2f, RollMontage);
    }
    // Si le bouton de visée est toujours pressé, réactive la visée
    if (bIsAimInputPressed)
        bIsAiming = true;
}

// Handler d'input Aim (Pressed)
void ARelicsCharacter::OnAimPressed()
{
    bIsAimInputPressed = true;
    // Active la visée uniquement si pas en saut
    if (!IsInAir && !bIsJumping)
        bIsAiming = true;
}

// Handler d'input Aim (Released)
void ARelicsCharacter::OnAimReleased()
{
    bIsAimInputPressed = false;
    bIsAiming = false;
}

void ARelicsCharacter::Jump()
{
    if (bIsRolling || !bCanJump) return;
    // Désactive la visée au début du saut
    bIsAiming = false;
    if (!GetCharacterMovement()->IsFalling())
    {
        bCanJump = false;
        bIsJumping = true;
        float ForwardValue = 0.f;
        float RightValue = 0.f;
        if (InputComponent)
        {
            ForwardValue = InputComponent->GetAxisValue(TEXT("MoveForward"));
            RightValue = InputComponent->GetAxisValue(TEXT("MoveRight"));
        }
        FVector InputDir = FVector::ZeroVector;
        if (Controller)
        {
            FRotator ControlRot = Controller->GetControlRotation();
            FRotator YawRot(0, ControlRot.Yaw, 0);
            FVector ForwardDir = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
            FVector RightDir = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);
            InputDir = ForwardDir * ForwardValue + RightDir * RightValue;
        }
        if (InputDir.SizeSquared() < 0.01f)
        {
            InputDir = GetActorForwardVector();
        }
        JumpDirection = InputDir.GetSafeNormal();
        SetActorRotation(JumpDirection.Rotation());
        bLockRotationDuringJump = true;
        FVector LaunchDir = JumpDirection * JumpForwardSpeed;
        LaunchDir.Z = GetCharacterMovement()->JumpZVelocity;
        LaunchCharacter(LaunchDir, true, true);
        ACharacter::Jump();
        OnJumpStarted();
        if (JumpMontage && GetMesh() && GetMesh()->GetAnimInstance())
        {
            GetMesh()->GetAnimInstance()->Montage_Play(JumpMontage, 1.0f);
        }
        FTimerDelegate CanJumpDelegate;
        CanJumpDelegate.BindLambda([this]()
        {
            bCanJump = true;
        });
        GetWorldTimerManager().SetTimer(RecoverTimerHandle, CanJumpDelegate, 1.0f, false);
        bWantsToJump = false;
    }
    else
    {
        bCanJump = true;
        bWantsToJump = true;
    }
}

void ARelicsCharacter::Landed(const FHitResult& Hit)
{
    ACharacter::Landed(Hit);
    bIsIdleJump = false;
    bCanRoll = true;
    bCanJump = true;
    bIsJumping = false;
    IsInAir = false;
    bLockRotationDuringJump = false;
    if (JumpMontage && GetMesh() && GetMesh()->GetAnimInstance())
    {
        GetMesh()->GetAnimInstance()->Montage_Stop(0.2f, JumpMontage);
    }
    OnJumpLanded();
    // Si le bouton de visée est toujours pressé, réactive la visée
    if (bIsAimInputPressed)
        bIsAiming = true;
}

void ARelicsCharacter::OnJumpLanded()
{
    // Logique supplémentaire pour la transition Idle/Run peut être ajoutée ici ou dans le Blueprint
}

bool ARelicsCharacter::IsIdleJumping() const
{
    return bIsIdleJump;
}

// Retourne le statut de roulade pour l'AnimBP
bool ARelicsCharacter::IsRolling() const
{
    return bIsRolling;
}

// Gestion de la visée TPS
void ARelicsCharacter::StartAim()
{
    // Empêche la visée pendant la roulade ou le saut
    if (bIsRolling || IsInAir || bIsJumping)
        return;
    bIsAiming = true;
}

void ARelicsCharacter::StopAim()
{
    bIsAiming = false;
}
