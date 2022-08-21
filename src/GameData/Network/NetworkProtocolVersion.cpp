#include "Base/precomp.h"

#include "GameData/Network/NetworkProtocolVersion.h"

namespace Network
{
	// 0 - reserved invalid version
	// 1 - initial version
	// 2 - player entity created packet
	u32 NetworkProtocolVersion = 2;
	// increase by one, and add a comment for each change in how we communicate over network
	// this includes:
	// any changes to network protocols and libraries
	// any addeed, removed, or changed messages
	// changes in serialization/deserialization logic
	// changing order of serialization/deserialization of message data
	// changing explicit order of sending reliable messages
	// reusing existing messages for new purposes (try to avoid that, by the way)
	// any bugfixes to the above
} // namespace Network
