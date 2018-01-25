#define BLADE_CHANNEL				ECC_GameTraceChannel1    // Saber blade channel

#include "Saber.h"
#include "Human.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Kismet/GameplayStatics.h"

ASaber::ASaber() :
	m_Alpha( 0.f ),
	m_eState( ESaberState::ESS_Closed ),
	m_pHuman( nullptr ),
	BladeThickness( 0.05f ),
	BladeLength( 1.f ),
	m_fMaxFlyDistance( 0.f ),
	OpeningSpeed( 1.5f ),
	ClosingSpeed( 2.0f ),
	RotationDirection( 0.f, 1.f, 0.f ),
	RotationSpeed( 35.f ),
	FlySpeed( 20.f ),
	ReturnSpeed( 30.f ),
	MinDistanceToHuman( 80.f )
{
	bReplicates = true;
	bReplicateMovement = true;

	PrimaryActorTick.bCanEverTick = true;

	Hilt = CreateDefaultSubobject< UStaticMeshComponent >( TEXT( "Hilt" ) );
	RootComponent = Hilt;
	Hilt->SetCollisionResponseToAllChannels( ECollisionResponse::ECR_Ignore );
	Hilt->SetCollisionResponseToChannel( ECC_WorldStatic, ECR_Overlap );

	Hilt->OnComponentBeginOverlap.AddDynamic( this, &ASaber::HiltOverlap );

	Blade = CreateDefaultSubobject< UStaticMeshComponent >( TEXT( "Blade" ) );
	Blade->AttachToComponent( RootComponent, FAttachmentTransformRules::KeepRelativeTransform );

	Blade->SetCollisionResponseToAllChannels( ECollisionResponse::ECR_Ignore );
	Blade->SetCollisionResponseToChannel( BLADE_CHANNEL, ECollisionResponse::ECR_Overlap );

	Blade->OnComponentBeginOverlap.AddDynamic( this, &ASaber::BladeOverlap );
}

void ASaber::GetLifetimeReplicatedProps( TArray<FLifetimeProperty> & OutLifetimeProps ) const
{
	Super::GetLifetimeReplicatedProps( OutLifetimeProps );

	//DOREPLIFETIME( ASaber, m_pHuman );
	DOREPLIFETIME( ASaber, m_Alpha );
	DOREPLIFETIME( ASaber, m_eState );
	DOREPLIFETIME( ASaber, m_fMaxFlyDistance );
	//DOREPLIFETIME( ASaber, OpeningSpeed );
	//DOREPLIFETIME( ASaber, ClosingSpeed );
	//DOREPLIFETIME( ASaber, BladeThickness );
	//DOREPLIFETIME( ASaber, BladeLength );
}

void ASaber::BeginPlay()
{
	Super::BeginPlay();
	
	bReplicates = true;
	bReplicateMovement = false;

	SetSaberState( ESaberState::ESS_Closing );
	UpdateTransform( GetTransform() );
}

