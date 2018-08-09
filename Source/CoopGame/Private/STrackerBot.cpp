// Fill out your copyright notice in the Description page of Project Settings.

#include "STrackerBot.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "AI/Navigation/NavigationSystem.h"
#include "AI/Navigation/NavigationPath.h"
#include "DrawDebugHelpers.h"
#include "SHealthComponent.h"

// Sets default values
ASTrackerBot::ASTrackerBot()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
    
    MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
    MeshComp->SetCanEverAffectNavigation(false);
    MeshComp->SetSimulatePhysics(true);
    RootComponent = MeshComp;
    
    HealthComp = CreateDefaultSubobject<USHealthComponent>(TEXT("HealthComp"));
    HealthComp->OnHealthChanged.AddDynamic(this, &ASTrackerBot::HandleTakeDamage);
    
    bUseVelocityChange = true;
    MovementForce = 1000;
    RequiredDistanceToTarget = 100;
}

void ASTrackerBot::HandleTakeDamage(USHealthComponent*  OwningHealthComp , float Health, float HealthDelta,const class UDamageType* DamageType , class AController* InstigateBy, AActor* DamageCauser)
{
    // Explode on hitpoint == 0
    
    // @TODO:Pulse the maerial on hit
    if(MatInst == nullptr)
    {
       MatInst = MeshComp->CreateAndSetMaterialInstanceDynamicFromMaterial(0, MeshComp->GetMaterial(0));
    }
    if(MatInst)
    {
       MatInst->SetScalarParameterValue("LastTimeDamageTaken", GetWorld()->TimeSeconds);
    }
   
    UE_LOG(LogTemp, Log, TEXT("Health %s of %s"),*FString::SanitizeFloat(Health),*GetName());
}

// Called when the game starts or when spawned
void ASTrackerBot::BeginPlay()
{
	Super::BeginPlay();
    
    // Find initial move-to
    NextPathPoint = GetNextPathPoint();
}

// Called every frame
void ASTrackerBot::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
    
    float DistanceToTarget = (GetActorLocation() - NextPathPoint).Size();
    
    if(DistanceToTarget <= RequiredDistanceToTarget)
    {
        NextPathPoint = GetNextPathPoint();
        //DrawDebugString(GetWorld(), GetActorLocation(), "Target Reached !");
    }
    else
    {
        
        // Keep moving towards next target
        FVector ForceDirection = NextPathPoint - GetActorLocation();
        ForceDirection.Normalize();

        ForceDirection *= MovementForce;
        
        MeshComp->AddForce(ForceDirection, NAME_None, bUseVelocityChange);
        DrawDebugDirectionalArrow(GetWorld(),GetActorLocation(),GetActorLocation()+ForceDirection,32,FColor::Yellow,false,0.0f,0,1.0f);
    }
        
    DrawDebugSphere(GetWorld(),NextPathPoint,20,12, FColor::Yellow,false,40.f,1.0f);

}

FVector ASTrackerBot::GetNextPathPoint()
{
    AActor* PlayerPawn = UGameplayStatics::GetPlayerPawn(this,0);
    
    UNavigationPath* NavPath = UNavigationSystem::FindPathToActorSynchronously(this, GetActorLocation(), PlayerPawn);
    if(NavPath->PathPoints.Num() > 1)
    {
      //Return next path in the path
      return NavPath->PathPoints[1];
    }

    // Failed to find path
    return GetActorLocation();
}