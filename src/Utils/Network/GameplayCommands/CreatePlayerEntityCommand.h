#pragma once

#include "Base/Types/BasicTypes.h"

#include "GameData/Geometry/Vector2D.h"
#include "GameData/Network/GameplayCommand.h"

namespace Network
{
	class CreatePlayerEntityCommand : public GameplayCommand
	{
	public:
		CreatePlayerEntityCommand(Vector2D pos, bool isOwner, u64 serverEntityId);

		void execute(World& world) const override;
		Ptr clone() const override;

	private:
		const Vector2D mPos;
		const bool mIsOwner;
		const u64 mServerEntityId;
	};
} // namespace Network
