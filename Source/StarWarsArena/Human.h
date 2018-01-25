// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine.h"
#include "UnrealNetwork.h"
#include "MoveSet.h"
#include "Human.generated.h"

class ASaber;
class UCharacterMovementComponent;
class UCapsuleComponent;

USTRUCT( BlueprintType )
struct FHumanStats
{
	GENERATED_BODY()

	UPROPERTY( EditDefaultsOnly, BlueprintReadWrite, meta = ( DisplayName = "Health" ) )
	int32 HS_Health;

	UPROPERTY( EditDefaultsOnly, BlueprintReadWrite, meta = ( DisplayName = "Stamina" ) )
	int32 HS_Stamina;

	FHumanStats()
	{
		HS_Health = HS_Stamina = 0;
	}

	FHumanStats( int32 Health, int32 Stamina )
	{
		HS_Health = Health;
		HS_Stamina = Stamina;
	}

	FHumanStats( int32 NewStat )
	{
		HS_Health = HS_Stamina = NewStat;
	}

	FHumanStats Add( FHumanStats other, FHumanStats MaxStats )
	{
		return FHumanStats( FMath::Clamp( HS_Health + other.HS_Health, 0, MaxStats.HS_Health ),
							FMath::Clamp( HS_Stamina + other.HS_Stamina, 0, MaxStats.HS_Stamina ) );
	}
};

UENUM( BlueprintType )
enum class EHumanState : uint8
{
	EHS_Stunned			UMETA( DisplayName = "Stunned" ),  // Stunned
	EHS_Impacted		UMETA( DisplayName = "Impacted" ), // After got hit
	EHS_Free			UMETA( DisplayName = "Free" ),     // Default state
	EHS_Attacking		UMETA( DisplayName = "Attacking" ),// During attack
	EHS_Acrobatic		UMETA( DisplayName = "Acrobatic" ),// During acrobatic stuff ( running on wall, rolling, etc )
	EHS_ChangingCombat	UMETA( DisplayName = "ChangingCombat" ), // Drawing/hiding saber
	EHS_Defending		UMETA( DisplayName = "Defending" ),	// While holding defend button and defending
	EHS_ThrowingSaber	UMETA( DisplayName = "ThrowingSaber" ) // While throwing saber or waiting until it returns from flight
};

UCLASS()
class STARWARSARENA_API AHuman : public ACharacter
{
	GENERATED_BODY()

protected:
	virtual void					BeginPlay() override;


	UPROPERTY( BlueprintReadWrite, EditAnywhere, Category = "Moving", Meta = ( DisplayName = "RunSpeed" ) )
	float							RunSpeed;

	UPROPERTY( BlueprintReadWrite, EditAnywhere, Category = "Moving", Meta = ( DisplayName = "WalkSpeed" ) )
	float							WalkSpeed;

	/* Delay after pressing jump button to perform jump */
	UPROPERTY( BlueprintReadWrite, EditAnywhere, Category = "Moving", Meta = ( DisplayName = "DelayBeforeJump" ) )
	float							DelayBeforeJump;

	/* Should controller wait after player pressed jump button to perform jump? */
	UPROPERTY( BlueprintReadWrite, EditAnywhere, Category = "Moving", Meta = ( DisplayName = "bShouldWaitBeforeJump" ) )
	bool							bShouldWaitBeforeJump = false;

	/* True if saber is in hands */
	UPROPERTY( BlueprintReadWrite, EditDefaultsOnly, Category = "SaberCombat", Meta = ( DisplayName = "bInCombat" ) )
	bool							bInCombat;												

	UPROPERTY( BlueprintReadWrite, EditAnywhere, Category = "SaberCombat", Meta = ( DisplayName = "MaxSaberFlyDistance" ) )
	float							MaxSaberFlyDistance;

	UPROPERTY( BlueprintReadWrite, EditAnywhere, Category = "SaberCombat", Meta = ( DisplayName = "MoveSetComponent" ) )
	UMoveSet *						MoveSet;

