// Copyright (c) 2024. Uncharted 2-inspired Third Person Character for Unreal Engine 5
#include "RelicsCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/InputComponent.h"
#include "Kismet/KismetMathLibrary.h"

ARelicsCharacter::ARelicsCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    // --- Caméra épaule ---
    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 300.0f;
    CameraBoom->bUsePawnControlRotation = true;
    CameraBoom->SocketOffset = TargetSocketOffset;

    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;

    bUseControllerRotationYaw = false;
    GetCharacterMovement()->bOrientRotationToMovement = false;
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

void ARelicsCharacter::BeginPlay()
{
    Super::BeginPlay();
}

// Tick : gère la rotation douce et la caméra épaule
void ARelicsCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    bIsFalling = GetCharacterMovement()->IsFalling();
    // --- Steering subtil pendant la roulade ---
    if (bIsRolling)
    {
        FVector SteerDir = FVector::ZeroVector;
        if (Controller)
        {
            const FRotator Rotation = Controller->GetControlRotation();
            const FRotator YawRotation(0, Rotation.Yaw, 0);
            const FVector ForwardDir = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
            const FVector RightDir = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
            SteerDir = ForwardDir * InputDirection.X + RightDir * InputDirection.Y;
        }
        if (SteerDir.Size() > 0.05f)
        {
            FVector Steer = SteerDir.GetSafeNormal() * SteeringRatio * RollImpulseSpeed * DeltaTime;
            FVector CurrentVel = GetVelocity();
            FVector Forward = CurrentVel.GetSafeNormal();
            float Angle = FMath::RadiansToDegrees(acosf(FVector::DotProduct(Forward, Steer.GetSafeNormal())));
            if (Angle < RollSteerMaxAngle)
            {
                FVector NewVel = CurrentVel + Steer;
                NewVel = NewVel.GetClampedToMaxSize(RollImpulseSpeed);
                LaunchCharacter(NewVel, true, false);
            }
        }
    }
    // --- Steering subtil pendant le saut ---
    if (bIsJumping && bIsFalling)
    {
        FVector SteerDir = FVector::ZeroVector;
        if (Controller)
        {
            const FRotator Rotation = Controller->GetControlRotation();
            const FRotator YawRotation(0, Rotation.Yaw, 0);
            const FVector ForwardDir = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
            const FVector RightDir = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
            SteerDir = ForwardDir * InputDirection.X + RightDir * InputDirection.Y;
        }
        if (SteerDir.Size() > 0.05f)
        {
            FVector Steer = SteerDir.GetSafeNormal() * SteeringRatio * SprintSpeed * DeltaTime;
            FVector CurrentVel = GetVelocity();
            FVector Forward = CurrentVel.GetSafeNormal();
            float Angle = FMath::RadiansToDegrees(acosf(FVector::DotProduct(Forward, Steer.GetSafeNormal())));
            if (Angle < 20.f)
            {
                FVector NewVel = CurrentVel + Steer;
                NewVel = NewVel.GetClampedToMaxSize(SprintSpeed * 0.65f);
                LaunchCharacter(NewVel, true, false);
            }
        }
    }
    // --- Rotation douce et inertie ---
    if (!bIsRolling && !bRollBlendOut && !bIsJumping && !bIsFalling)
    {
        RotateCharacterToMovement(DeltaTime);
    }
    // --- Caméra épaule dynamique ---
    if (CameraBoom)
    {
        CameraBoom->SocketOffset = FMath::VInterpTo(CameraBoom->SocketOffset, TargetSocketOffset, DeltaTime, CameraArmInterpSpeed);
    }
}

// Setup des inputs avec gestion des cooldowns et locks d'état
void ARelicsCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    check(PlayerInputComponent);
    PlayerInputComponent->BindAxis("MoveForward", this, &ARelicsCharacter::MoveForward);
    PlayerInputComponent->BindAxis("MoveRight", this, &ARelicsCharacter::MoveRight);
    PlayerInputComponent->BindAxis("Turn", this, &ARelicsCharacter::Turn);
    PlayerInputComponent->BindAxis("LookUp", this, &ARelicsCharacter::LookUp);
    PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ARelicsCharacter::Jump);
    PlayerInputComponent->BindAction("Sprint", IE_Pressed, this, &ARelicsCharacter::StartSprint);
    PlayerInputComponent->BindAction("Sprint", IE_Released, this, &ARelicsCharacter::StopSprint);
    PlayerInputComponent->BindAction("Roll", IE_Pressed, this, &ARelicsCharacter::StartRoll);
}

