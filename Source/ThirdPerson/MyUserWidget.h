// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MyUserWidget.generated.h"

UCLASS()
class THIRDPERSON_API UMyWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual void NativeConstruct() override;

private:
    UFUNCTION()
    void OnButtonClicked();

    UPROPERTY(meta = (BindWidget))
    class UButton* MyButton;

    UPROPERTY(meta = (BindWidget))
    class UEditableTextBox* MyTextBox;
};