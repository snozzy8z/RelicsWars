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
    , CameraSocketOffset(0.f, 75.f, 65.f)
    , TargetSocketOffset(0.f, 75.f, 65.f)
    , WalkSpeed(400.f)
    , SprintSpeed(500.f)
    , AimWalkSpeed(300.f) // Vitesse de marche en visant
    , bIsSprinting(false)
    , bIsRolling(false)
    , bIsAiming(false) // Initialise la visée à false
    , bCanRoll(true)
    , bCanJump(true)
    , LastJumpTime(-100.f)
    , JumpCooldown(2.0f)
    , bWantsToJump(false)
{
    PrimaryActorTick.bCanEverTick = true;
    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 300.0f;
    CameraBoom->bUsePawnControlRotation = true;
    CameraBoom->SocketOffset = CameraSocketOffset;
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
}

// Tick : gère la rotation fluide du personnage et l'interpolation dynamique de la caméra
void ARelicsCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    // Interpolation douce de la caméra et FOV selon le mode de visée
    if (CameraBoom)
    {
        CameraBoom->SocketOffset = FMath::VInterpTo(CameraBoom->SocketOffset, TargetSocketOffset, DeltaTime, CameraArmInterpSpeed);
    }
    if (FollowCamera)
    {
        float TargetFOV = bIsAiming ? AimFOV : DefaultFOV;
        float CurrentFOV = FollowCamera->FieldOfView;
        FollowCamera->SetFieldOfView(FMath::FInterpTo(CurrentFOV, TargetFOV, DeltaTime, 10.0f));
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
    PlayerInputComponent->BindAction("Aim", IE_Pressed, this, &ARelicsCharacter::StartAim);
    PlayerInputComponent->BindAction("Aim", IE_Released, this, &ARelicsCharacter::StopAim);
}

// --- Système de roulade avant fluide ---
void ARelicsCharacter::StartRoll()
{
    // Empêche la relance si une roulade est déjà en cours, si le personnage est en l'air, si l'anti-spam est actif ou si idle
    if (bIsRolling || !bCanRoll || GetCharacterMovement()->IsFalling())
        return;

    // Bloque la roulade en idle (vitesse horizontale quasi nulle)
    if (GetVelocity().Size2D() < 10.f)
        return;

    bIsRolling = true;
    bCanRoll = false;
    bCanJump = false; // Bloque le saut pendant la roulade
    // Stocke la friction originale
    SavedBrakingFrictionFactor = GetCharacterMovement()->BrakingFrictionFactor;
    // Désactive temporairement le freinage
    GetCharacterMovement()->BrakingFrictionFactor = 0.f;

    // Lance le personnage vers l'avant
    FVector LaunchDir = GetActorForwardVector() * 600.f;
    LaunchCharacter(LaunchDir, true, true);

    OnRollStarted(); // Event BP pour custom logic si besoin

    // Joue le montage de roulade
    if (RollMontage && GetMesh() && GetMesh()->GetAnimInstance())
    {
        GetMesh()->GetAnimInstance()->Montage_Play(RollMontage, 1.0f);
    }

    // Timer pour la fin de la roulade (0.8s)
    FTimerDelegate RollEndDelegate;
    RollEndDelegate.BindLambda([this]()
    {
        EndRoll();
        // Timer anti-spam : délai avant de pouvoir relancer (0.2s)
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
    // Remet la friction originale
    GetCharacterMovement()->BrakingFrictionFactor = SavedBrakingFrictionFactor;
    bIsRolling = false;
    bCanJump = true; // Autorise le saut après la roulade
    // Stoppe le montage de roulade
    if (RollMontage && GetMesh() && GetMesh()->GetAnimInstance())
    {
        GetMesh()->GetAnimInstance()->Montage_Stop(0.2f, RollMontage);
    }
}

// Affecte uniquement la valeur de l'axe
void ARelicsCharacter::MoveForward(float Value)
{
    // En mode visée, on ne bloque pas le déplacement en l'air
    if (bIsAiming)
    {
        AimForward = Value;
        // Déplacement TPS "in place" : avance/recule lentement, strafe possible
        if (Controller && FMath::Abs(Value) > KINDA_SMALL_NUMBER)
        {
            FRotator ControlRot = Controller->GetControlRotation();
            FRotator YawRot(0, ControlRot.Yaw, 0);
            FVector ForwardDir = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
            AddMovementInput(ForwardDir, Value * (AimWalkSpeed / WalkSpeed)); // SpeedMultiplier
        }
    }
    else
    {
        // Déplacement TPS classique
        if (GetCharacterMovement()->IsFalling())
            return;
        AimForward = Value;
        if (Controller && Value != 0.0f)
        {
            FRotator ControlRot = Controller->GetControlRotation();
            FRotator YawRot(0, ControlRot.Yaw, 0);
            FVector ForwardDir = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
            InputDirection += ForwardDir * Value;
        }
    }
}

void ARelicsCharacter::MoveRight(float Value)
{
    // En mode visée, on ne bloque pas le déplacement en l'air
    if (bIsAiming)
    {
        AimRight = Value;
        // Déplacement TPS "in place" : strafe gauche/droite lentement
        if (Controller && FMath::Abs(Value) > KINDA_SMALL_NUMBER)
        {
            FRotator ControlRot = Controller->GetControlRotation();
            FRotator YawRot(0, ControlRot.Yaw, 0);
            FVector RightDir = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);
            AddMovementInput(RightDir, Value * (AimWalkSpeed / WalkSpeed)); // SpeedMultiplier
        }
    }
    else
    {
        // Déplacement TPS classique
        if (GetCharacterMovement()->IsFalling())
            return;
        AimRight = Value;
        if (Controller && Value != 0.0f)
        {
            FRotator ControlRot = Controller->GetControlRotation();
            FRotator YawRot(0, ControlRot.Yaw, 0);
            FVector RightDir = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);
            InputDirection += RightDir * Value;
        }
    }
}

