#include "pch.h"
#include <api/Basic_Event.h>
#include <api/commands.h>
#include "commad.h"
#include <stddef.h>
#include <iostream>
#include <string>
#include "BDS.h"
#include "Helper.h"
#include <string_view>
unique_ptr<KVDBImpl> db;
playerMap<string> ORIG_NAME;
unordered_map<string, string> CNAME;
void loadCNAME() {
	db = MakeKVDB(GetDataPath("LLhelper"), false);
	db->iter([](string_view k, string_view v) {
		if (!k._Starts_with("b_"))
			CNAME.emplace(k, v);
		return true;
		});
}
bool oncmd_gmode(CommandOrigin const& ori, CommandOutput& outp, CommandSelector<Player>& s, int mode) {
	auto res = s.results(ori);
	if (!Command::checkHasTargets(res, outp)) return false;
	for (auto i : res) {
		setPlayerGameType(i, mode);
	}
	return true;
}

enum class BANOP :int {
	ban = 1,
	unban = 2,
	banip = 3
};
enum class BANOP_LIST :int {
	list = 1
};

void LOWERSTRING(string& S) {
	for (auto& i : S) {
		i = tolower(i);
	}
}
void addBanEntry(string const& entry, time_t timediff) {
	time_t next = timediff == 0 ? 0 : (time(0) + timediff);
	db->put("b_" + entry, to_view(next));
}
optional<time_t> getBanEntry(string const& entry) {
	string val;
	if (db->get("b_" + entry, val)) return { *(time_t*)val.data() };
	return {};
}
void removeBanEntry(string const& entry) {
	db->del("b_" + entry);
}

bool onCMD_BanList(CommandOrigin const& ori, CommandOutput& outp, MyEnum<BANOP_LIST>) {
	db->iter([&](string_view key, string_view val)->bool {
		if (key._Starts_with("b_")) {
			string banned{ key.substr(2) };
			if (banned.find('.') != banned.npos) {
				outp.addMessage(banned + " " + std::to_string(*(time_t*)val.data()));
			}
			else {
				xuid_t xid;
				std::stringstream ss{ banned };
				ss >> xid;
				auto strname = XIDREG::id2str(xid);
				outp.addMessage(banned + " (" + (strname.set ? strname.val() : "") + ") " + std::to_string(*(time_t*)val.data()));
			}
		}
		return true;
		});
	return true;
}
bool onCMD_Ban(CommandOrigin const& ori, CommandOutput& outp, MyEnum<BANOP> op, string& entry, optional<int>& time) {
	LOWERSTRING(entry);
	switch (op.val)
	{
	case BANOP::banip: {
		addBanEntry(entry, time.set ? time.val() : 0);
		return true;
	}
	case BANOP::ban: {
		addBanEntry(S(XIDREG::str2id(entry).val()), time.set ? time.val() : 0);
		liteloader::runcmdA("kick", QUOTE(entry));
		outp.success("ban " + QUOTE(entry) + " success");
		return true;
	}
				   break;
	case BANOP::unban: {
		if (getBanEntry(entry).set) {
			removeBanEntry(entry);
			return true;
		}
		else {
			auto XID = XIDREG::str2id(entry);
			if (!XID.set) {
				outp.error("not banned");
				return false;
			}
			else {
				removeBanEntry(S(XID.val()));
				return true;
			}
		}
	}
					 break;
	default:
		break;
	}
	return false;
}

bool onCMD_skick(CommandOrigin const& ori, CommandOutput& outp, string& target) {
	string playername = target;
	vector<Player*> plist = liteloader::getAllPlayers();
	Player* A = nullptr;
	for (auto p : plist) {
		if (offPlayer::getRealName(p) == playername) {
			A = p;
			break;
		}
	}
	if(A){
		forceKick(A, "kick");
		outp.addMessage("kick success");
		return true;
	}
	else
	{
		//no player
	}
}

bool oncmd_vanish(CommandOrigin const& ori, CommandOutput& outp) {
	auto wp = MakeWP(ori).val();
	auto sp = MakeSP(ori);
	VarULong ul(ZigZag(wp->getUniqueID().id));
	WBStream ws;
	ws.apply(ul);
	MyPkt<14> pk{ ws };
	vector<Player*> plist = liteloader::getAllPlayers();
	for (auto p : plist) {
		if (p != sp) {
			auto spp = (ServerPlayer*)p;
			spp->sendNetworkPacket(pk);
		}
	}
	outp.addMessage("Successfully opened. When you want to cancel, please join the server again.");
	return true;
}

