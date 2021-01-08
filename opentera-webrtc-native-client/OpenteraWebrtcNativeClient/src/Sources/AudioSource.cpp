#include <OpenteraWebrtcNativeClient/Sources/AudioSource.h>

using namespace opentera;
using namespace std;

/**
 * @brief Calculate the frame size in bytes
 *
 * @param bitsPerSample the audio stream sample size (8, 16 or 32 bits)
 * @param numberOfChannels Number of channels
 * @return The frame size in bytes
 *
 * @throw runtime_error if bitsPerSample is invalid
 */
size_t bytesPerFrame(int bitsPerSample, size_t numberOfChannels)
{
    switch (bitsPerSample)
    {
        case 8:
            return sizeof(int8_t) * numberOfChannels;
        case 16:
            return sizeof(int16_t) * numberOfChannels;
        case 32:
            return sizeof(int32_t) * numberOfChannels;
        default:
            throw runtime_error("Invalid bitsPerSample");
    }
}

/**
 * @brief Creates an AudioSource
 *
 * @param configuration the configuration applied to the audio stream by the audio transport layer
 * @param bitsPerSample The audio stream sample size (8, 16 or 32 bits)
 * @param sampleRate The audio stream sample rate
 * @param numberOfChannels The audio stream channel count
 */
AudioSource::AudioSource(AudioSourceConfiguration configuration,
        int bitsPerSample,
        int sampleRate,
        size_t numberOfChannels) :
        m_configuration(move(configuration)),
        m_bitsPerSample(bitsPerSample),
        m_sampleRate(sampleRate),
        m_numberOfChannels(numberOfChannels),
        m_bytesPerFrame(::bytesPerFrame(bitsPerSample, numberOfChannels)),
        m_dataIndex(0),
        m_data(m_bytesPerFrame * sampleRate / 100, 0),
        m_dataNumberOfFrames(m_data.size() / m_bytesPerFrame)
{
}

/**
 * Add the specified sink to the source
 * @param sink The sink to add
 */
void AudioSource::AddSink(webrtc::AudioTrackSinkInterface* sink)
{
    lock_guard<recursive_mutex> lock(m_sinkMutex);
    m_sinks.insert(sink);
}

/**
 * Remove The specified sink from the source
 * @param sink The sink to remove
 */
void AudioSource::RemoveSink(webrtc::AudioTrackSinkInterface* sink)
{
    lock_guard<recursive_mutex> lock(m_sinkMutex);
    m_sinks.erase(sink);
}

/**
 * @brief Indicates if this source is remote
 * @return Always false, the source is local
 */
bool AudioSource::remote() const
{
    return false;
}

/**
 * @brief Indicates if this source is live
 * @return Always kLive, the source is live
 */
webrtc::MediaSourceInterface::SourceState AudioSource::state() const
{
    return kLive;
}

/**
 * @return The audio source options
 */
const cricket::AudioOptions AudioSource::options() const
{
    return static_cast<cricket::AudioOptions>(m_configuration);
}

/**
 * @return The sample size
 */
size_t AudioSource::bytesPerSample() const
{
    return m_bitsPerSample / 8;
}

/**
 * @return The frame size
 */
size_t AudioSource::bytesPerFrame() const
{
    return m_bytesPerFrame;
}

/**
 * Send an audio frame
 * @param audioData The audio data
 * @param numberOfFrames The number of frames
 */
void AudioSource::sendFrame(const void* audioData, size_t numberOfFrames)
{
    lock_guard<recursive_mutex> lock(m_sinkMutex);

    size_t dataSize = m_bytesPerFrame * numberOfFrames;
    if (m_dataIndex + dataSize >= m_data.size())
    {
        size_t dataToCopySize = m_data.size() - m_dataIndex;
        memcpy(m_data.data() + m_dataIndex, audioData, dataToCopySize);
        m_dataIndex = 0;

        for (auto sinks : m_sinks)
        {
            sinks->OnData(m_data.data(), m_bitsPerSample, m_sampleRate, m_numberOfChannels, m_dataNumberOfFrames);
        }

        size_t remainingNumberOfFrames = (dataSize - dataToCopySize) / m_bytesPerFrame;
        if (remainingNumberOfFrames > 0)
        {
            sendFrame(reinterpret_cast<const uint8_t*>(audioData) + dataToCopySize, remainingNumberOfFrames);
        }
    }
    else
    {
        memcpy(m_data.data() + m_dataIndex, audioData, m_bytesPerFrame * numberOfFrames);
        m_dataIndex += dataSize;
    }
}

void AudioSource::AddRef() const
{
}

rtc::RefCountReleaseStatus AudioSource::Release() const
{
    return rtc::RefCountReleaseStatus::kOtherRefsRemained;
}
