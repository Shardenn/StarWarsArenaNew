#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MoveSet.generated.h"

class ACharacter;
class UAnimInstance;

USTRUCT( BlueprintType )
struct FAttackMontage
{
	GENERATED_BODY()

	UPROPERTY(  EditAnywhere, BlueprintReadWrite, Category = "AttackMontageStruct" )
	UAnimMontage * MontageAnimation;

	/* In percentage */
	UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "AttackMontageStruct" )
	int32 StaminaRequired = 0;
	/* In percentage */
	UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "AttackMontageStruct" )
	int32 ForceRequired = 0 ;
	/* In absolute amount */
	UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "AttackMontageStruct" )
	int32 DealtDamage = 0;

	float PlayRate = 1.f;

	FAttackMontage & operator=( FAttackMontage other )
	{
		MontageAnimation = other.MontageAnimation;
		StaminaRequired = other.StaminaRequired;
		ForceRequired = other.ForceRequired;
		DealtDamage = other.DealtDamage;
		PlayRate = other.PlayRate;

		return other;
	}

	FAttackMontage( UAnimMontage * Montage, int32 Stamina, int32 Force, int32 Damage, float PlayR = 1.f )
	{
		MontageAnimation = Montage;
		StaminaRequired = Stamina;
		ForceRequired = Force;
		DealtDamage = Damage;
		PlayRate = PlayR;
	}

	FAttackMontage( UAnimMontage * Montage, int32 InF, float PlayR = 1.f )
	{
		if( !InF || !Montage )
		{
			MontageAnimation = nullptr;
			StaminaRequired = 0;
			ForceRequired = 0;
			DealtDamage = 0;
		}
		else
		{
			FAttackMontage( Montage, InF, InF, InF, PlayR );
		}
	}

	/* NULL constructor */
	FAttackMontage()
	{
		FAttackMontage( nullptr, 0, 1.f );
	}
};

/**
* Class that defines Move Set of a character.
* Character should have an animation instance obviously.
* Opening attacks array stores what attacks to play first in combo -
* sort them from "front run" to "left front" in clockwise.
* Further attacks stores all further attacks - for now we
* select 2nd attacks based on presses done during first attack and then
* select random one
*
* 
*/
UCLASS( ClassGroup = ( Custom ), meta = ( BlueprintSpawnableComponent ) )
class STARWARSARENA_API UMoveSet : public UActorComponent
{
	GENERATED_BODY()

public:
	UMoveSet();
	
	FAttackMontage							GetOpeningMontage();

	FAttackMontage							GetNextMontage( float FirstPressDuration, float SecondPressDuration, FAttackMontage CurrentAttack );

	void									SetLongPressDuration( float NewLength )										{ m_fLongPressDuration = NewLength; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY( BlueprintReadWrite, EditAnywhere, meta = ( DisplayName = "OpeningAttacks" ) )
	TArray<FAttackMontage>					OpeningAttacks;

	UPROPERTY( BlueprintReadWrite, EditAnywhere, meta = ( DisplayName = "FurtherAttacks" ) )
	TMap<FName, FAttackMontage>				FurtherAttacks;

	UPROPERTY( BlueprintReadWrite, EditDefaultsOnly, Meta = ( DisplayName = "OpeningAttacksStats" ) )
	FAttackMontage							OpeningSttacksStats;
private:
	ACharacter		*						m_OwningCharacter;
	UAnimInstance	*						m_AnimationInstance;

	float									m_fLongPressDuration;

	float									m_MaxStamina;
	float									m_MaxForce;
};
