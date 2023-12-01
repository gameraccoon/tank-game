#pragma once

namespace GameplayInput
{
	enum class InputAxis
	{
		MoveHorizontal = 0,
		MoveVertical,

		// add new elements above this line
		Count
	};

	enum class InputKey
	{
		MoveUp = 0,
		MoveDown,
		MoveLeft,
		MoveRight,
		Shoot,

		// add new elements above this line
		Count
	};
}
