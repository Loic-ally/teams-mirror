#ifndef DATABASE_LOAD_HPP
#define DATABASE_LOAD_HPP

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

struct LoadedThread {
	myteams::Thread thread;
	std::vector<myteams::Message> replies;
};

struct LoadedChannel {
	myteams::Channel channel;
	std::vector<LoadedThread> threads;
};

struct LoadedTeam {
	myteams::Team team;
	std::vector<std::string> subscribedUsers;
	std::vector<LoadedChannel> channels;
};

} // namespace detail

class DatabaseLoader {
	public:
		explicit DatabaseLoader(std::filesystem::path baseDirectory = "server_data", char delimiter = '|');

		bool load(std::vector<myteams::User> &users, std::vector<myteams::Team> &teams) const;

	private:
		std::filesystem::path usersFilePath() const;
		std::filesystem::path teamsFilePath() const;
		std::filesystem::path teamSubscriptionsFilePath() const;
		std::filesystem::path channelsFilePath() const;
		std::filesystem::path threadsFilePath() const;
		std::filesystem::path messagesFilePath() const;

		std::filesystem::path _baseDirectory;
		char _delimiter;
};

} // namespace server::database

#endif // DATABASE_LOAD_HPP