void ASaber::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	switch ( m_eState )
	{
		/* Opening/closing saber */
		case ESaberState::ESS_Opening :
		{
			m_Alpha = FMath::Clamp( m_Alpha + DeltaTime * OpeningSpeed, 0.f, BladeLength );

			if( m_Alpha >= BladeLength )
				SetSaberState( ESaberState::ESS_Opened );

			UpdateTransform( GetTransform() );

			break;
		}

		case ESaberState::ESS_Closing :
		{
			m_Alpha = FMath::Clamp( m_Alpha - DeltaTime * ClosingSpeed, 0.f, BladeLength );

			if( m_Alpha <= 0.f )
				SetSaberState( ESaberState::ESS_Closed );

			UpdateTransform( GetTransform() );

			break;
		}

		/* Saber throw */
		case ESaberState::ESS_Flying :
		{
			if( !m_pHuman )
			{
				UE_LOG( LogTemp, Error, TEXT( "%s : saber does not have human attached, but State is Flying." ), *GetName() );
				return;
			}

			FVector FlyDirection = m_pHuman->GetControlRotation().Vector();

			if( FVector::Distance( GetActorLocation(), m_pHuman->GetActorLocation() ) < m_fMaxFlyDistance )
			{
				FTransform NewTransform( GetActorTransform() );
				
				NewTransform.SetLocation( GetActorLocation() + FlyDirection * FlySpeed );
				NewTransform.SetRotation( FQuat( GetActorRotation() ) * FQuat( RotationDirection * RotationSpeed ) );

				DrawDebugLine(
					GetWorld(), 
					GetActorLocation(), NewTransform.GetLocation(), 
					FColor::Green, 
					false, 15.f, 0, 
					5.f );

				UpdateTransform( NewTransform, true );				
			}
			else
			{
				SetSaberState( ESaberState::ESS_Returning );
			}
			
			break;
		}

		/* Saber return */
		case ESaberState::ESS_Returning :
		{
			if( !m_pHuman )
			{
				UE_LOG( LogTemp, Error, TEXT( "%s : saber does not have human attached, but State is Returning." ), *GetName() );
				return;
			}
			
			/* Rotate saber so that handle faces human */
			FTransform FlyTransform = GetActorTransform();
			FlyTransform.SetRotation( FQuat( ReturnDeltaRotation + m_pHuman->GetActorForwardVector().Rotation() ) );
			UpdateTransform( FlyTransform, true );

			/* Get hand socket location for saber to fly to */
			FVector Target;
			FRotator DiscardRotator;
			m_pHuman->GetMesh()->GetSocketWorldLocationAndRotation( FName( "SaberHand" ), Target, DiscardRotator ); 

			FVector ReturnDirection = ( Target - GetActorLocation() ).GetSafeNormal();

			/* If saber far enough - update transform so that saber flies to human. */
			if( FVector::Distance( GetActorLocation() + ReturnDirection * ReturnSpeed, m_pHuman->GetActorLocation() ) > MinDistanceToHuman )
			{
				FlyTransform.SetLocation( GetActorLocation() + ReturnDirection * ReturnSpeed );
				UpdateTransform( FlyTransform, true );
			}
			/* Otherwise put saber in hand */
			else
			{
				SetSaberState( ESaberState::ESS_Opened );
				m_pHuman->PutSaberInHand();
				m_pHuman->SetState( EHumanState::EHS_Free );
			}
			
			break;
		}
	}
}

void ASaber::SetSaberState( ESaberState NewState )
{
	if( HasAuthority() )
		Multicast_SetSaberState( NewState );
	else
		Server_SetSaberState( NewState );
}

void ASaber::Server_SetSaberState_Implementation( ESaberState NewState )
{
	Multicast_SetSaberState( NewState );
}

bool ASaber::Server_SetSaberState_Validate( ESaberState NewState )
{
	return true;
}

void ASaber::Multicast_SetSaberState_Implementation( ESaberState NewState )
{
	if ( NewState == m_eState )
		return;

	m_eState = NewState;

	OnSaberChangeState( NewState );
}

bool ASaber::Multicast_SetSaberState_Validate( ESaberState NewState )
{
	return true;
}

ESaberState ASaber::GetSaberState()
{
	return m_eState;
}

void ASaber::HiltOverlap( UPrimitiveComponent* OverlappedComp, 
						  AActor* OtherActor, 
						  UPrimitiveComponent* OtherComp, 
						  int32 OtherBodyIndex, 
						  bool bFromSweep, 
						  const FHitResult& SweepResult 
						)
{
	UE_LOG( LogTemp, Warning, TEXT( "%s saber's hilt overlapping %s." ), *GetName(), *OtherActor->GetName() );
	
	if( m_eState == ESaberState::ESS_Flying )
		SetSaberState( ESaberState::ESS_Returning );
}

void ASaber::BladeOverlap( UPrimitiveComponent* OverlappedComp, 
						   AActor* OtherActor, 
						   UPrimitiveComponent* OtherComp, 
						   int32 OtherBodyIndex, 
						   bool bFromSweep, 
						   const FHitResult& SweepResult 
						)
{
	if( HasAuthority() )
		Multicast_BladeOverlap( OtherActor );

	AHuman * OtherHuman = Cast<AHuman>( OtherActor );
	if( OtherHuman )
	{
		EHumanState OtherState = OtherHuman->GetState();

		if( OtherState == EHumanState::EHS_Attacking || OtherState == EHumanState::EHS_Defending )
			OnBladeOverlapCPP( EBladeOverlapResult::EBOR_DamageCancel );
		else
			OnBladeOverlapCPP( EBladeOverlapResult::EBOR_PlayerMesh );
	}
	else
	{
		OnBladeOverlapCPP( EBladeOverlapResult::EBOR_StaticMesh );
	}
}

