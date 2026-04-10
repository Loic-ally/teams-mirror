#include "save.hpp"

#include <array>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace {

std::string
escapeField(std::string_view value, char delimiter)
{
	std::string escaped;
	escaped.reserve(value.size());
	for (const char character : value) {
		if (character == '\\' || character == delimiter) {
			escaped.push_back('\\');
			escaped.push_back(character);
			continue;
		}
		if (character == '\n') {
			escaped.append("\\n");
			continue;
		}
		if (character == '\r') {
			escaped.append("\\r");
			continue;
		}
		escaped.push_back(character);
	}
	return escaped;
}

std::string
joinEscapedFields(const std::vector<std::string> &fields, char delimiter)
{
	std::string line;
	for (std::size_t index = 0; index < fields.size(); ++index) {
		if (index > 0)
			line.push_back(delimiter);
		line.append(escapeField(fields[index], delimiter));
	}
	return line;
}

struct SerializedLines {
	std::vector<std::string> users;
	std::vector<std::string> teams;
	std::vector<std::string> teamSubscriptions;
	std::vector<std::string> channels;
	std::vector<std::string> threads;
	std::vector<std::string> messages;
};

SerializedLines
serializeData(const std::vector<myteams::User> &users, const std::vector<myteams::Team> &teams, char delimiter)
{
	SerializedLines serialized;
	for (const myteams::User &user : users) {
		serialized.users.push_back(joinEscapedFields({
			std::string(user.getUuid()),
			std::string(user.getName()),
			user.isLoggedIn() ? "1" : "0"
		}, delimiter));
	}
	for (const myteams::Team &team : teams) {
		const std::string teamUuid(team.getUuid());
		serialized.teams.push_back(joinEscapedFields({
			teamUuid,
			std::string(team.getName()),
			std::string(team.getDescription())
		}, delimiter));
		for (const myteams::Team::UserUuid &subscribedUserUuid : team.getSubscribedUsers()) {
			serialized.teamSubscriptions.push_back(joinEscapedFields({
				teamUuid,
				std::string(subscribedUserUuid.data())
			}, delimiter));
		}
		for (const myteams::Channel &channel : team.getChannels()) {
			const std::string channelUuid(channel.getUuid());
			serialized.channels.push_back(joinEscapedFields({
				teamUuid,
				channelUuid,
				std::string(channel.getName()),
				std::string(channel.getDescription())
			}, delimiter));
			for (const myteams::Thread &thread : channel.getThreads()) {
				const std::string threadUuid(thread.getUuid());
				serialized.threads.push_back(joinEscapedFields({
					channelUuid,
					threadUuid,
					std::string(thread.getAuthorUuid()),
					std::to_string(static_cast<long long>(thread.getCreatedAt())),
					std::string(thread.getTitle()),
					std::string(thread.getBody())
				}, delimiter));
				for (const myteams::Message &message : thread.getReplies()) {
					serialized.messages.push_back(joinEscapedFields({
						threadUuid,
						std::string(message.getUuid()),
						std::string(message.getAuthorUuid()),
						std::to_string(static_cast<long long>(message.getCreatedAt())),
						std::string(message.getBody())
					}, delimiter));
				}
			}
		}
	}
	return serialized;
}

bool
writeLines(const std::filesystem::path &filePath, const std::vector<std::string> &lines)
{
	std::ofstream outputFile(filePath, std::ios::out | std::ios::trunc);
	if (!outputFile.is_open()) {
		std::cerr << "Failed to open save file: " << filePath << std::endl;
		return false;
	}
	for (const std::string &line : lines)
		outputFile << line << '\n';
	return outputFile.good();
}

} // namespace

namespace server::database {

DatabaseSaver::DatabaseSaver(std::filesystem::path baseDirectory, char delimiter)
	: _baseDirectory(std::move(baseDirectory)), _delimiter(delimiter)
{
}

bool
DatabaseSaver::save(const std::vector<myteams::User> &users, const std::vector<myteams::Team> &teams) const
{
	std::error_code error;
	std::filesystem::create_directories(_baseDirectory, error);
	if (error) {
		std::cerr << "Failed to create save directory: " << _baseDirectory << std::endl;
		return false;
	}
	const SerializedLines serialized = serializeData(users, teams, _delimiter);
	using LineView = std::reference_wrapper<const std::vector<std::string>>;
	using OutputFile = std::pair<std::filesystem::path, LineView>;
	std::array<OutputFile, 6> outputFiles {{
		{usersFilePath(), std::cref(serialized.users)},
		{teamsFilePath(), std::cref(serialized.teams)},
		{teamSubscriptionsFilePath(), std::cref(serialized.teamSubscriptions)},
		{channelsFilePath(), std::cref(serialized.channels)},
		{threadsFilePath(), std::cref(serialized.threads)},
		{messagesFilePath(), std::cref(serialized.messages)}
	}};
	bool saveSuccess = true;
	for (const OutputFile &outputFile : outputFiles)
		saveSuccess = writeLines(outputFile.first, outputFile.second.get()) && saveSuccess;
	if (saveSuccess) {
		std::cout << "Server state saved in " << _baseDirectory << std::endl;
	}
	return saveSuccess;
}

std::filesystem::path
DatabaseSaver::usersFilePath() const
{
	return _baseDirectory / "users.txt";
}

std::filesystem::path
DatabaseSaver::teamsFilePath() const
{
	return _baseDirectory / "teams.txt";
}

std::filesystem::path
DatabaseSaver::teamSubscriptionsFilePath() const
{
	return _baseDirectory / "team_subscriptions.txt";
}

std::filesystem::path
DatabaseSaver::channelsFilePath() const
{
	return _baseDirectory / "channels.txt";
}

std::filesystem::path
DatabaseSaver::threadsFilePath() const
{
	return _baseDirectory / "threads.txt";
}

std::filesystem::path
DatabaseSaver::messagesFilePath() const
{
	return _baseDirectory / "messages.txt";
}

} // namespace server::database
