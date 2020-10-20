#include <OpenteraWebrtcNativeClient/DataChannelClient.h>

#include <OpenteraWebrtcNativeClientTests/CallbackAwaiter.h>

#include <gtest/gtest.h>

#include <subprocess.hpp>

#include <filesystem>
#include <memory>
#include <thread>

using namespace introlab;
using namespace std;
namespace fs = std::filesystem;


const WebrtcConfiguration DefaultWebrtcConfiguration = WebrtcConfiguration::create(
{
    IceServer("stun:stun.l.google.com:19302")
});

class DataChannelClientTests : public ::testing::Test
{
    static unique_ptr<subprocess::Popen> m_signallingServerProcess;

protected:
    static void SetUpTestSuite()
    {
        fs::path testFilePath(__FILE__);
        fs::path pythonFilePath = testFilePath.parent_path().parent_path().parent_path().parent_path().parent_path()
                / "signalling-server" / "signalling_server.py";
        m_signallingServerProcess = make_unique<subprocess::Popen>("python3 " + pythonFilePath.string() +
                " --port 8080 --password abc", subprocess::input(subprocess::PIPE));
    }

    static void TearDownTestSuite()
    {
        if (m_signallingServerProcess)
        {
            m_signallingServerProcess->kill(9);
            m_signallingServerProcess->wait();
        }
    }
};
unique_ptr<subprocess::Popen> DataChannelClientTests::m_signallingServerProcess = nullptr;

class DisconnectedDataChannelClientTests : public ::testing::Test
{
protected:
    unique_ptr<DataChannelClient> m_client1;

    void SetUp() override
    {
        m_client1 = make_unique<DataChannelClient>(SignallingServerConfiguration::create("http://localhost:8080", "c1",
                sio::string_message::create("cd1"),"chat", ""),
                DefaultWebrtcConfiguration, DataChannelConfiguration::create());

        m_client1->setOnError([](const string& error) { ADD_FAILURE() << error; });
    }

    void TearDown() override
    {
        m_client1->closeSync();
    }
};

class WrongPasswordDataChannelClientTests : public DataChannelClientTests
{
protected:
    unique_ptr<DataChannelClient> m_client1;

    void SetUp() override
    {
        m_client1 = make_unique<DataChannelClient>(SignallingServerConfiguration::create("http://localhost:8080", "c1",
                sio::string_message::create("cd1"), "chat", ""),
                DefaultWebrtcConfiguration, DataChannelConfiguration::create());

        m_client1->setOnError([](const string& error) { ADD_FAILURE() << error; });
    }

    void TearDown() override
    {
        m_client1->closeSync();
    }
};

class SingleDataChannelClientTests : public DataChannelClientTests
{
protected:
    unique_ptr<DataChannelClient> m_client1;

    void SetUp() override
    {
        m_client1 = make_unique<DataChannelClient>(SignallingServerConfiguration::create("http://localhost:8080", "c1",
                sio::string_message::create("cd1"), "chat", "abc"),
                DefaultWebrtcConfiguration,DataChannelConfiguration::create());

        m_client1->setOnError([](const string& error) { ADD_FAILURE() << error; });
    }

    void TearDown() override
    {
        m_client1->closeSync();
    }
};

class RightPasswordDataChannelClientTests : public DataChannelClientTests
{
protected:
    unique_ptr<DataChannelClient> m_client1;
    unique_ptr<DataChannelClient> m_client2;
    unique_ptr<DataChannelClient> m_client3;

    void SetUp() override
    {
        CallbackAwaiter setupAwaiter(3, 15s);
        m_client1 = make_unique<DataChannelClient>(SignallingServerConfiguration::create("http://localhost:8080", "c1",
                sio::string_message::create("cd1"), "chat", "abc"),
                DefaultWebrtcConfiguration, DataChannelConfiguration::create());
        m_client2 = make_unique<DataChannelClient>(SignallingServerConfiguration::create("http://localhost:8080", "c2",
                sio::string_message::create("cd2"), "chat", "abc"),
                DefaultWebrtcConfiguration, DataChannelConfiguration::create());
        m_client3 = make_unique<DataChannelClient>(SignallingServerConfiguration::create("http://localhost:8080", "c3",
                sio::string_message::create("cd3"), "chat", "abc"),
                DefaultWebrtcConfiguration, DataChannelConfiguration::create());

        m_client1->setOnSignallingConnectionOpen([&] { setupAwaiter.done(); });
        m_client2->setOnSignallingConnectionOpen([&] { setupAwaiter.done(); });
        m_client3->setOnSignallingConnectionOpen([&] { setupAwaiter.done(); });

        m_client1->setOnError([](const string& error) { ADD_FAILURE() << error; });
        m_client2->setOnError([](const string& error) { ADD_FAILURE() << error; });
        m_client3->setOnError([](const string& error) { ADD_FAILURE() << error; });

        m_client1->connect();
        this_thread::sleep_for(250ms);
        m_client2->connect();
        this_thread::sleep_for(250ms);
        m_client3->connect();
        setupAwaiter.wait();

        m_client1->setOnSignallingConnectionOpen([] {});
        m_client2->setOnSignallingConnectionOpen([] {});
        m_client3->setOnSignallingConnectionOpen([] {});
    }

