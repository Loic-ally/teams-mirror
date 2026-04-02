#include "load.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace {

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

	const bool hasUsers = readTextFile(usersFilePath(), userLines);
	const bool hasTeams = readTextFile(teamsFilePath(), teamLines);
	const bool hasTeamSubscriptions = readTextFile(teamSubscriptionsFilePath(), teamSubscriptionLines);
	const bool hasChannels = readTextFile(channelsFilePath(), channelLines);
	const bool hasThreads = readTextFile(threadsFilePath(), threadLines);
	const bool hasMessages = readTextFile(messagesFilePath(), messageLines);

	if (!hasUsers && !hasTeams && !hasTeamSubscriptions && !hasChannels && !hasThreads && !hasMessages) {
		std::cout << "No saved database found. Starting with an empty state." << std::endl;
		return false;
	}

	for (const std::string &line : userLines) {
		if (line.empty())
			continue;
		const std::vector<std::string> fields = splitEscapedLine(line, _delimiter);
		if (fields.size() != 3) {
			std::cerr << "Skipping malformed users line: " << line << std::endl;
			continue;
		}
		users.emplace_back(fields[0], fields[1], parseBoolValue(fields[2]));
	}

	std::vector<LoadedTeam> loadedTeams;
	std::unordered_map<std::string, std::size_t> teamIndexes;
	for (const std::string &line : teamLines) {
		if (line.empty())
			continue;
		const std::vector<std::string> fields = splitEscapedLine(line, _delimiter);
		if (fields.size() != 3 || fields[0].empty()) {
			std::cerr << "Skipping malformed teams line: " << line << std::endl;
			continue;
		}
		if (teamIndexes.find(fields[0]) != teamIndexes.end()) {
			std::cerr << "Skipping duplicate team uuid: " << fields[0] << std::endl;
			continue;
		}

		LoadedTeam loadedTeam {
			myteams::Team(fields[0], fields[1], fields[2]),
			fields[0],
			{},
			{}
		};
		teamIndexes.emplace(loadedTeam.uuid, loadedTeams.size());
		loadedTeams.push_back(std::move(loadedTeam));
	}

	for (const std::string &line : teamSubscriptionLines) {
		if (line.empty())
			continue;
		const std::vector<std::string> fields = splitEscapedLine(line, _delimiter);
		if (fields.size() != 2 || fields[0].empty() || fields[1].empty()) {
			std::cerr << "Skipping malformed team_subscriptions line: " << line << std::endl;
			continue;
		}
		const auto teamIt = teamIndexes.find(fields[0]);
		if (teamIt == teamIndexes.end()) {
			std::cerr << "Skipping team subscription with unknown team uuid: " << fields[0] << std::endl;
			continue;
		}
		loadedTeams[teamIt->second].subscribedUsers.push_back(fields[1]);
	}

	std::unordered_map<std::string, std::pair<std::size_t, std::size_t>> channelIndexes;
	for (const std::string &line : channelLines) {
		if (line.empty())
			continue;
		const std::vector<std::string> fields = splitEscapedLine(line, _delimiter);
		if (fields.size() != 4 || fields[0].empty() || fields[1].empty()) {
			std::cerr << "Skipping malformed channels line: " << line << std::endl;
			continue;
		}
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
		LoadedChannel loadedChannel {
			myteams::Channel(fields[1], fields[2], fields[3]),
			fields[1],
			{}
		};
		const std::size_t channelIndex = channels.size();
		channels.push_back(std::move(loadedChannel));
		channelIndexes.emplace(fields[1], std::make_pair(teamIt->second, channelIndex));
	}

	std::unordered_map<std::string, std::tuple<std::size_t, std::size_t, std::size_t>> threadIndexes;
	for (const std::string &line : threadLines) {
		if (line.empty())
			continue;
		const std::vector<std::string> fields = splitEscapedLine(line, _delimiter);
		if (fields.size() != 6 || fields[0].empty() || fields[1].empty()) {
			std::cerr << "Skipping malformed threads line: " << line << std::endl;
			continue;
		}
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

		const std::size_t teamIndex = channelIt->second.first;
		const std::size_t channelIndex = channelIt->second.second;
		std::vector<LoadedThread> &threads = loadedTeams[teamIndex].channels[channelIndex].threads;
		LoadedThread loadedThread {
			myteams::Thread(
				fields[1],
				fields[2],
				createdAt,
				fields[4],
				fields[5]),
			fields[1],
			{}
		};
		const std::size_t threadIndex = threads.size();
		threads.push_back(std::move(loadedThread));
		threadIndexes.emplace(fields[1], std::make_tuple(teamIndex, channelIndex, threadIndex));
	}

	for (const std::string &line : messageLines) {
		if (line.empty())
			continue;
		const std::vector<std::string> fields = splitEscapedLine(line, _delimiter);
		if (fields.size() != 5 || fields[0].empty() || fields[1].empty()) {
			std::cerr << "Skipping malformed messages line: " << line << std::endl;
			continue;
		}

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

		const auto [teamIndex, channelIndex, threadIndex] = threadIt->second;
		loadedTeams[teamIndex].channels[channelIndex].threads[threadIndex].replies.emplace_back(
			fields[1],
			fields[2],
			createdAt,
			fields[4]);
	}

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
