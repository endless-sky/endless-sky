/* PacketHandler.cpp
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

#include "PacketHandler.h"

#include "PacketReader.h"

using namespace std;


void PacketHandler::RegisterHandler(NetworkPacket::PacketType type, HandlerFunc handler)
{
	handlers[type] = handler;
}


void PacketHandler::UnregisterHandler(NetworkPacket::PacketType type)
{
	handlers.erase(type);
}


bool PacketHandler::HasHandler(NetworkPacket::PacketType type) const
{
	return handlers.find(type) != handlers.end();
}


bool PacketHandler::Dispatch(const uint8_t *data, size_t size, NetworkConnection *connection)
{
	// Create packet reader and validate
	PacketReader reader(data, size);
	if(!reader.IsValid())
		return false;

	return Dispatch(reader, connection);
}


bool PacketHandler::Dispatch(PacketReader &reader, NetworkConnection *connection)
{
	// Get packet type
	NetworkPacket::PacketType type = reader.GetPacketType();

	// Find handler
	auto it = handlers.find(type);
	if(it == handlers.end())
		return false;

	// Execute handler
	it->second(reader, connection);
	return true;
}


size_t PacketHandler::GetHandlerCount() const
{
	return handlers.size();
}


void PacketHandler::Clear()
{
	handlers.clear();
}


bool PacketHandler::IsProtocolCompatible(uint16_t clientVersion, uint16_t serverVersion)
{
	// For now, require exact match
	// In future versions, we can implement backwards compatibility
	// e.g., if (serverVersion >= clientVersion && serverVersion - clientVersion <= 2)
	return clientVersion == serverVersion;
}


uint16_t PacketHandler::GetCurrentProtocolVersion()
{
	return NetworkPacket::PROTOCOL_VERSION;
}
