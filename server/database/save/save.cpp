#include "save.hpp"

#include <filesystem>
#include <fstream>
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

	std::vector<std::string> userLines;
	std::vector<std::string> teamLines;
	std::vector<std::string> teamSubscriptionLines;
	std::vector<std::string> channelLines;
	std::vector<std::string> threadLines;
	std::vector<std::string> messageLines;

	for (const myteams::User &user : users) {
		userLines.push_back(joinEscapedFields({
			std::string(user.getUuid()),
			std::string(user.getName()),
			user.isLoggedIn() ? "1" : "0"
		}, _delimiter));
	}

	for (const myteams::Team &team : teams) {
		const std::string teamUuid(team.getUuid());
		teamLines.push_back(joinEscapedFields({
			teamUuid,
			std::string(team.getName()),
			std::string(team.getDescription())
		}, _delimiter));

		for (const myteams::Team::UserUuid &subscribedUserUuid : team.getSubscribedUsers()) {
			teamSubscriptionLines.push_back(joinEscapedFields({
				teamUuid,
				std::string(subscribedUserUuid.data())
			}, _delimiter));
		}

		for (const myteams::Channel &channel : team.getChannels()) {
			const std::string channelUuid(channel.getUuid());
			channelLines.push_back(joinEscapedFields({
				teamUuid,
				channelUuid,
				std::string(channel.getName()),
				std::string(channel.getDescription())
			}, _delimiter));

			for (const myteams::Thread &thread : channel.getThreads()) {
				const std::string threadUuid(thread.getUuid());
				threadLines.push_back(joinEscapedFields({
					channelUuid,
					threadUuid,
					std::string(thread.getAuthorUuid()),
					std::to_string(static_cast<long long>(thread.getCreatedAt())),
					std::string(thread.getTitle()),
					std::string(thread.getBody())
				}, _delimiter));

				for (const myteams::Message &message : thread.getReplies()) {
					messageLines.push_back(joinEscapedFields({
						threadUuid,
						std::string(message.getUuid()),
						std::string(message.getAuthorUuid()),
						std::to_string(static_cast<long long>(message.getCreatedAt())),
						std::string(message.getBody())
					}, _delimiter));
				}
			}
		}
	}

	bool saveSuccess = true;
	saveSuccess = writeLines(usersFilePath(), userLines) && saveSuccess;
	saveSuccess = writeLines(teamsFilePath(), teamLines) && saveSuccess;
	saveSuccess = writeLines(teamSubscriptionsFilePath(), teamSubscriptionLines) && saveSuccess;
	saveSuccess = writeLines(channelsFilePath(), channelLines) && saveSuccess;
	saveSuccess = writeLines(threadsFilePath(), threadLines) && saveSuccess;
	saveSuccess = writeLines(messagesFilePath(), messageLines) && saveSuccess;

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
