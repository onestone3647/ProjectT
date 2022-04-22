// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/PTTargetingSystemComponent.h"
#include "PTPlayerCharacter.h"
#include "PTDummyCharacter.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"

UPTTargetingSystemComponent::UPTTargetingSystemComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	
	// Debug
	bDrawDebug = false;
	DebugTargetSphereSize = 30.0f;
	DebugTargetSphereSegment = 30.0f;

	// TargetingSystem
	bIsLockOnTarget = false;
	Target = nullptr;
	MaxSearchTargetableDistance = 2000.0f;
	InterpSpeed = 5.0f;
	LastTimeChangeTarget = 0.0f;
	TimeSinceLastChangeTarget = 0.0f;
	MinMouseValueToChangeTarget = 3.0f;
	MinDelayTimeToChangeTarget = 0.3f;
	bAnalogNeutralSinceLastSwitchTarget = false;
	MinAnalogValueToChangeTarget = 0.2f;
}

void UPTTargetingSystemComponent::BeginPlay()
{
	Super::BeginPlay();

	InitializePlayerReference();
}

void UPTTargetingSystemComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateCameraLock();
	UpdateDrawDebug();
}

#pragma region Debug
void UPTTargetingSystemComponent::UpdateDrawDebug()
{
	if(bDrawDebug)
	{
		// 탐지하는 범위를 드로우 디버그합니다.
		DrawDebugSphere(GetWorld(), PTPlayerOwner->GetActorLocation(), MaxSearchTargetableDistance,
						DebugTargetSphereSegment, FColor::Green);

		// Target을 드로우 디버그합니다.
		if(IsValid(Target) == true)
		{
			APTDummyCharacter* TargetPTDummyCharacter = Cast<APTDummyCharacter>(Target);
			if(TargetPTDummyCharacter)
			{
				DrawDebugSphere(GetWorld(), TargetPTDummyCharacter->GetTargetAimLocation(),
								DebugTargetSphereSize, DebugTargetSphereSegment, FColor::Red);
			}
		}

		// Target을 제외한 TargetableActors를 드로우 디버그합니다.
		TArray<AActor*> TargetableActors = SearchTargetableActors();
		TargetableActors.Remove(Target);
		for(AActor* TargetableActor : TargetableActors)
		{
			APTDummyCharacter* TargetablePTDummyCharacter = Cast<APTDummyCharacter>(TargetableActor);
			if(TargetablePTDummyCharacter)
			{
				DrawDebugSphere(GetWorld(), TargetablePTDummyCharacter->GetTargetAimLocation(),
								DebugTargetSphereSize, DebugTargetSphereSegment, FColor::Green);
			}
		}
	}
}
#pragma endregion 

#pragma region PlayerReference
void UPTTargetingSystemComponent::InitializePlayerReference()
{
	APTPlayerCharacter* NewPTPlayerOwner = Cast<APTPlayerCharacter>(GetOwner());
	if(IsValid(NewPTPlayerOwner) == true)
	{
		PTPlayerOwner = NewPTPlayerOwner;
	}
	
	PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	PlayerCameraManager = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0);
}
#pragma endregion

#pragma region TargetingSystem
bool UPTTargetingSystemComponent::IsLockOnTarget() const
{
	return bIsLockOnTarget && IsValid(Target) == true;
}

void UPTTargetingSystemComponent::ExecuteLockOnTarget()
{
	SetTarget(FindTarget());
	if(IsValid(Target) == true)
	{
		EnableCameraLock();
	}
	else
	{
		CancelLockOnTarget();
	}
}

void UPTTargetingSystemComponent::CancelLockOnTarget()
{
	DisableCameraLock();
	
	Target = nullptr;
}

