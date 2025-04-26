#pragma once

#include "v_sound_service.h"

#include <AL/al.h>
#include <AL/alc.h>

namespace storm
{

class OpenAlSoundService final : public VSoundService
{
public:
    OpenAlSoundService();
    ~OpenAlSoundService() override;

    bool Init() override;
    uint32_t RunSection() override;
    void RunStart() override;
    TSD_ID SoundPlay(const std::string_view &name, const SoundPlayOptions &options) override;
    TSD_ID SoundPlay(const std::string_view &name, eSoundType _type, eVolumeType _volumeType, bool _simpleCache,
                     bool _looped, bool _cached, int32_t _time, const CVECTOR *_startPosition, float _minDistance,
                     float _maxDistance, int32_t _loopPauseTime, float _volume, int32_t _prior) override;
    TSD_ID SoundDuplicate(TSD_ID _sourceID) override;
    void SoundSet3DParam(TSD_ID _id, eSoundMessage _message, const void *_op) override;
    void SoundStop(TSD_ID _id, int32_t _time) override;
    void SoundRelease(TSD_ID _id) override;
    void SoundSetVolume(TSD_ID _id, float _volume) override;
    bool SoundIsPlaying(TSD_ID _id) override;
    float SoundGetPosition(TSD_ID _id) override;
    void SoundRestart(TSD_ID _id) override;
    void SoundResume(TSD_ID _id, int32_t _time) override;
    void SetMasterVolume(float _fxVolume, float _musicVolume, float _speechVolume) override;
    void GetMasterVolume(float *_fxVolume, float *_musicVolume, float *_speechVolume) override;
    void SetPitch(float _pitch) override;
    float GetPitch() override;
    void SetCameraPosition(const CVECTOR &_cameraPosition) override;
    void SetCameraOrientation(const CVECTOR &_nose, const CVECTOR &_head) override;
    void ResetScheme() override;
    bool SetScheme(const char *_schemeName) override;
    bool AddScheme(const char *_schemeName) override;
    void SetEnabled(bool _enabled) override;
    void LoadAliasFile(const char *_filename) override;
    void SetActiveWithFade(bool active) override;
    void ShowEditor(bool &active) override;

private:
    ALCdevice *device_{};
    ALCcontext *context_{};
    std::vector<ALuint> buffers_{};
};

}
