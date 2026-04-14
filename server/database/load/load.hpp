#ifndef DATABASE_LOAD_HPP
#define DATABASE_LOAD_HPP

#ifdef _WIN32
#pragma once
#endif

#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>
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

} // namespace detail

class DatabaseLoader {
	public:
		explicit DatabaseLoader(std::filesystem::path baseDirectory = "server_data", char delimiter = '|');

		bool load(std::vector<myteams::User> &users, std::vector<myteams::Team> &teams, std::vector<myteams::Message> &messages) const;

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

#endif // DATABASE_LOAD_HPP
