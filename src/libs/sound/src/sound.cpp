#include "v_sound_service.h"
#include "shared/messages.h"

#include "sound.h"

#include "core.h"
#include "vma.hpp"

CREATE_CLASS(SOUND)

#define MSG_SOUND_ALIAS_ADD 77017 //"s"          alias_name

//--------------------------------------------------------------------
SOUND::SOUND()
    : soundService(nullptr), renderer(nullptr)
{
}

//--------------------------------------------------------------------
SOUND::~SOUND()
{
}

//--------------------------------------------------------------------
bool SOUND::Init()
{
    // GUARD(SOUND::Init)

    soundService = static_cast<VSoundService *>(core.GetService("SoundService"));
    if (!soundService)
        core.Trace("!SOUND: Can`t create sound service");

    renderer = static_cast<VDX9RENDER *>(core.GetService("dx9render"));
    core.AddToLayer(REALIZE, GetId(), -1);

    return true;
    // UNGUARD
}

//--------------------------------------------------------------------
uint64_t SOUND::ProcessMessage(MESSAGE &message)
{
    ////GUARD(SOUND::ProcessMessage)

    if (!soundService)
        return 0;

    auto code = message.Long();
    CVECTOR vector, vector2;
    int32_t temp;
    int32_t id, tempLong;
    float v1, v2, v3;
    float pitch;
    VDATA *vd1, *vd2, *vd3;
    VDATA *pd;
    uint32_t outValue = 0;

    switch (code)
    {
    case MSG_SOUND_SET_MASTER_VOLUME:
        v1 = message.Float();
        v2 = message.Float();
        v3 = message.Float();
        soundService->SetMasterVolume(v1, v2, v3);
        break;
    case MSG_SOUND_GET_MASTER_VOLUME:
        soundService->GetMasterVolume(&v1, &v2, &v3);
        vd1 = message.ScriptVariablePointer();
        vd2 = message.ScriptVariablePointer();
        vd3 = message.ScriptVariablePointer();
        vd1->Set(v1);
        vd2->Set(v2);
        vd3->Set(v3);
        break;
    case MSG_SOUND_SET_PITCH:
        pitch = message.Float();
        soundService->SetPitch(pitch);
        break;
    case MSG_SOUND_GET_PITCH:
        pitch = soundService->GetPitch();
        pd = message.ScriptVariablePointer();
        pd->Set(pitch);
        break;
    case MSG_SOUND_SET_CAMERA_POSITION:
        vector.x = message.Float();
        vector.y = message.Float();
        vector.z = message.Float();
        soundService->SetCameraPosition(vector);
        break;
    case MSG_SOUND_SET_CAMERA_ORIENTATION:
        vector.x = message.Float();
        vector.y = message.Float();
        vector.z = message.Float();
        vector2.x = message.Float();
        vector2.y = message.Float();
        vector2.z = message.Float();
        soundService->SetCameraOrientation(vector, vector2);
        break;
    case MSG_SOUND_PLAY: {
        const std::string &name = message.String();

        VSoundService::SoundPlayOptions options{};
        options.type = static_cast<eSoundType>(message.Long());

        // try to read as many parameters   as we can
        if (message.GetCurrentFormatType())
            options.volumeType = static_cast<eVolumeType>(message.Long());
        if (message.GetCurrentFormatType())
            options.simpleCache = message.Long();
        if (message.GetCurrentFormatType())
            options.looped = message.Long();
        if (message.GetCurrentFormatType())
            options.cached = message.Long();
        if (message.GetCurrentFormatType())
            options.fadeInTime = message.Long();
        // boal fix 28.10.06
        if (options.type == SOUND_MP3_STEREO || options.type == SOUND_WAV_STEREO)
        {
            if (options.looped) // stereo OGG, looped
            {
                if (message.GetCurrentFormatType())
                    options.loopPauseTime = message.Long();
                if (message.GetCurrentFormatType())
                    options.volume = message.Float();
                if (message.GetCurrentFormatType() == 'l')
                    options.loopStart = message.Long();
                if (message.GetCurrentFormatType() == 'l')
                    options.loopEnd = message.Long();
            }
        }
        else
        {
            CVECTOR startPosition = {};
            if (message.GetCurrentFormatType())
                startPosition.x = message.Float();
            if (message.GetCurrentFormatType())
                startPosition.y = message.Float();
            if (message.GetCurrentFormatType())
                startPosition.z = message.Float();
            options.startPosition = startPosition;
            if (message.GetCurrentFormatType())
                options.minDistance = message.Float();
            if (message.GetCurrentFormatType())
                options.maxDistance = message.Float();
        }

        outValue = static_cast<uint32_t>(soundService->SoundPlay(name, options));
        break;
    }
    case MSG_SOUND_STOP:
        id = message.Long();
        tempLong = message.Long();
        soundService->SoundStop(id, tempLong);
        break;
    case MSG_SOUND_RELEASE:
        id = message.Long();
        soundService->SoundRelease(id);
        break;
    case MSG_SOUND_DUPLICATE:
        id = message.Long();
        outValue = static_cast<uint32_t>(soundService->SoundDuplicate(id));
        break;
    case MSG_SOUND_SET_3D_PARAM:
        id = message.Long();
        tempLong = message.Long();
        switch (tempLong)
        {
        case SOUND_PARAM_MAX_DISTANCE:
            vector.x = message.Float();
            // memcpy(&(vector.x) ,message.Pointer(), sizeof(float));
            soundService->SoundSet3DParam(id, SM_MAX_DISTANCE, &(vector.x));
            break;
        case SOUND_PARAM_MIN_DISTANCE:
            vector.x = message.Float();
            // memcpy(&(vector.x) ,message.Pointer(), sizeof(float));
            soundService->SoundSet3DParam(id, SM_MIN_DISTANCE, &(vector.x));
            break;
        case SOUND_PARAM_POSITION:
            vector.x = message.Float();
            vector.y = message.Float();
            vector.z = message.Float();
            // memcpy(&(vector.x) ,message.Pointer(), sizeof(float));
            // memcpy(&(vector.y) ,message.Pointer()+sizeof(float), sizeof(float));
            // memcpy(&(vector.z) ,message.Pointer()+(sizeof(float) << 1), sizeof(float));
            soundService->SoundSet3DParam(id, SM_POSITION, &vector);
            break;
        }
        break;
    case MSG_SOUND_SET_VOLUME:
        id = message.Long();
        vector.x = message.Float();
        soundService->SoundSetVolume(id, vector.x);
        break;
    case MSG_SOUND_IS_PLAYING:
        id = message.Long();
        outValue = soundService->SoundIsPlaying(id);
        break;
    case MSG_SOUND_GET_POSITION:
        id = message.Long();
        outValue = static_cast<uint32_t>(soundService->SoundGetPosition(id) * 100.0f);
        break;
    case MSG_SOUND_RESTART:
        id = message.Long();
        soundService->SoundRestart(id);
        break;
    case MSG_SOUND_RESUME:
        id = message.Long();
        temp = message.Long();
        soundService->SoundResume(id, temp);
        break;
    case MSG_SOUND_SCHEME_RESET:
        soundService->ResetScheme();
        break;
    case MSG_SOUND_SCHEME_SET: {
        const std::string &tempString = message.String();
        soundService->SetScheme(tempString.c_str());
        break;
    }
    case MSG_SOUND_SCHEME_ADD: {
        const std::string &tempString = message.String();
        soundService->AddScheme(tempString.c_str());
        break;
    }

    case MSG_SOUND_SET_ENABLED:
        soundService->SetEnabled(message.Long() != 0);
        break;

    case MSG_SOUND_ALIAS_ADD: {
        const std::string &tempString = message.String();
        soundService->LoadAliasFile(tempString.c_str());
        break;
    }
    }

    return outValue;
    ////UNGUARD
}

//--------------------------------------------------------------------
void SOUND::Realize(uint32_t dTime)
{
    if (!soundService)
        return;
}

//--------------------------------------------------------------------
//--------------------------------------------------------------------
