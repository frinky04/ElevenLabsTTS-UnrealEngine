#pragma once

#include "CoreMinimal.h"
#include "Interfaces/IHttpRequest.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "ElevenLabsRequest.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActionCompleted, USoundWave*, SoundWave);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActionFailed, const FString&, ErrorMessage);

UCLASS()
class ELEVENLABSAPI_API UElevenLabsRequest : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "ElevenLabs", meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Text To Speech"))
	static UElevenLabsRequest* TextToSpeech(UObject* WorldContextObject, const FString& VoiceId, const FString& Text, float Stability, float SimilarityBoost, const
	                                        FString& API_Key);

	UPROPERTY(BlueprintAssignable)
	FOnActionCompleted OnCompleted;

	UPROPERTY(BlueprintAssignable)
	FOnActionFailed OnFailed;

private:
	void RequestTextToSpeech(const FString& VoiceId, const FString& Text, float Stability, float SimilarityBoost, const FString& API_Key);
	void OnRequestCompleted(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	USoundWave* CreateSoundWaveFromResponse(const TArray<uint8>& AudioData);

	UPROPERTY()
	UObject* WorldContextObject;
};