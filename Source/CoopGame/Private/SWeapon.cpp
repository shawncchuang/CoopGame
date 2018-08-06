// Fill out your copyright notice in the Description page of Project Settings.

#include "SWeapon.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Components/SkeletalMeshComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "CoopGame.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"

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
    BaseDamage = 20.0f;
    RateOfFire = 600;
    
    // this actor replicates to network clients. When this actor is spawned on the server it will be sent to clients as well. 
    SetReplicates(true);
    
    NetUpdateFrequency = 66.0f;
    MinNetUpdateFrequency = 33.0f;
}

void ASWeapon::BeginPlay()
{
    Super::BeginPlay();
    
    TimeBetweenShots = 60 / RateOfFire;
    
}

void ASWeapon::Fire()
{
    // Trace the world , from pawn eyes to cross chair location
    
    if(Role < ROLE_Authority)
    {
        // only called from clients
        ServerFire();
        
    }
    
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
        
        EPhysicalSurface SurfaceType = SurfaceType_Default;
        
        FHitResult Hit;
        if(GetWorld()->LineTraceSingleByChannel(Hit, EyeLocation, TraceEnd, COLLISION_WEAPON, QueryParams ))
        {
            // Blocking Hit !! Process damage
            
            AActor* HitActor = Hit.GetActor();
            
            SurfaceType = UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get());
            
            float ActualDamage = BaseDamage;
            if(SurfaceType == SURFACE_FLESHVULNERABLE)
            {
                ActualDamage *= 4.0f;
            }
            
            UGameplayStatics::ApplyPointDamage(HitActor, ActualDamage , ShotDirection, Hit, MyOwner->GetInstigatorController(), this, DamageType);
            
            PlayImpactEffects(SurfaceType, Hit.ImpactPoint);
            
            TracerEndPoint = Hit.ImpactPoint;
        }
        
        PlayFireEffects(TracerEndPoint);
        
      
        
        if(Role == ROLE_Authority)
        {
            HitScanTrace.TraceTo = TracerEndPoint;
            HitScanTrace.SurfaceType = SurfaceType;
        }
        
        LastFireTime = GetWorld()->TimeSeconds;
        
    
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

void ASWeapon::StartFire() {
    
    float FirstDelay = FMath::Max(LastFireTime + TimeBetweenShots - GetWorld()->TimeSeconds, 0.0f) ;
    
    GetWorldTimerManager().SetTimer(TimerHandle_TimebetweeenShots,this, &ASWeapon::Fire,TimeBetweenShots,true,FirstDelay);
    
     
}

void ASWeapon::StopFire() {
    GetWorldTimerManager().ClearTimer(TimerHandle_TimebetweeenShots);
}

void ASWeapon::OnRep_HitScanTrace() {
   // Play cosmeitc FX
    PlayFireEffects(HitScanTrace.TraceTo);
    PlayImpactEffects(HitScanTrace.SurfaceType, HitScanTrace.TraceTo );
}


void ASWeapon::ServerFire_Implementation()
{
    // Server function
    Fire();
}

bool ASWeapon::ServerFire_Validate()
{
    return true;
    
}

void ASWeapon::PlayImpactEffects(EPhysicalSurface SurfaceType , FVector ImpactPoint)
{
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
        
        FVector Muzzlelocation = MeshComp->GetSocketLocation(MuzzleSocketName);
        FVector ShotDirection = ImpactPoint - Muzzlelocation;
        ShotDirection.Normalize();
        
        UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), SelectedEffect, ImpactPoint, ShotDirection.Rotation());
    }
    
}

void ASWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    
    // replicate everyone except wherever has actually shot this weapon
   DOREPLIFETIME_CONDITION(ASWeapon, HitScanTrace , COND_SkipOwner);
 
}