void UPTTargetingSystemComponent::ChangeLockOnTargetForTurnValue(EPTInputMode InputMode, float TurnValue)
{
	APTDummyCharacter* TargetPTDummyCharacter = Cast<APTDummyCharacter>(Target);
	if(IsValid(TargetPTDummyCharacter) == true && TargetPTDummyCharacter->IsDead() == false)
	{
		TargetPTDummyCharacter->SelectedTarget(false);
	}

	switch(InputMode)
	{
	case EPTInputMode::InputMode_Mouse:
		TimeSinceLastChangeTarget = GetWorld()->GetRealTimeSeconds() - LastTimeChangeTarget;
		if(FMath::Abs(TurnValue) > MinMouseValueToChangeTarget && TimeSinceLastChangeTarget > MinDelayTimeToChangeTarget)
		{
			if(TurnValue < 0.0f)
			{
				SetTarget(FindDirectionalTarget(EPTTargetDirection::TargetDirection_Left));
			}
			else
			{
				SetTarget(FindDirectionalTarget(EPTTargetDirection::TargetDirection_Right));
			}

			LastTimeChangeTarget = GetWorld()->GetRealTimeSeconds();
		}
		break;
	case EPTInputMode::InputMode_Gamepad:
		if(FMath::Abs(TurnValue) < MinAnalogValueToChangeTarget)
		{
			bAnalogNeutralSinceLastSwitchTarget = true;
		}

		if(FMath::Abs(TurnValue) >= MinAnalogValueToChangeTarget && bAnalogNeutralSinceLastSwitchTarget)
		{
			if(TurnValue < KINDA_SMALL_NUMBER)
			{
				SetTarget(FindDirectionalTarget(EPTTargetDirection::TargetDirection_Left));
			}
			else if(TurnValue > KINDA_SMALL_NUMBER)
			{
				SetTarget(FindDirectionalTarget(EPTTargetDirection::TargetDirection_Right));
			}

			bAnalogNeutralSinceLastSwitchTarget = false;
		}
		break;
	default:
		break;
	}

	EnableCameraLock();
}

void UPTTargetingSystemComponent::UpdateCameraLock()
{
	if(bIsLockOnTarget)
	{
		if(IsValid(Target) == true && PTPlayerOwner->GetDistanceTo(Target) <= MaxSearchTargetableDistance)
		{
			APTDummyCharacter* TargetPTDummyCharacter = Cast<APTDummyCharacter>(Target);
			if(IsValid(TargetPTDummyCharacter) == true && TargetPTDummyCharacter->IsDead() == false)
			{
				if(PlayerController)
				{
					const FRotator InterpToTarget = CalculateInterpToTarget(Target);
					PlayerController->SetControlRotation(InterpToTarget);
				}
			}
			else
			{
				CancelLockOnTarget();
				ExecuteLockOnTarget();
			}
		}
		else
		{
			CancelLockOnTarget();
		}
	}
}

void UPTTargetingSystemComponent::EnableCameraLock()
{
	APTDummyCharacter* TargetPTDummyCharacter = Cast<APTDummyCharacter>(Target);
	if(IsValid(TargetPTDummyCharacter) == true)
	{
		bIsLockOnTarget = true;
		TargetPTDummyCharacter->SelectedTarget(true);
	}
}

void UPTTargetingSystemComponent::DisableCameraLock()
{
	bIsLockOnTarget = false;

	APTDummyCharacter* TargetPTDummyCharacter = Cast<APTDummyCharacter>(Target);
	if(IsValid(TargetPTDummyCharacter) == true)
	{
		TargetPTDummyCharacter->SelectedTarget(false);
	}
}

void UPTTargetingSystemComponent::SetTarget(AActor* NewTarget)
{
	if(IsValid(NewTarget) == true)
	{
		Target = NewTarget;
	}
}

AActor* UPTTargetingSystemComponent::FindTarget() const
{
	TArray<AActor*> TargetableActors = SearchTargetableActors();
	AActor* NewTarget = nullptr;
	float DistanceFromCenterOfViewport = 0.0f;
	const FVector2D ViewportSize = GEngine->GameViewport->Viewport->GetSizeXY();

	// 화면의 중앙에 가까운 액터를 찾아 NewTarget으로 설정합니다.
	int32 ArrayIndex = 0;
	for(AActor* TargetableActor : TargetableActors)
	{
		const float ActorScreenPosition = UKismetMathLibrary::Abs(GetScreenPositionOfActor(TargetableActor).Get<0>().X
																	- ViewportSize.X / 2.0f);

		if(ArrayIndex == 0)
		{
			DistanceFromCenterOfViewport = ActorScreenPosition;
		}
		else
		{
			if(ActorScreenPosition < DistanceFromCenterOfViewport)
			{
				DistanceFromCenterOfViewport = ActorScreenPosition;
			}
			else
			{
				continue;
			}
		}

		NewTarget = TargetableActor;
		ArrayIndex++;
	}

	return NewTarget;
}