	UPROPERTY( BlueprintReadWrite, EditAnywhere, Category = "SaberCombat", Meta = ( DisplayName = "AnimationCutTime" ) )
	float							AnimationCutTime;

	UPROPERTY( BlueprintReadWrite, EditAnywhere, Category = "SaberCombat", Meta = ( DisplayName = "ReverseAttackPlayRate" ) )
	float							ReverseAttackPlayRate = 1.5f;

	/* Max time in seconds that player can be mini stanned */
	UPROPERTY( BlueprintReadWrite, EditAnywhere, Category = "SaberCombat", Meta = ( DisplayName = "MaxImpactTime" ) )
	float							MaxImpactTime = 1.f;

	UPROPERTY( BlueprintReadWrite, EditDefaultsOnly, Category = "Stats", Meta = ( DisplayName = "StartingStats" ) )
	FHumanStats						StartingStats;

	UPROPERTY( BlueprintReadWrite, EditDefaultsOnly, Category = "Stats", Meta = ( DisplayName = "StatsRestoreSpeed" ) )
	FHumanStats						StatsRestoreSpeed;

	UPROPERTY( BlueprintReadWrite, EditDefaultsOnly, Category = "Stats", Meta = ( DisplayName = "StaminaRestoreWhileDefending" ) )
	int32							StaminaRestoreWhileDefending;

	UPROPERTY( BlueprintReadWrite, EditDefaultsOnly, Category = "Stats", Meta = ( DisplayName = "FreeStaminaRestoreSpeed" ) )
	int32							FreeStaminaRestoreSpeed;
	//============================= Events ======================================//

	/* Called when attack button is pressed */
	UFUNCTION( BlueprintImplementableEvent, Category = "Human", Meta = ( DisplayName = "OnAttack" ) )
	void							OnAttack();

	/* Called when player released attack button.  Attack HolTime - amount of time player was holding button */
	UFUNCTION( BlueprintImplementableEvent, Category = "Human", Meta = ( DisplayName = "OnStopAttack" ) )
	void							OnStopAttack( float AttackHoldTime );

	/* Called when player presses throw saber button */
	UFUNCTION( BlueprintImplementableEvent, Category = "Human", Meta = ( DisplayName = "OnThrowSaber" ) )
	void							OnThrowSaber();

	/* Called when player releases throw saber button.
	@param ThrowHoldTime contains amount of time button was pressed */
	UFUNCTION( BlueprintImplementableEvent, Category = "Human", Meta = ( DisplayName = "OnStopThrowingSaber" ) )
	void							OnStopThrowingSaber( float ThrowHoldTime );
	
	/* Called when player drawing/hiding sword */
	UFUNCTION( BlueprintImplementableEvent, Category = "Human", Meta = ( DisplayName = "OnToggleCombat" ) )
	void							OnToggleCombat( bool NewCombatState );

	/* Called when player changes state (from free to attacking, from ChangingCombat to Free, etc) */
	UFUNCTION( BlueprintImplementableEvent, Category = "Human", Meta = ( DisplayName = "OnChangeState" ) )
	void							OnChangeState( EHumanState NewState );
public:
									AHuman();

	virtual void					GetLifetimeReplicatedProps( TArray<FLifetimeProperty>& OutLifetimeProps ) const override;

	virtual void					Tick(float DeltaTime) override;

	virtual void					SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual float TakeDamage( float DamageAmount,
							 FDamageEvent const & DamageEvent,
							 AController * EventInstigator,
							 AActor * DamageCauser
	) override;

	UFUNCTION( BlueprintPure, Category = "Human", Meta = ( DisplayName = "GetCurrentStats" ) )
	FHumanStats						GetCurrentStats()																			{ return m_CurrentStats; }

	UFUNCTION( BlueprintPure, Category = "Human", Meta = ( DisplayName = "GetCurrentlyPlayingAttack" ) )
	FAttackMontage					GetCurrentlyPlayingAttack()																	{ return m_CurrentAttack; }

