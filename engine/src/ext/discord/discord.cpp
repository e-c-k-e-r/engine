#include <uf/ext/discord/discord.h>
#define _CRT_SECURE_NO_WARNINGS

#include <array>
#include <cassert>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <vector>

#include <uf/utils/hook/hook.h>
#include <uf/utils/serialize/serializer.h>

#if defined(_WIN32)
#pragma pack(push, 1)
struct BitmapImageHeader {
	uint32_t const structSize{sizeof(BitmapImageHeader)};
	int32_t width{0};
	int32_t height{0};
	uint16_t const planes{1};
	uint16_t const bpp{32};
	uint32_t const pad0{0};
	uint32_t const pad1{0};
	uint32_t const hres{2835};
	uint32_t const vres{2835};
	uint32_t const pad4{0};
	uint32_t const pad5{0};

	BitmapImageHeader& operator=(BitmapImageHeader const&) = delete;
};

struct BitmapFileHeader {
	uint8_t const magic0{'B'};
	uint8_t const magic1{'M'};
	uint32_t size{0};
	uint32_t const pad{0};
	uint32_t const offset{sizeof(BitmapFileHeader) + sizeof(BitmapImageHeader)};

	BitmapFileHeader& operator=(BitmapFileHeader const&) = delete;
};
#pragma pack(pop)
#endif

struct DiscordState {
	::discord::User currentUser;

	std::unique_ptr<::discord::Core> core;
};

namespace {
	volatile bool interrupted{false};
	DiscordState state{};
}

