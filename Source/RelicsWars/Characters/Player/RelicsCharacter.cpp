// Copyright Epic Games, Inc. All Rights Reserved.
#include "RelicsWars/Characters/Player/RelicsCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/InputComponent.h"
#include "Kismet/KismetMathLibrary.h"

// Constructeur : initialise la cam�ra third-person, le spring arm et les param�tres de rotation
ARelicsCharacter::ARelicsCharacter()
    : InputDirection(FVector::ZeroVector)
    , TargetRotation(FRotator::ZeroRotator)
    , CharacterRotationInterpSpeed(8.0f)
    , CameraSocketOffset(0.f, 60.f, 70.f) // d�calage �paule droite, hauteur r�aliste
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
    CameraBoom->TargetArmLength = 350.0f; // distance cam�ra r�aliste TPS
    CameraBoom->bUsePawnControlRotation = true;
    CameraBoom->SocketOffset = CameraSocketOffset; // d�calage �paule droite
    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;
    // Correction : d�sactive la rotation auto
    bUseControllerRotationYaw = false;
    GetCharacterMovement()->bOrientRotationToMovement = false;
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed; // Initialise la vitesse de marche par dfaut
    GetCharacterMovement()->JumpZVelocity = 300.f; // Hauteur de saut rDUITE
}

void ARelicsCharacter::BeginPlay()
{
    Super::BeginPlay();
    // S'assure que la cam�ra est bien positionn�e au d�marrage
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

// Tick : g�re la rotation fluide du personnage et l'interpolation dynamique de la cam�ra
void ARelicsCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    // Blend fluide entre TPS et vis�e
    CameraBlendAlpha = FMath::FInterpTo(CameraBlendAlpha, bIsAiming ? 1.0f : 0.0f, DeltaTime, CameraBlendSpeed);
    if (CameraBoom)
    {
        CameraBoom->SocketOffset = FMath::Lerp(TPS_SocketOffset, Aim_SocketOffset, CameraBlendAlpha);
    }
    if (FollowCamera)
    {
        FollowCamera->SetFieldOfView(FMath::Lerp(TPS_FOV, Aim_FOV, CameraBlendAlpha));
    }
    // Option Pro : tourner le mesh en m�me temps que la cam�ra en vis�e
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
    // Saut diff�r�
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
    // Reset InputDirection apr�s traitement pour �viter la rotation � l'arr�t
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

// --- Syst�me de roulade avant fluide ---
void ARelicsCharacter::StartRoll()
{
    // Emp�che la relance si une roulade est d�j� en cours, si le cooldown est actif, ou si le personnage n'est pas au sol
    if (bIsRolling || !bCanRoll || GetCharacterMovement()->IsFalling())
        return;

    // Bloque la roulade en idle (vitesse horizontale quasi nulle)
    if (GetVelocity().Size2D() < 10.f)
        return;

    // D�sactive la vis�e au d�but de la roulade
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
    // Si le bouton de vis�e est toujours press�, r�active la vis�e
    if (bIsAimInputPressed)
        bIsAiming = true;
}

// Handler d'input Aim (Pressed)
void ARelicsCharacter::OnAimPressed()
{
    bIsAimInputPressed = true;
    // --- Gestion avanc�e de la vis�e pendant le saut ---
    if (IsInAir || bIsJumping)
    {
        bWantsToAimAfterJump = true; // Met � jour l'�tat si le joueur appuie pendant le saut
        return;
    }
    // Active la vis�e uniquement si pas en roulade ni en saut
    if (!bIsRolling && !IsInAir && !bIsJumping)
        bIsAiming = true;
}

// Handler d'input Aim (Released)
void ARelicsCharacter::OnAimReleased()
{
    bIsAimInputPressed = false;
    // --- Gestion avanc�e de la vis�e pendant le saut ---
    if (IsInAir || bIsJumping)
    {
        bWantsToAimAfterJump = false; // Met � jour l'�tat si le joueur rel�che pendant le saut
        return;
    }
    bIsAiming = false;
}

// Affecte uniquement la valeur de l'axe
void ARelicsCharacter::MoveForward(float Value)
{
    // En mode vis�e, on ne bloque pas le d�placement en l'air
    if (bIsAiming)
    {
        AimForward = Value;
        // D�placement TPS "in place" : avance/recule lentement, strafe possible
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
        // D�placement TPS classique
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
    // En mode vis�e, on ne bloque pas le d�placement en l'air
    if (bIsAiming)
    {
        AimRight = Value;
        // D�placement TPS "in place" : strafe gauche/droite lentement
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
        // D�placement TPS classique
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
        OnSprintBlockedFeedback(); // Feedback BP/FX si sprint bloqu�
        return;
    }
    bIsSprinting = true;
    GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
    SprintTimeLeft = SprintDurationMax;
}

// Sprint : d�sactive le sprint
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
        // D�sactive la vis�e si active
        if (bIsAiming)
        {
            bIsAiming = false;
            bWantsToAimAfterJump = bIsAimInputPressed;
            UE_LOG(LogTemp, Log, TEXT("Vis�e d�sactiv�e lors du saut."));
        }
        else
        {
            bWantsToAimAfterJump = false;
        }
        // Calcul direction du saut
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
            InputDir = GetActorForwardVector(); // Saut en avant par d�faut
        }
        JumpDirection = InputDir.GetSafeNormal();
        SetActorRotation(JumpDirection.Rotation());
        bLockRotationDuringJump = true;
        FVector LaunchDir = JumpDirection * JumpForwardSpeed;
        LaunchDir.Z = GetCharacterMovement()->JumpZVelocity;
        LaunchCharacter(LaunchDir, true, true);
        ACharacter::Jump();
        // Joue explicitement le montage d'animation de saut
        if (JumpMontage && GetMesh() && GetMesh()->GetAnimInstance())
        {
            GetMesh()->GetAnimInstance()->Montage_Play(JumpMontage, 1.0f);
        }
        // Appelle OnJumpStarted m�me si on vient de la vis�e
        OnJumpStarted();
        FTimerDelegate CanJumpDelegate;
        CanJumpDelegate.BindLambda([this]
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
    bLockRotationDuringJump = false; // D�verrouille la rotation normale
    if (JumpMontage && GetMesh() && GetMesh()->GetAnimInstance())
    {
        GetMesh()->GetAnimInstance()->Montage_Stop(0.2f, JumpMontage);
    }
    OnJumpLanded();
    // Gestion de la vis�e apr�s le saut (identique � la roulade)
    if (bWantsToAimAfterJump && bIsAimInputPressed)
    {
        bIsAiming = true; // R�active la vis�e
        UE_LOG(LogTemp, Log, TEXT("Vis�e r�activ�e apr�s atterrissage (saut)."));
    }
    else
    {
        bIsAiming = false;
    }
    bWantsToAimAfterJump = false; // Reset
}

void ARelicsCharacter::OnJumpLanded()
{
    // Logique suppl�mentaire pour la transition Idle/Run peut �tre ajout�e ici ou dans le Blueprint
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

// Gestion de la vis�e TPS
void ARelicsCharacter::StartAim()
{
    // Emp�che la vis�e pendant la roulade ou le saut
    if (bIsRolling || IsInAir || bIsJumping)
        return;
    bIsAiming = true;
}

void ARelicsCharacter::StopAim()
{
    bIsAiming = false;
}
