#include "StatsHandler.h"
#include "stdafx.h"
#include "Globals.h"
#include "Util/hash.h"
#include "H2MOD\Modules\Utils\Utils.h"
#include "libcurl/curl/curl.h"
#include "H2MOD/Modules/Console/ConsoleCommands.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <iostream>
#include "XLive/XNet/upnp.h"
#include "H2MOD/Modules/Startup/Startup.h"
#ifdef _DEBUG
#pragma comment(lib, "libcurl_a_debug.lib")
#else
#pragma comment(lib, "libcurl_a.lib")
#endif

StatsHandler::StatsHandler()
= default;
struct stringMe {
	char *ptr;
	size_t len;
};
static void init_string(struct stringMe *s) {
	s->len = 0;
	s->ptr = (char*)malloc(s->len + 1);
	if (s->ptr == NULL) {
		fprintf(stderr, "malloc() failed\n");
		exit(EXIT_FAILURE);
	}
	s->ptr[0] = '\0';
}

wchar_t* StatsHandler::getPlaylistFile()
{
	auto output = h2mod->GetAddress<wchar_t*>(0, 0x46ECC4);
	return output;
}
std::string StatsHandler::getChecksum()
{
	std::string output;
	if (!hashes::calc_file_md5(getPlaylistFile(), output))
		output = "";
	return output;
}
int StatsHandler::uploadPlaylist()
{
	wchar_t* playlist_file = getPlaylistFile();
	char* pFile = (char*)malloc(sizeof(char) * wcslen(playlist_file) + 1);
	wcstombs2(pFile, playlist_file, wcslen(playlist_file) + 1);
#if !_DEBUG
	LOG_INFO_GAME(pFile);
#endif
	std::string checksum = getChecksum();
	if (checksum == "")
	{
		LOG_ERROR_GAME(L"[H2MOD] playlistVerified failed to Hash Playlist file");
		return -1;
	}
#if !_DEBUG
	LOG_INFO_GAME(checksum);
#endif		

	CURL *curl;
	CURLcode result;
	curl_mime *form = NULL;
	curl_mimepart *field = NULL;
	struct curl_slist *headerlist = NULL;
	static const char buf[] = "Expect:";
	curl_global_init(CURL_GLOBAL_ALL);
	curl = curl_easy_init();
	if(!curl)
	{
		LOG_ERROR_GAME(L"[H2MOD] uploadPlaylist failed to init curl");
		return -1;
	}
	form = curl_mime_init(curl);
	field = curl_mime_addpart(form);
	curl_mime_name(field, "file");
	curl_mime_filedata(field, pFile);
	field = curl_mime_addpart(form);
	curl_mime_name(field, "Type");
	curl_mime_data(field, "PlaylistUpload", CURL_ZERO_TERMINATED);
	field = curl_mime_addpart(form);
	curl_mime_name(field, "Playlist_Checksum");
	curl_mime_data(field, checksum.c_str(), CURL_ZERO_TERMINATED);
	headerlist = curl_slist_append(headerlist, buf);

	curl_easy_setopt(curl, CURLOPT_URL, "https://www.halo2pc.com/test-pages/CartoStat/API/post.php");
	curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);
	result = curl_easy_perform(curl);
	if(result != CURLE_OK)
	{
		LOG_ERROR_GAME(L"[H2MOD]::playlistVerified failed to execute curl");
		//LOG_ERROR_GAME(curl_easy_strerror(result));
		return -1;
	}
	else
	{
		int response_code;
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
#if !_DEBUG
		LOG_INFO_GAME(std::to_string(response_code));
#endif
		return response_code;
	}
	return -1;
}