TArray<AActor*> UPTTargetingSystemComponent::SearchTargetableActors() const
{
	const FVector PlayerLocation = PTPlayerOwner->GetActorLocation();

	TArray<FHitResult> HitResults;
	TArray<AActor*> TargetableActors;

	// ECollisionChannel::ECC_GameTraceChannel2는 TraceChannels의 Targeting을 나타냅니다.
	bool bIsHit = GetWorld()->SweepMultiByChannel(HitResults, PlayerLocation, PlayerLocation, FQuat::Identity,
													ECollisionChannel::ECC_GameTraceChannel2, FCollisionShape::MakeSphere(MaxSearchTargetableDistance));
	if(bIsHit)
	{
		for(FHitResult Hit : HitResults)
		{
			if(Hit.Actor.IsValid() == true)
			{
				APTDummyCharacter* HitedAI = Cast<APTDummyCharacter>(Hit.Actor);
				if(IsValid(HitedAI) == true && HitedAI->IsDead() == false)
				{
					TTuple<FVector2D, bool> ActorScreenPosition = GetScreenPositionOfActor(HitedAI);
					if(IsInViewport(ActorScreenPosition.Get<0>()) == true && ActorScreenPosition.Get<1>() == true)
					{
						// 중복되지 않게 TargetableActors Array에 추가합니다.
						TargetableActors.AddUnique(Cast<AActor>(Hit.Actor));
					}
				}
			}
		}
	}
	
	return TargetableActors;
}

AActor* UPTTargetingSystemComponent::FindDirectionalTarget(EPTTargetDirection NewTargetDirection)
{
	AActor* NewTarget = nullptr;
	TArray<AActor*> TargetableActors = SearchTargetableActors();
	const int32 CurrentTargetIndex = TargetableActors.Find(Target);

	// 현재 Target이 TargetableActors 배열에 존재할 경우 배열에서 제거합니다.
	if(CurrentTargetIndex != INDEX_NONE)
	{
		TargetableActors.Remove(Target);
	}

	// 현재 Target을 기준으로 좌우에 있는 Target을 탐색합니다.
	TArray<AActor*> LeftTargetableActors;
	TArray<AActor*> RightTargetableActors;

	for(AActor* TargetableActor : TargetableActors)
	{
		APTDummyCharacter* TargetablePRAICharacter = Cast<APTDummyCharacter>(TargetableActor);
		if(IsValid(TargetablePRAICharacter) == true && TargetablePRAICharacter->IsDead() == false)
		{
			switch(WhichSideOfTarget(TargetablePRAICharacter))
			{
			case EPTTargetDirection::TargetDirection_Left:
				LeftTargetableActors.AddUnique(TargetablePRAICharacter);
				break;
			case EPTTargetDirection::TargetDirection_Right:
				RightTargetableActors.AddUnique(TargetablePRAICharacter);
				break;
			default:
				break;
			}
		}
	}

	switch(NewTargetDirection)
	{
	case EPTTargetDirection::TargetDirection_Left:
		if(LeftTargetableActors.Num() > 0)
		{
			// 왼쪽에서 내적이 가장 큰 Target
			NewTarget = GetTargetByDotProduct(LeftTargetableActors, true);
		}
		else
		{
			// 오른쪽에서 내적이 가장 작은 Target
			NewTarget = GetTargetByDotProduct(RightTargetableActors, false);
		}
		break;
	case EPTTargetDirection::TargetDirection_Right:
		if(RightTargetableActors.Num() > 0)
		{
			// 오른쪽에서 내적이 가장 큰 Target
			NewTarget = GetTargetByDotProduct(RightTargetableActors, true);
		}
		else
		{
			// 왼쪽에서 내적이 가장 작은 Target
			NewTarget = GetTargetByDotProduct(LeftTargetableActors, false);
		}
		break;
	default:
		break;
	}

	if(IsValid(NewTarget) == true)
	{
		return NewTarget;
	}
	
	return nullptr;
}