    void TearDown() override
    {
        m_client1->closeSync();
        m_client2->closeSync();
        m_client3->closeSync();
    }
};


TEST_F(DisconnectedDataChannelClientTests, isConnected_shouldReturnFalse)
{
    EXPECT_FALSE(m_client1->isConnected());
}

TEST_F(DisconnectedDataChannelClientTests, isRtcConnected_shouldReturnFalse)
{
    EXPECT_FALSE(m_client1->isRtcConnected());
}

TEST_F(DisconnectedDataChannelClientTests, id_shouldReturnAnEmptyString)
{
    EXPECT_EQ(m_client1->id(), "");
}

TEST_F(DisconnectedDataChannelClientTests, getConnectedRoomClientIds_shouldReturnAnEmptyVector)
{
    EXPECT_EQ(m_client1->getConnectedRoomClientIds().size(), 0);
}

TEST_F(DisconnectedDataChannelClientTests, getRoomClients_shouldReturnAnEmptyVector)
{
    EXPECT_EQ(m_client1->getRoomClients().size(), 0);
}


TEST_F(WrongPasswordDataChannelClientTests, connect_shouldGenerateAnError)
{
    CallbackAwaiter awaiter(2, 15s);
    m_client1->setOnSignallingConnectionOpen([&]
    {
        ADD_FAILURE();
        awaiter.done();
        awaiter.done();
    });
    m_client1->setOnSignallingConnectionError([&](const string& error)
    {
        EXPECT_EQ(m_client1->isConnected(), false);
        EXPECT_EQ(m_client1->isRtcConnected(), false);
        EXPECT_EQ(m_client1->id(), "");
        EXPECT_EQ(m_client1->getConnectedRoomClientIds().size(), 0);
        EXPECT_EQ(m_client1->getRoomClients().size(), 0);
        awaiter.done();
    });
    m_client1->setOnSignallingConnectionClosed([&]
    {
        awaiter.done();
    });

    m_client1->connect();
    awaiter.wait();

    m_client1->setOnSignallingConnectionOpen([] {});
    m_client1->setOnSignallingConnectionError([](const string& error) {});
    m_client1->setOnSignallingConnectionClosed([] {});
}


TEST_F(SingleDataChannelClientTests, onRoomClientsChanged_mustBeCallAfterTheConnection)
{
    CallbackAwaiter awaiter(2, 15s);
    m_client1->setOnSignallingConnectionOpen([&] { awaiter.done(); });
    m_client1->setOnRoomClientsChanged([&](const vector<RoomClient>& roomClients)
    {
        EXPECT_NE(roomClients.size(), 1);
        EXPECT_EQ(count(roomClients.begin(), roomClients.end(),
                RoomClient(m_client1->id(), "c1", sio::string_message::create("cd1"), true)), 1);
        awaiter.done();
    });

    m_client1->connect();
    awaiter.wait();

    m_client1->setOnSignallingConnectionOpen([] {});
}


TEST_F(RightPasswordDataChannelClientTests, isConnected_shouldReturnTrue)
{
    EXPECT_TRUE(m_client1->isConnected());
    EXPECT_TRUE(m_client2->isConnected());
    EXPECT_TRUE(m_client3->isConnected());
}

TEST_F(RightPasswordDataChannelClientTests, isRtcConnected_shouldReturnFalse)
{
    EXPECT_FALSE(m_client1->isRtcConnected());
    EXPECT_FALSE(m_client2->isRtcConnected());
    EXPECT_FALSE(m_client3->isRtcConnected());
}

TEST_F(RightPasswordDataChannelClientTests, id_shouldNotReturnAnEmptyString)
{
    EXPECT_NE(m_client1->id(), "");
    EXPECT_NE(m_client2->id(), "");
    EXPECT_NE(m_client3->id(), "");
}

TEST_F(RightPasswordDataChannelClientTests, getConnectedRoomClientIds_shouldNotReturnAnEmptyVector)
{
    EXPECT_EQ(m_client1->getConnectedRoomClientIds().size(), 0);
    EXPECT_EQ(m_client2->getConnectedRoomClientIds().size(), 0);
    EXPECT_EQ(m_client3->getConnectedRoomClientIds().size(), 0);
}

TEST_F(RightPasswordDataChannelClientTests, getRoomClient_shouldReturnTheSpecifiedClientOrDefault)
{
    EXPECT_EQ(m_client1->getRoomClient(m_client1->id()),
            RoomClient(m_client1->id(), "c1", sio::string_message::create("cd1"), true));
    EXPECT_EQ(m_client1->getRoomClient(m_client2->id()),
            RoomClient(m_client2->id(), "c2", sio::string_message::create("cd2"), false));
    EXPECT_EQ(m_client1->getRoomClient(m_client3->id()),
            RoomClient(m_client3->id(), "c3", sio::string_message::create("cd3"), false));

    EXPECT_EQ(m_client1->getRoomClient(""), RoomClient());
}

