#include "load.hpp"

#include <array>
#include <charconv>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <vector>

namespace {

constexpr std::size_t USER_FIELD_COUNT = 3;
constexpr std::size_t TEAM_FIELD_COUNT = 3;
constexpr std::size_t TEAM_SUBSCRIPTION_FIELD_COUNT = 2;
constexpr std::size_t CHANNEL_FIELD_COUNT = 4;
constexpr std::size_t THREAD_FIELD_COUNT = 6;
constexpr std::size_t MESSAGE_FIELD_COUNT = 5;

struct RecordFormat {
	char delimiter;
	std::size_t expectedFieldCount;
	std::string_view sourceName;
	bool requireFirstField;
	bool requireSecondField;
};

struct ChannelPosition {
	std::size_t teamIndex;
	std::size_t channelIndex;
};

struct ThreadPosition {
	std::size_t teamIndex;
	std::size_t channelIndex;
	std::size_t threadIndex;
};

using TeamIndexes = std::unordered_map<std::string, std::size_t>;
using ChannelIndexes = std::unordered_map<std::string, ChannelPosition>;
using ThreadIndexes = std::unordered_map<std::string, ThreadPosition>;

std::vector<std::string>
splitEscapedLine(const std::string &line, char delimiter)
{
	std::vector<std::string> fields;
	std::string currentField;
	bool escaping = false;

	for (const char character : line) {
		if (escaping) {
			switch (character) {
			case 'n':
				currentField.push_back('\n');
				break;
			case 'r':
				currentField.push_back('\r');
				break;
			default:
				currentField.push_back(character);
				break;
			}
			escaping = false;
			continue;
		}

		if (character == '\\') {
			escaping = true;
			continue;
		}
		if (character == delimiter) {
			fields.push_back(currentField);
			currentField.clear();
			continue;
		}
		currentField.push_back(character);
	}

	if (escaping)
		currentField.push_back('\\');
	fields.push_back(currentField);
	return fields;
}

bool
parseTimeValue(const std::string &field, std::time_t &outTime)
{
	try {
		std::size_t consumed = 0;
		const long long parsedTime = std::stoll(field, &consumed, 10);
		if (consumed != field.size())
			return false;
		outTime = static_cast<std::time_t>(parsedTime);
		return true;
	} catch (...) {
		return false;
	}
}

bool
parseBoolValue(const std::string &field)
{
	return field == "1" || field == "true" || field == "TRUE" || field == "True";
}

bool
parseRecord(
	const std::string &line,
	const RecordFormat &format,
	std::vector<std::string> &fields)
{
	fields = splitEscapedLine(line, format.delimiter);
	if (fields.size() != format.expectedFieldCount) {
		std::cerr << "Skipping malformed " << format.sourceName << " line: " << line << std::endl;
		return false;
	}
	if (format.requireFirstField && fields.at(0).empty()) {
		std::cerr << "Skipping malformed " << format.sourceName << " line: " << line << std::endl;
		return false;
	}
	if (format.requireSecondField && fields.at(1).empty()) {
		std::cerr << "Skipping malformed " << format.sourceName << " line: " << line << std::endl;
		return false;
	}
	return true;
}

bool
readTextFile(const std::filesystem::path &filePath, std::vector<std::string> &lines)
{
	std::ifstream inputFile(filePath);
	if (!inputFile.is_open()) {
		if (!std::filesystem::exists(filePath))
			std::cout << "No save file found: " << filePath << std::endl;
		else
			std::cerr << "Failed to open save file: " << filePath << std::endl;
		return false;
	}

	std::string line;
	while (std::getline(inputFile, line))
		lines.push_back(line);
	return true;
}

} // namespace

