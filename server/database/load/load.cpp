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
	long long parsedTime = 0;
	const char *begin = field.data();
	const char *end = begin + field.size();
	const auto [position, errorCode] = std::from_chars(begin, end, parsedTime);
	if (errorCode != std::errc() || position != end)
		return false;
	outTime = static_cast<std::time_t>(parsedTime);
	return true;
}

bool
parseBoolValue(const std::string &field)
{
	return field == "1" || field == "true" || field == "TRUE" || field == "True";
}

bool
parseRecord(
	const std::string &line,
	char delimiter,
	std::size_t expectedFieldCount,
	std::string_view sourceName,
	std::vector<std::string> &fields,
	bool requireFirstField,
	bool requireSecondField)
{
	fields = splitEscapedLine(line, delimiter);
	if (fields.size() != expectedFieldCount) {
		std::cerr << "Skipping malformed " << sourceName << " line: " << line << std::endl;
		return false;
	}
	if (requireFirstField && fields[0].empty()) {
		std::cerr << "Skipping malformed " << sourceName << " line: " << line << std::endl;
		return false;
	}
	if (requireSecondField && fields[1].empty()) {
		std::cerr << "Skipping malformed " << sourceName << " line: " << line << std::endl;
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

namespace {

void
loadUsers(const std::vector<std::string> &userLines, char delimiter, std::vector<myteams::User> &users)
{
	std::vector<std::string> fields;
	for (const std::string &line : userLines) {
		if (line.empty())
			continue;
		if (!parseRecord(line, delimiter, USER_FIELD_COUNT, "users", fields, false, false))
			continue;
		users.emplace_back(fields[0].c_str(), fields[1].c_str(), parseBoolValue(fields[2]));
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
	for (const std::string &line : teamLines) {
		if (line.empty())
			continue;
		if (!parseRecord(line, delimiter, TEAM_FIELD_COUNT, "teams", fields, true, false))
			continue;
		if (teamIndexes.find(fields[0]) != teamIndexes.end()) {
			std::cerr << "Skipping duplicate team uuid: " << fields[0] << std::endl;
			continue;
		}

		loadedTeams.push_back({
			myteams::Team(fields[0].c_str(), fields[1].c_str(), fields[2].c_str()),
			{},
			{}
		});
		teamIndexes.emplace(fields[0], loadedTeams.size() - 1);
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
	for (const std::string &line : teamSubscriptionLines) {
		if (line.empty())
			continue;
		if (!parseRecord(line, delimiter, TEAM_SUBSCRIPTION_FIELD_COUNT, "team_subscriptions", fields, true, true))
			continue;

		const auto teamIt = teamIndexes.find(fields[0]);
		if (teamIt == teamIndexes.end()) {
			std::cerr << "Skipping team subscription with unknown team uuid: " << fields[0] << std::endl;
			continue;
		}
		loadedTeams[teamIt->second].subscribedUsers.push_back(fields[1]);
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
	for (const std::string &line : channelLines) {
		if (line.empty())
			continue;
		if (!parseRecord(line, delimiter, CHANNEL_FIELD_COUNT, "channels", fields, true, true))
			continue;
		if (channelIndexes.find(fields[1]) != channelIndexes.end()) {
			std::cerr << "Skipping duplicate channel uuid: " << fields[1] << std::endl;
			continue;
		}
		const auto teamIt = teamIndexes.find(fields[0]);
		if (teamIt == teamIndexes.end()) {
			std::cerr << "Skipping channel with unknown team uuid: " << fields[0] << std::endl;
			continue;
		}
		std::vector<LoadedChannel> &channels = loadedTeams[teamIt->second].channels;
		const std::size_t channelIndex = channels.size();
		channels.push_back({
			myteams::Channel(fields[1].c_str(), fields[2].c_str(), fields[3].c_str()),
			{}
		});
		channelIndexes.emplace(fields[1], ChannelPosition { teamIt->second, channelIndex });
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
	for (const std::string &line : threadLines) {
		if (line.empty())
			continue;
		if (!parseRecord(line, delimiter, THREAD_FIELD_COUNT, "threads", fields, true, true))
			continue;
		if (threadIndexes.find(fields[1]) != threadIndexes.end()) {
			std::cerr << "Skipping duplicate thread uuid: " << fields[1] << std::endl;
			continue;
		}
		std::time_t createdAt = 0;
		if (!parseTimeValue(fields[3], createdAt)) {
			std::cerr << "Skipping thread with invalid timestamp: " << line << std::endl;
			continue;
		}
		const auto channelIt = channelIndexes.find(fields[0]);
		if (channelIt == channelIndexes.end()) {
			std::cerr << "Skipping thread with unknown channel uuid: " << fields[0] << std::endl;
			continue;
		}
		const std::size_t teamIndex = channelIt->second.teamIndex;
		const std::size_t channelIndex = channelIt->second.channelIndex;
		std::vector<LoadedThread> &threads = loadedTeams[teamIndex].channels[channelIndex].threads;
		const std::size_t threadIndex = threads.size();
		threads.push_back({
			myteams::Thread(
				fields[1].c_str(),
				fields[2].c_str(),
				createdAt,
				fields[4].c_str(),
				fields[5].c_str()),
			{}
		});
		threadIndexes.emplace(fields[1], ThreadPosition { teamIndex, channelIndex, threadIndex });
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
	for (const std::string &line : messageLines) {
		if (line.empty())
			continue;
		if (!parseRecord(line, delimiter, MESSAGE_FIELD_COUNT, "messages", fields, true, true))
			continue;
		std::time_t createdAt = 0;
		if (!parseTimeValue(fields[3], createdAt)) {
			std::cerr << "Skipping message with invalid timestamp: " << line << std::endl;
			continue;
		}
		const auto threadIt = threadIndexes.find(fields[0]);
		if (threadIt == threadIndexes.end()) {
			std::cerr << "Skipping message with unknown thread uuid: " << fields[0] << std::endl;
			continue;
		}
		const ThreadPosition &threadPosition = threadIt->second;
		loadedTeams[threadPosition.teamIndex].channels[threadPosition.channelIndex].threads[threadPosition.threadIndex].replies.emplace_back(
			fields[1].c_str(),
			fields[2].c_str(),
			createdAt,
			fields[4].c_str());
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

} // namespace

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