// --- Déplacement fluide Naughty Dog ---
void ARelicsCharacter::MoveForward(float Value)
{
    // Autorise le déplacement pendant la roulade, sauf le saut
    if (bIsJumping || bIsFalling) return;
    InputDirection.X = Value;
}
void ARelicsCharacter::MoveRight(float Value)
{
    if (bIsJumping || bIsFalling) return;
    InputDirection.Y = Value;
}
void ARelicsCharacter::Turn(float Value)
{
    // Autorise la rotation caméra même pendant le saut et la roulade
    AddControllerYawInput(Value);
}
void ARelicsCharacter::LookUp(float Value)
{
    AddControllerPitchInput(Value);
}

// --- Sprint ---
void ARelicsCharacter::StartSprint()
{
    if (bIsRolling || bRollBlendOut || bIsJumping || bIsFalling) return;
    bIsSprinting = true;
    GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
}
void ARelicsCharacter::StopSprint()
{
    bIsSprinting = false;
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

// --- Saut Uncharted 2 ---
void ARelicsCharacter::Jump()
{
    // Saut autorisé uniquement si au sol, pas en roulade, pas déjà en saut, pas en cooldown, pas en l'air
    if (bIsRolling || bRollBlendOut || bIsJumping || bIsFalling || bJumpOnCooldown || GetCharacterMovement()->IsFalling()) return;
    bIsJumping = true;
    bJumpOnCooldown = true;
    // Transforme InputDirection en direction monde alignée à la caméra
    FVector LaunchDir = FVector::ZeroVector;
    if (Controller)
    {
        const FRotator Rotation = Controller->GetControlRotation();
        const FRotator YawRotation(0, Rotation.Yaw, 0);
        const FVector ForwardDir = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
        const FVector RightDir = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
        LaunchDir = ForwardDir * InputDirection.X + RightDir * InputDirection.Y;
    }
    LaunchDir = LaunchDir.Size() > 0.1f ? LaunchDir.GetSafeNormal() : GetActorForwardVector();
    FVector LaunchImpulse = LaunchDir * (SprintSpeed * 0.65f) + FVector(0,0,JumpUpImpulse);
    LaunchCharacter(LaunchImpulse, false, true);
    UE_LOG(LogTemp, Warning, TEXT("Jump: jump started in camera-aligned direction"));
    // Bloque tout input pendant le saut
    GetWorldTimerManager().SetTimer(JumpBlendOutTimerHandle, this, &ARelicsCharacter::EndJumpBlendOut, JumpBlendOutTime, false);
    // Cooldown strict anti-spam
    GetWorldTimerManager().SetTimer(JumpCooldownHandle, this, &ARelicsCharacter::EndJumpCooldown, JumpCooldown, false);
}

// Fin du blending du saut : réactive le contrôle après un court délai
void ARelicsCharacter::EndJumpBlendOut()
{
    bIsJumping = false;
}

// Fin du cooldown de saut
void ARelicsCharacter::EndJumpCooldown()
{
    bJumpOnCooldown = false;
}

// Atterrissage : transition AnimBP et restauration du contrôle
void ARelicsCharacter::Landed(const FHitResult& Hit)
{
    Super::Landed(Hit);
    bIsJumping = false;
    bIsFalling = false;
}

// --- Roulade Uncharted 2 Multiplayer ---
void ARelicsCharacter::StartRoll()
{
    // Protection anti-spam : return immédiat avant toute logique si lock d'état
    if (bIsRolling || bRollBlendOut || bIsJumping || bIsFalling || bRollOnCooldown || GetCharacterMovement()->IsFalling())
    {
        UE_LOG(LogTemp, Warning, TEXT("StartRoll IGNORED: bIsRolling=%d, bRollBlendOut=%d, bIsJumping=%d, bIsFalling=%d, bRollOnCooldown=%d"), bIsRolling, bRollBlendOut, bIsJumping, bIsFalling, bRollOnCooldown);
        return;
    }
    // Impossible de lancer une roulade si à l'arrêt
    FVector RollDir = FVector::ZeroVector;
    float InputMag = InputDirection.Size();
    if (Controller)
    {
        const FRotator Rotation = Controller->GetControlRotation();
        const FRotator YawRotation(0, Rotation.Yaw, 0);
        const FVector ForwardDir = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
        const FVector RightDir = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
        RollDir = ForwardDir * InputDirection.X + RightDir * InputDirection.Y;
    }
    if (RollDir.Size() < 0.1f)
    {
        RollDir = GetVelocity().Size() > 0.1f ? GetVelocity().GetSafeNormal() : GetActorForwardVector();
    }
    else
    {
        RollDir = RollDir.GetSafeNormal();
    }
    float Speed = GetVelocity().Size();
    if (Speed < 10.f && InputMag < 0.1f)
    {
        UE_LOG(LogTemp, Warning, TEXT("StartRoll ignored: cannot roll while idle!"));
        return;
    }
    // --- Démarrage unique de la roulade ---
    bIsRolling = true;
    bIsSprinting = false;
    bRollOnCooldown = true;
    SavedFriction = GetCharacterMovement()->GroundFriction;
    SavedBrakingDeceleration = GetCharacterMovement()->BrakingDecelerationWalking;
    GetCharacterMovement()->GroundFriction = RollFriction;
    GetCharacterMovement()->BrakingDecelerationWalking = RollBrakingDeceleration;
    float Impulse = FMath::Max(SprintSpeed, Speed);
    LaunchCharacter(RollDir * Impulse, true, false);
    UE_LOG(LogTemp, Warning, TEXT("StartRoll: roll started in camera-aligned direction"));
    GetWorldTimerManager().SetTimer(RollTimerHandle, this, &ARelicsCharacter::EndRoll, RollDuration, false);
    GetWorldTimerManager().SetTimer(RollCooldownHandle, this, &ARelicsCharacter::EndRollCooldown, RollCooldown, false);
    bBlendOutActive = false;
}

void ARelicsCharacter::EndRoll()
{
    bIsRolling = false;
    bRollBlendOut = true;
    // Ne lance le blend-out qu'une seule fois
    if (!bBlendOutActive)
    {
        bBlendOutActive = true;
        GetWorldTimerManager().SetTimer(RollBlendOutTimerHandle, this, &ARelicsCharacter::EndRollBlendOut, RollBlendOutTime, false);
        UE_LOG(LogTemp, Warning, TEXT("EndRoll: Blend-out started"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("EndRoll: Blend-out already active, ignored"));
    }
}

void ARelicsCharacter::EndRollBlendOut()
{
    bRollBlendOut = false;
    bBlendOutActive = false;
    // Restaure friction et décélération à la valeur d'origine, même si spam
    GetCharacterMovement()->GroundFriction = 8.0f;
    GetCharacterMovement()->BrakingDecelerationWalking = 2048.0f;
    UE_LOG(LogTemp, Warning, TEXT("EndRollBlendOut: Friction and deceleration forcibly restored"));
    // Restaure le sprint instantanément si le joueur court
    if (bIsSprinting)
    {
        GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
        UE_LOG(LogTemp, Warning, TEXT("EndRollBlendOut: Sprint speed restored after roll"));
    }
}

void ARelicsCharacter::InterpRollFrictionTick()
{
    RollFrictionLerpElapsed += 0.01f;
    float Alpha = FMath::Clamp(RollFrictionLerpElapsed / RollFrictionLerpDuration, 0.f, 1.f);
    GetCharacterMovement()->GroundFriction = FMath::Lerp(RollFrictionLerpStart, RollFrictionLerpEnd, Alpha);
    GetCharacterMovement()->BrakingDecelerationWalking = FMath::Lerp(RollBrakingLerpStart, RollBrakingLerpEnd, Alpha);
    if (Alpha >= 1.f)
    {
        GetWorldTimerManager().ClearTimer(RollFrictionLerpTimerHandle);
    }
}

void ARelicsCharacter::EndRollCooldown()
{
    bRollOnCooldown = false;
}

// --- Rotation douce du personnage vers la direction d'entrée ---
void ARelicsCharacter::RotateCharacterToMovement(float DeltaTime)
{
    // Bloque rotation pendant la roulade ou le saut ou en l'air
    if (bIsRolling || bRollBlendOut || bIsJumping || bIsFalling) return;
    if (Controller)
    {
        const FRotator Rotation = Controller->GetControlRotation();
        const FRotator YawRotation(0, Rotation.Yaw, 0);
        const FVector ForwardDir = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
        const FVector RightDir = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
        FVector MoveDir = ForwardDir * InputDirection.X + RightDir * InputDirection.Y;
        if (MoveDir.Size() > 0.1f)
        {
            AddMovementInput(MoveDir.GetSafeNormal(), MoveDir.Size());
            FRotator CurrentRotation = GetActorRotation();
            FRotator DesiredRotation = MoveDir.Rotation();
            TargetRotation = FMath::RInterpTo(CurrentRotation, DesiredRotation, DeltaTime, CharacterRotationInterpSpeed);
            SetActorRotation(TargetRotation);
        }
    }
}

// --- Cover System (contextuel, fluide) ---
void ARelicsCharacter::EnterCover()
{
    bIsInCover = true;
    // Animation et logique de couverture pilotées par AnimBP et collision
}
void ARelicsCharacter::ExitCover()
{
    bIsInCover = false;
    // Animation et logique de sortie de couverture
}
