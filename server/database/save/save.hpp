#ifndef DATABASE_SAVE_HPP
#define DATABASE_SAVE_HPP

#ifdef _WIN32
#pragma once
#endif

#include <filesystem>
#include <string>
#include <vector>

#include "server/models/team/team.hpp"
#include "server/models/user/user.hpp"

namespace server::database {

namespace detail {

struct SerializedLines {
	std::vector<std::string> users;
	std::vector<std::string> teams;
	std::vector<std::string> teamSubscriptions;
	std::vector<std::string> channels;
	std::vector<std::string> threads;
	std::vector<std::string> messages;
	std::vector<std::string> privateMessages;
};

} // namespace detail

class DatabaseSaver {
	public:
		explicit DatabaseSaver(std::filesystem::path baseDirectory = "server_data", char delimiter = '|');

		bool save(const std::vector<myteams::User> &users, const std::vector<myteams::Team> &teams, std::vector<myteams::Message> &messages) const;

	private:
		std::filesystem::path usersFilePath() const;
		std::filesystem::path teamsFilePath() const;
		std::filesystem::path teamSubscriptionsFilePath() const;
		std::filesystem::path channelsFilePath() const;
		std::filesystem::path threadsFilePath() const;
		std::filesystem::path messagesFilePath() const;
        std::filesystem::path privateMessagesFilePath() const;

		std::filesystem::path _baseDirectory;
		char _delimiter;
};

} // namespace server::database

#endif // DATABASE_SAVE_HPP
