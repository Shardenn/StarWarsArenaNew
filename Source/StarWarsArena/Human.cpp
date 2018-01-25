#include "Human.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Objects/Saber.h"

#include "EngineUtils.h"

AHuman::AHuman() :
	WalkSpeed( 280.f ),
	RunSpeed( 350.f ),
	bWaitBeforeJump( false ),
	bInCombat( false ),
	m_eState( EHumanState::EHS_Free ),
	AnimationCutTime( 0.35f ),
	StartingStats( 100, 100 ),
	StatsRestoreSpeed( 0, 10 ),
	MaxSaberFlyDistance( 1400 ),
	bShouldWaitBeforeJump( true ),
	DelayBeforeJump( 0.1f )
{
	bReplicates = true;
	bReplicateMovement = true;

	SetActorTickEnabled( true );
	PrimaryActorTick.bCanEverTick = true;
	
	MoveSet = CreateDefaultSubobject<UMoveSet>( TEXT( "MoveSet" ) );
	AddOwnedComponent( MoveSet );
}

void AHuman::GetLifetimeReplicatedProps( TArray<FLifetimeProperty> & OutLifetimeProps ) const
{
	Super::GetLifetimeReplicatedProps( OutLifetimeProps );

	DOREPLIFETIME( AHuman, m_eState );
	DOREPLIFETIME( AHuman, m_CurrentStats );
	DOREPLIFETIME( AHuman, bHoldingAttack );
	DOREPLIFETIME( AHuman, m_CurrentAttack );
}

void AHuman::BeginPlay()
{
	Super::BeginPlay();

	m_CurrentStats = StartingStats;
	UpdateStats( StartingStats );

	SetReplicates( true );
	SetReplicateMovement( true );
}

void AHuman::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	if ( bShouldWaitBeforeJump && bWaitBeforeJump )
	{
		CurrentJumpTimer += DeltaTime;

		if ( CurrentJumpTimer >= DelayBeforeJump )
		{
			bPressedJump = true;
			CurrentJumpTimer = 0.f;
			bWaitBeforeJump = false;
		}
	}

	if ( bHoldingAttack ) fTimeHoldingAttack += DeltaTime;
	if ( bHoldingThrow ) fTimeHoldingThrow += DeltaTime;

	/* Stats restoring */
	if( HasAuthority() )
	{
		CurrentlyRestoredInteger += DeltaTime * StatsRestoreSpeed.HS_Stamina;
		if( CurrentlyRestoredInteger >= 1 )
		{
			Server_UpdateStats( FHumanStats( 0, 1 ) );

			CurrentlyRestoredInteger = 0.f;
		}
	}

	switch( m_eState )
	{
		case EHumanState::EHS_Attacking :
		{
			m_CurAttackLengthCounter -= DeltaTime;

			if( m_CurAttackLengthCounter <= 0.f )
			{
				FAttackMontage NextPossibleAttack = MoveSet->GetNextMontage( m_fFirstPress, m_fSecondPress, m_CurrentAttack );

				if( CanPerformAttack( NextPossibleAttack ) )
				{
					PlayAttack( NextPossibleAttack );
				}
				else
				{
					m_eState = EHumanState::EHS_Free;
					m_CurrentAttack = FAttackMontage();
				}

				m_fFirstPress = m_fSecondPress = 0.f;
			}

			break;
		}
		case EHumanState::EHS_Impacted :
		{
			m_CurrentImpactCounter -= DeltaTime;

			if( m_CurrentImpactCounter <= 0.f )
			{
				SetState( EHumanState::EHS_Free );
				m_CurrentImpactCounter = 0.f;
			}
		}

		break;
	}

}

