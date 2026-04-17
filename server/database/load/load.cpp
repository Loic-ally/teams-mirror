#include "load.hpp"
#include "models/message/message.hpp"

#include <array>
#include <charconv>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

static constexpr std::size_t USER_FIELD_COUNT = 3;
static constexpr std::size_t TEAM_FIELD_COUNT = 3;
static constexpr std::size_t TEAM_SUBSCRIPTION_FIELD_COUNT = 2;
static constexpr std::size_t CHANNEL_FIELD_COUNT = 4;
static constexpr std::size_t THREAD_FIELD_COUNT = 6;
static constexpr std::size_t MESSAGE_FIELD_COUNT = 5;
static constexpr std::size_t PRIVATE_MESSAGE_FIELD_COUNT = 5;

static std::vector<std::string>
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

static bool
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

static bool
parseBoolValue(const std::string &field)
{
	return field == "1" || field == "true" || field == "TRUE" || field == "True";
}

static bool
parseRecord(
	const std::string &line,
	const server::database::detail::RecordFormat &format,
	std::vector<std::string> &fields)
{
	fields = splitEscapedLine(line, format.delimiter);
	if (fields.size() != format.expectedFieldCount) {
		std::cout << "Skipping malformed " << format.sourceName << " line: " << line << std::endl;
		return false;
	}
	if (format.requireFirstField && fields.at(0).empty()) {
		std::cout << "Skipping malformed " << format.sourceName << " line: " << line << std::endl;
		return false;
	}
	if (format.requireSecondField && fields.at(1).empty()) {
		std::cout << "Skipping malformed " << format.sourceName << " line: " << line << std::endl;
		return false;
	}
	return true;
}

static bool
readTextFile(const std::filesystem::path &filePath, std::vector<std::string> &lines)
{
	std::ifstream inputFile(filePath);
	if (!inputFile.is_open()) {
		if (!std::filesystem::exists(filePath))
			std::cout << "No save file found: " << filePath << std::endl;
		else
			std::cout << "Failed to open save file: " << filePath << std::endl;
		return false;
	}

	std::string line;
	while (std::getline(inputFile, line))
		lines.push_back(line);
	return true;
}


namespace server::database {

using detail::LoadedChannel;
using detail::LoadedTeam;
using detail::LoadedThread;
using detail::RecordFormat;
using detail::ChannelPosition;
using detail::ThreadPosition;
using detail::TeamIndexes;
using detail::ChannelIndexes;
using detail::ThreadIndexes;

static void
loadUsers(const std::vector<std::string> &userLines, char delimiter, std::vector<myteams::User> &users)
{
	std::vector<std::string> fields;
	const RecordFormat userFormat { delimiter, USER_FIELD_COUNT, "users", false, false };
	for (const std::string &line : userLines) {
		if (line.empty())
			continue;
		if (!parseRecord(line, userFormat, fields))
			continue;
		users.emplace_back(fields.at(0), fields.at(1), parseBoolValue(fields.at(2)));
	}
}

static void
loadPrivateMessages(const std::vector<std::string> &privateMessageLines, char delimiter, std::vector<myteams::Message> &messages)
{
	std::vector<std::string> fields;
	const RecordFormat messageFormat { delimiter, PRIVATE_MESSAGE_FIELD_COUNT, "privateMessage", true, true };
	for (const std::string &line : privateMessageLines) {
		if (line.empty())
			continue;
		if (!parseRecord(line, messageFormat, fields))
			continue;
		std::time_t createdAt = 0;
		if (!parseTimeValue(fields.at(2), createdAt)) {
			std::cout << "Skipping message with invalid timestamp: " << line << std::endl;
			continue;
		}
		messages.emplace_back(
			fields.at(0),
			fields.at(1),
			createdAt,
			fields.at(3),
            fields.at(4));
	}
}

static void
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
			std::cout << "Skipping duplicate team uuid: " << fields.at(0) << std::endl;
			continue;
		}

		loadedTeams.push_back({
			myteams::Team(fields.at(0), fields.at(1), fields.at(2)),
			{},
			{}
		});
		teamIndexes.emplace(fields.at(0), loadedTeams.size() - 1);
	}
}

static void
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
			std::cout << "Skipping team subscription with unknown team uuid: " << fields.at(0) << std::endl;
			continue;
		}
		loadedTeams.at(teamIt->second).subscribedUsers.push_back(fields.at(1));
	}
}