int StatsHandler::verifyPlaylist()
{
	std::string http_request_body = "https://www.halo2pc.com/test-pages/CartoStat/API/get.php?Type=PlaylistCheck&Playlist_Checksum=";
	wchar_t* playlist_file = getPlaylistFile();
#if _DEBUG
	LOG_INFO_GAME(L"[H2MOD] playlistVerified loaded playlist is {0}", playlist_file);
#endif
	std::string checksum = getChecksum();
	if (checksum == "")
	{
		LOG_ERROR_GAME(L"[H2MOD] playlistVerified failed to Hash Playlist file");
		return -1;
	}
	else
	{
#if _DEBUG
		LOG_INFO_GAME(checksum);
#endif		
		http_request_body.append(checksum);
#if _DEBUG
		LOG_INFO_GAME(http_request_body);
#endif
		char* Response;
		CURL *curl;
		CURLcode curlResult;
		CURLcode global_init = curl_global_init(CURL_GLOBAL_ALL);
		if(global_init != CURLE_OK)
		{
			LOG_INFO_GAME(L"[H2MOD]::playlistVerified failed to init curl");
			return -1;
		}
		curl = curl_easy_init();
		if(curl)
		{
			//Set the URL for the GET
			curl_easy_setopt(curl, CURLOPT_URL, http_request_body.c_str());
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
			curlResult = curl_easy_perform(curl);
			if(curlResult != CURLE_OK)
			{
				LOG_ERROR_GAME(L"[H2MOD]::playlistVerified failed to execute curl");
				//LOG_ERROR_GAME(curl_easy_strerror(curlResult));
				return -1;
			} else
			{
				int response_code;
				curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
#if _DEBUG
				LOG_INFO_GAME(std::to_string(response_code));
#endif
				return response_code;
			}
			curl_easy_cleanup(curl);
		}
	}
	return -1;
}

//Really should make structs for these but I don't want to take the time to learn how.
static const int baseOffset  = 0x4dc722;
static const int rtPCROffset = 0x4DD1EE;
static const int PCROffset   = 0x49F6B0;