void AHuman::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis( "MoveForward",						this, &AHuman::MoveForward );
	PlayerInputComponent->BindAxis( "MoveRight",						this, &AHuman::MoveRight );
	PlayerInputComponent->BindAxis( "Turn",								this, &AHuman::AddControllerYawInput );
	PlayerInputComponent->BindAxis( "LookUp",							this, &AHuman::AddControllerPitchInput );

	PlayerInputComponent->BindAction( "Jump", IE_Pressed,				this, &AHuman::OnJumpStart );
	PlayerInputComponent->BindAction( "Jump", IE_Released,				this, &AHuman::OnJumpEnd );
	//PlayerInputComponent->BindAction( "Run", IE_Pressed,				this, &AHuman::StartRunning );
	//PlayerInputComponent->BindAction( "Run", IE_Released,				this, &AHuman::StopRunning );

	PlayerInputComponent->BindAction( "ToggleShowWeapon", IE_Pressed,	this, &AHuman::ToggleCombat );

	PlayerInputComponent->BindAction( "Attack", IE_Pressed,				this, &AHuman::Attack );
	PlayerInputComponent->BindAction( "Attack", IE_Released,			this, &AHuman::StopAttack );

	PlayerInputComponent->BindAction( "ThrowSaber", IE_Pressed,			this, &AHuman::ThrowSaber );
	PlayerInputComponent->BindAction( "ThrowSaber", IE_Released,		this, &AHuman::StopThrowingSaber );

	PlayerInputComponent->BindAction( "Defend", IE_Pressed,				this, &AHuman::SwitchDefending );
	// TODO uncomment - commented for debugging
	//PlayerInputComponent->BindAction( "Defend", IE_Released,			this, &AHuman::SwitchDefending );
}

void AHuman::MoveForward( float Value )
{
	if ( !Controller || Value == 0.f )
		return;

	FRotator Rotation = Controller->GetControlRotation();

	if ( GetCharacterMovement()->IsFalling() || GetCharacterMovement()->IsMovingOnGround() )
		Rotation.Pitch = 0.f;

	const FVector Direction = FRotationMatrix( Rotation ).GetScaledAxis( EAxis::X );

	AddMovementInput( Direction, Value );
}

void AHuman::MoveRight( float Value )
{
	if ( !Controller || Value == 0.f )
		return;

	FRotator Rotation = Controller->GetControlRotation();

	const FVector Direction = FRotationMatrix( Rotation ).GetScaledAxis( EAxis::Y );

	AddMovementInput( Direction, Value );
}

void AHuman::OnJumpStart()
{
	bWaitBeforeJump = true;
}

void AHuman::OnJumpEnd()
{
	bPressedJump = false;
}

//======================== Movement functions END here ======================//
void AHuman::ToggleCombat()
{
	if( m_eState != EHumanState::EHS_Free )
		return;

	bInCombat = !bInCombat;

	OnToggleCombat( bInCombat );
}

float AHuman::TakeDamage( float DamageAmount, FDamageEvent const & DamageEvent, AController * EventInstigator, AActor * DamageCauser )
{
	int32 DamagePoints = FPlatformMath::RoundToInt( DamageAmount );
	int32 DamageToApply = FMath::Clamp( DamagePoints, 0, m_CurrentStats.HS_Health );

	//if( HasAuthority() )
	//	Multicast_UpdateStats( FHumanStats( -DamageToApply, 0 ) );
	//else
		Server_UpdateStats( FHumanStats( -DamageToApply, 0 ) );
	
	return DamageToApply;
}

void AHuman::Attack()
{
	if ( !bInCombat )
		return;

	bHoldingAttack = true;

	/* If we pressed second time during attack, wait until player release button, return cut time */
	if( m_fFirstPress != 0.f && m_CurrentAttack.MontageAnimation != nullptr )
	{
		m_CurAttackLengthCounter += m_CurrentAttack.MontageAnimation->GetPlayLength() * m_CurrentAttack.PlayRate * AnimationCutTime;
	}

	OnAttack();
}