	UFUNCTION( BlueprintPure, Category = "Human",  Meta = ( DisplayName = "IsInCombat" ) )																			
	bool							IsInCombat()																				{ return bInCombat; }

	UFUNCTION( BlueprintPure, Category = "Human", Meta = ( DisplayName = "GetState" ) )
	EHumanState						GetState()																					{ return m_eState; }
	
	UFUNCTION( BlueprintCallable, Category = "Human", Meta = ( DisplayName = "SetState" ) )
	void							SetState( EHumanState NewState );

	UFUNCTION( BlueprintPure, Category = "Human", Meta = ( DisplayName = "IsHoldingAttack" ) )
	bool							IsHoldingAttack()																			{ return bHoldingAttack; }

	UFUNCTION( BlueprintPure, Category = "Human", Meta = ( DisplayName = "IsHoldingThrow" ) )
	bool							IsHoldingThrow()																			{ return bHoldingThrow; }

	UFUNCTION( BlueprintCallable, Category = "Human", Meta = ( DisplayName = "SetSaber" ) )
	void							SetSaber( ASaber * NewSaber )																{ m_Saber = NewSaber; }

	UFUNCTION( BlueprintPure, Category = "Human", Meta = ( DisplayName = "GetSaber" ) )
	ASaber *						GetSaber()																					{ return m_Saber; }

	UFUNCTION( BlueprintCallable, Category = "Human", Meta = ( DisplayName = "PutSaberInBelt" ) )
	void							PutSaberInBelt();

	UFUNCTION( BlueprintCallable, Category = "Human", Meta = ( DisplayName = "PutSaberInHand" ) )
	void							PutSaberInHand();
	
	/* Updates stats on server, adding @param to current stats */
	UFUNCTION( BlueprintCallable, Category = "Human", Meta = ( DisplayName = "UpdateStats" ) )
	void							UpdateStats( FHumanStats DeltaStats );

	UFUNCTION( BlueprintCallable, Category = "Human", Meta = ( DisplayName = "SetImpactCounter" ) )
	void							SetImpactCounter( float NewCounter )														{ m_CurrentImpactCounter = NewCounter; }

	UFUNCTION( BlueprintCallable, Category = "Human", Meta = ( DisplayName = "OnAttackDefendingEnemy" ) )
	void							OnAttackDefendingEnemy( AHuman * Enemy );

	UFUNCTION( BlueprintCallable, Category = "Human", Meta = ( DisplayName = "OnAttackAttackingEnemy" ) )
	void							OnAttackAttackingEnemy( AHuman * Enemy );

	UFUNCTION( BlueprintCallable, Category = "Human", Meta = ( DisplayName = "PlayCurrentAttackInReverse" ) )
	void							PlayCurrentAttackInReverse();

	/* Returns if player has enough stamina, force, checks nullptr */
	UFUNCTION( BlueprintCallable, Category = "Human", Meta = ( DisplayName = "CanPerformAttack" ) )
	bool							CanPerformAttack( FAttackMontage AttackMont );
private:
	UFUNCTION()
	void							MoveForward( float Value );
	UFUNCTION()
	void							MoveRight( float Value );
	UFUNCTION()
	void							OnJumpStart();
	UFUNCTION()
	void							OnJumpEnd();
	UFUNCTION()
	void							ToggleCombat();
	UFUNCTION()
	void							Attack();
	UFUNCTION()
	void							StopAttack();
	UFUNCTION()
	void							SwitchDefending();

	UFUNCTION()
	void							ThrowSaber();
	UFUNCTION()
	void							StopThrowingSaber();

	protected:
	/// Saber variables
	ASaber *						m_Saber;
	UPROPERTY( Replicated )
	bool							bHoldingAttack = false;
	float							fTimeHoldingAttack = 0.f;
	bool							bHoldingThrow = false;
	float							fTimeHoldingThrow = 0.f;
	bool							bHoldingDefend = false;

