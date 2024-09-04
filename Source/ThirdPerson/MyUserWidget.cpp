// Fill out your copyright notice in the Description page of Project Settings.


#include "MyUserWidget.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "ThirdPersonCharacter.h" /

void UMyWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (MyButton)
    {
        MyButton->OnClicked.AddDynamic(this, &UMyWidget::OnButtonClicked);
    }
}

void UMyWidget::OnButtonClicked()
{
    UE_LOG(LogTemp, Warning, TEXT("Button Clicked!"));
    if (MyTextBox)
    {
        FString InputText = MyTextBox->GetText().ToString();

        // Get the player controller
        APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
        if (PlayerController)
        {
            // Get the controlled character
            AThirdPersonCharacter* Character = Cast<AThirdPersonCharacter>(PlayerController->GetPawn());
            if (Character)
            {
                // Call the function in ThirdPersonCharacter
                Character->SubmitButton(InputText);
            }
        }
    }
}
