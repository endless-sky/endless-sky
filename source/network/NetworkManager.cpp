/* NetworkManager.cpp
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

#include "NetworkManager.h"

#include <iostream>

using namespace std;


// Initialize static members
int NetworkManager::initializeCount = 0;
bool NetworkManager::initialized = false;


NetworkManager::NetworkManager()
	: host(nullptr), totalPacketsSent(0), totalPacketsReceived(0),
	  totalBytesSent(0), totalBytesReceived(0)
{
}


NetworkManager::~NetworkManager()
{
	DestroyHost();
}


bool NetworkManager::Initialize()
{
	if(initializeCount == 0)
	{
		if(enet_initialize() != 0)
		{
			cerr << "[NetworkManager] Failed to initialize ENet" << endl;
			return false;
		}
		initialized = true;
		if(NetworkConstants::NETWORK_VERBOSE_LOGGING)
			cout << "[NetworkManager] ENet initialized successfully" << endl;
	}

	initializeCount++;
	return true;
}


void NetworkManager::Deinitialize()
{
	if(initializeCount > 0)
	{
		initializeCount--;
		if(initializeCount == 0 && initialized)
		{
			enet_deinitialize();
			initialized = false;
			if(NetworkConstants::NETWORK_VERBOSE_LOGGING)
				cout << "[NetworkManager] ENet deinitialized" << endl;
		}
	}
}


bool NetworkManager::IsInitialized()
{
	return initialized;
}


bool NetworkManager::CreateHost(const ENetAddress *address, size_t peerCount, size_t channelCount)
{
	// Don't create if already exists
	if(host)
		return false;

	host = enet_host_create(
		address,
		peerCount,
		channelCount,
		NetworkConstants::INCOMING_BANDWIDTH,
		NetworkConstants::OUTGOING_BANDWIDTH
	);

	if(!host)
	{
		cerr << "[NetworkManager] Failed to create ENet host" << endl;
		return false;
	}

	if(NetworkConstants::NETWORK_VERBOSE_LOGGING)
		cout << "[NetworkManager] ENet host created successfully" << endl;

	return true;
}


void NetworkManager::DestroyHost()
{
	if(host)
	{
		enet_host_destroy(host);
		host = nullptr;

		if(NetworkConstants::NETWORK_VERBOSE_LOGGING)
			cout << "[NetworkManager] ENet host destroyed" << endl;
	}
}


void NetworkManager::UpdateStatistics()
{
	if(!host)
		return;

	totalPacketsSent = host->totalSentPackets;
	totalPacketsReceived = host->totalReceivedPackets;
	totalBytesSent = host->totalSentData;
	totalBytesReceived = host->totalReceivedData;
}