void UF_API ext::discord::initialize(){

	::discord::Core* core{};
	auto result = ::discord::Core::Create(600081027943366656, DiscordCreateFlags_Default, &core);
	state.core.reset(core);
	if (!state.core) {
		std::cout << "Failed to instantiate discord core! (err " << static_cast<int>(result) << ")\n";
		std::exit(-1);
	}

	state.core->SetLogHook(
		::discord::LogLevel::Debug, [](::discord::LogLevel level, const char* message) {
			std::cerr << "Log(" << static_cast<uint32_t>(level) << "): " << message << "\n";
		}
	);
/*
	core->UserManager().OnCurrentUserUpdate.Connect([&state]() {
		state.core->UserManager().GetCurrentUser(&state.currentUser);

		std::cout << "Current user updated: " << state.currentUser.GetUsername() << "#" << state.currentUser.GetDiscriminator() << "\n";

		state.core->UserManager().GetUser(170271159441424384, [](::discord::Result result, ::discord::User const& user) {
			if (result == ::discord::Result::Ok) {
				std::cout << "Get " << user.GetUsername() << "\n";
			}
			else {
				std::cout << "Failed to get user!\n";
			}
		});

		::discord::ImageHandle handle{};
		handle.SetId(state.currentUser.GetId());
		handle.SetType(::discord::ImageType::User);
		handle.SetSize(256);

		state.core->ImageManager().Fetch( handle, true, [&state](::discord::Result res, ::discord::ImageHandle handle) {
			if (res == ::discord::Result::Ok) {
				::discord::ImageDimensions dims{};
				state.core->ImageManager().GetDimensions(handle, &dims);
				std::cout << "Fetched " << dims.GetWidth() << "x" << dims.GetHeight() << " avatar!\n";
				std::vector<uint8_t> data;
				data.reserve(dims.GetWidth() * dims.GetHeight() * 4);
				uint8_t* d = data.data();
				state.core->ImageManager().GetData(handle, d, data.size());
#if defined(_WIN32)
				auto fileSize = data.size() + sizeof(BitmapImageHeader) + sizeof(BitmapFileHeader);

				BitmapImageHeader imageHeader;
				imageHeader.width = static_cast<int32_t>(dims.GetWidth());
				imageHeader.height = static_cast<int32_t>(dims.GetHeight());

				BitmapFileHeader fileHeader;
				fileHeader.size = static_cast<uint32_t>(fileSize);

				FILE* fp = fopen("avatar.bmp", "wb");
				fwrite(&fileHeader, sizeof(BitmapFileHeader), 1, fp);
				fwrite(&imageHeader, sizeof(BitmapImageHeader), 1, fp);

				for (auto y = 0u; y < dims.GetHeight(); ++y) {
				  auto pixels = reinterpret_cast<uint32_t const*>(data.data());
				  auto invY = dims.GetHeight() - y - 1;
				  fwrite(
					&pixels[invY * dims.GetWidth()], sizeof(uint32_t) * dims.GetWidth(), 1, fp);
				}

				fflush(fp);
				fclose(fp);
#endif
			} else {
				std::cout << "Failed fetching avatar. (err " << static_cast<int>(res) << ")\n";
			}
		});
	});
*/
/*
	state.core->ActivityManager().RegisterCommand("run/command/foo/bar/baz/here.exe");
	state.core->ActivityManager().RegisterSteam(123123321);
*/
/*
	state.core->ActivityManager().OnActivityJoin.Connect( [](const char* secret) {
		std::cout << "Join " << secret << "\n";
	});
	state.core->ActivityManager().OnActivitySpectate.Connect( [](const char* secret) {
		std::cout << "Spectate " << secret << "\n";
	});
	state.core->ActivityManager().OnActivityJoinRequest.Connect([](::discord::User const& user) {
		std::cout << "Join Request " << user.GetUsername() << "\n";
	});
	state.core->ActivityManager().OnActivityInvite.Connect( [](::discord::ActivityActionType, ::discord::User const& user, ::discord::Activity const&) {
		std::cout << "Invite " << user.GetUsername() << "\n";
	});

	state.core->LobbyManager().OnLobbyUpdate.Connect( [](std::int64_t lobbyId) {
		std::cout << "Lobby update " << lobbyId << "\n";
	});

	state.core->LobbyManager().OnLobbyDelete.Connect( [](std::int64_t lobbyId, std::uint32_t reason) {
		std::cout << "Lobby delete " << lobbyId << " (reason: " << reason << ")\n";
	});

	state.core->LobbyManager().OnMemberConnect.Connect( [](std::int64_t lobbyId, std::int64_t userId) {
		std::cout << "Lobby member connect " << lobbyId << " userId " << userId << "\n";
	});

	state.core->LobbyManager().OnMemberUpdate.Connect( [](std::int64_t lobbyId, std::int64_t userId) {
		std::cout << "Lobby member update " << lobbyId << " userId " << userId << "\n";
	});

	state.core->LobbyManager().OnMemberDisconnect.Connect( [](std::int64_t lobbyId, std::int64_t userId) {
		std::cout << "Lobby member disconnect " << lobbyId << " userId " << userId << "\n";
	});

	state.core->LobbyManager().OnLobbyMessage.Connect([&](std::int64_t lobbyId,
														  std::int64_t userId,
														  std::uint8_t* payload,
														  std::uint32_t payloadLength) {
		std::vector<uint8_t> buffer{};
		buffer.resize(payloadLength);
		memcpy(buffer.data(), payload, payloadLength);
		std::cout << "Lobby message " << lobbyId << " from " << userId << " of length "
				  << payloadLength << " bytes.\n";

		char fourtyNinetySix[4096];
		state.core->LobbyManager().GetLobbyMetadataValue(lobbyId, "foo", fourtyNinetySix);

		std::cout << "Metadata for key foo is " << fourtyNinetySix << "\n";
	});
*/
/*
	state.core->LobbyManager().OnSpeaking.Connect( [&](std::int64_t, std::int64_t userId, bool speaking) {
		std::cout << "User " << userId << " is " << (speaking ? "" : "NOT ") << "speaking.\n";
	});
*/



	uf::hooks.addHook( "discord:Activity.Update", [&](const std::string& event)->std::string{
		static uf::Serializer canonical; if ( canonical["details"].isNull() ) {
			canonical["large image"] = "icon";
			canonical["large text"] = "Playing";
		}
		uf::Serializer json = event;

		{
			if ( json["details"].isString() ) canonical["details"] = json["details"];
			if ( json["state"].isString() ) canonical["state"] = json["state"];
			if ( json["small image"].isString() ) canonical["small image"] = json["small image"];
			if ( json["small text"].isString() ) canonical["small text"] = json["small text"];
			if ( json["large image"].isString() ) canonical["large image"] = json["large image"];
			if ( json["large text"].isString() ) canonical["large text"] = json["large text"];
		}

		::discord::Activity activity{};
		if ( canonical["details"].isString() ) activity.SetDetails(canonical["details"].asCString());
		if ( canonical["state"].isString() ) activity.SetState(canonical["state"].asCString());
		if ( canonical["small image"].isString() ) activity.GetAssets().SetSmallImage(canonical["small image"].asCString());
		if ( canonical["small text"].isString() ) activity.GetAssets().SetSmallText(canonical["small text"].asCString());
		if ( canonical["large image"].isString() ) activity.GetAssets().SetLargeImage(canonical["large image"].asCString());
		if ( canonical["large text"].isString() ) activity.GetAssets().SetLargeText(canonical["large text"].asCString());
		activity.SetType(::discord::ActivityType::Playing);
		state.core->ActivityManager().UpdateActivity(activity, [](::discord::Result result) {
			std::cout << ((result == ::discord::Result::Ok) ? "Succeeded" : "Failed") << " updating activity!\n";
		});

		return "true";
	});


/*
	::discord::LobbyTransaction lobby{};
	state.core->LobbyManager().GetLobbyCreateTransaction(&lobby);
	lobby.SetCapacity(2);
	lobby.SetMetadata("foo", "bar");
	lobby.SetMetadata("baz", "bat");
	lobby.SetType(::discord::LobbyType::Public);
	state.core->LobbyManager().CreateLobby(
	  lobby, [&state](::discord::Result result, ::discord::Lobby const& lobby) {
		  if (result == ::discord::Result::Ok) {
			  std::cout << "Created lobby with secret " << lobby.GetSecret() << "\n";
			  std::array<uint8_t, 234> data{};
			  state.core->LobbyManager().SendLobbyMessage(
				lobby.GetId(),
				reinterpret_cast<uint8_t*>(data.data()),
				data.size(),
				[](::discord::Result result) {
					std::cout << "Sent message. Result: " << static_cast<int>(result) << "\n";
				});
		  }
		  else {
			  std::cout << "Failed creating lobby. (err " << static_cast<int>(result) << ")\n";
		  }

		  ::discord::LobbySearchQuery query{};
		  state.core->LobbyManager().GetSearchQuery(&query);
		  query.Limit(1);
		  state.core->LobbyManager().Search(query, [&state](::discord::Result result) {
			  if (result == ::discord::Result::Ok) {
				  std::int32_t lobbyCount{};
				  state.core->LobbyManager().LobbyCount(&lobbyCount);
				  std::cout << "Lobby search succeeded with " << lobbyCount << " lobbies.\n";
				  for (auto i = 0; i < lobbyCount; ++i) {
					  ::discord::LobbyId lobbyId{};
					  state.core->LobbyManager().GetLobbyId(i, &lobbyId);
					  std::cout << "  " << lobbyId << "\n";
				  }
			  }
			  else {
				  std::cout << "Lobby search failed. (err " << static_cast<int>(result) << ")\n";
			  }
		  });
	  });
*/
/*
	state.core->RelationshipManager().OnRefresh.Connect([&]() {
		std::cout << "Relationships refreshed!\n";

		state.core->RelationshipManager().Filter( [](::discord::Relationship const& relationship) -> bool {
			return relationship.GetType() == ::discord::RelationshipType::Friend;
		});

		std::int32_t friendCount{0};
		state.core->RelationshipManager().Count(&friendCount);

		state.core->RelationshipManager().Filter( [&](::discord::Relationship const& relationship) -> bool {
			return relationship.GetType() == ::discord::RelationshipType::Friend && relationship.GetUser().GetId() < state.currentUser.GetId();
		});

		std::int32_t filteredCount{0};
		state.core->RelationshipManager().Count(&filteredCount);

		::discord::Relationship relationship{};
		for (auto i = 0; i < filteredCount; ++i) {
			state.core->RelationshipManager().GetAt(i, &relationship);
			std::cout << relationship.GetUser().GetId() << " "
					  << relationship.GetUser().GetUsername() << "#"
					  << relationship.GetUser().GetDiscriminator() << "\n";
		}
	});

	state.core->RelationshipManager().OnRelationshipUpdate.Connect( [](::discord::Relationship const& relationship) {
		std::cout << "Relationship with " << relationship.GetUser().GetUsername() << " updated!\n";
	});
*/
}