char* StatsHandler::buildJSON()
{
	typedef rapidjson::GenericDocument<rapidjson::UTF16<> > WDocument;
	typedef rapidjson::GenericValue<rapidjson::UTF16<> > WValue;
	WDocument document;
	WValue value;
	document.SetObject();


	//Inital Properties
	rapidjson::Document::AllocatorType& allocator = document.GetAllocator();
	auto checksum = getChecksum();
	auto wchecksum = std::wstring(checksum.begin(), checksum.end());
	value.SetString(wchecksum.c_str(), allocator);
	document.AddMember(L"PlaylistChecksum", value, allocator);
	auto Scenario = h2mod->GetAddress<wchar_t*>(0, 0x4DC504);

	std::wstring ScenarioPath = std::wstring(Scenario);
	std::wstring Scen(ScenarioPath.substr(ScenarioPath.rfind('\\') + 1));

	value.SetString(Scen.c_str(), allocator);
	document.AddMember(L"Scenario", value, allocator);

	//Build the Variant Object
	WValue Variant(rapidjson::kObjectType);
	auto VariantName = h2mod->GetAddress<wchar_t*>(0, 0x4DC3D4);
	value.SetString(VariantName, allocator);
	Variant.AddMember(L"Name", value, allocator);
	BYTE VariantType = *h2mod->GetAddress<BYTE*>(0, 0x4dc414);
	Variant.AddMember(L"Type", VariantType, allocator);
	
	WValue VariantSettings(rapidjson::kObjectType);
	BYTE TeamPlay = *h2mod->GetAddress<BYTE*>(0, 0x992880);
	VariantSettings.AddMember(L"Team Play", TeamPlay, allocator);
	Variant.AddMember(L"Settings", VariantSettings, allocator);
	document.AddMember(L"Variant", Variant, allocator);
	
	//Build the Server Object
	WValue Server(rapidjson::kObjectType);
	unsigned long ServerXUID = *h2mod->GetAddress<unsigned long*>(0, 0x52FC50);
	value.SetString(std::to_wstring(ServerXUID).c_str(), allocator);
	Server.AddMember(L"XUID", value, allocator);
	//h2mod->get_local_player_name(0)
	auto ServerName = h2mod->GetAddress<wchar_t*>(0, 0x52FC88);
	value.SetString(ServerName, allocator);
	Server.AddMember(L"Name", value, allocator);
	document.AddMember(L"Server", Server, allocator);

	//Players
	WValue Players(rapidjson::kArrayType);
	for(auto i = 0; i < 16; i++)
	{
		int calcBaseOffset  = baseOffset + (i * 0x94);
		int calcRTPCROffset = rtPCROffset + (i * 0x36A);
		int calcPCROffset = 0;
		int EndgameIndex = 0;
		WValue Player(rapidjson::kObjectType);
		auto Gamertag = h2mod->GetAddress<wchar_t*>(0, calcBaseOffset + 0xA);
		for(auto j = 0; j < 16; j++)
		{
			auto tGamertag = h2mod->GetAddress<wchar_t*>(0, PCROffset + (i * 0x110));
			if(std::wstring(Gamertag) == std::wstring(tGamertag))
			{
				calcPCROffset = PCROffset + (i * 0x110);
				EndgameIndex = i;
				break;
			}
		}
		auto XUID = *h2mod->GetAddress<unsigned long*>(0, calcBaseOffset);
		if (XUID == 0) //Skip if it doesnt exists
			continue;

		auto PrimaryColor = *h2mod->GetAddress<BYTE*>(0, calcBaseOffset + 0x4A);
		auto SecondaryColor = *h2mod->GetAddress<BYTE*>(0, calcBaseOffset + 0x4B);
		auto PrimaryEmblem = *h2mod->GetAddress<BYTE*>(0, calcBaseOffset + 0x4C);
		auto SecondaryEmblem = *h2mod->GetAddress<BYTE*>(0, calcBaseOffset + 0x4D);
		auto PlayerModel = *h2mod->GetAddress<BYTE*>(0, calcBaseOffset + 0x4E);
		auto EmblemForeground = *h2mod->GetAddress<BYTE*>(0, calcBaseOffset + 0x4F);
		auto EmblemBackground = *h2mod->GetAddress<BYTE*>(0, calcBaseOffset + 0x50);
		auto EmblemToggle = *h2mod->GetAddress<BYTE*>(0, calcBaseOffset + 0x51);
		auto ClanDescripton = h2mod->GetAddress<wchar_t*>(0, calcBaseOffset + 0x5A);
		auto ClanTag = h2mod->GetAddress<wchar_t*>(0, calcBaseOffset + 0x7A);
		auto Team = *h2mod->GetAddress<BYTE*>(0, calcBaseOffset + 0x86);
		auto Handicap = *h2mod->GetAddress<BYTE*>(0, calcBaseOffset + 0x87);
		auto Rank = *h2mod->GetAddress<BYTE*>(0, calcBaseOffset + 0x88);
		auto Nameplate = *h2mod->GetAddress<BYTE*>(0, calcBaseOffset + 0x8B);
		
		auto Place = h2mod->GetAddress<wchar_t*>(0, calcPCROffset + 0xE0);
		auto Score = h2mod->GetAddress<wchar_t*>(0, calcPCROffset + 0x40);
		auto Kills = *h2mod->GetAddress<unsigned short*>(0, calcRTPCROffset);
		auto Assists = *h2mod->GetAddress<unsigned short*>(0, calcRTPCROffset + 0x2);
		auto Deaths = *h2mod->GetAddress<unsigned short*>(0, calcRTPCROffset + 0x4);
		auto Betrayals = *h2mod->GetAddress<unsigned short*>(0, calcRTPCROffset + 0x6);
		auto Suicides = *h2mod->GetAddress<unsigned short*>(0, calcRTPCROffset + 0x8);
		auto BestSpree = *h2mod->GetAddress<unsigned short*>(0, calcRTPCROffset + 0xA);
		auto TimeAlive = *h2mod->GetAddress<unsigned short*>(0, calcRTPCROffset + 0xC);
		auto ShotsFired = *h2mod->GetAddress<unsigned short*>(0, calcPCROffset + 0x84);
		auto ShotsHit = *h2mod->GetAddress<unsigned short*>(0, calcPCROffset + 0x88);
		auto HeadShots = *h2mod->GetAddress<unsigned short*>(0, calcPCROffset + 0x8C);

		auto FlagScores = *h2mod->GetAddress<unsigned short*>(0, calcRTPCROffset + 0xE);
		auto FlagSteals = *h2mod->GetAddress<unsigned short*>(0, calcRTPCROffset + 0x10);
		auto FlagSaves = *h2mod->GetAddress<unsigned short*>(0, calcRTPCROffset + 0x12);
		auto FlagUnk = *h2mod->GetAddress<unsigned short*>(0, calcRTPCROffset + 0x14);

		auto BombScores = *h2mod->GetAddress<unsigned short*>(0, calcRTPCROffset + 0x18);
		auto BombKills = *h2mod->GetAddress<unsigned short*>(0, calcRTPCROffset + 0x1A);
		auto BombGrabs = *h2mod->GetAddress<unsigned short*>(0, calcRTPCROffset + 0x1C);

		auto BallScore = *h2mod->GetAddress<unsigned short*>(0, calcRTPCROffset + 0x20);
		auto BallKills = *h2mod->GetAddress<unsigned short*>(0, calcRTPCROffset + 0x22);
		auto BallCarrierKills = *h2mod->GetAddress<unsigned short*>(0, calcRTPCROffset + 0x24);

		auto KingKillsAsKing = *h2mod->GetAddress<unsigned short*>(0, calcRTPCROffset + 0x26);
		auto KingKilledKings = *h2mod->GetAddress<unsigned short*>(0, calcRTPCROffset + 0x28);
		
		auto JuggKilledJuggs = *h2mod->GetAddress<unsigned short*>(0, calcRTPCROffset + 0x3C);
		auto JuggKillsAsJugg = *h2mod->GetAddress<unsigned short*>(0, calcRTPCROffset + 0x3E);
		auto JuggTime = *h2mod->GetAddress<unsigned short*>(0, calcRTPCROffset + 0x40);

		auto TerrTaken = *h2mod->GetAddress<unsigned short*>(0, calcRTPCROffset + 0x48);
		auto TerrLost = *h2mod->GetAddress<unsigned short*>(0, calcRTPCROffset + 0x46);

		value.SetInt(EndgameIndex);
		Player.AddMember(L"EndGameIndex", value, allocator);
		value.SetString(std::to_wstring(XUID).c_str(), allocator);
		Player.AddMember(L"XUID", value, allocator);
		value.SetString(Gamertag, allocator);
		Player.AddMember(L"Gamertag", value, allocator);
		Player.AddMember(L"PrimaryColor", PrimaryColor, allocator);
		Player.AddMember(L"SecondaryColor", SecondaryColor, allocator);
		Player.AddMember(L"PrimaryEmblem", PrimaryEmblem, allocator);
		Player.AddMember(L"SecondaryEmblem", SecondaryEmblem, allocator);
		Player.AddMember(L"PlayerModel", PlayerModel, allocator);
		Player.AddMember(L"EmblemForeground", EmblemForeground, allocator);
		Player.AddMember(L"EmblemBackground", EmblemBackground, allocator);
		Player.AddMember(L"EmblemToggle", EmblemToggle, allocator);
		value.SetString(ClanDescripton, allocator);
		Player.AddMember(L"ClanDescription", value, allocator);
		value.SetString(ClanTag, allocator);
		Player.AddMember(L"ClanTag", value, allocator);
		Player.AddMember(L"Team", Team, allocator);
		Player.AddMember(L"Handicap", Handicap, allocator);
		Player.AddMember(L"Rank", Rank, allocator);
		Player.AddMember(L"Nameplate", Nameplate, allocator);
		value.SetString(Place, allocator);
		Player.AddMember(L"Place", value, allocator);
		value.SetString(Score, allocator);
		Player.AddMember(L"Score", value, allocator);
		value.SetInt(Kills);
		Player.AddMember(L"Kills", Kills, allocator);
		value.SetInt(Assists);
		Player.AddMember(L"Assists", value, allocator);
		value.SetInt(Deaths);
		Player.AddMember(L"Deaths", value, allocator);
		value.SetInt(Betrayals);
		Player.AddMember(L"Betrayals", value, allocator);
		value.SetInt(Suicides);
		Player.AddMember(L"Suicides", value, allocator);
		value.SetInt(BestSpree);
		Player.AddMember(L"BestSpree", value, allocator);
		value.SetInt(TimeAlive);
		Player.AddMember(L"TimeAlive", value, allocator);
		value.SetInt(ShotsFired);
		Player.AddMember(L"ShotsFired", value, allocator);
		value.SetInt(ShotsHit);
		Player.AddMember(L"ShotsHit", value, allocator);
		value.SetInt(HeadShots);
		Player.AddMember(L"HeadShots", value, allocator);
		value.SetInt(FlagScores);
		Player.AddMember(L"FlagScores", value, allocator);
		value.SetInt(FlagSteals);
		Player.AddMember(L"FlagSteals", value, allocator);
		value.SetInt(FlagSaves);
		Player.AddMember(L"FlagSaves", value, allocator);
		value.SetInt(FlagUnk);
		Player.AddMember(L"FlagUnk", value, allocator);
		value.SetInt(BombScores);
		Player.AddMember(L"BombScores", value, allocator);
		value.SetInt(BombKills);
		Player.AddMember(L"BombKills", value, allocator);
		value.SetInt(BombGrabs);
		Player.AddMember(L"BombGrabs", value, allocator);
		value.SetInt(BallScore);
		Player.AddMember(L"BallScore", value, allocator);
		value.SetInt(BallKills);
		Player.AddMember(L"BallKills", value, allocator);
		value.SetInt(BallCarrierKills);
		Player.AddMember(L"BallCarrierKills", value, allocator);
		value.SetInt(KingKillsAsKing);
		Player.AddMember(L"KingKillsAsKing", value, allocator);
		value.SetInt(KingKilledKings);
		Player.AddMember(L"KingKilledKings", value, allocator);
		value.SetInt(JuggKilledJuggs);
		Player.AddMember(L"JuggKilledJuggs", value, allocator);
		value.SetInt(JuggKillsAsJugg);
		Player.AddMember(L"JuggKillsAsJugg", value, allocator);
		value.SetInt(JuggTime);
		Player.AddMember(L"JuggTime", value, allocator);
		value.SetInt(TerrTaken);
		Player.AddMember(L"TerrTaken", value, allocator);
		value.SetInt(TerrLost);
		Player.AddMember(L"TerrLost", value, allocator);
		
		WValue Medals(rapidjson::kArrayType);
		for(auto j = 0; j < 24; j++)
		{
			value.SetInt(*h2mod->GetAddress<unsigned short*>(0, calcRTPCROffset + 0x4A + (j * 2)));
			Medals.PushBack(value, allocator);
		}
		Player.AddMember(L"MedalData", Medals, allocator);

		WValue Weapons(rapidjson::kArrayType);
		for(auto j = 0; j < 36; j++)
		{
			WValue Weapon(rapidjson::kArrayType);
			for(auto k = 0; k < 6; k++)
			{
				value.SetInt(*h2mod->GetAddress<unsigned short*>(0, calcRTPCROffset + 0xDE + (j * 0x10) + (k * 2)));
				Weapon.PushBack(value, allocator);
			}
			Weapons.PushBack(Weapon, allocator);
		}
		Player.AddMember(L"WeaponData", Weapons, allocator);

		Players.PushBack(Player, allocator);
	}
	document.AddMember(L"Players", Players, allocator);


	//Export the file
	rapidjson::StringBuffer strbuf;
	rapidjson::Writer<rapidjson::StringBuffer, rapidjson::UTF16<>> writer(strbuf);
	document.Accept(writer);
	std::string json(strbuf.GetString(), strbuf.GetSize());

	wchar_t fileOutPath[1024];
	time_t timer;
	struct tm y2k = { 0 };
	double seconds;

	y2k.tm_hour = 0;   y2k.tm_min = 0; y2k.tm_sec = 0;
	y2k.tm_year = 100; y2k.tm_mon = 0; y2k.tm_mday = 1;

	time(&timer);
	seconds = difftime(timer, mktime(&y2k));
	wchar_t unix[100];

	swprintf(unix, 100, L"%.f", seconds);
	
	swprintf(fileOutPath, 1024, L"%wsstats\\%s-%s.json", H2ProcessFilePath, ServerName, unix);
	std::ofstream of(fileOutPath);
	of << json;
	if (!of.good()) throw std::runtime_error("Can't write the JSON string to the file!");
	return "";

	//rapidjson::Value array(rapidjson::kArrayType);
	//rapidjson::Document::AllocatorType& allocator = document.GetAllocator();
	
}