void ASaber::UpdateTransform( FTransform NewTransfrom, bool bUpdatePosition )
{
	if( HasAuthority() )
		Multicast_UpdateTransform( NewTransfrom, bUpdatePosition );
	else
		Server_UpdateTransform( NewTransfrom, bUpdatePosition );
}

void ASaber::Server_UpdateTransform_Implementation( FTransform NewTransfrom, bool bUpdatePosition )
{	
	Multicast_UpdateTransform( NewTransfrom, bUpdatePosition );
}

bool ASaber::Server_UpdateTransform_Validate( FTransform NewTransfrom, bool bUpdatePosition )
{
	return true;
}

void ASaber::Multicast_UpdateTransform_Implementation( FTransform NewTransfrom, bool bUpdatePosition )
{
	//UE_LOG( LogTemp, Warning, TEXT( "Server running UpdateTransform with %s. bUpdatePosistion is %d" ), 
	//		*NewTransfrom.GetLocation().ToString(), bUpdatePosition);

	Blade->SetRelativeScale3D( FVector( BladeThickness, BladeThickness, m_Alpha ) );

	if( bUpdatePosition )
	{
		SetActorLocation( NewTransfrom.GetLocation() );
		SetActorRotation( NewTransfrom.GetRotation() );
	}
}

void ASaber::Server_BladeOverlap_Implementation( AActor * OverlappedActor )
{
	Multicast_BladeOverlap( OverlappedActor );
}

bool ASaber::Server_BladeOverlap_Validate( AActor * OverlappedActor )
{
	return true;
}

void ASaber::Multicast_BladeOverlap_Implementation( AActor * OverlappedActor )
{
	AHuman * OtherHuman = Cast<AHuman>( OverlappedActor );

	UE_LOG( LogTemp, Warning, TEXT( "Saber blade overlapped %s on server." ), *OverlappedActor->GetName() );

	if( !OtherHuman || OtherHuman == m_pHuman )
		return;

	EHumanState MyHumanState = m_pHuman->GetState();
	EHumanState OtherHumanState = OtherHuman->GetState();

	if( bApplyDamageWithoutAttack )
	{
		if( MyHumanState != EHumanState::EHS_Attacking &&
			MyHumanState != EHumanState::EHS_ThrowingSaber )
			return;
	}

	if( MyHumanState == EHumanState::EHS_Attacking )
	{
		if( OtherHumanState == EHumanState::EHS_Defending )
		{
			m_pHuman->OnAttackDefendingEnemy( OtherHuman );
		}
		else if( OtherHumanState == EHumanState::EHS_Attacking )
		{
			m_pHuman->OnAttackAttackingEnemy( OtherHuman );
		}
		else
		{
			TSubclassOf<UDamageType> DamageType;
			UGameplayStatics::ApplyDamage( OtherHuman, m_pHuman->GetCurrentlyPlayingAttack().DealtDamage, m_pHuman->GetController(), this, DamageType );
		}
	}

	
}

bool ASaber::Multicast_BladeOverlap_Validate( AActor * OverlappedActor )
{
	return true;
}

void ASaber::LaunchSaber( float MaxDistance )
{
	Server_LaunchSaber( MaxDistance );	
}

void ASaber::Server_LaunchSaber_Implementation( float NewMaxFlyDist )
{
	if( !m_pHuman || !( m_eState == ESaberState::ESS_Opened || m_eState == ESaberState::ESS_Opening ) )
	{
		UE_LOG( LogTemp, Error, TEXT( "%s : saber does not have human attached, but LaucnSaber was called." ), *GetName() );
		
		return;
	}
	FTransform NewTrans( GetActorRotation(), m_pHuman->GetActorLocation() + m_pHuman->GetControlRotation().Vector() * MinDistanceToHuman );
	Multicast_DetachSaber( NewTrans );
	
	m_fMaxFlyDistance = NewMaxFlyDist;	
	
	SetSaberState( ESaberState::ESS_Flying );

	OnSaberThrown();
}

void ASaber::StopSaber()
{
	Server_StopSaber();
}

void ASaber::Server_StopSaber_Implementation()
{
	if( m_eState == ESaberState::ESS_Flying )
	{
		m_fMaxFlyDistance = 0.f;
	}

	OnSaberReturnStart();
}

void ASaber::Multicast_DetachSaber_Implementation( FTransform NewTransform )
{
	DetachFromActor( FDetachmentTransformRules::KeepWorldTransform );
	UpdateTransform( NewTransform, true );
}