void AHuman::StopAttack()
{
	bHoldingAttack = false;

	if( !bInCombat )
		return;

	if( m_eState == EHumanState::EHS_Attacking )
	{
		if( m_fFirstPress == 0.f )
			m_fFirstPress = fTimeHoldingAttack;
		else
			m_fSecondPress = fTimeHoldingAttack;

		/* If it first player's press and anything is playing, then cut current montage for dynamic action.
		If it's player's second release during attack => immideatly play new one */
		if( CanPerformAttack( MoveSet->GetNextMontage( m_fFirstPress, m_fSecondPress, m_CurrentAttack ) ) )
		{
			float CurrentAttackDuration = m_CurrentAttack.MontageAnimation->GetPlayLength() * m_CurrentAttack.PlayRate;
			m_CurAttackLengthCounter -= CurrentAttackDuration * AnimationCutTime + ( m_fSecondPress != 0.f ) * CurrentAttackDuration;
		}

	}
	else if( m_eState == EHumanState::EHS_Free )
	{
		/* TODO Make MoveSet safe pointed */
		FAttackMontage PossibleAttack = MoveSet->GetNextMontage( fTimeHoldingAttack, 0.f, m_CurrentAttack );
		if( CanPerformAttack( PossibleAttack ) )
		{
			PlayAttack( PossibleAttack );
		}
	}

	SetState( EHumanState::EHS_Attacking );
	OnStopAttack( fTimeHoldingAttack );
	
	fTimeHoldingAttack = 0.f;
}

bool AHuman::CanPerformAttack( FAttackMontage AttackMont )
{
	if( !AttackMont.MontageAnimation ||
		AttackMont.StaminaRequired > m_CurrentStats.HS_Stamina
		//AttackMont.ForceRequired > m_CurrentStats.HS_Force
		)
		return false;

	return true;
}

/* Also nils First and Second press duration timers */
void AHuman::PlayAttack( FAttackMontage AttackToPlay )
{
	m_CurrentAttack = AttackToPlay;
	m_fFirstPress = m_fSecondPress = 0.f;
	
	if( Role < ROLE_Authority )
		Server_PlayAttack( AttackToPlay );
	else
		Multicast_PlayAttack( AttackToPlay );	

	Server_UpdateStats( FHumanStats( 0, -AttackToPlay.StaminaRequired ) );
}

void AHuman::Server_PlayAttack_Implementation( FAttackMontage Mont )
{
	Multicast_PlayAttack( Mont );
}

bool AHuman::Server_PlayAttack_Validate( FAttackMontage Mont ) 
{ 
	if( Mont.MontageAnimation == nullptr )
	{
		UE_LOG( LogTemp, Error, TEXT( "%s tried to send NULL animation to server." ), *GetName() );
		//return false;
	}
	
	return true;
}

void AHuman::Multicast_PlayAttack_Implementation( FAttackMontage AttackToPlay )
{
	/* Set duration of currently played attack for counting to end */
	if( !AttackToPlay.MontageAnimation )
		return;
	
	m_CurrentAttack = AttackToPlay;
	m_fFirstPress = m_fSecondPress = 0.f;
	m_CurAttackLengthCounter = GetMesh()->GetAnimInstance()->Montage_Play( AttackToPlay.MontageAnimation, AttackToPlay.PlayRate, EMontagePlayReturnType::Duration );
}

bool AHuman::Multicast_PlayAttack_Validate( FAttackMontage Mont ) 
{ 
	if( //m_CurrentStats.HS_Health <= 0 ||
		//m_CurrentStats.HS_Stamina <= Mont.StaminaRequired ||
		Mont.MontageAnimation == nullptr 
		)
	{
		UE_LOG( LogTemp, Error, TEXT( "%s with stats: Health=%d, Stamina=%d tried to run on server animation with error." ), 
				*GetName(), 
				m_CurrentStats.HS_Health,
				m_CurrentStats.HS_Stamina );
		if( Mont.MontageAnimation == nullptr )
			UE_LOG( LogTemp, Error, TEXT( "%s tried to run NULL animation" ), *GetName() );
	}

	return true;
}

