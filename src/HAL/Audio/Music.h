#pragma once

#ifndef DEDICATED_SERVER

#include "Base/Types/String/Path.h"

#include "GameData/Resources/Resource.h"

struct _Mix_Music;
typedef struct _Mix_Music Mix_Music;

namespace Audio
{
	class Music : public Resource
	{
	public:
		Music() = default;
		explicit Music(const ResourcePath& path);

		Music(const Music&) = delete;
		Music& operator=(const Music&) = delete;
		Music(Music&&) = delete;
		Music& operator=(Music&&) = delete;

		~Music() override;

		Mix_Music* getRawMusic() const;

		bool isValid() const override;

	private:
		Mix_Music* mMusic = nullptr;
	};
}

#endif // !DEDICATED_SERVER