static void
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
			std::cout << "Skipping duplicate channel uuid: " << fields.at(1) << std::endl;
			continue;
		}
		const auto teamIt = teamIndexes.find(fields.at(0));
		if (teamIt == teamIndexes.end()) {
			std::cout << "Skipping channel with unknown team uuid: " << fields.at(0) << std::endl;
			continue;
		}
		std::vector<LoadedChannel> &channels = loadedTeams.at(teamIt->second).channels;
		const std::size_t channelIndex = channels.size();
		channels.push_back({
			myteams::Channel(fields.at(1), fields.at(2), fields.at(3)),
			{}
		});
		channelIndexes.emplace(fields.at(1), ChannelPosition { teamIt->second, channelIndex });
	}
}

static void
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
			std::cout << "Skipping duplicate thread uuid: " << fields.at(1) << std::endl;
			continue;
		}
		std::time_t createdAt = 0;
		if (!parseTimeValue(fields.at(3), createdAt)) {
			std::cout << "Skipping thread with invalid timestamp: " << line << std::endl;
			continue;
		}
		const auto channelIt = channelIndexes.find(fields.at(0));
		if (channelIt == channelIndexes.end()) {
			std::cout << "Skipping thread with unknown channel uuid: " << fields.at(0) << std::endl;
			continue;
		}
		const std::size_t teamIndex = channelIt->second.teamIndex;
		const std::size_t channelIndex = channelIt->second.channelIndex;
		std::vector<LoadedThread> &threads = loadedTeams.at(teamIndex).channels.at(channelIndex).threads;
		const std::size_t threadIndex = threads.size();
		threads.push_back({
			myteams::Thread(
				fields.at(1),
				fields.at(2),
				createdAt,
				fields.at(4),
				fields.at(5)),
			{}
		});
		threadIndexes.emplace(fields.at(1), ThreadPosition { teamIndex, channelIndex, threadIndex });
	}
}

static void
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
			std::cout << "Skipping message with invalid timestamp: " << line << std::endl;
			continue;
		}
		const auto threadIt = threadIndexes.find(fields.at(0));
		if (threadIt == threadIndexes.end()) {
			std::cout << "Skipping message with unknown thread uuid: " << fields.at(0) << std::endl;
			continue;
		}
		const ThreadPosition &threadPosition = threadIt->second;
		loadedTeams.at(threadPosition.teamIndex)
			.channels.at(threadPosition.channelIndex)
			.threads.at(threadPosition.threadIndex)
			.replies.emplace_back(
			fields.at(1),
			fields.at(2),
			createdAt,
			fields.at(4));
	}
}

static void
materializeTeams(std::vector<LoadedTeam> &loadedTeams, std::vector<myteams::Team> &teams)
{
	for (LoadedTeam &loadedTeam : loadedTeams) {
		myteams::Team team = loadedTeam.team;
		for (const std::string &subscribedUser : loadedTeam.subscribedUsers)
			team.addSubscribedUser(subscribedUser);

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
DatabaseLoader::load(std::vector<myteams::User> &users, std::vector<myteams::Team> &teams, std::vector<myteams::Message> &messages) const
{
	users.clear();
	teams.clear();
	std::vector<std::string> userLines;
	std::vector<std::string> teamLines;
	std::vector<std::string> teamSubscriptionLines;
	std::vector<std::string> channelLines;
	std::vector<std::string> threadLines;
	std::vector<std::string> messageLines;
	std::vector<std::string> privateMessageLines;
	using FileLines = std::pair<std::filesystem::path, std::reference_wrapper<std::vector<std::string>>>;
	std::array<FileLines, 7> sourceFiles {{
		{usersFilePath(), userLines},
		{teamsFilePath(), teamLines},
		{teamSubscriptionsFilePath(), teamSubscriptionLines},
		{channelsFilePath(), channelLines},
		{threadsFilePath(), threadLines},
		{messagesFilePath(), messageLines},
		{privateMessagesFilePath(), privateMessageLines}
	}};
	bool hasSavedData = false;
	for (FileLines &file : sourceFiles)
		hasSavedData = readTextFile(file.first, file.second.get()) || hasSavedData;
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
	loadPrivateMessages(privateMessageLines, _delimiter, messages);
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

std::filesystem::path
DatabaseLoader::privateMessagesFilePath() const
{
	return _baseDirectory / "privateMessages.txt";
}

} // namespace server::database
