#include "ElevenLabsRequest.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Serialization/JsonSerializer.h"
#include "Sound/SoundWave.h"

#define MINIMP3_IMPLEMENTATION
#pragma warning(push)
#pragma warning(disable: 4456)
#pragma warning(disable: 4706)
#include "minimp3.h"
#include "minimp3_ex.h"
#pragma warning(pop)

UElevenLabsRequest* UElevenLabsRequest::TextToSpeech(UObject* WorldContextObject, const FString& VoiceId, const FString& Text, float Stability, float SimilarityBoost, const FString& API_Key)
{
    UElevenLabsRequest* Request = NewObject<UElevenLabsRequest>();
    Request->WorldContextObject = WorldContextObject;
    Request->RequestTextToSpeech(VoiceId, Text, Stability, SimilarityBoost, API_Key);
    return Request;
}

void UElevenLabsRequest::RequestTextToSpeech(const FString& VoiceId, const FString& Text, float Stability, float SimilarityBoost, const FString& API_Key)
{
    FString Url = FString::Printf(TEXT("https://api.elevenlabs.io/v1/text-to-speech/%s/stream"), *VoiceId);
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
    HttpRequest->SetURL(Url);
    HttpRequest->SetVerb("POST");
    HttpRequest->SetHeader("Content-Type", "application/json");
    HttpRequest->SetHeader("xi-api-key", API_Key);

    TSharedPtr<FJsonObject> RequestJson = MakeShareable(new FJsonObject());
    RequestJson->SetStringField("text", Text);

    TSharedPtr<FJsonObject> VoiceSettingsJson = MakeShareable(new FJsonObject());
    VoiceSettingsJson->SetNumberField("stability", Stability);
    VoiceSettingsJson->SetNumberField("similarity_boost", SimilarityBoost);

    RequestJson->SetObjectField("voice_settings", VoiceSettingsJson);

    FString RequestBody;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
    FJsonSerializer::Serialize(RequestJson.ToSharedRef(), Writer);

    HttpRequest->SetContentAsString(RequestBody);
    HttpRequest->OnProcessRequestComplete().BindUObject(this, &UElevenLabsRequest::OnRequestCompleted);
    HttpRequest->ProcessRequest();
}

void UElevenLabsRequest::OnRequestCompleted(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    if (bWasSuccessful && Response->GetResponseCode() == 200)
    {
        const TArray<uint8>& AudioData = Response->GetContent();
        USoundWave* SoundWave = CreateSoundWaveFromResponse(AudioData);
        OnCompleted.Broadcast(SoundWave);
    }
    else
    {
        FString ErrorMessage = FString::Printf(TEXT("Request failed. Response code: %d"), Response->GetResponseCode());
        OnFailed.Broadcast(ErrorMessage);
    }
}

USoundWave* UElevenLabsRequest::CreateSoundWaveFromResponse(const TArray<uint8>& AudioData)
{
    mp3dec_t Mp3Decoder;
    mp3dec_init(&Mp3Decoder);

    mp3dec_file_info_t FileInfo;
    int LoadResult = mp3dec_load_buf(&Mp3Decoder, AudioData.GetData(), AudioData.Num(), &FileInfo, NULL, NULL);

    if (LoadResult != 0)
    {
        // Error loading MP3 data
        return nullptr;
    }

    USoundWave* SoundWave = NewObject<USoundWave>(USoundWave::StaticClass());
    if (SoundWave)
    {
        SoundWave->RawPCMDataSize = FileInfo.samples * sizeof(int16_t);
        SoundWave->RawPCMData = static_cast<uint8*>(FMemory::Malloc(SoundWave->RawPCMDataSize));
        FMemory::Memcpy(SoundWave->RawPCMData, FileInfo.buffer, SoundWave->RawPCMDataSize);

        // Set the necessary SoundWave properties
        SoundWave->NumChannels = FileInfo.channels;
        SoundWave->Duration = static_cast<float>(FileInfo.samples) / FileInfo.hz;
        SoundWave->SetSampleRate(FileInfo.hz);
        SoundWave->SoundGroup = SOUNDGROUP_Voice;
        SoundWave->bLooping = false;
    }

    // Free the decoded buffer and return the SoundWave object
    free(FileInfo.buffer);
    return SoundWave;
}