#include "MoveSet.h"
#include "GameFramework/Character.h"
#include "Animation/AnimInstance.h"

UMoveSet::UMoveSet() :
	m_MaxStamina( 100.f ),
	m_MaxForce( 100.f ),
	m_fLongPressDuration( 0.3f )
{
	PrimaryComponentTick.bCanEverTick = false;

	for( auto OpeningAttack : OpeningAttacks )
	{
		OpeningAttack.MontageAnimation = nullptr;

		OpeningAttack.StaminaRequired = OpeningSttacksStats.StaminaRequired;
		OpeningAttack.DealtDamage = OpeningSttacksStats.DealtDamage;
		OpeningAttack.ForceRequired = OpeningSttacksStats.ForceRequired;
	}
}

void UMoveSet::BeginPlay()
{
	Super::BeginPlay();
	
	m_OwningCharacter  = Cast<ACharacter>( GetOwner() );
	m_AnimationInstance = m_OwningCharacter->GetMesh()->GetAnimInstance();
	
	if( !m_OwningCharacter )
	{
		UE_LOG( LogTemp, Error, TEXT( "MoveSet %s does not have owner." ), *GetName() );
		DestroyComponent();
	}
	
	if( !m_AnimationInstance )
	{
		UE_LOG( LogTemp, Error, TEXT( "MoveSet's %s owner %s does not have AnimInstance." ), *GetName(), *m_OwningCharacter->GetName() );
		DestroyComponent();
	}

	if( OpeningAttacks.Num() < 1 || FurtherAttacks.Num() < 1 )
		UE_LOG( LogTemp, Warning, TEXT( "MoveSet %s has 0 opening or further attacks in it." ), *GetName() );

}
/*
void UMoveSet::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
}
*/
FAttackMontage UMoveSet::GetOpeningMontage()
{
	float CurrentDirection = m_AnimationInstance->CalculateDirection( m_OwningCharacter->GetVelocity(), m_OwningCharacter->GetActorRotation() );

	/* Add 45/2 degrees to direction. If direction < 0 add 360 more */
	CurrentDirection += CurrentDirection < 0.f ? 382.5f : 22.5f;

	return OpeningAttacks[ FMath::Clamp( FMath::TruncToInt( CurrentDirection / 45.f ), 0, 7 ) ];
}

FAttackMontage UMoveSet::GetNextMontage( float FirstPressDuration, float SecondPressDuration, FAttackMontage CurrentAttack )
{
	if( FirstPressDuration == 0 && SecondPressDuration == 0 )
	{
		return FAttackMontage( nullptr, 0 );
	}

	FAttackMontage AttackToReturn;

	if( !CurrentAttack.MontageAnimation )
		AttackToReturn = GetOpeningMontage();

	else if( SecondPressDuration == 0.f )
	{
		if( FirstPressDuration < m_fLongPressDuration )
			AttackToReturn = FurtherAttacks[ FName( "Weak" ) ];
		else
			AttackToReturn = FurtherAttacks[ FName( "Strong" ) ];

	}
	else
	{
		if( FirstPressDuration < m_fLongPressDuration && SecondPressDuration < m_fLongPressDuration )
			AttackToReturn = FurtherAttacks[ FName( "WeakWeak" ) ];
		
		else if( FirstPressDuration < m_fLongPressDuration && SecondPressDuration >= m_fLongPressDuration )
			AttackToReturn = FurtherAttacks[ FName( "WeakStrong" ) ];

		else if( FirstPressDuration >= m_fLongPressDuration && SecondPressDuration < m_fLongPressDuration )
			AttackToReturn = FurtherAttacks[ FName( "StrongWeak" ) ];

		else if( FirstPressDuration >= m_fLongPressDuration && SecondPressDuration >= m_fLongPressDuration )
			AttackToReturn = FurtherAttacks[ FName( "StrongStrong" ) ];
	}

	return AttackToReturn;
}