enum class CNAMEOP :int {
	set = 1,
	remove = 2
};

bool onCMD_CNAME(CommandOrigin const& ori, CommandOutput& p, MyEnum<CNAMEOP> op, string& src, optional<string>& name) {
		string playername = src;
		vector<Player*> plist = liteloader::getAllPlayers();
		Player* A = nullptr;
		for (auto p : plist) {
			if (offPlayer::getRealName(p) == playername) {
				A = p;
				break;
			}
		}
		if (A) {
			if (op == CNAMEOP::set) {
				auto& str = name.val();
				for (int i = 0; i < str.size(); ++i) {
					if (str[i] == '^') str[i] = '\n';
				}
				CNAME[src] = str;
				db->put(src, str);
				optional<WPlayer> aa = WPlayer(*(ServerPlayer*)A);
				if (!aa.set) {
					p.addMessage("Player not online!we will only save the custom name.");
					return false;
				}
				aa.val()->setName(str);
				ORIG_NAME[aa.val().v] = aa.val().getName();
				p.addMessage("Set success");
			}
			else {
				CNAME.erase(src);
				db->del(src);
				optional<WPlayer> aa = WPlayer(*(ServerPlayer*)A);
				if (!aa.set) {
					p.addMessage("Player not online!we will only delete the custom name.");
					return false;
				}
				aa.val()->setName(src);
				ORIG_NAME._map.erase(aa.val().v);
				p.addMessage("delete success");
			}
			return true;
		}
}


bool onCMD_Trans(CommandOrigin const& ori, CommandOutput& outp, CommandSelector<Player>& p, string& host, optional<int> port) {
	int P = port.set ? port.val() : 19132;
	auto res = p.results(ori);
	if (!Command::checkHasTargets(res, outp)) return false;
	WBStream ws;
	ws.apply(MCString(host), (unsigned short)P);
	MyPkt<0x55, false> trpk(ws);
	for (auto i : res) {
		((ServerPlayer*)i)->sendNetworkPacket(trpk);
	}
	return true;
}

static bool onReload(CommandOrigin const& ori, CommandOutput& outp) {
	loadCfg();
	return true;
}

void REGCMD() {
	loadCNAME();
	Event::addEventListener([](RegCmdEV e) {
		CMDREG::SetCommandRegistry(e.CMDRg);
		CEnum<BANOP> _1("banop", { "ban","unban","banip" });
		CEnum<BANOP_LIST> _2("banoplist", { "list" });
		CEnum<CNAMEOP> _3("cnameop", { "set","rm" });
		MakeCommand("gmode", "set your gametype", 1);
		CmdOverload(gmode, oncmd_gmode, "target", "mode");
		MakeCommand("ban", "blacklist", 1);
		CmdOverload(ban, onCMD_Ban, "op", "target", "time");
		CmdOverload(ban, onCMD_BanList, "list");
		MakeCommand("transfer", "transfer player to another server", 1);
		CmdOverload(transfer, onCMD_Trans, "target", "host", "port");
		MakeCommand("hreload", "reload cmdhelper", 1);
		CmdOverload(hreload, onReload);
		MakeCommand("skick", "force kick", 1);
		CmdOverload(skick, onCMD_skick, "target");
		MakeCommand("cname", "custom name", 1);
		CmdOverload(cname, onCMD_CNAME, "op", "target", "name");
		MakeCommand("vanish", "hide yourself", 1);
		CmdOverload(vanish, oncmd_vanish);
		});
}

THook(void, "?_onClientAuthenticated@ServerNetworkHandler@@AEAAXAEBVNetworkIdentifier@@AEBVCertificate@@@Z", void* snh, NetworkIdentifier& neti, Certificate& cert) {
	original(snh, neti, cert);
	auto xuid = ExtendedCertificate::getXuid(cert);
	auto be1 = getBanEntry(xuid);
	auto IP = liteloader::getIP(neti);
	auto be2 = getBanEntry(IP);
	auto nh = mc->getServerNetworkHandler();
	if (be1.set) {
		if (be1.val() != 0 && be1.val() < time(0)) {
			removeBanEntry(xuid);
		}
		else {
			nh->onDisconnect(neti);
		}
	}
	if (be2.set) {
		if (be2.val() != 0 && be2.val() < time(0)) {
			removeBanEntry(IP);
		}
		else {
			nh->onDisconnect(neti);
		}
	}
}

