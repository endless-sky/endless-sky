/* NetworkManager.h
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

#pragma once

#include "NetworkConstants.h"

#include <enet/enet.h>
#include <string>
#include <functional>


// Base class for network management, providing common functionality for both
// client and server networking. Handles ENet initialization and cleanup.
class NetworkManager {
public:
	NetworkManager();
	virtual ~NetworkManager();

	// Prevent copying
	NetworkManager(const NetworkManager &) = delete;
	NetworkManager &operator=(const NetworkManager &) = delete;

	// Initialize the network system (must be called before use)
	static bool Initialize();
	static void Deinitialize();
	static bool IsInitialized();

	// Check if this manager is active
	bool IsActive() const { return host != nullptr; }

	// Get network statistics
	uint32_t GetTotalPacketsSent() const { return totalPacketsSent; }
	uint32_t GetTotalPacketsReceived() const { return totalPacketsReceived; }
	uint64_t GetTotalBytesSent() const { return totalBytesSent; }
	uint64_t GetTotalBytesReceived() const { return totalBytesReceived; }

	// Process network events (call once per frame)
	virtual void Update() = 0;

	// Shutdown the network connection
	virtual void Shutdown() = 0;


protected:
	// Create the ENet host (called by derived classes)
	bool CreateHost(const ENetAddress *address, size_t peerCount, size_t channelCount);

	// Destroy the ENet host
	void DestroyHost();

	// Update statistics
	void UpdateStatistics();

	// ENet host (server or client)
	ENetHost *host;

	// Network statistics
	uint32_t totalPacketsSent;
	uint32_t totalPacketsReceived;
	uint64_t totalBytesSent;
	uint64_t totalBytesReceived;


private:
	// Track ENet initialization
	static int initializeCount;
	static bool initialized;
};
