

#include "MyNPC.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SphereComponent.h"
#include "MyPlayerCharacter.h"
#include "MySaveGame.h"

AMyNPC::AMyNPC()
{
	PrimaryActorTick.bCanEverTick = true;

	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;

	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	InteractionSphere->SetupAttachment(RootComponent);
	InteractionSphere->SetSphereRadius(200.f);

	InteractionSphere->OnComponentBeginOverlap.AddDynamic(this, &AMyNPC::OnOverlapBegin);
	InteractionSphere->OnComponentEndOverlap.AddDynamic(this, &AMyNPC::OnOverlapEnd);
}


void AMyNPC::BeginPlay()
{
	Super::BeginPlay();
	
}


void AMyNPC::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

    FRotator NewRot = FMath::RInterpTo(
        GetActorRotation(),
        TargetRotation,
        DeltaTime,
        5.f
    );

    SetActorRotation(NewRot);
}

void AMyNPC::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void AMyNPC::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult)
{
    if (AMyPlayerCharacter* Player = Cast<AMyPlayerCharacter>(OtherActor))
    {
        CurrentOverlappingPlayer = Player;
        Player->SetCurrentInteractable(this);
 
        UE_LOG(LogTemp, Warning, TEXT("Player Enter NPC Range"));
    }
}

void AMyNPC::OnOverlapEnd(UPrimitiveComponent* OverlappedComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex)
{
    if (OtherActor == CurrentOverlappingPlayer)
    {
        CurrentOverlappingPlayer->SetCurrentInteractable(nullptr);
        CurrentOverlappingPlayer->HideInteractPrompt();

        UE_LOG(LogTemp, Warning, TEXT("Player Leave NPC Range"));
    }
}


void AMyNPC::Interact_Implementation(AActor* Interactor)
{
    AMyPlayerCharacter* Player = Cast<AMyPlayerCharacter>(Interactor);
    if (!Player) return;

    FVector Direction = Player->GetActorLocation() - GetActorLocation();
    FRotator LookAtRotation = Direction.Rotation();
    LookAtRotation.Pitch = 0.f;
    LookAtRotation.Roll = 0.f;

    TargetRotation = LookAtRotation;
    
    if (!bHasLearnedAbility) {
        Player->StartDialogue(DialogueLines);
        bHasLearnedAbility = true;
        Player->UnlockMorphByTag(MorphTags);
    }
    else 
    {
        Player->StartDialogue(LearnedDialogueLines);
    }
   

   
}

void AMyNPC::SaveData_Implementation(UMySaveGame* GameData)
{
    GameData->NPCPositions.Add(NPCID, GetActorLocation());
    GameData->NPCAbility.Add(NPCID, bHasLearnedAbility);
} 

void AMyNPC::LoadData_Implementation(UMySaveGame* GameData)
{
    if (FVector* Location = GameData->NPCPositions.Find(NPCID))
    {
        SetActorLocation(*Location);
    }

    if (bool* learnedAbility = GameData->NPCAbility.Find(NPCID)) 
    {
        bHasLearnedAbility = *learnedAbility;
    }
}