namespace server::database {

using detail::LoadedChannel;
using detail::LoadedTeam;
using detail::LoadedThread;

void
loadUsers(const std::vector<std::string> &userLines, char delimiter, std::vector<myteams::User> &users)
{
	std::vector<std::string> fields;
	const RecordFormat userFormat { delimiter, USER_FIELD_COUNT, "users", false, false };
	for (const std::string &line : userLines) {
		if (line.empty())
			continue;
		if (!parseRecord(line, userFormat, fields))
			continue;
		users.emplace_back(fields.at(0).c_str(), fields.at(1).c_str(), parseBoolValue(fields.at(2)));
	}
}

void
loadTeams(
	const std::vector<std::string> &teamLines,
	char delimiter,
	std::vector<LoadedTeam> &loadedTeams,
	TeamIndexes &teamIndexes)
{
	std::vector<std::string> fields;
	const RecordFormat teamFormat { delimiter, TEAM_FIELD_COUNT, "teams", true, false };
	for (const std::string &line : teamLines) {
		if (line.empty())
			continue;
		if (!parseRecord(line, teamFormat, fields))
			continue;
		if (teamIndexes.find(fields.at(0)) != teamIndexes.end()) {
			std::cerr << "Skipping duplicate team uuid: " << fields.at(0) << std::endl;
			continue;
		}

		loadedTeams.push_back({
			myteams::Team(fields.at(0).c_str(), fields.at(1).c_str(), fields.at(2).c_str()),
			{},
			{}
		});
		teamIndexes.emplace(fields.at(0), loadedTeams.size() - 1);
	}
}

void
loadTeamSubscriptions(
	const std::vector<std::string> &teamSubscriptionLines,
	char delimiter,
	const TeamIndexes &teamIndexes,
	std::vector<LoadedTeam> &loadedTeams)
{
	std::vector<std::string> fields;
	const RecordFormat teamSubscriptionFormat {
		delimiter,
		TEAM_SUBSCRIPTION_FIELD_COUNT,
		"team_subscriptions",
		true,
		true
	};
	for (const std::string &line : teamSubscriptionLines) {
		if (line.empty())
			continue;
		if (!parseRecord(line, teamSubscriptionFormat, fields))
			continue;

		const auto teamIt = teamIndexes.find(fields.at(0));
		if (teamIt == teamIndexes.end()) {
			std::cerr << "Skipping team subscription with unknown team uuid: " << fields.at(0) << std::endl;
			continue;
		}
		loadedTeams.at(teamIt->second).subscribedUsers.push_back(fields.at(1));
	}
}

void
loadChannels(
	const std::vector<std::string> &channelLines,
	char delimiter,
	const TeamIndexes &teamIndexes,
	std::vector<LoadedTeam> &loadedTeams,
	ChannelIndexes &channelIndexes)
{
	std::vector<std::string> fields;
	const RecordFormat channelFormat { delimiter, CHANNEL_FIELD_COUNT, "channels", true, true };
	for (const std::string &line : channelLines) {
		if (line.empty())
			continue;
		if (!parseRecord(line, channelFormat, fields))
			continue;
		if (channelIndexes.find(fields.at(1)) != channelIndexes.end()) {
			std::cerr << "Skipping duplicate channel uuid: " << fields.at(1) << std::endl;
			continue;
		}
		const auto teamIt = teamIndexes.find(fields.at(0));
		if (teamIt == teamIndexes.end()) {
			std::cerr << "Skipping channel with unknown team uuid: " << fields.at(0) << std::endl;
			continue;
		}
		std::vector<LoadedChannel> &channels = loadedTeams.at(teamIt->second).channels;
		const std::size_t channelIndex = channels.size();
		channels.push_back({
			myteams::Channel(fields.at(1).c_str(), fields.at(2).c_str(), fields.at(3).c_str()),
			{}
		});
		channelIndexes.emplace(fields.at(1), ChannelPosition { teamIt->second, channelIndex });
	}
}

void
loadThreads(
	const std::vector<std::string> &threadLines,
	char delimiter,
	const ChannelIndexes &channelIndexes,
	std::vector<LoadedTeam> &loadedTeams,
	ThreadIndexes &threadIndexes)
{
	std::vector<std::string> fields;
	const RecordFormat threadFormat { delimiter, THREAD_FIELD_COUNT, "threads", true, true };
	for (const std::string &line : threadLines) {
		if (line.empty())
			continue;
		if (!parseRecord(line, threadFormat, fields))
			continue;
		if (threadIndexes.find(fields.at(1)) != threadIndexes.end()) {
			std::cerr << "Skipping duplicate thread uuid: " << fields.at(1) << std::endl;
			continue;
		}
		std::time_t createdAt = 0;
		if (!parseTimeValue(fields.at(3), createdAt)) {
			std::cerr << "Skipping thread with invalid timestamp: " << line << std::endl;
			continue;
		}
		const auto channelIt = channelIndexes.find(fields.at(0));
		if (channelIt == channelIndexes.end()) {
			std::cerr << "Skipping thread with unknown channel uuid: " << fields.at(0) << std::endl;
			continue;
		}
		const std::size_t teamIndex = channelIt->second.teamIndex;
		const std::size_t channelIndex = channelIt->second.channelIndex;
		std::vector<LoadedThread> &threads = loadedTeams.at(teamIndex).channels.at(channelIndex).threads;
		const std::size_t threadIndex = threads.size();
		threads.push_back({
			myteams::Thread(
				fields.at(1).c_str(),
				fields.at(2).c_str(),
				createdAt,
				fields.at(4).c_str(),
				fields.at(5).c_str()),
			{}
		});
		threadIndexes.emplace(fields.at(1), ThreadPosition { teamIndex, channelIndex, threadIndex });
	}
}

void
loadMessages(
	const std::vector<std::string> &messageLines,
	char delimiter,
	const ThreadIndexes &threadIndexes,
	std::vector<LoadedTeam> &loadedTeams)
{
	std::vector<std::string> fields;
	const RecordFormat messageFormat { delimiter, MESSAGE_FIELD_COUNT, "messages", true, true };
	for (const std::string &line : messageLines) {
		if (line.empty())
			continue;
		if (!parseRecord(line, messageFormat, fields))
			continue;
		std::time_t createdAt = 0;
		if (!parseTimeValue(fields.at(3), createdAt)) {
			std::cerr << "Skipping message with invalid timestamp: " << line << std::endl;
			continue;
		}
		const auto threadIt = threadIndexes.find(fields.at(0));
		if (threadIt == threadIndexes.end()) {
			std::cerr << "Skipping message with unknown thread uuid: " << fields.at(0) << std::endl;
			continue;
		}
		const ThreadPosition &threadPosition = threadIt->second;
		loadedTeams.at(threadPosition.teamIndex)
			.channels.at(threadPosition.channelIndex)
			.threads.at(threadPosition.threadIndex)
			.replies.emplace_back(
			fields.at(1).c_str(),
			fields.at(2).c_str(),
			createdAt,
			fields.at(4).c_str());
	}
}

void
materializeTeams(std::vector<LoadedTeam> &loadedTeams, std::vector<myteams::Team> &teams)
{
	for (LoadedTeam &loadedTeam : loadedTeams) {
		myteams::Team team = loadedTeam.team;
		for (const std::string &subscribedUser : loadedTeam.subscribedUsers)
			team.addSubscribedUser(subscribedUser.c_str());

		for (LoadedChannel &loadedChannel : loadedTeam.channels) {
			myteams::Channel channel = loadedChannel.channel;
			for (LoadedThread &loadedThread : loadedChannel.threads) {
				myteams::Thread thread = loadedThread.thread;
				for (const myteams::Message &reply : loadedThread.replies)
					thread.addReply(reply);
				channel.addThread(thread);
			}
			team.addChannel(channel);
		}
		teams.push_back(team);
	}
}

DatabaseLoader::DatabaseLoader(std::filesystem::path baseDirectory, char delimiter)
	: _baseDirectory(std::move(baseDirectory)), _delimiter(delimiter)
{
}

bool
DatabaseLoader::load(std::vector<myteams::User> &users, std::vector<myteams::Team> &teams) const
{
	users.clear();
	teams.clear();
	std::vector<std::string> userLines;
	std::vector<std::string> teamLines;
	std::vector<std::string> teamSubscriptionLines;
	std::vector<std::string> channelLines;
	std::vector<std::string> threadLines;
	std::vector<std::string> messageLines;
	struct FileLines {
		std::filesystem::path path;
		std::vector<std::string> *lines;
	};
	std::array<FileLines, 6> sourceFiles {{
		{usersFilePath(), &userLines},
		{teamsFilePath(), &teamLines},
		{teamSubscriptionsFilePath(), &teamSubscriptionLines},
		{channelsFilePath(), &channelLines},
		{threadsFilePath(), &threadLines},
		{messagesFilePath(), &messageLines}
	}};
	bool hasSavedData = false;
	for (FileLines &file : sourceFiles)
		hasSavedData = readTextFile(file.path, *file.lines) || hasSavedData;
	if (!hasSavedData) {
		std::cout << "No saved database found. Starting with an empty state." << std::endl;
		return false;
	}
	std::vector<LoadedTeam> loadedTeams;
	TeamIndexes teamIndexes;
	ChannelIndexes channelIndexes;
	ThreadIndexes threadIndexes;

	loadUsers(userLines, _delimiter, users);
	loadTeams(teamLines, _delimiter, loadedTeams, teamIndexes);
	loadTeamSubscriptions(teamSubscriptionLines, _delimiter, teamIndexes, loadedTeams);
	loadChannels(channelLines, _delimiter, teamIndexes, loadedTeams, channelIndexes);
	loadThreads(threadLines, _delimiter, channelIndexes, loadedTeams, threadIndexes);
	loadMessages(messageLines, _delimiter, threadIndexes, loadedTeams);
	materializeTeams(loadedTeams, teams);

	std::cout << "Loaded database: "
		<< users.size() << " users, "
		<< teams.size() << " teams." << std::endl;
	return true;
}

std::filesystem::path
DatabaseLoader::usersFilePath() const
{
	return _baseDirectory / "users.txt";
}

std::filesystem::path
DatabaseLoader::teamsFilePath() const
{
	return _baseDirectory / "teams.txt";
}

std::filesystem::path
DatabaseLoader::teamSubscriptionsFilePath() const
{
	return _baseDirectory / "team_subscriptions.txt";
}

std::filesystem::path
DatabaseLoader::channelsFilePath() const
{
	return _baseDirectory / "channels.txt";
}

std::filesystem::path
DatabaseLoader::threadsFilePath() const
{
	return _baseDirectory / "threads.txt";
}

std::filesystem::path
DatabaseLoader::messagesFilePath() const
{
	return _baseDirectory / "messages.txt";
}

} // namespace server::database
