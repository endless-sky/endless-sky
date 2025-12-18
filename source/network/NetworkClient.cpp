/* NetworkClient.cpp
 * Copyright (c) 2025 by Endless Sky Development Team
 *
 * Endless Sky is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later version.
 *
 * Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include "NetworkClient.h"

#include <iostream>

using namespace std;


NetworkClient::NetworkClient()
	: NetworkManager(), serverPeer(nullptr),
	  state(NetworkConstants::ConnectionState::DISCONNECTED),
	  serverPort(0)
{
}


NetworkClient::~NetworkClient()
{
	Disconnect();
}


bool NetworkClient::Connect(const string &hostname, uint16_t port)
{
	if(state != NetworkConstants::ConnectionState::DISCONNECTED)
	{
		cerr << "[NetworkClient] Already connected or connecting" << endl;
		return false;
	}

	// Create client host (no address = don't bind, just connect)
	if(!CreateHost(nullptr, 1, NetworkConstants::CHANNEL_COUNT))
	{
		cerr << "[NetworkClient] Failed to create client" << endl;
		return false;
	}

	// Set up server address
	ENetAddress address;
	enet_address_set_host(&address, hostname.c_str());
	address.port = port;

	// Start connection
	serverPeer = enet_host_connect(host, &address, NetworkConstants::CHANNEL_COUNT, 0);
	if(!serverPeer)
	{
		cerr << "[NetworkClient] Failed to initiate connection" << endl;
		DestroyHost();
		return false;
	}

	state = NetworkConstants::ConnectionState::CONNECTING;
	serverAddress = hostname;
	serverPort = port;
	connectionStartTime = chrono::steady_clock::now();

	if(NetworkConstants::NETWORK_VERBOSE_LOGGING)
		cout << "[NetworkClient] Connecting to " << hostname << ":" << port << "..." << endl;

	return true;
}


void NetworkClient::Disconnect()
{
	if(state == NetworkConstants::ConnectionState::DISCONNECTED)
		return;

	if(NetworkConstants::NETWORK_VERBOSE_LOGGING)
		cout << "[NetworkClient] Disconnecting..." << endl;

	if(serverPeer)
	{
		enet_peer_disconnect(serverPeer, 0);

		// Wait a bit for disconnection acknowledgment
		ENetEvent event;
		while(enet_host_service(host, &event, NetworkConstants::DISCONNECTION_TIMEOUT_MS) > 0)
		{
			switch(event.type)
			{
				case ENET_EVENT_TYPE_RECEIVE:
					enet_packet_destroy(event.packet);
					break;

				case ENET_EVENT_TYPE_DISCONNECT:
					serverPeer = nullptr;
					break;

				default:
					break;
			}
		}

		// Force disconnect if still connected
		if(serverPeer)
		{
			enet_peer_reset(serverPeer);
			serverPeer = nullptr;
		}
	}

	DestroyHost();
	state = NetworkConstants::ConnectionState::DISCONNECTED;

	if(NetworkConstants::NETWORK_VERBOSE_LOGGING)
		cout << "[NetworkClient] Disconnected" << endl;
}


void NetworkClient::Update()
{
	if(!IsActive())
		return;

	// Check for connection timeout
	UpdateConnectionState();

	ENetEvent event;

	// Process all pending events
	while(enet_host_service(host, &event, 0) > 0)
	{
		switch(event.type)
		{
			case ENET_EVENT_TYPE_CONNECT:
				HandleConnectEvent(event);
				break;

			case ENET_EVENT_TYPE_RECEIVE:
				HandleReceiveEvent(event);
				break;

			case ENET_EVENT_TYPE_DISCONNECT:
				HandleDisconnectEvent(event);
				break;

			case ENET_EVENT_TYPE_NONE:
				break;
		}
	}

	// Update statistics
	UpdateStatistics();
}


bool NetworkClient::IsConnected() const
{
	return state == NetworkConstants::ConnectionState::CONNECTED;
}


bool NetworkClient::IsConnecting() const
{
	return state == NetworkConstants::ConnectionState::CONNECTING;
}


bool NetworkClient::SendToServer(const void *data, size_t size,
	NetworkConstants::Channel channel, bool reliable)
{
	if(!IsConnected() || !serverPeer)
		return false;

	// Validate packet size
	if(size > NetworkConstants::MAX_PACKET_SIZE)
		return false;

	// Create packet
	uint32_t flags = reliable ? ENET_PACKET_FLAG_RELIABLE : 0;
	ENetPacket *packet = enet_packet_create(data, size, flags);
	if(!packet)
		return false;

	// Send to server
	int result = enet_peer_send(serverPeer, static_cast<uint8_t>(channel), packet);
	return result == 0;
}


uint32_t NetworkClient::GetRoundTripTime() const
{
	return serverPeer ? serverPeer->roundTripTime : 0;
}


float NetworkClient::GetPacketLossPercent() const
{
	if(!serverPeer || serverPeer->packetsSent == 0)
		return 0.0f;

	return (static_cast<float>(serverPeer->packetsLost) / static_cast<float>(serverPeer->packetsSent)) * 100.0f;
}


void NetworkClient::HandleConnectEvent(ENetEvent &event)
{
	state = NetworkConstants::ConnectionState::CONNECTED;

	if(NetworkConstants::NETWORK_VERBOSE_LOGGING)
	{
		auto duration = chrono::duration_cast<chrono::milliseconds>(
			chrono::steady_clock::now() - connectionStartTime
		);
		cout << "[NetworkClient] Connected to " << serverAddress << ":" << serverPort
		     << " (took " << duration.count() << "ms)" << endl;
	}

	// Notify callback
	if(onConnected)
		onConnected();
}


void NetworkClient::HandleDisconnectEvent(ENetEvent &event)
{
	if(NetworkConstants::NETWORK_VERBOSE_LOGGING)
		cout << "[NetworkClient] Disconnected from server" << endl;

	serverPeer = nullptr;
	state = NetworkConstants::ConnectionState::DISCONNECTED;

	// Notify callback
	if(onDisconnected)
		onDisconnected();
}


void NetworkClient::HandleReceiveEvent(ENetEvent &event)
{
	// Notify callback with packet data
	if(onPacketReceived)
		onPacketReceived(event.packet->data, event.packet->dataLength);

	// Clean up packet
	enet_packet_destroy(event.packet);
}


void NetworkClient::UpdateConnectionState()
{
	// Check for connection timeout
	if(state == NetworkConstants::ConnectionState::CONNECTING)
	{
		auto duration = chrono::duration_cast<chrono::milliseconds>(
			chrono::steady_clock::now() - connectionStartTime
		);

		if(duration.count() > NetworkConstants::CONNECTION_TIMEOUT_MS)
		{
			cerr << "[NetworkClient] Connection timed out" << endl;
			state = NetworkConstants::ConnectionState::TIMED_OUT;

			if(serverPeer)
			{
				enet_peer_reset(serverPeer);
				serverPeer = nullptr;
			}

			DestroyHost();

			// Notify callback
			if(onConnectionFailed)
				onConnectionFailed();
		}
	}
}
