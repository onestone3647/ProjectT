// Fill out your copyright notice in the Description page of Project Settings.


#include "PTPlayerCharacter.h"
#include "Components/PTTargetingSystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"

APTPlayerCharacter::APTPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// CapsuleComponent
	GetCapsuleComponent()->InitCapsuleSize(36.0f, 94.0f);

	// SkeletalMesh
	GetMesh()->SetRelativeLocationAndRotation(FVector(0.0f, 0.0f, -94.0f), FRotator(0.0f, -90.0f, 0.0f));
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> SK_Mannequin(TEXT("/Game/Mannequin/Character/Mesh/SK_Mannequin"));
	if(SK_Mannequin.Succeeded() == true)
	{
		GetMesh()->SetSkeletalMesh(SK_Mannequin.Object);
	}

	// Controller
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// CharacterMovement
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->bUseControllerDesiredRotation = false;

	// SpringArm
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	BaseSpringArmLength = 400.0f;
	SpringArm->TargetArmLength = BaseSpringArmLength;
	SpringArm->SocketOffset = FVector(0.0f, 0.0f, 200.0f);
	SpringArm->bUsePawnControlRotation = true;										// 컨트롤러를 기준으로 SpringArm을 회전합니다.
	SpringArm->bEnableCameraLag = true;
	SpringArm->CameraLagSpeed = 10.0f;

	// FollowCamera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;									// 카메라가 SpringArm을 기준으로 회전하지 않습니다.
	FollowCamera->SetRelativeRotation(FRotator(-20.0f, 0.0f, 0.0f));

	// Camera
	BaseTurnRate = 45.0f;
	BaseLookUpRate = 45.0f;
	
	// TargetingSystem
	TargetingSystem = CreateDefaultSubobject<UPTTargetingSystemComponent>(TEXT("TargetingSystem"));
}

void APTPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

void APTPlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void APTPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("Turn", this, &APTPlayerCharacter::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &APTPlayerCharacter::LookUp);

	PlayerInputComponent->BindAxis("TurnRate", this, &APTPlayerCharacter::TurnRate);
	PlayerInputComponent->BindAxis("LookUpRate", this, &APTPlayerCharacter::LookUpRate);

	PlayerInputComponent->BindAxis("MoveForward", this, &APTPlayerCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &APTPlayerCharacter::MoveRight);
	
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &APTPlayerCharacter::Jump);
	
	PlayerInputComponent->BindAction("TargetLockOn", IE_Pressed, this, &APTPlayerCharacter::ExecuteLockOnTarget);
}

#pragma region Camera
void APTPlayerCharacter::Turn(float Value)
{
	if(TargetingSystem->IsLockOnTarget() == true && Value != 0.0f)
	{
		TargetingSystem->ChangeLockOnTargetForTurnValue(EPTInputMode::InputMode_Mouse, Value);
	}
	else
	{
		AddControllerYawInput(Value);
	}
}

void APTPlayerCharacter::LookUp(float Value)
{
	if(TargetingSystem->IsLockOnTarget() == false)
	{
		AddControllerPitchInput(Value);
	}
}

void APTPlayerCharacter::TurnRate(float Rate)
{
	if(TargetingSystem->IsLockOnTarget() == true && Rate != 0.0f)
	{
		TargetingSystem->ChangeLockOnTargetForTurnValue(EPTInputMode::InputMode_Gamepad, Rate);		
	}
	else
	{
		AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
	}
}

void APTPlayerCharacter::LookUpRate(float Rate)
{
	if(TargetingSystem->IsLockOnTarget() == false)
	{
		AddControllerPitchInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
	}
}
#pragma endregion

#pragma region CharacterMovement
void APTPlayerCharacter::MoveForward(float Value)
{
	if ((Controller != nullptr) && (Value != 0.0f))
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void APTPlayerCharacter::MoveRight(float Value)
{
	if ( (Controller != nullptr) && (Value != 0.0f) )
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
	
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		AddMovementInput(Direction, Value);
	}
}
#pragma endregion 

#pragma region TargetingSystem
void APTPlayerCharacter::ExecuteLockOnTarget()
{
	if(TargetingSystem->IsLockOnTarget() == false)
	{
		TargetingSystem->ExecuteLockOnTarget();
	}
	else
	{
		TargetingSystem->CancelLockOnTarget();
	}
}
#pragma endregion 

