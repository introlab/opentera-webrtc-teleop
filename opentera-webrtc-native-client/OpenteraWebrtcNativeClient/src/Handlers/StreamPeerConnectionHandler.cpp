#include <OpenteraWebrtcNativeClient/Handlers/StreamPeerConnectionHandler.h>

#include <utility>

using namespace introlab;
using namespace rtc;
using namespace webrtc;
using namespace std;

StreamPeerConnectionHandler::StreamPeerConnectionHandler(
    string id,
    Client peerClient,
    bool isCaller,
    function<void(const string&, const sio::message::ptr&)> sendEvent,
    function<void(const string&)> onError,
    function<void(const Client&)> onClientConnected,
    function<void(const Client&)> onClientDisconnected,
    scoped_refptr<VideoTrackInterface>  videoTrack,
    shared_ptr<VideoSink> videoSink,
    scoped_refptr<AudioTrackInterface> audioTrack,
    shared_ptr<AudioTrackSinkInterface> audioSink) :
    PeerConnectionHandler(move(id), move(peerClient), isCaller, move(sendEvent), move(onError), move(onClientConnected), move(onClientDisconnected)),
    m_videoTrack(move(videoTrack)),
    m_videoSink(move(videoSink)),
    m_audioTrack(move(audioTrack)),
    m_audioSink(move(audioSink))
{

}

void StreamPeerConnectionHandler::setPeerConnection(
        const scoped_refptr<PeerConnectionInterface>& peerConnection)
{
    if (m_videoTrack != nullptr)
    {
        peerConnection->AddTrack(m_videoTrack, vector<string>());
    }
    if (m_audioTrack != nullptr)
    {
        peerConnection->AddTrack(m_audioTrack, vector<string>());
    }

    PeerConnectionHandler::setPeerConnection(peerConnection);
}

void StreamPeerConnectionHandler::OnAddStream(scoped_refptr<MediaStreamInterface> stream)
{
    VideoTrackVector videoTracks = stream->GetVideoTracks();
    if (!videoTracks.empty() && m_videoSink != nullptr)
    {
        videoTracks.front()->AddOrUpdateSink(m_videoSink.get(), m_videoSink->wants());
    }

    AudioTrackVector audioTracks = stream->GetAudioTracks();
    if (!audioTracks.empty() && m_audioSink != nullptr)
    {
        audioTracks.front()->AddSink(m_audioSink.get());
    }
}

void StreamPeerConnectionHandler::OnRemoveStream(scoped_refptr<MediaStreamInterface> stream)
{

}
