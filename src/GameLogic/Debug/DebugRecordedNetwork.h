#pragma once

#include <cstdio>

#include "EngineData/Network/ConnectionId.h"

#include "HAL/Network/NetworkStructs.h"

class ArgumentsParser;

class DebugRecordedNetwork
{
public:
	explicit DebugRecordedNetwork(int instanceIndex);
	~DebugRecordedNetwork();

	void processFrame(std::vector<std::pair<ConnectionId, HAL::Network::Message>>& messagesRef);
	void processArguments(const ArgumentsParser& arguments);

private:
	enum class RecordingMode
	{
		None,
		Record,
		Replay
	};

	using OneFrameNetworkData = std::vector<std::pair<ConnectionId, HAL::Network::Message>>;

private:
	void appendNetworkDataToFile(const OneFrameNetworkData& networkData) const;
	static std::vector<OneFrameNetworkData> loadNetworkDataFromFile(const std::string& path);

private:
	RecordingMode mRecordingMode = RecordingMode::None;
	std::vector<OneFrameNetworkData> mRecordedNetworkData;
	size_t mRecordedNetworkDataIdx = 0;

	// right now STL doesn't support reading std::byte from file, so we resort to C-style file IO
	std::FILE* mRecordedNetworkDataFile = nullptr;

	int mInstanceIndex;
};