void AHuman::OnAttackDefendingEnemy( AHuman * Enemy )
{
	SetImpactCounter( m_CurrentAttack.StaminaRequired / StartingStats.HS_Stamina * MaxImpactTime );
	SetState( EHumanState::EHS_Impacted );

	Enemy->UpdateStats( FHumanStats( 0, -m_CurrentAttack.StaminaRequired ) );

	GetMesh()->GetAnimInstance()->Montage_Stop
	( 
		m_CurrentAttack.MontageAnimation->BlendOut.GetBlendTime(),
		m_CurrentAttack.MontageAnimation 
	);
	//PlayCurrentAttackInReverse();
}

void AHuman::OnAttackAttackingEnemy( AHuman * Enemy )
{
	/* Set impact state and timer to character with weaker attack */
	FAttackMontage EnemyAttack = Enemy->GetCurrentlyPlayingAttack();
	float ImpactLength = ( EnemyAttack.StaminaRequired - m_CurrentAttack.StaminaRequired ) / StartingStats.HS_Health * MaxImpactTime;
	
	if( ImpactLength > 0.f )
	{
		SetImpactCounter( ImpactLength );
		SetState( EHumanState::EHS_Impacted );
	}
	else
	{
		Enemy->SetImpactCounter( -ImpactLength );
		Enemy->SetState( EHumanState::EHS_Impacted );
	}

	/* Decrease stamina of player based on his enemy's attack strength */
	UpdateStats( FHumanStats( 0, Enemy->GetCurrentlyPlayingAttack().StaminaRequired ) );
	Enemy->UpdateStats( FHumanStats( 0, -m_CurrentAttack.StaminaRequired ) );

	GetMesh()->GetAnimInstance()->Montage_Stop(
		m_CurrentAttack.MontageAnimation->BlendOut.GetBlendTime(),
		m_CurrentAttack.MontageAnimation
	);
	//PlayCurrentAttackInReverse();
	Enemy->GetMesh()->GetAnimInstance()->Montage_Stop(
		m_CurrentAttack.MontageAnimation->BlendOut.GetBlendTime(),
		m_CurrentAttack.MontageAnimation
	);
	//Enemy->PlayCurrentAttackInReverse();
}

void AHuman::PlayCurrentAttackInReverse()
{
	float StartMontageAt = m_CurAttackLengthCounter;// m_CurrentAttack.MontageAnimation->GetPlayLength() * m_CurrentAttack.PlayRate - m_CurAttackLengthCounter;

	GetMesh()->GetAnimInstance()->Montage_Play(
		m_CurrentAttack.MontageAnimation, -1.f * ReverseAttackPlayRate,
		EMontagePlayReturnType::Duration, StartMontageAt );

	//m_CurAttackLengthCounter = 0.f;
	//m_CurAttackLengthCounter = m_CurrentAttack.MontageAnimation->GetPlayLength() *
		//m_CurrentAttack.PlayRate * ReverseAttackPlayRate;
}

void AHuman::ThrowSaber()
{
	bHoldingThrow = true;

	SetState( EHumanState::EHS_ThrowingSaber );

	if( m_Saber )
		m_Saber->LaunchSaber( MaxSaberFlyDistance );

	OnThrowSaber();
}

void AHuman::StopThrowingSaber()
{
	bHoldingThrow = false;

	if( m_Saber )
		m_Saber->StopSaber();

	OnStopThrowingSaber( fTimeHoldingThrow );

	fTimeHoldingThrow = 0.f;
}

void AHuman::SwitchDefending()
{
	if( !bInCombat )
		return;

	bHoldingDefend = !bHoldingDefend;

	if( bHoldingDefend && m_eState == EHumanState::EHS_Free )
	{
		SetState( EHumanState::EHS_Defending );
		StatsRestoreSpeed.HS_Stamina = StaminaRestoreWhileDefending;
		SetMaxWalkSpeed( WalkSpeed );
	}
	else if( !bHoldingDefend && m_eState == EHumanState::EHS_Defending )
	{
		SetState( EHumanState::EHS_Free );
		StatsRestoreSpeed.HS_Stamina = FreeStaminaRestoreSpeed;
		SetMaxWalkSpeed( RunSpeed );
	}
}

