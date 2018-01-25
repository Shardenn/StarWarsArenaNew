// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine.h"
#include "UnrealNetwork.h"
#include "Saber.generated.h"

class AHuman;
class UBoxComponent;
class UStaticMeshComponent;

UENUM( BlueprintType )
enum class ESaberState : uint8
{
	ESS_Opened				UMETA( DisplayName = "Opened" ),
	ESS_Closed				UMETA( DisplayName = "Closed" ),
	ESS_Opening				UMETA( DisplayName = "Opening" ),
	ESS_Closing				UMETA( DisplayName = "Closing" ),
	ESS_Flying				UMETA( DisplayName = "Flying" ),
	ESS_Returning			UMETA( DisplayName = "Returning" )
};

UENUM( BlueprintType )
enum class EBladeOverlapResult : uint8
{
	EBOR_PlayerMesh			UMETA( DisplayName = "PlayerMesh" ),
	EBOR_DamageCancel		UMETA( DisplayName = "DamageCancel" ),
	EBOR_StaticMesh			UMETA( DisplayName = "StaticMesh" )
};

UCLASS()
class STARWARSARENA_API ASaber : public AActor
{
	GENERATED_BODY()
	
public:	
	ASaber();

	virtual void						GetLifetimeReplicatedProps( TArray<FLifetimeProperty>& OutLifetimeProps ) const override;

	UFUNCTION( BlueprintCallable, Category = "Saber", Meta = ( DisplayName = "SetSaberState" ) )
	void								SetSaberState( ESaberState NewState );

	UFUNCTION( BlueprintPure, Category = "Saber", Meta = ( DisplayName = "GetSaberState" ) )
	ESaberState							GetSaberState();

	UFUNCTION( BlueprintCallable, Meta = ( DisplayName = "LaunchSaber" ) )
	void								LaunchSaber( float fMaxDistance );

	UFUNCTION( BlueprintCallable, Meta = ( DisplayName = "LaunchSaber" ) )
	void								StopSaber();
	
	UFUNCTION( BlueprintCallable, Meta = ( DisplayName = "SetHuman" ) )
	void								SetHuman( AHuman * NewHuman )						{ m_pHuman = NewHuman; }
	
	UFUNCTION( BlueprintCallable, Meta = ( DisplayName = "GetHuman" ) )
	AHuman *							GetHuman()											{ return m_pHuman; }	

protected:
	virtual void						BeginPlay() override;
	virtual void						Tick(float DeltaTime) override;
	
	UPROPERTY( BlueprintReadWrite, EditAnywhere, Meta = ( DisplayName = "Handle" ) )
	UStaticMeshComponent *				Hilt;

	UPROPERTY( BlueprintReadWrite, EditAnywhere, Meta = ( DisplayName = "Blade" ) )
	UStaticMeshComponent *				Blade;

	/* Turning on speed of saber blade */
	UPROPERTY( Replicated, BlueprintReadWrite, EditAnywhere, Meta = ( DisplayName = "OpeningSpeed" ) )
	float								OpeningSpeed;

	/* Turning on speed of saber blade */
	UPROPERTY( Replicated, BlueprintReadWrite, EditAnywhere, Meta = ( DisplayName = "ClosingSpeed" ) )
	float								ClosingSpeed;

	/* Blade thickness. This parameter will set blade's scale to (BladeThickness, BladeThickness, Z). Z depends on SaberState */
	UPROPERTY( Replicated, BlueprintReadWrite, EditDefaultsOnly, Meta = ( DisplayName = "BladeThickness" ) )
	float								BladeThickness;

	/* Length of the blade, from 0 to 1 */
	UPROPERTY( Replicated, BlueprintReadWrite, EditDefaultsOnly, Meta = ( DisplayName = "BladeLength" ) )
	float								BladeLength;
	
	/* How to position saber when it was thrown in first frame */
	UPROPERTY( BlueprintReadWrite, EditDefaultsOnly, Meta = ( DisplayName = "StartingTransform" ) )
	FTransform							StartingTransform;

	/* How to rotate saber according to human's view when it is returning to him */
	UPROPERTY( BlueprintReadWrite, EditDefaultsOnly, Meta = ( DisplayName = "ReturnDeltaRotation" ) )
	FRotator							ReturnDeltaRotation;

	/* How to rotate saber during flight */
	UPROPERTY( BlueprintReadWrite, EditDefaultsOnly, Meta = ( DisplayName = "RotationDirection" ) )
	FRotator							RotationDirection;

	/* Speed of fly rotation */
	UPROPERTY( BlueprintReadWrite, EditDefaultsOnly, Meta = ( DisplayName = "RotationSpeed" ) )
	float								RotationSpeed;

	/* Fly speed */
	UPROPERTY( BlueprintReadWrite, EditDefaultsOnly, Meta = ( DisplayName = "FlySpeed" ) )
	float								FlySpeed;

	/* Fly speed when saber is returning */
	UPROPERTY( BlueprintReadWrite, EditDefaultsOnly, Meta = ( DisplayName = "ReturnSpeed" ) )
	float								ReturnSpeed;
	
	/* At what distance saber is counted as returned */
	UPROPERTY( BlueprintReadWrite, EditDefaultsOnly, Meta = ( DisplayName = "MinDistanceToHuman" ) )
	float								MinDistanceToHuman;
	
