// Fill out your copyright notice in the Description page of Project Settings.

#include "SHealthComponent.h"


// Sets default values for this component's properties
USHealthComponent::USHealthComponent()
{

    DefaultHeath = 100;
}


// Called when the game starts
void USHealthComponent::BeginPlay()
{
	Super::BeginPlay();

    AActor* MyOwner = GetOwner();
    
    if(MyOwner)
    {
        
        MyOwner->OnTakeAnyDamage.AddDynamic(this, &USHealthComponent::HandleTakeAnyDamage);
        
    }
    Health = DefaultHeath;
    
    
	
}

void USHealthComponent::HandleTakeAnyDamage(AActor *DamagedActor, float Damage, const class UDamageType *DamageType, class AController *InstigateBy, AActor *DamageCauser) {
   
    if(Damage <= 0.0f)
    {
        return;
    }
    
    // UPdate health clamped
    Health = FMath::Clamp(Health - Damage, 0.0f, DefaultHeath);
    
    UE_LOG(LogTemp, Log, TEXT("Health changed : %s"), *FString::SanitizeFloat(Health));
    
    OnHealthChanged.Broadcast(this, Health, Damage, DamageType, InstigateBy, DamageCauser );
}





