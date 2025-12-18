/* NetworkConnection.cpp
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

#include "NetworkConnection.h"

#include <sstream>

using namespace std;


// Initialize static member
uint32_t NetworkConnection::nextConnectionId = 1;


NetworkConnection::NetworkConnection(ENetPeer *peer)
	: peer(peer), state(NetworkConstants::ConnectionState::CONNECTED),
	  connectTime(chrono::steady_clock::now()), connectionId(nextConnectionId++)
{
}


void NetworkConnection::SetState(NetworkConstants::ConnectionState newState)
{
	state = newState;
}


string NetworkConnection::GetAddress() const
{
	if(!peer)
		return "0.0.0.0";

	ostringstream oss;
	oss << (peer->address.host & 0xFF) << "."
		<< ((peer->address.host >> 8) & 0xFF) << "."
		<< ((peer->address.host >> 16) & 0xFF) << "."
		<< ((peer->address.host >> 24) & 0xFF);
	return oss.str();
}


uint16_t NetworkConnection::GetPort() const
{
	return peer ? peer->address.port : 0;
}


uint32_t NetworkConnection::GetRoundTripTime() const
{
	return peer ? peer->roundTripTime : 0;
}


uint32_t NetworkConnection::GetPacketsSent() const
{
	return peer ? peer->packetsSent : 0;
}


uint32_t NetworkConnection::GetPacketsLost() const
{
	return peer ? peer->packetsLost : 0;
}


float NetworkConnection::GetPacketLossPercent() const
{
	if(!peer || peer->packetsSent == 0)
		return 0.0f;

	return (static_cast<float>(peer->packetsLost) / static_cast<float>(peer->packetsSent)) * 100.0f;
}


uint64_t NetworkConnection::GetConnectionDurationMs() const
{
	auto now = chrono::steady_clock::now();
	auto duration = chrono::duration_cast<chrono::milliseconds>(now - connectTime);
	return duration.count();
}


bool NetworkConnection::SendPacket(const void *data, size_t size, NetworkConstants::Channel channel, bool reliable)
{
	if(!peer || !IsConnected())
		return false;

	// Validate packet size
	if(size > NetworkConstants::MAX_PACKET_SIZE)
		return false;

	// Create packet with appropriate flags
	uint32_t flags = reliable ? ENET_PACKET_FLAG_RELIABLE : 0;
	ENetPacket *packet = enet_packet_create(data, size, flags);
	if(!packet)
		return false;

	// Send packet on specified channel
	int result = enet_peer_send(peer, static_cast<uint8_t>(channel), packet);
	return result == 0;
}


void NetworkConnection::Disconnect(uint32_t data)
{
	if(peer && IsConnected())
	{
		enet_peer_disconnect(peer, data);
		state = NetworkConstants::ConnectionState::DISCONNECTING;
	}
}


void NetworkConnection::DisconnectNow(uint32_t data)
{
	if(peer)
	{
		enet_peer_disconnect_now(peer, data);
		state = NetworkConstants::ConnectionState::DISCONNECTED;
		peer = nullptr;
	}
}


bool NetworkConnection::IsConnected() const
{
	return state == NetworkConstants::ConnectionState::CONNECTED;
}


bool NetworkConnection::IsDisconnecting() const
{
	return state == NetworkConstants::ConnectionState::DISCONNECTING;
}
