/* NetworkServer.cpp
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

#include "NetworkServer.h"

#include <iostream>
#include <algorithm>

using namespace std;


NetworkServer::NetworkServer()
	: NetworkManager(), port(0)
{
}


NetworkServer::~NetworkServer()
{
	Stop();
}


bool NetworkServer::Start(uint16_t serverPort)
{
	if(IsRunning())
	{
		cerr << "[NetworkServer] Server is already running" << endl;
		return false;
	}

	// Set up server address
	ENetAddress address;
	address.host = ENET_HOST_ANY;  // Bind to all interfaces
	address.port = serverPort;

	// Create the server host
	if(!CreateHost(&address, NetworkConstants::MAX_CLIENTS, NetworkConstants::CHANNEL_COUNT))
	{
		cerr << "[NetworkServer] Failed to create server on port " << serverPort << endl;
		return false;
	}

	port = serverPort;
	cout << "[NetworkServer] Server started on port " << port << endl;
	return true;
}


void NetworkServer::Stop()
{
	if(!IsRunning())
		return;

	cout << "[NetworkServer] Stopping server..." << endl;

	// Disconnect all clients
	for(auto &connection : connections)
	{
		if(connection)
			connection->DisconnectNow();
	}
	connections.clear();

	// Destroy host
	DestroyHost();

	cout << "[NetworkServer] Server stopped" << endl;
}


void NetworkServer::Update()
{
	if(!IsRunning())
		return;

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


bool NetworkServer::SendToClient(NetworkConnection &connection, const void *data, size_t size,
	NetworkConstants::Channel channel, bool reliable)
{
	if(!IsRunning() || !connection.IsConnected())
		return false;

	return connection.SendPacket(data, size, channel, reliable);
}


void NetworkServer::BroadcastToAll(const void *data, size_t size,
	NetworkConstants::Channel channel, bool reliable)
{
	if(!IsRunning())
		return;

	for(auto &connection : connections)
	{
		if(connection && connection->IsConnected())
			connection->SendPacket(data, size, channel, reliable);
	}

	// Flush all outgoing packets
	if(host)
		enet_host_flush(host);
}


void NetworkServer::BroadcastToAllExcept(NetworkConnection &except, const void *data, size_t size,
	NetworkConstants::Channel channel, bool reliable)
{
	if(!IsRunning())
		return;

	for(auto &connection : connections)
	{
		if(connection && connection->IsConnected() && connection->GetConnectionId() != except.GetConnectionId())
			connection->SendPacket(data, size, channel, reliable);
	}

	// Flush all outgoing packets
	if(host)
		enet_host_flush(host);
}


void NetworkServer::DisconnectClient(NetworkConnection &connection, uint32_t data)
{
	if(IsRunning() && connection.IsConnected())
		connection.Disconnect(data);
}


void NetworkServer::HandleConnectEvent(ENetEvent &event)
{
	// Create new connection
	auto connection = make_unique<NetworkConnection>(event.peer);

	if(NetworkConstants::NETWORK_VERBOSE_LOGGING)
	{
		cout << "[NetworkServer] Client connected: " << connection->GetAddress()
		     << ":" << connection->GetPort()
		     << " (ID: " << connection->GetConnectionId() << ")" << endl;
	}

	// Store connection pointer in peer data for later lookup
	event.peer->data = connection.get();

	// Notify callback
	if(onClientConnected)
		onClientConnected(*connection);

	// Add to connections list
	connections.push_back(move(connection));
}


void NetworkServer::HandleDisconnectEvent(ENetEvent &event)
{
	// Find and remove connection
	NetworkConnection *connection = FindConnection(event.peer);
	if(!connection)
		return;

	if(NetworkConstants::NETWORK_VERBOSE_LOGGING)
	{
		cout << "[NetworkServer] Client disconnected: " << connection->GetAddress()
		     << ":" << connection->GetPort()
		     << " (ID: " << connection->GetConnectionId() << ")" << endl;
	}

	// Notify callback
	if(onClientDisconnected)
		onClientDisconnected(*connection);

	// Remove from connections list
	connections.erase(
		remove_if(connections.begin(), connections.end(),
			[connection](const unique_ptr<NetworkConnection> &c) {
				return c.get() == connection;
			}),
		connections.end()
	);

	// Clear peer data
	event.peer->data = nullptr;
}


void NetworkServer::HandleReceiveEvent(ENetEvent &event)
{
	// Find connection
	NetworkConnection *connection = FindConnection(event.peer);
	if(!connection)
	{
		enet_packet_destroy(event.packet);
		return;
	}

	// Notify callback with packet data
	if(onPacketReceived)
		onPacketReceived(*connection, event.packet->data, event.packet->dataLength);

	// Clean up packet
	enet_packet_destroy(event.packet);
}


NetworkConnection *NetworkServer::FindConnection(ENetPeer *peer)
{
	if(!peer || !peer->data)
		return nullptr;

	return static_cast<NetworkConnection *>(peer->data);
}
