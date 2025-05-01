#include "openal_sound_service.hpp"

#include "wav_loader.hpp"

#include <spdlog/spdlog.h>
#include <vma.hpp>

namespace
{

constexpr auto BUFFER_COUNT = 2;

using SoundService = storm::OpenAlSoundService;
CREATE_SERVICE(SoundService);

template <typename T>
T ErrorHandler(T result, const std::string_view &file, size_t line, const std::string_view &func,
               const std::string_view &expr)
{
    auto get_error_str = [](const size_t code) -> std::string_view {
        switch (code)
        {
        case AL_NO_ERROR:
            return "No error";
        case AL_INVALID_NAME:
            return "Invalid name";
        case AL_INVALID_ENUM:
            return "Invalid enum";
        case AL_INVALID_VALUE:
            return "Invalid value";
        case AL_INVALID_OPERATION:
            return "Invalid operation";
        case AL_OUT_OF_MEMORY:
            return "Out of memory";
        default:
            return "Unknown error";
        }
    };

    if (const auto &error = alGetError(); error != AL_NO_ERROR)
    {
        spdlog::trace("[{}:{}:{}] {} ({})", file, func, line, get_error_str(error), expr);
    }
    return result;
}

#define CHECK_AL_ERROR(expr) ErrorHandler(expr, __FILE__, __LINE__, __func__, #expr)
#define CHECK_AL_ERROR_VOID(expr) expr; ErrorHandler(true, __FILE__, __LINE__, __func__, #expr)

} // namespace

namespace storm
{
OpenAlSoundService::OpenAlSoundService() = default;

OpenAlSoundService::~OpenAlSoundService()
{
    if (buffers_.size() > 0)
    {
        CHECK_AL_ERROR_VOID(alDeleteBuffers(BUFFER_COUNT, buffers_.data()));
    }

    // CHECK_AL_ERROR(alcMakeContextCurrent(nullptr));
    if (context_)
    {
        alcDestroyContext(context_);
    }

    if (device_)
    {
        alcCloseDevice(device_);
    }
}

bool OpenAlSoundService::Init()
{
    device_ = alcOpenDevice(nullptr);
    assert(device_);
    context_ = alcCreateContext(device_, nullptr);
    assert(context_);
    CHECK_AL_ERROR(alcMakeContextCurrent(context_));

    buffers_.resize(BUFFER_COUNT);
    CHECK_AL_ERROR_VOID(alGenBuffers(BUFFER_COUNT, buffers_.data()));

    return true;
}
uint32_t OpenAlSoundService::RunSection()
{
    return {};
}
void OpenAlSoundService::RunStart()
{
}
TSD_ID OpenAlSoundService::SoundPlay(const std::string_view &name, const SoundPlayOptions &options)
{
    AudioResource audio = WavLoader().LoadFromFile(fmt::format("resource/sounds/{}", name));
    const auto &sampleData = audio.GetSampleData();
    CHECK_AL_ERROR_VOID(alBufferData(buffers_[0], AL_FORMAT_STEREO16, sampleData.data(), sampleData.size_bytes(), audio.GetSampleRate()));

    ALuint sources[1]{};
    CHECK_AL_ERROR_VOID(alGenSources(1, sources));

    CHECK_AL_ERROR_VOID(alSourcef(sources[0], AL_GAIN, options.volume));
    CHECK_AL_ERROR_VOID(alSourcei(sources[0], AL_LOOPING, options.looped));

    CHECK_AL_ERROR_VOID(alSourceQueueBuffers(sources[0], 1, buffers_.data()));
    CHECK_AL_ERROR_VOID(alSourcePlay(sources[0]));

    ALint source_state{};
    CHECK_AL_ERROR_VOID(alGetSourcei(sources[0], AL_SOURCE_STATE, &source_state));

    return {};
}
TSD_ID OpenAlSoundService::SoundPlay(const std::string_view &name, eSoundType _type, eVolumeType _volumeType,
                                     bool _simpleCache, bool _looped, bool _cached, int32_t _time,
                                     const CVECTOR *_startPosition, float _minDistance, float _maxDistance,
                                     int32_t _loopPauseTime, float _volume, int32_t _prior)
{
    SoundPlayOptions options {
        .type = _type,
        .volumeType = _volumeType,
        .startPosition = *_startPosition,
        .minDistance = _minDistance,
        .maxDistance = _maxDistance,
        .volume = _volume,
        .fadeInTime = _time,
        .loopPauseTime = _loopPauseTime,
        .priority = _prior,
        .simpleCache = _simpleCache,
        .looped = _looped,
        .cached = _cached,
    };
    return SoundPlay(name, options);
}
TSD_ID OpenAlSoundService::SoundDuplicate(TSD_ID _sourceID)
{
    __debugbreak();
    return {};
}
void OpenAlSoundService::SoundSet3DParam(TSD_ID _id, eSoundMessage _message, const void *_op)
{
    __debugbreak();
}
void OpenAlSoundService::SoundStop(TSD_ID _id, int32_t _time)
{
    __debugbreak();
}
void OpenAlSoundService::SoundRelease(TSD_ID _id)
{
    __debugbreak();
}
void OpenAlSoundService::SoundSetVolume(TSD_ID _id, float _volume)
{
    __debugbreak();
}
bool OpenAlSoundService::SoundIsPlaying(TSD_ID _id)
{
    __debugbreak();
    return {};
}
float OpenAlSoundService::SoundGetPosition(TSD_ID _id)
{
    __debugbreak();
    return {};
}
void OpenAlSoundService::SoundRestart(TSD_ID _id)
{
    __debugbreak();
}
void OpenAlSoundService::SoundResume(TSD_ID _id, int32_t _time)
{
    __debugbreak();
}
void OpenAlSoundService::SetMasterVolume(float _fxVolume, float _musicVolume, float _speechVolume)
{
    __debugbreak();
}
void OpenAlSoundService::GetMasterVolume(float *_fxVolume, float *_musicVolume, float *_speechVolume)
{
    __debugbreak();
}
void OpenAlSoundService::SetPitch(float _pitch)
{
    __debugbreak();
}
float OpenAlSoundService::GetPitch()
{
    __debugbreak();
    return {};
}
void OpenAlSoundService::SetCameraPosition(const CVECTOR &_cameraPosition)
{
    __debugbreak();
}
void OpenAlSoundService::SetCameraOrientation(const CVECTOR &_nose, const CVECTOR &_head)
{
    __debugbreak();
}
void OpenAlSoundService::ResetScheme()
{
    __debugbreak();
}
bool OpenAlSoundService::SetScheme(const char *_schemeName)
{
    __debugbreak();
    return {};
}
bool OpenAlSoundService::AddScheme(const char *_schemeName)
{
    __debugbreak();
    return {};
}
void OpenAlSoundService::SetEnabled(bool _enabled)
{
    __debugbreak();
}
void OpenAlSoundService::LoadAliasFile(const char *_filename)
{
    __debugbreak();
}
void OpenAlSoundService::SetActiveWithFade(bool active)
{
    // __debugbreak();
}
void OpenAlSoundService::ShowEditor(bool &active)
{
}

} // namespace storm