void AHuman::SetMaxWalkSpeed( float NewSpeed )
{
	if( HasAuthority() )
		Multicast_SetWalkSpeed( NewSpeed );
	else
		Server_SetWalkSpeed( NewSpeed );
}

void AHuman::Server_SetWalkSpeed_Implementation( float NewSpeed )
{
	Multicast_SetWalkSpeed( NewSpeed );
}

void AHuman::Multicast_SetWalkSpeed_Implementation( float NewSpeed )
{
	GetCharacterMovement()->MaxWalkSpeed = NewSpeed;
}

void AHuman::SetState( EHumanState NewState )
{
	if( m_eState == EHumanState::EHS_Attacking && NewState != EHumanState::EHS_Attacking )
	{
		m_CurAttackLengthCounter = m_fFirstPress = m_fSecondPress = 0.f;
	}

	if( Role < ROLE_Authority )
		Server_SetState( NewState );
	else
		Multicast_SetState( NewState );
}

void AHuman::Server_SetState_Implementation( EHumanState NewState )
{
	Multicast_SetState( NewState );
}

bool AHuman::Server_SetState_Validate( EHumanState NewState )
{
	return true;
}

void AHuman::Multicast_SetState_Implementation( EHumanState NewState )
{
	if( NewState == m_eState )
		return;

	m_eState = NewState;

	OnChangeState( m_eState );
}

bool AHuman::Multicast_SetState_Validate( EHumanState NewState )
{
	return true;
}

void AHuman::PutSaberInBelt()
{
	if( HasAuthority() )
		Multicast_PutSaberInSlot( FName( "SaberBelt" ) );
	else
		Server_PutSaberInSlot( FName( "SaberBelt" ) );
}

void AHuman::PutSaberInHand()
{
	if( HasAuthority() )
		Multicast_PutSaberInSlot( FName( "SaberHand" ) );
	else
		Server_PutSaberInSlot( FName( "SaberHand" ) );
}

void AHuman::Server_PutSaberInSlot_Implementation( FName SlotName )
{
	Multicast_PutSaberInSlot( SlotName );
}

void AHuman::Multicast_PutSaberInSlot_Implementation( FName SlotName )
{
	if( !m_Saber )
	{
		UE_LOG( LogTemp, Error, TEXT( "%s tried to put NULL saber in slot %s." ), *GetName(), *SlotName.ToString() );
		return;
	}
	
	FAttachmentTransformRules Rules( EAttachmentRule::SnapToTarget, true );

	m_Saber->AttachToComponent( GetMesh(), Rules, SlotName );
}

void AHuman::UpdateStats( FHumanStats DeltaStats )
{
	/*
	if( HasAuthority() )
		Multicast_UpdateStats( DeltaStats );
	else
		Server_UpdateStats( DeltaStats );
	*/

	Server_UpdateStats( DeltaStats );
}

void AHuman::Server_UpdateStats_Implementation( FHumanStats DeltaStats )
{
	//Multicast_UpdateStats( DeltaStats );
	m_CurrentStats = m_CurrentStats.Add( DeltaStats, StartingStats );
	if( DeltaStats.HS_Stamina > 1 )
		UE_LOG( LogTemp, Warning, TEXT( "Running updatestats Server. DeltaStamina is %d" ), DeltaStats.HS_Stamina );
}

bool AHuman::Server_UpdateStats_Validate( FHumanStats DeltaStats )
{
	return true;
}

void AHuman::Multicast_UpdateStats_Implementation( FHumanStats DeltaStats )
{
	m_CurrentStats = m_CurrentStats.Add( DeltaStats, StartingStats );
	if( DeltaStats.HS_Stamina > 1 )
		UE_LOG( LogTemp, Warning, TEXT( "Running updatestats Multicasted. DeltaStamina is %d" ), DeltaStats.HS_Stamina );
}

bool AHuman::Multicast_UpdateStats_Validate( FHumanStats DeltaStats )
{
	return true;
}