TEST_F(RightPasswordDataChannelClientTests, getRoomClients_shouldReturnAllClients)
{
    auto roomClients1 = m_client1->getRoomClients();
    ASSERT_EQ(roomClients1.size(), 3);
    EXPECT_EQ(count(roomClients1.begin(), roomClients1.end(),
        RoomClient(m_client1->id(), "c1", sio::string_message::create("cd1"), true)), 1);
    EXPECT_EQ(count(roomClients1.begin(), roomClients1.end(),
        RoomClient(m_client2->id(), "c2", sio::string_message::create("cd2"), false)), 1);
    EXPECT_EQ(count(roomClients1.begin(), roomClients1.end(),
        RoomClient(m_client3->id(), "c3", sio::string_message::create("cd3"), false)), 1);

    auto roomClients2 = m_client2->getRoomClients();
    ASSERT_EQ(roomClients2.size(), 3);
    EXPECT_EQ(count(roomClients2.begin(), roomClients2.end(),
                    RoomClient(m_client1->id(), "c1", sio::string_message::create("cd1"), false)), 1);
    EXPECT_EQ(count(roomClients2.begin(), roomClients2.end(),
                    RoomClient(m_client2->id(), "c2", sio::string_message::create("cd2"), true)), 1);
    EXPECT_EQ(count(roomClients2.begin(), roomClients2.end(),
                    RoomClient(m_client3->id(), "c3", sio::string_message::create("cd3"), false)), 1);

    auto roomClients3 = m_client3->getRoomClients();
    ASSERT_EQ(roomClients3.size(), 3);
    EXPECT_EQ(count(roomClients3.begin(), roomClients3.end(),
                    RoomClient(m_client1->id(), "c1", sio::string_message::create("cd1"), false)), 1);
    EXPECT_EQ(count(roomClients3.begin(), roomClients3.end(),
                    RoomClient(m_client2->id(), "c2", sio::string_message::create("cd2"), false)), 1);
    EXPECT_EQ(count(roomClients3.begin(), roomClients3.end(),
                    RoomClient(m_client3->id(), "c3", sio::string_message::create("cd3"), true)), 1);
}

TEST_F(RightPasswordDataChannelClientTests, callAll_shouldCallAllClients)
{
    CallbackAwaiter awaiter1(2, 60s);
    CallbackAwaiter awaiter2(2, 60s);
    CallbackAwaiter awaiter3(2, 60s);

    m_client1->setOnDataChannelOpen([this, &awaiter1](const Client& client)
    {
        if (awaiter1.done())
        {
            EXPECT_TRUE(m_client1->isRtcConnected());
            auto connectedRoomClientIds = m_client1->getConnectedRoomClientIds();
            ASSERT_EQ(connectedRoomClientIds.size(), 2);
            EXPECT_EQ(count(connectedRoomClientIds.begin(), connectedRoomClientIds.end(), m_client2->id()), 1);
            EXPECT_EQ(count(connectedRoomClientIds.begin(), connectedRoomClientIds.end(), m_client3->id()), 1);
        }
    });
    m_client2->setOnDataChannelOpen([this, &awaiter2](const Client& client)
    {
        if (awaiter2.done())
        {
            EXPECT_TRUE(m_client2->isRtcConnected());
            auto connectedRoomClientIds = m_client2->getConnectedRoomClientIds();
            ASSERT_EQ(connectedRoomClientIds.size(), 2);
            EXPECT_EQ(count(connectedRoomClientIds.begin(), connectedRoomClientIds.end(), m_client1->id()), 1);
            EXPECT_EQ(count(connectedRoomClientIds.begin(), connectedRoomClientIds.end(), m_client3->id()), 1);
        }
    });
    m_client3->setOnDataChannelOpen([this, &awaiter3](const Client& client)
    {
        if (awaiter3.done())
        {
            EXPECT_TRUE(m_client3->isRtcConnected());
            auto connectedRoomClientIds = m_client3->getConnectedRoomClientIds();
            ASSERT_EQ(connectedRoomClientIds.size(), 2);
            EXPECT_EQ(count(connectedRoomClientIds.begin(), connectedRoomClientIds.end(), m_client1->id()), 1);
            EXPECT_EQ(count(connectedRoomClientIds.begin(), connectedRoomClientIds.end(), m_client2->id()), 1);
        }
    });

    this_thread::sleep_for(10s);

    m_client1->callAll();
    awaiter1.wait();
    awaiter2.wait();
    awaiter3.wait();

    m_client1->setOnDataChannelOpen([](const Client& client) {});
    m_client2->setOnDataChannelOpen([](const Client& client) {});
    m_client3->setOnDataChannelOpen([](const Client& client) {});
}