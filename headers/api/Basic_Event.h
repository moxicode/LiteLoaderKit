#pragma once
#include <string>
#include <liteloader.h>
#include <functional>
#include <mc\mass.h>
using namespace std;
typedef unsigned long long xuid_t;
class ServerPlayer;
class Player;

class JoinEV {
public:
	ServerPlayer* Player;
	string IP;
	xuid_t xuid;
};

class LeftEV {
public:
	ServerPlayer* Player;
	xuid_t xuid;
};

class ChatEV {
public:
	ServerPlayer* Player;
	string msg;
};

class ChangeDimEV {
public:
	Player* Player;
};

class PlayerUseCmdEV {
public:
	Player* Player;
	string cmd;
	MCRESULT mc_result;
};

class ServerStartedEV {

};

class PlayerDeathEV {
public:
	ServerPlayer* Player;
};

namespace Event {
	LIAPI void addEventListener(function<void(JoinEV)> callback);
	LIAPI void addEventListener(function<void(LeftEV)> callback);
	LIAPI void addEventListener(function<void(ChatEV)> callback);
	LIAPI void addEventListener(function<void(ChangeDimEV)> callback);
	LIAPI void addEventListener(function<void(ServerStartedEV)> callback);
	LIAPI void addEventListener(function<void(PlayerUseCmdEV)> callback);
	LIAPI void addEventListener(function<void(PlayerDeathEV)> callback);
};