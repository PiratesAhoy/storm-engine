#pragma once

#include "c_vector.h"
#include "service.h"
#include "sound_defines.h"
#include "v_sound_service.h"

#include <optional>
#include <string_view>
#include <vector>

///////////////////////////////////////////////////////////////////
// DEFINES & TYPES
///////////////////////////////////////////////////////////////////

class TSD_ID
{
  public:
    TSD_ID() : id_(0xFFFFFFFF)
    {
    }

    TSD_ID(uint32_t id) : id_(id)
    {
    }

    TSD_ID(int32_t id) : id_(id)
    {
    }

    bool master()
    {
        return id_ == 0;
    }

    uint16_t index() const
    {
        return (id_ & 0xFFFF) - 1;
    }

    uint16_t stamp() const
    {
        return (id_ >> 16) & 0xFFFF;
    }

    static TSD_ID createId(uint16_t index)
    {
        static uint16_t stamp_counter;
        return (index + 1) | (static_cast<uint32_t>(++stamp_counter) << 16);
    }

    operator int32_t()
    {
        return id_;
    }

  private:
    uint32_t id_;
};

using tSoundStatistics = struct
{
    int32_t soundsCount, maxSoundsCount, bytesInBuffers, maxBytesInBuffers, bytesCached, maxBytesCached, totalMem,
        freeMem;
};

///////////////////////////////////////////////////////////////////
// INTERFACE DEFINITION
///////////////////////////////////////////////////////////////////
class VSoundService : public SERVICE
{
  public:
    constexpr static std::string_view ServiceName = "SoundService";

    bool Init() override = 0;
    uint32_t RunSection() override = 0;
    void RunStart() override = 0;

    // Sound entries functions
    //
    // + play returns id of new loaded sound
    // + in most functions _id = 0 equals "apply to all"
    // + play assumes _name a plain file name
    // + for now loop_pause_time works only for OGG-STEREO sounds
    ///////////////////////////////////////////////////////////////
    struct SoundPlayOptions
    {
        eSoundType type{eSoundType::MP3_STEREO};
        eVolumeType volumeType{eVolumeType::VOLUME_FX};
        std::optional<CVECTOR> startPosition{};
        std::optional<float> minDistance{};
        std::optional<float> maxDistance{};
        float volume{1};
        int32_t fadeInTime{0};
        int32_t loopPauseTime{0};
        int32_t priority{128};
        std::optional<uint32_t> loopStart{};
        std::optional<uint32_t> loopEnd{};
        bool simpleCache{false};
        bool looped{false};
        bool cached{false};
    };
    virtual TSD_ID SoundPlay(const std::string_view &name, const SoundPlayOptions& options) = 0;
    virtual TSD_ID SoundPlay(const std::string_view &name,
                             eSoundType _type,          // sound type
                             eVolumeType _volumeType,   // volume type
                             bool _simpleCache = false, // cache only, not play
                             bool _looped = false,      // looped?
                             bool _cached = false,      // unload after stoppping?
                             int32_t _time = 0,         // fade in, if _time > 0
                             const CVECTOR *_startPosition = nullptr, float _minDistance = -1.0f,
                             float _maxDistance = -1.0f, int32_t _loopPauseTime = 0, float _volume = 1.0f,
                             int32_t _prior = 128) = 0;

    virtual TSD_ID SoundDuplicate(TSD_ID _sourceID) = 0;
    virtual void SoundSet3DParam(TSD_ID _id, eSoundMessage _message, const void *_op) = 0;
    virtual void SoundStop(TSD_ID _id, int32_t _time = 0) = 0;
    virtual void SoundRelease(TSD_ID _id) = 0;
    virtual void SoundSetVolume(TSD_ID _id, float _volume) = 0;
    virtual bool SoundIsPlaying(TSD_ID _id) = 0;
    virtual float SoundGetPosition(TSD_ID _id) = 0;
    virtual void SoundRestart(TSD_ID _id) = 0;
    virtual void SoundResume(TSD_ID _id, int32_t _time = 0) = 0;

    // Service functions
    virtual void SetMasterVolume(float _fxVolume, float _musicVolume, float _speechVolume) = 0;
    virtual void GetMasterVolume(float *_fxVolume, float *_musicVolume, float *_speechVolume) = 0;
    virtual void SetPitch(float _pitch) = 0;
    virtual float GetPitch() = 0;
    virtual void SetCameraPosition(const CVECTOR &_cameraPosition) = 0;
    virtual void SetCameraOrientation(const CVECTOR &_nose, const CVECTOR &_head) = 0;

    virtual void ResetScheme() = 0;
    virtual bool SetScheme(const char *_schemeName) = 0;
    virtual bool AddScheme(const char *_schemeName) = 0;

    virtual void SetEnabled(bool _enabled) = 0;
    virtual void LoadAliasFile(const char *_filename) = 0;

    virtual void SetActiveWithFade(bool active) = 0;

    virtual void ShowEditor(bool &active) = 0;

    tSoundStatistics soundStatistics;
};

/*
API_SERVICE_START("sound service")
    DECLARE_MAIN_SERVICE(SoundService)
API_SERVICE_END(SoundService)
*/
