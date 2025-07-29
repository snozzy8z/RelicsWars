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
    , WalkSpeed(350.f) // Vitesse de marche réaliste Uncharted 2
    , SprintSpeed(500.f) // Vitesse de sprint réaliste Uncharted 2
    , bIsSprinting(false)
{
    PrimaryActorTick.bCanEverTick = true;

    // SpringArm pour vue épaule
    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 300.0f;
    CameraBoom->bUsePawnControlRotation = true;
    CameraBoom->SocketOffset = CameraSocketOffset;

    // Caméra third-person
    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;

    // Désynchronise la rotation du personnage et de la caméra
    bUseControllerRotationYaw = false;
    GetCharacterMovement()->bOrientRotationToMovement = false;

    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed; // Initialise la vitesse de marche par défaut
}

void ARelicsCharacter::BeginPlay()
{
    Super::BeginPlay();
}

// Tick : gère la rotation fluide du personnage et l'interpolation dynamique de la caméra
void ARelicsCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    RotateCharacterToMovement(DeltaTime);
    // Interpole dynamiquement la position de la caméra (SocketOffset)
    if (CameraBoom)
    {
        CameraBoom->SocketOffset = FMath::VInterpTo(CameraBoom->SocketOffset, TargetSocketOffset, DeltaTime, 6.0f);
    }
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
    // Sprint bindings
    PlayerInputComponent->BindAction("Sprint", IE_Pressed, this, &ARelicsCharacter::StartSprint);
    PlayerInputComponent->BindAction("Sprint", IE_Released, this, &ARelicsCharacter::StopSprint);
}

// Affecte uniquement la valeur de l'axe
void ARelicsCharacter::MoveForward(float Value)
{
    InputDirection.X = Value;
}

void ARelicsCharacter::MoveRight(float Value)
{
    InputDirection.Y = Value;
}

void ARelicsCharacter::Turn(float Value)
{
    AddControllerYawInput(Value);
}

void ARelicsCharacter::LookUp(float Value)
{
    AddControllerPitchInput(Value);
}

// Rotation fluide du personnage vers la direction du mouvement
void ARelicsCharacter::RotateCharacterToMovement(float DeltaTime)
{
    if (Controller)
    {
        const FRotator Rotation = Controller->GetControlRotation();
        const FRotator YawRotation(0, Rotation.Yaw, 0);
        const FVector ForwardDir = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
        const FVector RightDir = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
        FVector MoveDir = ForwardDir * InputDirection.X + RightDir * InputDirection.Y;
        if (MoveDir.Size() > 0.1f)
        {
            // Applique le mouvement dans la bonne direction
            AddMovementInput(MoveDir.GetSafeNormal(), MoveDir.Size());
            // Interpolation de la rotation du personnage
            FRotator CurrentRotation = GetActorRotation();
            FRotator DesiredRotation = MoveDir.Rotation();
            TargetRotation = FMath::RInterpTo(CurrentRotation, DesiredRotation, DeltaTime, CharacterRotationInterpSpeed);
            SetActorRotation(TargetRotation);
        }
    }
}

// Sprint : active le sprint
void ARelicsCharacter::StartSprint()
{
    bIsSprinting = true;
    GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
}

// Sprint : désactive le sprint
void ARelicsCharacter::StopSprint()
{
    bIsSprinting = false;
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

// Saut Uncharted : override Jump pour appliquer une impulsion vers l'avant
void ARelicsCharacter::Jump()
{
    // Calcule la vitesse horizontale (ignore Z)
    float Speed2D = GetVelocity().Size2D();
    // Détecte le saut immobile : vitesse < 10 uu/s
    bIsIdleJump = (Speed2D < 10.f);

    // Interdit le saut en idle
    if (bIsIdleJump)
    {
        return; // Ignore la demande de saut si idle
    }

    // Appelle le saut classique ou directionnel
    Super::Jump();
    // Animation Blueprint : utilise IsIdleJumping() pour piloter la transition
}

bool ARelicsCharacter::IsIdleJumping() const
{
    return bIsIdleJump;
}

void ARelicsCharacter::Landed(const FHitResult& Hit)
{
    Super::Landed(Hit);
    bIsIdleJump = false; // Reset du statut à l'atterrissage
    // FX, son, reset d'états
}