void UF_API ext::discord::tick() {
	state.core->RunCallbacks();
}


/*
#include <cstdio>
#include <iostream>
#include <assert.h>

#define DISCORD_REQUIRE(x) assert(x == DiscordResult_Ok)
namespace {
	struct Application {
		struct IDiscordCore* core;
		struct IDiscordUserManager* users;
		struct IDiscordAchievementManager* achievements;
		struct IDiscordActivityManager* activities;
		struct IDiscordRelationshipManager* relationships;
		struct IDiscordApplicationManager* application;
		struct IDiscordLobbyManager* lobbies;
		DiscordUserId user_id;
	};

	void UpdateActivityCallback(void* data, enum EDiscordResult result) {
		DISCORD_REQUIRE(result);
	}

	bool RelationshipPassFilter(void* data, struct DiscordRelationship* relationship) {
		return (relationship->type == DiscordRelationshipType_Friend);
	}

	bool RelationshipSnowflakeFilter(void* data, struct DiscordRelationship* relationship) {
		struct Application* app = (struct Application*)data;
		return (relationship->type == DiscordRelationshipType_Friend && relationship->user.id < app->user_id);
	}

	void OnRelationshipsRefresh(void* data) {
		struct Application* app = (struct Application*)data;
		struct IDiscordRelationshipManager* module = app->relationships;

		module->filter(module, app, RelationshipPassFilter);

		int32_t unfiltered_count = 0;
		DISCORD_REQUIRE(module->count(module, &unfiltered_count));

		module->filter(module, app, RelationshipSnowflakeFilter);

		int32_t filtered_count = 0;
		DISCORD_REQUIRE(module->count(module, &filtered_count));

		printf("=== Cool Friends ===\n");
		for (int32_t i = 0; i < filtered_count; i += 1) {
			struct DiscordRelationship relationship;
			DISCORD_REQUIRE(module->get_at(module, i, &relationship));

			printf("%lld %s#%s\n",
				   relationship.user.id,
				   relationship.user.username,
				   relationship.user.discriminator);
		}
		printf("(%d friends less cool than you omitted)\n", unfiltered_count - filtered_count);

		struct DiscordActivity activity;
		memset(&activity, 0, sizeof(activity));
		sprintf(activity.details, "Cooler than %d friends", unfiltered_count - filtered_count);
		sprintf(activity.state, "%d friends total", unfiltered_count);

		app->activities->update_activity(app->activities, &activity, app, UpdateActivityCallback);
	}

	void OnUserUpdated(void* data) {
		struct Application* app = (struct Application*)data;
		struct DiscordUser user;
		app->users->get_current_user(app->users, &user);
		app->user_id = user.id;
	}

	void OnOAuth2Token(void* data, enum EDiscordResult result, struct DiscordOAuth2Token* token) {
		if (result == DiscordResult_Ok) {
			printf("OAuth2 token: %s\n", token->access_token);
		} else {
			printf("GetOAuth2Token failed with %d\n", (int)result);
		}
	}

	void OnLobbyConnect(void* data, enum EDiscordResult result, struct DiscordLobby* lobby) {
		printf("LobbyConnect returned %d\n", (int)result);
	}
}

namespace {
	struct Application app;
}

void UF_API ext::::discord::initialize() {
	memset(&app, 0, sizeof(app));

	struct IDiscordUserEvents users_events;
	memset(&users_events, 0, sizeof(users_events));
	users_events.on_current_user_update = OnUserUpdated;

	struct IDiscordActivityEvents activities_events;
	memset(&activities_events, 0, sizeof(activities_events));

	struct IDiscordRelationshipEvents relationships_events;
	memset(&relationships_events, 0, sizeof(relationships_events));
	relationships_events.on_refresh = OnRelationshipsRefresh;

	struct DiscordCreateParams params;
	DiscordCreateParamsSetDefault(&params);
	params.client_id = 600081027943366656;
	params.flags = DiscordCreateFlags_Default;
	params.event_data = &app;
	params.activity_events = &activities_events;
	params.relationship_events = &relationships_events;
	params.user_events = &users_events;
	DISCORD_REQUIRE(DiscordCreate(DISCORD_VERSION, &params, &app.core));

	app.users = app.core->get_user_manager(app.core);
	app.achievements = app.core->get_achievement_manager(app.core);
	app.activities = app.core->get_activity_manager(app.core);
	app.application = app.core->get_application_manager(app.core);
	app.lobbies = app.core->get_lobby_manager(app.core);

	app.lobbies->connect_lobby_with_activity_secret( app.lobbies, "X4QbKBKWUfU93wtxNasxNi4R3NigTVYQ", &app, OnLobbyConnect);

	app.application->get_oauth2_token(app.application, &app, OnOAuth2Token);

	DiscordBranch branch;
	app.application->get_current_branch(app.application, &branch);
	printf("Current branch %s\n", branch);

	app.relationships = app.core->get_relationship_manager(app.core);
}

void UF_API ext::::discord::tick() {
	DISCORD_REQUIRE(app.core->run_callbacks(app.core));
}
*/