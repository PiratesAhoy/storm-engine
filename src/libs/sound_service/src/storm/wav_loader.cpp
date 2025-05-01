#include "wav_loader.hpp"

#include "file_service.h"

#include <iostream>

namespace storm
{

AudioResource WavLoader::LoadFromFile(const std::string_view &name)
{
    AudioResource result{};

    const std::string name_str{name};
    std::fstream fs = fio->_CreateFile(name_str.c_str(), std::ios::binary | std::ios::in);

    if (!fs.is_open())
    {
        throw std::runtime_error("Failed to open file");
    }

    struct RiffHeader
    {
        char riffID[4];
        uint32_t riffSize;
        char waveID[4];
    } riff_header{};

    fs.read(reinterpret_cast<char *>(&riff_header), sizeof(riff_header));

    if (std::string_view(riff_header.riffID, 4) != "RIFF" || std::string_view(riff_header.waveID, 4) != "WAVE")
    {
        throw std::runtime_error("Invalid WAVE file");
    }

    struct ChunkHeader
    {
        char chunkID[4];
        uint32_t chunkSize;
    };

    auto readChunkHeader = [] (std::fstream& file) -> std::optional<ChunkHeader> {
        ChunkHeader result{};
        file.read(result.chunkID, 4);
        if (file.gcount() < 4)
        {
            return {};
        }
        file.read(reinterpret_cast<char*>(&result.chunkSize), sizeof(result.chunkSize));
        if (!file.good() )
        {
            return {};
        }
        return result;
    };

    struct WAVHeader {
        uint16_t audioFormat;    // Audio format (1 = PCM)
        uint16_t numChannels;    // Number of channels
        uint32_t sampleRate;     // Sampling rate
        uint32_t byteRate;       // (SampleRate * NumChannels * BitsPerSample) / 8
        uint16_t blockAlign;     // (NumChannels * BitsPerSample) / 8
        uint16_t bitsPerSample;  // Number of bits per sample
    };

    struct WAVFile {
        WAVHeader header;
        std::vector<uint8_t> audioData;
    } wav_file{};

    // Read chunks
    bool fmtFound = false;
    bool dataFound = false;
    while (fs /*&& (!fmtFound || !dataFound)*/)
    {
        const auto chunk = readChunkHeader(fs);
        if (!chunk)
        {
            break;
        }

        const auto chunk_id = std::string_view(chunk->chunkID, 4);
        if (chunk_id == "fmt ")
        {
            if (chunk->chunkSize < 16)
            {
                throw std::runtime_error("Invalid fmt chunk size");
            }

            fs.read(reinterpret_cast<char *>(&wav_file.header.audioFormat), sizeof(WAVHeader));
            if (chunk->chunkSize > sizeof(WAVHeader))
            {
                // Skip any extra bytes in fmt chunk
                fs.seekg(chunk->chunkSize - sizeof(WAVHeader), std::ios::cur);
            }

            result.sampleRate_ = wav_file.header.sampleRate;

            fmtFound = true;
        }
        else if (chunk_id == "data")
        {
            result.samples_.resize(chunk->chunkSize / 2);
            // wav_file.audioData.resize(chunk->chunkSize);
            fs.read(reinterpret_cast<char *>(result.samples_.data()), chunk->chunkSize);
            dataFound = true;
        }
        else
        {
            // Skip unrecognized chunk
            fs.seekg(chunk->chunkSize, std::ios::cur);
        }

        // If chunk size is odd, skip padding byte (chunks are aligned to even sizes)
        if (chunk->chunkSize % 2 == 1)
        {
            fs.seekg(1, std::ios::cur);
        }
    }

    if (!fmtFound)
    {
        throw std::runtime_error("Missing 'fmt ' chunk");
    }

    if (!dataFound)
    {
        throw std::runtime_error("Missing 'data ' chunk");
    }

    return result;
}

}
