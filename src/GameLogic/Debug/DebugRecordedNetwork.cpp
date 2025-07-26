#include "EngineCommon/precomp.h"

#include "GameLogic/Debug/DebugRecordedNetwork.h"

#include <format>

#include "EngineCommon/Types/Serialization.h"

#include "EngineUtils/Application/ArgumentsParser.h"

DebugRecordedNetwork::DebugRecordedNetwork(const int instanceIndex)
	: mInstanceIndex(instanceIndex)
{
}

DebugRecordedNetwork::~DebugRecordedNetwork()
{
	if (mRecordedNetworkDataFile)
	{
		fclose(mRecordedNetworkDataFile);
	}
}

void DebugRecordedNetwork::processFrame(std::vector<std::pair<ConnectionId, HAL::Network::Message>>& messagesRef)
{
	switch (mRecordingMode)
	{
	case RecordingMode::None:
		break;
	case RecordingMode::Record:
		appendNetworkDataToFile(messagesRef);
		break;
	case RecordingMode::Replay:
		if (mRecordedNetworkDataIdx >= mRecordedNetworkData.size())
		{
			return;
		}

		messagesRef = mRecordedNetworkData[mRecordedNetworkDataIdx];
		++mRecordedNetworkDataIdx;
		break;
	default:
		ReportError("Unknown debug network mode");
		break;
	}
}

void DebugRecordedNetwork::processArguments(const ArgumentsParser& arguments)
{
	if (arguments.hasArgument("record-network"))
	{
		mRecordingMode = RecordingMode::Record;
		const auto argumentOption = arguments.getArgumentValue("record-network");
		if (!argumentOption.has_value())
		{
			ReportError("No file name specified for recording network data");
			return;
		}

		const std::string& fileName = *argumentOption + std::format("_{}.netmsgs", mInstanceIndex);

		mRecordedNetworkDataFile = std::fopen(fileName.c_str(), "wb");
		AssertFatal(mRecordedNetworkDataFile, "Failed to open file for recording network data %s", fileName.c_str());
	}
	else if (arguments.hasArgument("replay-network"))
	{
		mRecordingMode = RecordingMode::Replay;
		const auto argumentOption = arguments.getArgumentValue("replay-network");
		if (!argumentOption.has_value())
		{
			ReportError("No file name specified for replaying network data");
			return;
		}
		mRecordedNetworkData = loadNetworkDataFromFile(*argumentOption + std::format("_{}.netmsgs", mInstanceIndex));
	}
}

void DebugRecordedNetwork::appendNetworkDataToFile(const OneFrameNetworkData& networkData) const
{
	if (!mRecordedNetworkDataFile)
	{
		ReportError("No file to record input data");
		return;
	}

	std::vector<std::byte> recordedDataBytes;
	// assume one message size is 16 bytes on average
	recordedDataBytes.reserve(networkData.size() * (sizeof(ConnectionId) + sizeof(u32) + 16));

	Serialization::AppendNumber<u32>(recordedDataBytes, static_cast<u32>(networkData.size()));
	for (const auto& [connectionId, message] : networkData)
	{
		// keep it in case of reconnects, peer-to-peer, or recording on server
		Serialization::AppendNumber<ConnectionId>(recordedDataBytes, connectionId);

		const std::span<const std::byte> dataSpan = message.getDataRef();
		Serialization::AppendNumber<u32>(recordedDataBytes, static_cast<u32>(dataSpan.size()));
		recordedDataBytes.insert(recordedDataBytes.end(), dataSpan.begin(), dataSpan.end());
	}

	std::fwrite(recordedDataBytes.data(), sizeof(std::byte), recordedDataBytes.size(), mRecordedNetworkDataFile);
}

std::vector<DebugRecordedNetwork::OneFrameNetworkData> DebugRecordedNetwork::loadNetworkDataFromFile(const std::string& path)
{
	std::vector<OneFrameNetworkData> result;
	std::FILE* file = std::fopen(path.c_str(), "rb");
	if (!file)
	{
		ReportError("Failed to open network data file: %s", path.c_str());
		return result;
	}

	std::vector<std::byte> recordedDataBytes;
	// read all bytes from file into inputDataBytes
	std::fseek(file, 0, SEEK_END);
	const size_t fileSize = std::ftell(file);
	recordedDataBytes.resize(fileSize);
	std::rewind(file);
	const size_t readSize = std::fread(recordedDataBytes.data(), sizeof(std::byte), recordedDataBytes.size(), file);
	std::fclose(file);

	if (readSize != fileSize)
	{
		ReportError("Incorrect input data file size, expected %d, got %d", fileSize, readSize);
		return result;
	}

	size_t cursorPos = 0;

	while (cursorPos < recordedDataBytes.size())
	{
		// message from one frame of network data
		const u32 networkDataSize = Serialization::ReadNumber<u32>(recordedDataBytes, cursorPos).value_or(0);
		result.push_back({});
		result.back().reserve(networkDataSize);
		for (size_t i = 0; i < networkDataSize; ++i)
		{
			const ConnectionId connectionId = Serialization::ReadNumber<ConnectionId>(recordedDataBytes, cursorPos).value_or(0);
			const u32 messageDataSize = Serialization::ReadNumber<u32>(recordedDataBytes, cursorPos).value_or(0);
			result.back().emplace_back(connectionId, HAL::Network::Message(recordedDataBytes.data() + cursorPos, messageDataSize));
			cursorPos += messageDataSize;
		}
	}

	return result;
}
