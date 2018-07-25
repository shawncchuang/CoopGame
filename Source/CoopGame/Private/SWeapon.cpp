// Fill out your copyright notice in the Description page of Project Settings.

#include "SWeapon.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Components/SkeletalMeshComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "CoopGame.h"

static int32 DebugWeaponDrawing = 0;
FAutoConsoleVariableRef CVARDebugWeaponDrawing(
                        TEXT("COOP.DebugWeapons"),
                        DebugWeaponDrawing,
                        TEXT("Draw Debug Lines for Weapons"),
                        ECVF_Cheat);


// Sets default values
ASWeapon::ASWeapon()
{
 
    
    MeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComp"));
    RootComponent = MeshComp;

    MuzzleSocketName = "MuzzleSocket";
    TracerTargetName = "Target";
}


void ASWeapon::Fire()
{
    // Trace the world , from pawn eyes to cross chair location
    
    AActor* MyOwner = GetOwner();
    if(MyOwner)
    {
        FVector EyeLocation;
        FRotator EyeRotation;
        MyOwner->GetActorEyesViewPoint(EyeLocation, EyeRotation);
        
        FVector ShotDirection = EyeRotation.Vector();
        FVector TraceEnd = EyeLocation + (ShotDirection * 10000);
        
        FCollisionQueryParams QueryParams;
        QueryParams.AddIgnoredActor(MyOwner);
        QueryParams.AddIgnoredActor(this);
        QueryParams.bTraceComplex = true;
        QueryParams.bReturnPhysicalMaterial = true;
        
        // Particle "Target" parameter
        FVector TracerEndPoint = TraceEnd;
        
        FHitResult Hit;
        if(GetWorld()->LineTraceSingleByChannel(Hit,EyeLocation,TraceEnd, ECC_Visibility, QueryParams ))
        {
            // Blocking Hit !! Process damage
            
            AActor* HitActor = Hit.GetActor();
            
            UGameplayStatics::ApplyPointDamage(HitActor, 20.0f, ShotDirection, Hit, MyOwner->GetInstigatorController(), this, DamageType);
            
          
            EPhysicalSurface SurfaceType = UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get());
            
            UParticleSystem* SelectedEffect = nullptr;
            
            switch(SurfaceType)
            {
                case SURFACE_FLESHDEFAULT:
                case SURFACE_FLESHVULNERABLE:
                     SelectedEffect = FlashImpactEffect;
                    break;
            default:
                    SelectedEffect = DefaultImpactEffect;
                    break;
            }
           
            if(SelectedEffect)
            {
                UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), DefaultImpactEffect, Hit.ImpactPoint, Hit.ImpactNormal.Rotation());
            }
            
            TracerEndPoint = Hit.ImpactPoint;
        }
        
        PlayFireEffects(TracerEndPoint);
    
        if(DebugWeaponDrawing > 0)
        {
         DrawDebugLine(GetWorld(), EyeLocation, TraceEnd, FColor::White, false,1.0f, 0, 1.0f);
        }
    }
   
    
}

void ASWeapon::PlayFireEffects(FVector TraceEnd)
{
    if(MuzzleEffect)
    {
        UGameplayStatics::SpawnEmitterAttached(MuzzleEffect, MeshComp, MuzzleSocketName);
    }
    
    if(TracerEffect)
    {
        FVector MuzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName);
        
        UParticleSystemComponent* TracerComp =  UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), TracerEffect, MuzzleLocation);
        if(TracerComp)
        {
            //In this case , P_SmokeTrail particle system component will be TracerComp
            TracerComp->SetVectorParameter(TracerTargetName, TraceEnd);
        }
    }
    
    APawn* MyOwner = Cast<APawn>(GetOwner());
    if(MyOwner)
    {
     APlayerController* PC = Cast<APlayerController>(MyOwner->GetController());
        if(PC)
        {
            PC->ClientPlayCameraShake(FireCamShake);
        }
        
    }
}