	/// Movement variables
	float							CurrentJumpTimer;
	/// To activate counter in tick func
	bool							bWaitBeforeJump;

	/// Attacks control variables
	float							m_fFirstPress = 0.f;
	float							m_fSecondPress = 0.f;

	UFUNCTION( NetMulticast, Reliable, WithValidation )
	void							Multicast_UpdateStats( FHumanStats DeltaStats );
	void							Multicast_UpdateStats_Implementation( FHumanStats DeltaStats );
	bool							Multicast_UpdateStats_Validate( FHumanStats DeltaStats );

	UFUNCTION( Server, Reliable, WithValidation )
	void							Server_UpdateStats( FHumanStats DeltaStats );
	void							Server_UpdateStats_Implementation( FHumanStats DeltaStats );
	bool							Server_UpdateStats_Validate( FHumanStats DeltaStats );

	UFUNCTION( NetMulticast, Reliable, WithValidation )
	void							Multicast_SetState( EHumanState NewState );
	void							Multicast_SetState_Implementation( EHumanState NewState );
	bool							Multicast_SetState_Validate( EHumanState NewState );
	
	UFUNCTION( Server, Reliable, WithValidation )
	void							Server_SetState( EHumanState NewState );
	void							Server_SetState_Implementation( EHumanState NewState );
	bool							Server_SetState_Validate( EHumanState NewState );
	/// Attack ( including network ) functions
	/* This function if ran on server promote it to all connections */
	UFUNCTION( NetMulticast, Reliable, WithValidation )
	void							Multicast_PlayAttack( FAttackMontage Mont );
	void							Multicast_PlayAttack_Implementation( FAttackMontage Mont );
	bool							Multicast_PlayAttack_Validate( FAttackMontage Mont );
	
	/* This function runs on server side */
	UFUNCTION( Server, Reliable, WithValidation )
	void							Server_PlayAttack( FAttackMontage Mont );
	void							Server_PlayAttack_Implementation( FAttackMontage Mont );
	bool							Server_PlayAttack_Validate( FAttackMontage Mont );
							
	/* Putting saber in slots */
	UFUNCTION( NetMulticast, Reliable, WithValidation )
	void							Multicast_PutSaberInSlot( FName SlotName );
	void							Multicast_PutSaberInSlot_Implementation( FName SlotName );
	bool							Multicast_PutSaberInSlot_Validate( FName SlotName ) { return true;  }

	UFUNCTION( Server, Reliable, WithValidation )
	void							Server_PutSaberInSlot( FName SlotName );
	void							Server_PutSaberInSlot_Implementation( FName SlotName );
	bool							Server_PutSaberInSlot_Validate( FName SlotName ) { return true;  }

	void SetMaxWalkSpeed( float NewSpeed );

	UFUNCTION( NetMulticast, Reliable, WithValidation )
	void							Multicast_SetWalkSpeed( float NewSpeed );
	void							Multicast_SetWalkSpeed_Implementation( float NewSpeed );
	bool							Multicast_SetWalkSpeed_Validate( float NewSpeed ) { return true; }

	UFUNCTION( Server, Reliable, WithValidation )
	void							Server_SetWalkSpeed( float NewSpeed );
	void							Server_SetWalkSpeed_Implementation( float NewSpeed );
	bool							Server_SetWalkSpeed_Validate( float NewSpeed ) { return true; }

	void							PlayAttack( FAttackMontage AttackToPlay );
	/* It is float but has name Integer. Yes. Needed to count DeltaTime (float) and add 1 to some int */
	float							CurrentlyRestoredInteger = 0.f;
	UPROPERTY( Replicated )
	EHumanState						m_eState;

	float							m_CurAttackLengthCounter;
	UPROPERTY( Replicated )
	FAttackMontage					m_CurrentAttack;
	
	float							m_CurrentImpactCounter = 0.f;

	/// Stats variables
	UPROPERTY( Replicated )
	FHumanStats						m_CurrentStats;
	
};