	UPROPERTY( BlueprintReadWrite, EditDefaultsOnly, Category = "Attacks", Meta = ( DisplayName = "BaseDamage" ) )
	int32								BaseDamage = 10;

	/* Do we need to damage other character if we are not attacking but just walking with saber? */
	UPROPERTY( BlueprintReadWrite, EditDefaultsOnly, Category = "Attacks", Meta = ( DisplayName = "bApplyDamageWithoutAttack" ) )
	bool								bApplyDamageWithoutAttack = false;

	UFUNCTION( BlueprintImplementableEvent, Category = "Saber", Meta = ( DisplayName = "OnSaberChangeState" ) )
	void								OnSaberChangeState( ESaberState NewState );

	UFUNCTION( BlueprintImplementableEvent, Category = "Saber", Meta = ( DisplayName = "OnSaberThrown" ) )
	void								OnSaberThrown();

	UFUNCTION( BlueprintImplementableEvent, Category = "Saber", Meta = ( DisplayName = "OnSaberReturnStart" ) )
	void								OnSaberReturnStart();

	UFUNCTION( BlueprintImplementableEvent, Category = "Saber", Meta = ( DisplayName = "OnBladeOverlapCPP" ) )
	void								OnBladeOverlapCPP( EBladeOverlapResult Result );
private:
	/* Set saber state */
	UFUNCTION( Server, Reliable, WithValidation )
	void								Server_SetSaberState( ESaberState NewState );
	void								Server_SetSaberState_Implementation( ESaberState NewState );
	bool								Server_SetSaberState_Validate( ESaberState NewState );

	UFUNCTION( NetMulticast, Reliable, WithValidation )
	void								Multicast_SetSaberState( ESaberState NewState );
	void								Multicast_SetSaberState_Implementation( ESaberState NewState );
	bool								Multicast_SetSaberState_Validate( ESaberState NewState );

	/* Update saber's blade length and location/rotation */
	UFUNCTION( Server, Reliable, WithValidation )
	void								Server_UpdateTransform( FTransform NewTransfrom, bool bUpdatePosition = false );
	void								Server_UpdateTransform_Implementation( FTransform NewTransfrom, bool bUpdatePosition = false );
	bool								Server_UpdateTransform_Validate( FTransform NewTransfrom, bool bUpdatePosition = false );

	UFUNCTION( NetMulticast, Reliable, WithValidation )
	void								Multicast_UpdateTransform( FTransform NewTransfrom, bool bUpdatePosition = false );
	void								Multicast_UpdateTransform_Implementation( FTransform NewTransfrom, bool bUpdatePosition = false );
	bool								Multicast_UpdateTransform_Validate( FTransform NewTransfrom, bool bUpdatePosition = false ) { return true; }

	//void UpdateTransformOnServer( FTransform NewTransform, bool bUpdatePosition = false );

	/* On blade overlap human */
	UFUNCTION( Server, Reliable, WithValidation )
	void								Server_BladeOverlap( AActor * OverlappedActor );
	void								Server_BladeOverlap_Implementation( AActor * OverlappedActor );
	bool								Server_BladeOverlap_Validate( AActor * OverlappedActor );

	UFUNCTION( NetMulticast, Reliable, WithValidation )
	void								Multicast_BladeOverlap( AActor * OverlappedActor );
	void								Multicast_BladeOverlap_Implementation( AActor * OverlappedActor );
	bool								Multicast_BladeOverlap_Validate( AActor * OverlappedActor );

	UFUNCTION( NetMulticast, Reliable, WithValidation )
	void								Multicast_DetachSaber( FTransform NewTransform );
	void								Multicast_DetachSaber_Implementation( FTransform NewTransform );
	bool								Multicast_DetachSaber_Validate( FTransform NewTransform ) { return true; }

	UFUNCTION( Server, Reliable, WithValidation )
	void								Server_LaunchSaber( float NewMaxFlyDist );
	void								Server_LaunchSaber_Implementation( float NewMaxFlyDist );
	bool								Server_LaunchSaber_Validate( float NewMaxFlyDist ) { return true; }
	
	UFUNCTION( Server, Reliable, WithValidation )
	void								Server_StopSaber();
	void								Server_StopSaber_Implementation();
	bool								Server_StopSaber_Validate() { return true; }

	/* Updates saber's transform on server.
	* @param NewTransform - new transform to set
	* @param bUpdatePosition - if true, Location and Rotation will be updated. Otherwise only scale will be updated.
	*/
	void								UpdateTransform( FTransform NewTransfrom, bool bUpdatePosition = false );
	UPROPERTY( Replicated )
	AHuman *							m_pHuman;
	UPROPERTY( Replicated )
	float								m_Alpha;
	UPROPERTY( Replicated )
	float								m_fMaxFlyDistance;

	UPROPERTY( Replicated )
	ESaberState							m_eState;
	
	UFUNCTION()
	void HiltOverlap( UPrimitiveComponent* OverlappedComp,
						  AActor* OtherActor,
						  UPrimitiveComponent* OtherComp,
						  int32 OtherBodyIndex,
						  bool bFromSweep,
						  const FHitResult& SweepResult );
	
	UFUNCTION()
	void BladeOverlap( UPrimitiveComponent* OverlappedComp,
						  AActor* OtherActor,
						  UPrimitiveComponent* OtherComp,
						  int32 OtherBodyIndex,
						  bool bFromSweep,
						  const FHitResult& SweepResult );

};
