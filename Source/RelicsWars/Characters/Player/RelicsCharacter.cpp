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
    , bIsSprinting(false)
    , bIsRolling(false)
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
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
    GetCharacterMovement()->JumpZVelocity = 400.f;
}

void ARelicsCharacter::BeginPlay()
{
    Super::BeginPlay();
}

// Tick : gère la rotation fluide du personnage et l'interpolation dynamique de la caméra
void ARelicsCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    RotateCharacterToMovement(DeltaTime); // Réactivé : le personnage se tourne dans la direction du déplacement
    if (CameraBoom)
    {
        CameraBoom->SocketOffset = FMath::VInterpTo(CameraBoom->SocketOffset, TargetSocketOffset, DeltaTime, 6.0f);
    }
    IsInAir = GetCharacterMovement()->IsFalling(); // Met à jour IsInAir à chaque tick
    // Saut différé : si le joueur veut sauter et le perso est au sol
    if (bWantsToJump && GetCharacterMovement()->IsMovingOnGround())
    {
        // Timer court pour garantir la synchronisation AnimBP (50ms)
        FTimerDelegate DelayedJumpDelegate;
        DelayedJumpDelegate.BindLambda([this]()
        {
            if (bWantsToJump && GetCharacterMovement()->IsMovingOnGround())
            {
                ACharacter::Jump();
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
    // Bind roulade sur la touche C (action "Roll")
    PlayerInputComponent->BindAction("Roll", IE_Pressed, this, &ARelicsCharacter::StartRoll);
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
    if (GetCharacterMovement()->IsFalling())
        return;
    if (Controller && Value != 0.0f)
    {
        FRotator ControlRot = Controller->GetControlRotation();
        FRotator YawRot(0, ControlRot.Yaw, 0);
        FVector ForwardDir = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
        InputDirection += ForwardDir * Value;
    }
}

void ARelicsCharacter::MoveRight(float Value)
{
    if (GetCharacterMovement()->IsFalling())
        return;
    if (Controller && Value != 0.0f)
    {
        FRotator ControlRot = Controller->GetControlRotation();
        FRotator YawRot(0, ControlRot.Yaw, 0);
        FVector RightDir = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);
        InputDirection += RightDir * Value;
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
    if (JumpMontage && GetMesh() && GetMesh()->GetAnimInstance())
    {
        GetMesh()->GetAnimInstance()->Montage_Stop(0.2f, JumpMontage);
    }
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