void ARelicsCharacter::Turn(float Value)
{
    AddControllerYawInput(Value);
}

void ARelicsCharacter::LookUp(float Value)
{
    AddControllerPitchInput(Value);
}

// Sprint : active le sprint
void ARelicsCharacter::StartSprint()
{
    if (bIsSprintOnCooldown || bIsSprinting)
    {
        OnSprintBlockedFeedback(); // Feedback BP/FX si sprint bloqué
        return;
    }
    bIsSprinting = true;
    GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
    SprintTimeLeft = SprintDurationMax;
}

// Sprint : désactive le sprint
void ARelicsCharacter::StopSprint()
{
    bIsSprinting = false;
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

void ARelicsCharacter::ResetSprintCooldown()
{
    bIsSprintOnCooldown = false;
    SprintTimeLeft = SprintDurationMax;
}

// Saut Uncharted : override Jump pour appliquer une impulsion vers l'avant
void ARelicsCharacter::Jump()
{
    if (bIsRolling || !bCanJump) return;
    if (!GetCharacterMovement()->IsFalling())
    {
        bCanJump = false;
        bIsJumping = true;
        // Prend UNIQUEMENT la direction d'input au moment du saut
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
            InputDir = GetActorForwardVector(); // Saut en avant par défaut
        }
        JumpDirection = InputDir.GetSafeNormal();
        // Oriente instantanément le mesh dans la direction du saut
        SetActorRotation(JumpDirection.Rotation());
        // Verrouille la rotation pendant le saut
        bLockRotationDuringJump = true;
        // Lance le saut dans cette direction SEULE
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
    bLockRotationDuringJump = false; // Déverrouille la rotation normale
    if (JumpMontage && GetMesh() && GetMesh()->GetAnimInstance())
    {
        GetMesh()->GetAnimInstance()->Montage_Stop(0.2f, JumpMontage);
    }
    OnJumpLanded();
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
    bIsAiming = true;
    GetCharacterMovement()->MaxWalkSpeed = AimWalkSpeed;
    bIsSprinting = false;
    // Oriente le mesh dans l'axe caméra
    if (Controller)
    {
        FRotator ControlRot = Controller->GetControlRotation();
        FRotator YawRot(0, ControlRot.Yaw, 0);
        SetActorRotation(YawRot);
    }
    TargetSocketOffset = AimSocketOffset;
    if (FollowCamera)
    {
        FollowCamera->SetFieldOfView(AimFOV);
    }
}

void ARelicsCharacter::StopAim()
{
    bIsAiming = false;
    GetCharacterMovement()->MaxWalkSpeed = DefaultWalkSpeed;
    TargetSocketOffset = DefaultSocketOffset;
    if (FollowCamera)
    {
        FollowCamera->SetFieldOfView(DefaultFOV);
    }
}