EPTTargetDirection UPTTargetingSystemComponent::WhichSideOfTarget(AActor* NewTargetableActor) const
{
	const FVector TargetActorDirection = UKismetMathLibrary::GetDirectionUnitVector(PlayerCameraManager->GetCameraLocation(),
																					Target->GetActorLocation());
	const FVector NewTargetableActorDirection = UKismetMathLibrary::GetDirectionUnitVector(PlayerCameraManager->GetCameraLocation(),
																							NewTargetableActor->GetActorLocation());
	const FVector NewDirection = UKismetMathLibrary::Cross_VectorVector(TargetActorDirection, NewTargetableActorDirection);
	const float NewDot = UKismetMathLibrary::Dot_VectorVector(NewDirection, GetOwner()->GetActorUpVector());

	if(NewDot >= 0.0f)
	{
		return EPTTargetDirection::TargetDirection_Right;
	}
	
	return EPTTargetDirection::TargetDirection_Left;
}

AActor* UPTTargetingSystemComponent::GetTargetByDotProduct(TArray<AActor*> TargetableActors, bool bIsLargestDot)
{
	float DotProduct = 0.0f;
	AActor* NewTarget = nullptr;
	int32 ArrayIndex = 0;

	if(TargetableActors.Num() == 0)
	{
		return nullptr;
	}

	for(AActor* TargetableActor : TargetableActors)
	{
		if(ArrayIndex == 0)
		{
			DotProduct = CalculateDotProductToTarget(TargetableActor);
			NewTarget = TargetableActor;
		}
		else
		{
			if(bIsLargestDot)
			{
				if(CalculateDotProductToTarget(TargetableActor) > DotProduct)
				{
					DotProduct = CalculateDotProductToTarget(TargetableActor);
					NewTarget = TargetableActor;
				}
			}
			else
			{
				if(CalculateDotProductToTarget(TargetableActor) < DotProduct)
				{
					DotProduct = CalculateDotProductToTarget(TargetableActor);
					NewTarget = TargetableActor;
				}
			}
		}

		ArrayIndex++;
	}

	return NewTarget;
}

float UPTTargetingSystemComponent::CalculateDotProductToTarget(AActor* NewTargetableActor) const
{
	const FVector TargetActorDirection = UKismetMathLibrary::GetDirectionUnitVector(GetOwner()->GetActorLocation(),
																					Target->GetActorLocation());
	const FVector NewTargetableActorDirection = UKismetMathLibrary::GetDirectionUnitVector(GetOwner()->GetActorLocation(),
																							NewTargetableActor->GetActorLocation());
	
	return UKismetMathLibrary::Dot_VectorVector(TargetActorDirection, NewTargetableActorDirection);
}

TTuple<FVector2D, bool> UPTTargetingSystemComponent::GetScreenPositionOfActor(AActor* SearchActor) const
{
	FVector2D ScreenLocation = FVector2D::ZeroVector;
	bool bResult = UGameplayStatics::ProjectWorldToScreen(PlayerController, SearchActor->GetActorLocation(),
														ScreenLocation);

	return MakeTuple(ScreenLocation, bResult);
}

bool UPTTargetingSystemComponent::IsInViewport(FVector2D ActorScreenPosition) const
{
	FVector2D ViewportSize = GEngine->GameViewport->Viewport->GetSizeXY();

	return ActorScreenPosition.X > 0.0f && ActorScreenPosition.Y > 0.0f
			&& ActorScreenPosition.X <= ViewportSize.X && ActorScreenPosition.Y <= ViewportSize.Y;
}

FRotator UPTTargetingSystemComponent::CalculateInterpToTarget(AActor* InterpToTarget) const
{
	if(IsValid(InterpToTarget) == true)
	{
		const FRotator PlayerControllerRotator = PlayerController->GetControlRotation();
		const FVector PlayerLocation = PTPlayerOwner->GetActorLocation();
		const FVector TargetLocation = InterpToTarget->GetActorLocation();

		// 2개의 위치 벡터를 입력하여 2번 째 인자의 위치 벡터를 바라보는 방향정보를 반환합니다.
		const FRotator LookAtTarget = UKismetMathLibrary::FindLookAtRotation(PlayerLocation, TargetLocation);
		// 1번 째 인자에서 2번 째 인자로 회전 보간합니다.
		const FRotator RInterpToRotator = FMath::RInterpTo(PlayerControllerRotator, LookAtTarget,
															GetWorld()->GetDeltaSeconds(), InterpSpeed);

		return RInterpToRotator;
	}
	
	return FRotator::ZeroRotator;
}

AActor* UPTTargetingSystemComponent::GetTarget() const
{
	return Target;
}
#pragma endregion 

