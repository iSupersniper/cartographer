#include "stdafx.h"
#include "H2MOD.h"

#include "Blam/Engine/FileSystem/FiloInterface.h"
#include "H2MOD/Discord/DiscordInterface.h"
#include "H2MOD/Modules/Config/Config.h"
#include "H2MOD/Modules/HitFix/HitFix.h"
#include "H2MOD/Modules/Input/Mouseinput.h"
#include "H2MOD/Modules/MainMenu/Ranks.h"
#include "H2MOD/Modules/MapFix/MPMapFix.h"
#include "H2MOD/Modules/Networking/Memory/bitstream.h"
#include "H2MOD/Modules/OnScreenDebug/OnscreenDebug.h"
#include "H2MOD/Modules/ServerConsole/ServerConsole.h"
#include "H2MOD/Modules/Tweaks/Tweaks.h"
#include "H2MOD/Tags/global_tags_interface.h"
#include "H2MOD/Variants/GunGame/GunGame.h"
#include "H2MOD/Variants/H2X/H2X.h"
#include "H2MOD/Tags/MetaLoader/tag_loader.h"

H2MOD* h2mod = new H2MOD();
GunGame* gunGame = new GunGame();
Halo2Final* h2f = new Halo2Final();
DeviceShop* device_shop = new DeviceShop();
XboxTick* xboxTickHandler = new XboxTick();
Infection* infectionHandler = new Infection();
FireFight* fireFightHandler = new FireFight();
HeadHunter* headHunterHandler = new HeadHunter();
VariantPlayer* variant_player = new VariantPlayer();

extern int H2GetInstanceId();
extern XUID xFakeXuid[4];
std::unordered_map<int, int> object_to_variant;

bool b_H2X = false;
bool b_GunGame = false;
bool b_FireFight = false;
bool b_XboxTick = false;
bool b_Infection = false;
bool b_Halo2Final = false;
bool b_HeadHunter = false;

std::unordered_map<wchar_t*, bool&> GametypesMap
{
	{ L"h2x", b_H2X },
	{ L"ogh2", b_XboxTick },
	{ L"h2f", b_Halo2Final },
	{ L"gungame", b_GunGame },
	{ L"zombies", b_Infection },
	{ L"infection", b_Infection },
	{ L"wareconomy", b_FireFight },
	{ L"headhunter", b_HeadHunter },
	{ L"graverobber", b_HeadHunter }
};

int GAME_BUILD = 11122;
int EXECUTABLE_VERSION = 4;

int character_datum_from_index(BYTE index)
{
	DWORD global_scnr = *(DWORD*)(*h2mod->GetAddress<DWORD*>(0x479E74) + 0x17C);
	DWORD var2 = *h2mod->GetAddress<DWORD*>(0x482290);
	DWORD var3 = var2 + global_scnr;
	DWORD var4 = var3 + (8 * index) + 4;
	DWORD char_tag_datum = *(DWORD*)var4;

	return char_tag_datum;
}

int get_player_index_from_datum(DatumIndex unit_datum)
{
	DatumIterator<ObjectHeader> objectsIt(game_state_objects_header);
	BipedObjectDefinition *unit_object = (BipedObjectDefinition*)objectsIt.get_data_at_index(unit_datum.ToAbsoluteIndex())->object;
	return unit_object->PlayerDatum.ToAbsoluteIndex();
}

enum game_lobby_states : int
{
	not_in_lobby,
	in_lobby,
	unk1,
	in_game,
	unk2,
	joining_lobby
};

game_lobby_states call_get_lobby_state()
{
	typedef game_lobby_states(__cdecl get_lobby_state)();
	auto p_get_lobby_state = h2mod->GetAddress<get_lobby_state*>(0x1AD660);

	return p_get_lobby_state();
}

#pragma region engine calls

int __cdecl call_get_object(DatumIndex object_datum_index, int object_type)
{
	//LOG_TRACE_GAME("call_get_object( object_datum_index: %08X, object_type: %08X )", object_datum_index, object_type);

	typedef int(__cdecl get_object)(DatumIndex object_datum_index, int object_type);
	auto p_get_object = h2mod->GetAddress<get_object*>(0x1304E3, 0x11F3A6);
	return p_get_object(object_datum_index, object_type);
}

int __cdecl call_unit_reset_equipment(DatumIndex unit_datum_index)
{
	//LOG_TRACE_GAME("unit_reset_equipment(unit_datum_index: %08X)", unit_datum_index);
	typedef int(__cdecl unit_reset_equipment)(DatumIndex unit_datum_index);
	auto p_unit_reset_equipment = h2mod->GetAddress<unit_reset_equipment*>(0x1441E0, 0x133030);
	if (unit_datum_index != -1 && unit_datum_index != 0)
	{
		return p_unit_reset_equipment(unit_datum_index);
	}

	return 0;
}

void __cdecl call_hs_object_destroy(DatumIndex object_datum_index)
{
	//LOG_TRACE_GAME("hs_object_destory(object_datum_index: %08X)", object_datum_index);
	typedef void(__cdecl hs_object_destroy)(DatumIndex object_datum_index);
	auto p_hs_object_destroy = h2mod->GetAddress<hs_object_destroy*>(0xFDCFD, 0x124ED5); // update dedi

	return p_hs_object_destroy(object_datum_index);
}

signed int __cdecl call_unit_inventory_next_weapon(unsigned short unit)
{
	//LOG_TRACE_GAME("unit_inventory_next_weapon(unit: %08X)", unit);
	typedef signed int(__cdecl unit_inventory_next_weapon)(unsigned short unit);
	auto p_unit_inventory_next_weapon = h2mod->GetAddress<unit_inventory_next_weapon*>(0x139E04);

	return p_unit_inventory_next_weapon(unit);
}

bool __cdecl call_assign_equipment_to_unit(DatumIndex unit, int object_index, short unk)
{
	//LOG_TRACE_GAME("assign_equipment_to_unit(unit: %08X,object_index: %08X,unk: %08X)", unit, object_index, unk);
	typedef bool(__cdecl assign_equipment_to_unit)(DatumIndex unit, int object_index, short unk);
	auto passign_equipment_to_unit = h2mod->GetAddress<assign_equipment_to_unit*>(0x1442AA, 0x1330FA);

	return passign_equipment_to_unit(unit, object_index, unk);
}

void __cdecl call_object_placement_data_new(ObjectPlacementData* s_object_placement_data, DatumIndex object_definition_index, DatumIndex object_owner, int unk)
{
	//LOG_TRACE_GAME("object_placement_data_new(s_object_placement_data: %08X,",s_object_placement_data);
	//LOG_TRACE_GAME("object_definition_index: %08X, object_owner: %08X, unk: %08X)", object_definition_index, object_owner, unk);

	typedef void(__cdecl object_placement_data_new)(void*, DatumIndex, DatumIndex, int);
	auto pobject_placement_data_new = h2mod->GetAddress<object_placement_data_new*>(0x132163, 0x121033);

	pobject_placement_data_new(s_object_placement_data, object_definition_index, object_owner, unk);
}

/*int __cdecl call_entity_datum_to_gamestate_datum(int entity_datum)
{
	typedef int(__cdecl entity_datum_to_gamestate_datum)(int entity_datum);
	auto p_entity_datum_to_gamestate_datum = h2mod->GetAddress<entity_datum_to_gamestate_datum>(0x1F2211);

	return pentity_datum_to_gamestate_datum(entity_datum);
}*/

signed int __cdecl call_object_new(ObjectPlacementData* pObject)
{
	//LOG_TRACE_GAME("object_new(pObject: %08X)", pObject);

	typedef int(__cdecl object_new)(void*);
	auto p_object_new = h2mod->GetAddress<object_new*>(0x136CA7, 0x125B77);

	return p_object_new(pObject);

}

bool __cdecl call_add_object_to_sync(DatumIndex gamestate_object_datum)
{
	typedef int(__cdecl add_object_to_sync)(DatumIndex gamestate_object_datum);
	auto p_add_object_to_sync = h2mod->GetAddress<add_object_to_sync*>(0x1B8D14, 0x1B2C44);

	return p_add_object_to_sync(gamestate_object_datum);
}

/* We should really make this stuff into a struct/class, and access it that way it'd be much cleaner... */
int get_actor_datum_from_unit_datum(int unit_datum)
{
	int unit_ptr = call_get_object(unit_datum, 3);
	if (unit_ptr)
	{
		return *(int*)((BYTE*)unit_ptr + 0x130);
	}

	return -1;
}

/* This looks at the actors table to get the character datum which is assigned to the specific actor. */
int get_char_datum_from_actor(int actor_datum)
{
	__int16 actor_index = actor_datum & 0xFFFF;
	DWORD actor_table_ptr = *h2mod->GetAddress<DWORD*>(0xA965DC);
	DWORD actor_table = *(DWORD*)((BYTE*)actor_table_ptr + 0x44);
	DWORD actor = (DWORD)((BYTE*)actor_table + (actor_index * 0x898));
	int character_datum = *(int*)((BYTE*)actor+0x54);

	return character_datum;
}

/*This is to get the datum of the last player who damaged the datum/unit provided */
int get_damage_owner(int damaged_unit_index)
{
	int damaged_player_ptr = call_get_object(damaged_unit_index, 3);
	if (damaged_player_ptr)
	{
		return *(int*)((BYTE*)damaged_player_ptr + 0xC8); // player_ptr/unit_ptr + 0xC8 = damaging player this works on vehicles/AI and such too.
	}

	return -1;
}

int __cdecl call_get_character_sid_from_datum(int char_datum)
{
	typedef int(__cdecl get_character_sid_from_datum)(int datum);
	auto p_get_character_sid_from_datum = h2mod->GetAddress<get_character_sid_from_datum*>(0x31796C);

	return p_get_character_sid_from_datum(char_datum);
}

int __cdecl call_fill_creation_data_from_object_index(int object_index, void* creation_data)
{
	typedef int(__cdecl fill_creation_data_from_object_index)(int datum,void* creation_data);
	auto p_fill_creation_data_from_object_index = h2mod->GetAddress<fill_creation_data_from_object_index*>(0x1F24ED);

	return p_fill_creation_data_from_object_index(object_index, creation_data);
}

signed int __cdecl object_new_hook(ObjectPlacementData* new_object)
{
	int variant_index = *(int*)((char*)new_object + 0xC);
	signed int result = call_object_new(new_object);

	//unsigned __int16 object_index = result & 0xFFFF;

	object_to_variant[result] = variant_index;

	//wchar_t DebugText[255] = { 0 };
	//SecureZeroMemory(DebugText, sizeof(DebugText));
	//wsprintf(DebugText, L"AI object_new hook - object_index: %08X - variant_index: %08X - datum: %08X", result, variant_index);

	//LOG_TRACE_GAME_N("AI object_new hook - object_index: %08X - variant_index: %08X - datum: %08X", result, variant_index);

	//addDebugText(DebugText);

	return result;
}

typedef void(__stdcall *tc_simulation_unit_entity_definition_creation_encode)(void* thisptr, int creation_data_size, void* creation_data, int a3, void* packet);
tc_simulation_unit_entity_definition_creation_encode pc_simulation_unit_entity_definition_encode;

void __stdcall c_simulation_unit_entity_definition_creation_encode(void *thisptr, int creation_data_size, void* creation_data, int a3, void* packet)
{
	//LOG_TRACE_GAME_N("c_simulation_unit_entity_definition_creation_encode()\r\nthisptr: %08X, creation_data_size: %i, creation_data: %08X, a3: %i, packet: %08X", thisptr, creation_data_size, creation_data, a3, packet);
	int object_permutation_index = *(int*)((char*)creation_data + 0x24);
	if (object_permutation_index != -1)
	{
		//LOG_TRACE_GAME_N("creation_data+0x24: %08X", object_permutation_index);

		bitstream::p_data_encode_bool()(packet, "object-permutation-exists", 1);
		bitstream::p_data_encode_integer()(packet, "object-permutation-index", object_permutation_index, 32);
		//LOG_TRACE_GAME_N("c_simulation_unit_entity_encode - object-permutation-exists packet: %08X, *packet: %08X", packet, *(int*)packet);

	}
	else
		bitstream::p_data_encode_bool()(packet, "object-permutation-exists", 0);

	pc_simulation_unit_entity_definition_encode(thisptr, creation_data_size, creation_data, a3, packet);
}


typedef bool(__stdcall *tc_simulation_unit_entity_definition_creation_decode)(void* thisptr, int creation_data_size, void* creation_data,void* packet);
tc_simulation_unit_entity_definition_creation_decode pc_simulation_unit_entity_definition_decode;

bool __stdcall c_simulation_unit_entity_definition_creation_decode(void *thisptr, int creation_data_size, void* creation_data, void* packet)
{
	//LOG_TRACE_GAME_N("c_simulation_unit_entity_definition_creation_decode()\r\nthisptr: %08X, creation_data_size: %i, creation_data: %08X, packet: %08X", thisptr, creation_data_size, creation_data, packet);

	if (bitstream::p_data_decode_bool()(packet, "object-permutation-exists"))
	{
		//LOG_TRACE_GAME_N("c_simulation_unit_entity_decode - object-permutation-exists packet: %08X, *packet: %08X", packet, *(int*)packet);
		int object_permutation_index = bitstream::p_data_decode_integer()(packet, "object-permutation-index", 32);
		*(int*)((char*)creation_data + 0x24) = object_permutation_index;

		//LOG_TRACE_GAME_N("object_permutation_index: %08X", object_permutation_index);
	}
	else
		*(int*)((char*)creation_data + 0x24) = -1;

	bool ret = pc_simulation_unit_entity_definition_decode(thisptr, creation_data_size, creation_data, packet);
	return ret;
}

typedef int(__stdcall *tset_unit_creation_data)(unsigned int object_index, void* object_creation_data);
tset_unit_creation_data pset_unit_creation_data;

int __stdcall set_unit_creation_data_hook(unsigned int object_index, void* object_creation_data)
{
	int result = pset_unit_creation_data(object_index, object_creation_data);

	if (object_to_variant[object_index] != 0)
	{
		//We should have allocated an additional 4 bytes above 0x24 so we'll write our in between 0x24 and 0x28
		*(int*)((char*)object_creation_data + 0x24) = object_to_variant[object_index];

		/*wchar_t DebugText[255] = { 0 };
		SecureZeroMemory(DebugText, sizeof(DebugText));
		wsprintf(DebugText, L"AI unit_creation_data_setup hook - object_index: %08X - variant_index: %08X", object_index, object_to_variant[object_index]);

		LOG_TRACE_GAME_N("set_unit_creation_data_hook - object_index: %08X, variant_index: %08X", object_index, object_to_variant[object_index]);
		addDebugText(DebugText);*/
	}
	else
		*(int*)((char*)object_creation_data + 0x24) = -1;

	return result;
}

typedef bool(__cdecl *tset_unit_color_data)(int, unsigned __int16, int a3);
tset_unit_color_data pset_unit_color_data;

bool __cdecl set_unit_color_data_hook(int a1, unsigned __int16 a2, int a3)
{
	int object_creation_data = a1 - 0x10;
	int object_permutation_index = *(int*)((char*)object_creation_data + 0x24);

	LOG_TRACE_GAME("set_unit_color_data_hook - {:x}", object_permutation_index);

	if ( object_permutation_index == -1)
		return pset_unit_color_data(a1, a2, a3);

	return 0;
}

typedef bool(__stdcall *tcreate_unit_hook)(void*, int, int, void*);
tcreate_unit_hook pcreate_unit_hook;

bool __stdcall create_unit_hook(void* pCreationData, int a2, int a3, void* pObject)
{
	if (*(int*)((char*)pCreationData + 0x24) != -1)
	{
		//wchar_t DebugText[255] = { 0 };
		//SecureZeroMemory(DebugText, sizeof(DebugText));
		//wsprintf(DebugText, L"create_unit_hook - variant type: %08X - ", *(int*)((char*)pCreationData + 0x24));

		//addDebugText(DebugText);

		*(int*)((char*)pObject + 0xC) = *(int*)((char*)pCreationData + 0x24);
	}

	return pcreate_unit_hook(pCreationData, a2, a3, pObject);
}

wchar_t* H2MOD::GetLobbyGameVariantName()
{
	return h2mod->GetAddress<wchar_t*>(0x97777C, 0x534A18);
}

void H2MOD::exit_game()
{
	if (h2mod->Server)
		return;

	if (GetMapType() != MapType::MAIN_MENU)
	{
		// request_squad_browser
		WriteValue<BYTE>(h2mod->GetAddress(0x978BAC), 1);

		typedef void(__cdecl load_main_menu_with_context)(int context);
		auto load_main_menu_with_context_impl = h2mod->GetAddress<load_main_menu_with_context*>(0x08EAF);
		load_main_menu_with_context_impl(0);
	}

	typedef int(__cdecl leave_game_type)(int a1);
	auto leave_game = h2mod->GetAddress<leave_game_type*>(0x216388);
	leave_game(0);
}

#pragma endregion

typedef int(__cdecl *show_error_screen)(int a1, signed int a2, int a3, __int16 a4, int a5, int a6);
show_error_screen show_error_screen_method;

int __cdecl showErrorScreen(int a1, signed int a2, int a3, __int16 a4, int a5, int a6) {
	if (a2 == 0x117)
	{
		LOG_TRACE_FUNC("Ignoring need to reinstall maps");
		return 0;
	}
	return show_error_screen_method(a1, a2, a3, a4, a5, a6);
}

typedef signed int(__cdecl *wcsncpy_s_hook)(int a1, unsigned int a2, wchar_t* a3, int a4);
wcsncpy_s_hook p_wcsncpy_s_hook;

//lets you follow the call path of any string that is displayed (in a debugger)
signed int __cdecl stringDisplayHook(int a1, unsigned int a2, wchar_t* a3, int a4) {
	return p_wcsncpy_s_hook(a1, a2, a3, a4);
}

void H2MOD::handle_command(std::string command) {
	commands->handle_command(command);
}

void H2MOD::handle_command(std::wstring command) {
	commands->handle_command(std::string(command.begin(), command.end()));
}

bool H2MOD::is_team_play() {
	//0x971A90 only works in lobby (not in game)
	//0x978CB4 works in both
	DWORD ptr = *h2mod->GetAddress<DWORD*>(0x978CB4);
	ptr += 0x1C68;
	return *(BYTE*)ptr;
}

#pragma region PlayerFunctions

float H2MOD::get_distance(int playerIndex1, int playerIndex2) {
	Real::Point3D points_distance;
	Real::Point3D* player1 = nullptr;
	Real::Point3D* player2 = nullptr;

	player1 = h2mod->get_player_coords(playerIndex1);
	player2 = h2mod->get_player_coords(playerIndex2);
	
	points_distance.X = abs(player1->X - player2->X);
	points_distance.Y = abs(player1->Y - player2->Y);
	points_distance.Z = abs(player1->Z - player2->Z);

	return sqrt(pow(points_distance.X, 2) + pow(points_distance.Y, 2) + pow(points_distance.Z, 2));
}

Real::Point3D* H2MOD::get_player_coords(int playerIndex) {
	BYTE* player_unit = get_player_unit_from_player_index(playerIndex);
	if (player_unit != nullptr)
		return reinterpret_cast<Point3D*>(player_unit + 0x64);

	return nullptr;
}

BYTE* H2MOD::get_player_unit_from_player_index(int playerIndex) {
	DatumIndex unit_datum = get_unit_datum_from_player_index(playerIndex);
	if (unit_datum.IsNull())
		return nullptr;

	DatumIterator<ObjectHeader> objectsIt(game_state_objects_header);
	return (BYTE*)objectsIt.get_data_at_index(unit_datum.ToAbsoluteIndex())->object;
}

void call_give_player_weapon(int PlayerIndex, DatumIndex WeaponId, bool bReset)
{
	//LOG_TRACE_GAME("GivePlayerWeapon(PlayerIndex: %08X, WeaponId: %08X)", PlayerIndex, WeaponId);

	DatumIndex unit_datum = h2mod->get_unit_datum_from_player_index(PlayerIndex);

	if (unit_datum != -1)
	{
		ObjectPlacementData nObject;

		call_object_placement_data_new(&nObject, WeaponId, unit_datum, 0);

		int object_index = call_object_new(&nObject);

		if (bReset == true)
			call_unit_reset_equipment(unit_datum);

		call_assign_equipment_to_unit(unit_datum, object_index, 1);
	}
}

wchar_t* H2MOD::get_local_player_name()
{
	return h2mod->GetAddress<wchar_t*>(0x51A638);
}

int H2MOD::get_player_index_from_name(wchar_t* playername)
{
	PlayerIterator playersIt;

	while (playersIt.get_next_player())
	{
		wchar_t* comparename = playersIt.get_current_player_name();

		LOG_TRACE_GAME(L"[H2MOD]::get_player_index_from_name( {0} : {1} )", playername, comparename);

		if (wcscmp(comparename, playername))
		{
			return playersIt.get_current_player_index();
		}
	}
	return -1;
}

wchar_t* H2MOD::get_player_name_from_player_index(int playerIndex)
{
	PlayerIterator playersIt;
	return playersIt.get_data_at_index(playerIndex)->properties.player_name;
}

int H2MOD::get_player_index_from_unit_datum(DatumIndex unit_datum_index)
{
	PlayerIterator playersIt;
	while (playersIt.get_next_player())
	{
		DatumIndex unit_datum_index_check = playersIt.get_current_player_data()->BipedUnitDatum;
		LOG_TRACE_FUNC("Checking datum: {0:x} - index: {1} against datum: {2:x}", unit_datum_index_check.ToInt(), playersIt.get_current_player_index(), unit_datum_index.ToInt());

		if (unit_datum_index == unit_datum_index_check)
			return playersIt.get_current_player_index();
	}
	return -1;
}

DatumIndex H2MOD::get_unit_datum_from_player_index(int playerIndex)
{
	PlayerIterator playersIt;
	if (!playersIt.get_data_at_index(playerIndex)->BipedUnitDatum.IsNull())
		return playersIt.get_data_at_index(playerIndex)->BipedUnitDatum;

	return -1;
}

int H2MOD::get_unit_index_from_player_index(int playerIndex)
{
	PlayerIterator playersIt;
	if (!playersIt.get_data_at_index(playerIndex)->BipedUnitDatum.IsNull())
		return playersIt.get_data_at_index(playerIndex)->BipedUnitDatum.ToAbsoluteIndex();

	return -1;
}

//can be used on clients and server
void H2MOD::set_unit_biped(Player::Biped biped_type, int playerIndex)
{
	PlayerIterator playersIt;
	if (playerIndex >= 0 && playerIndex < 16)
		playersIt.get_data_at_index(playerIndex)->properties.profile.player_caracter_type = biped_type;
}

BYTE H2MOD::get_unit_team_index(DatumIndex unit_datum_index)
{
	BYTE tIndex = 0;
	int unit_object = call_get_object(unit_datum_index, 3);
	if (unit_object)
	{
		tIndex = *(BYTE*)((BYTE*)unit_object + 0x13C);
	}
	return tIndex;
}

void H2MOD::set_unit_team_index(int unit_datum_index, BYTE team)
{
	int unit_object = call_get_object(unit_datum_index, 3);
	if (unit_object)
	{
		*(BYTE*)((BYTE*)unit_object + 0x13C) = team;
	}
}

void H2MOD::set_unit_speed_patch(bool hackit) {
	//TODO: create a way to undo the patch in the case when more than just infection relies on this.
	//Enable Speed Hacks

	BYTE assmPatchSpeed[8];
	memset(assmPatchSpeed, 0x90, 8);
	WriteBytes(h2mod->GetAddress(0x6AB7f, 0x6A3BA), assmPatchSpeed, 8);
}

void H2MOD::set_unit_speed(float speed, int playerIndex)
{
	PlayerIterator playersIt;
	if (playerIndex >= 0 && playerIndex < 16)
		playersIt.get_data_at_index(playerIndex)->unit_speed = speed;
}

void H2MOD::set_local_grenades(BYTE type, BYTE count, int playerIndex)
{
	DatumIndex unit_datum_index = h2mod->get_unit_datum_from_player_index(playerIndex);

	int unit_object = call_get_object(unit_datum_index, 3);

	if (unit_object)
	{
		if (type == GrenadeType::Frag)
			*(BYTE*)((BYTE*)unit_object + 0x252) = count;
		if (type == GrenadeType::Plasma)
			*(BYTE*)((BYTE*)unit_object + 0x253) = count;
	}

}

BYTE H2MOD::get_local_team_index()
{
	return *h2mod->GetAddress<BYTE*>(0x51A6B4);
}
#pragma endregion

void H2MOD::DisableSound(int sound)
{
	LOG_TRACE_GAME("tag data address: {:x}", tags::get_tag_data());
	switch (sound)
	{
	case SoundType::Slayer:
		LOG_TRACE_GAME("tag data address + 0xd7dfb4 = {:p}", tags::get_tag_data()[0xd7dfb4]);
		*(DWORD*)(&tags::get_tag_data()[0xd7e05c]) = -1;
		*(DWORD*)(tags::get_tag_data()[0xd7dfb4]) = -1;
		break;

	case SoundType::GainedTheLead:
		*(DWORD*)(tags::get_tag_data()[0xd7ab34]) = -1;
		*(DWORD*)(tags::get_tag_data()[0xd7ac84]) = -1;
		break;

	case SoundType::LostTheLead:
		*(DWORD*)(tags::get_tag_data()[0xd7ad2c]) = -1;
		*(DWORD*)(tags::get_tag_data()[0xd7add4]) = -1;
		break;

	case SoundType::TeamChange:
		*(DWORD*)(tags::get_tag_data()[0xd7b9a4]) = -1;
		break;
	}
}

void H2MOD::CustomSoundPlay(const wchar_t* soundName, int delay)
{
	auto playSound = [=]()
	{
		//std::unique_lock<std::mutex> lck(h2mod->sound_mutex);
		std::chrono::high_resolution_clock::time_point timePoint = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(delay);

		LOG_TRACE_GAME(L"[H2MOD-SoundQueue] - attempting to play sound {0} - delaying {1} miliseconds first", soundName, delay);

		if (delay > 0)
			std::this_thread::sleep_until(timePoint);

		PlaySound(soundName, NULL, SND_FILENAME | SND_NODEFAULT);
	};

	if (!h2mod->Server)
		std::thread(playSound).detach();
}

typedef char(__cdecl *player_death)(int unit_datum_index, int a2, char a3, char a4);
player_death pplayer_death;

typedef void(__stdcall *update_player_score)(void* thisptr, unsigned short a2, int a3, int a4, int a5, char a6);
update_player_score pupdate_player_score;

/* This is technically closer to object death than player-death as it impacts anything with health at all. */

char __cdecl OnPlayerDeath(int unit_datum_index, int a2, char a3, char a4)
{

	//LOG_TRACE_GAME("OnPlayerDeath(unit_datum_index: %08X, a2: %08X, a3: %08X, a4: %08X)", unit_datum_index,a2,a3,a4);

	/* The first value within a2 ( *(DWORD*)(a2) ) appears to be the datum_index of a player from the gamestate_player_table */

	/* This is the unit of the player who last damaged the object*/
	int damaging_player_unit = get_damage_owner(unit_datum_index);

	if (b_HeadHunter)
	{
		DatumIndex dead_player = DatumIndex(unit_datum_index);
		headHunterHandler->playerDeath->SetDeadPlayer(dead_player); // set this so we can spawn a skull on their position.
		headHunterHandler->playerDeath->execute();
	}

	if (b_FireFight)
	{
		/* Hack until rest of code is changed to support DatumIndex vs int*/
		DatumIndex ai_datum = DatumIndex(unit_datum_index);

		/*
			In firefight we want to track AI deaths and execute on them to grant points.
		*/
		fireFightHandler->playerDeath->SetPlayerIndex(*(DatumIndex*)(a2)); // this is the player datum of player who killed the datum.
		fireFightHandler->playerDeath->SetKilledDatum(ai_datum);
		fireFightHandler->playerDeath->execute();
	}

	if (b_GunGame) 
	{
		gunGame->playerDeath->setUnitDatumIndex(unit_datum_index);
		gunGame->playerDeath->execute();
	}

	if (b_Infection) {
		infectionHandler->playerDeath->setUnitDatumIndex(unit_datum_index);
		infectionHandler->playerDeath->execute();
	}

	return pplayer_death(unit_datum_index, a2, a3, a4);
}


void __stdcall OnPlayerScore(void* thisptr, unsigned short a2, int a3, int a4, int a5, char a6)
{
	//LOG_TRACE_GAME("update_player_score_hook ( thisptr: %08X, a2: %08X, a3: %08X, a4: %08X, a5: %08X, a6: %08X )", thisptr, a2, a3, a4, a5, a6);
	//20/10/2018 18:46:51.541 update_player_score_hook ( thisptr: 3000595C, a2: 00000000, a3: 00000002, a4: 00000001, a5: 00000007, a6: 00000001 )
	// / 10 / 2018 18:46 : 51.541 update_player_score_hook(thisptr : 3000595C, a2 : 00000000, a3 : 00000000, a4 : 00000001, a5 : FFFFFFFF, a6 : 00000000)
	//	20 / 10 / 2018 18 : 46 : 51.541 update_player_score_hook(thisptr : 3000595C, a2 : 00000001, a3 : 00000003, a4 : 00000001, a5 : 00000009, a6: 00000001)

	//20 / 10 / 2018 18:48 : 39.756 update_player_score_hook(thisptr : 3000595C, a2 : 00000001, a3 : 00000002, a4 : 00000001, a5 : 00000007, a6 : 00000001)
	//	20 / 10 / 2018 18 : 48 : 39.756 update_player_score_hook(thisptr : 3000595C, a2 : 00000001, a3 : 00000000, a4 : 00000001, a5 : FFFFFFFF, a6 : 00000000)
	//	20 / 10 / 2018 18 : 48 : 39.756 update_player_score_hook(thisptr : 3000595C, a2 : 00000000, a3 : 00000003, a4 : 00000001, a5 : 00000009, a6: 00000001)

	if (a5 == -1)
	{
		if(b_HeadHunter)
			return;
	}

	if (a5 == 7) //player got a kill?
	{
		int PlayerIndex = a2;

		if (b_GunGame) {
			gunGame->playerKill->setPlayerIndex(PlayerIndex);
			gunGame->playerKill->execute();
		}

		if (b_HeadHunter)
		{
			return;
		}
	}

	pupdate_player_score(thisptr, a2, a3, a4, a5, a6);
}

void H2MOD::DisableWeaponPickup(bool b_Enable)
{
	//Client Sided Patch
	DWORD offset = h2mod->GetAddress(0x55EFA);
	BYTE assm[5] = { 0xE8, 0x18, 0xE0, 0xFF, 0xFF };
	if (!b_Enable)
		memset(assm, 0x90, 5);
	
	WriteBytes(offset, assm, 5);
}

int OnAutoPickUpHandler(DatumIndex player_datum, DatumIndex object_datum)
{
	int(_cdecl* AutoHandler)(DatumIndex, DatumIndex);
	AutoHandler = (int(_cdecl*)(DatumIndex, DatumIndex))h2mod->GetAddress(0x57AA5, 0x5FF9D);

	if (b_HeadHunter)
	{
		headHunterHandler->itemInteraction->SetPlayerIndex(player_datum);
		bool handled = headHunterHandler->itemInteraction->SetInteractedObject(object_datum);
		headHunterHandler->itemInteraction->execute();

		if (handled)
			return 0;
	}

	return AutoHandler(player_datum, object_datum);
}

/*
TODO: might be useful for updating player count while in-lobby
typedef bool(__cdecl *PacketHandler)(void *packet, int size, void *data);

int __cdecl player_add_packet_handler(void *packet, int size, void *data)
{
	update_player_count();
	PacketHandler game_player_add_handler = reinterpret_cast<PacketHandler>(h2mod->GetAddress() + 0x1F06B6);
	return game_player_add_handler(packet, size, data);
}

bool __cdecl player_remove_packet_handler(void *packet, int size, void *data)
{
	update_player_count();
	PacketHandler game_player_remove_handler = reinterpret_cast<PacketHandler>(h2mod->GetAddress() + 0x1F08BC);
	return game_player_remove_handler(packet, size, data);
}
*/

void get_object_table_memory()
{
	game_state_actors = *h2mod->GetAddress<s_datum_array**>(0xA965DC, 0x9A1C5C);
	game_state_players = *h2mod->GetAddress<s_datum_array**>(0x4A8260, 0x4D64C4);
	game_state_objects_header = *h2mod->GetAddress<s_datum_array**>(0x4E461C, 0x50C8EC);
}

typedef bool(__cdecl *map_cache_load)(void* map_load_settings);
map_cache_load p_map_cache_load;

bool __cdecl OnMapLoad(void* map_load_settings)
{
	bool result = p_map_cache_load(map_load_settings);
	if (result == false) // verify if the game didn't fail to load the map
		return false;

	tags::run_callbacks();
	MapType map_type = *reinterpret_cast<MapType*>(map_load_settings);
	h2mod->SetMapType(map_type);

	isLobby = true;
	get_object_table_memory();

	H2Tweaks::setCrosshairPos(H2Config_crosshair_offset);
	H2Tweaks::setFOV(H2Config_field_of_view);
	H2Tweaks::setVehicleFOV(H2Config_vehicle_field_of_view);
	
	if (h2mod->GetMapType() == MapType::MAIN_MENU)
	{
		addDebugText("Map Type: Main-Menu");
		object_to_variant.clear();

		if (!NetworkSession::localPeerIsSessionHost()) {
			advLobbySettings->resetLobbySettings();
		}

		if (b_Halo2Final && !h2mod->Server) {
			h2f->Dispose();
			b_Halo2Final = false;
		}

		if (b_Infection) {
			infectionHandler->deinitializer->execute();
			b_Infection = false;
		}

		if (b_GunGame) {
			gunGame->deinitializer->execute();
			b_GunGame = false;
		}

		H2Tweaks::disableAI_MP();
		UIRankPatch();
		H2Tweaks::disable60FPSCutscenes();
		H2Tweaks::setHz();

		return result;
	}		

	wchar_t* variant_name = h2mod->GetLobbyGameVariantName();
	LOG_INFO_GAME(L"[h2mod] OnMapLoad map type {0}, variant name {1}", h2mod->GetMapType(), variant_name);
	BYTE GameState = *h2mod->GetAddress<BYTE*>(0x420FC4, 0x3C40AC);

	for (auto gametype_it : GametypesMap)
		gametype_it.second = false; // reset custom gametypes state

	H2Tweaks::setSavedSens();

	if (h2mod->GetMapType() == MapType::MULTIPLAYER_MAP)
	{
		addDebugText("Map type: Multiplayer");

		for (auto gametype_it : GametypesMap)
		{
			if (StrStrIW(variant_name, gametype_it.first)) {
				LOG_INFO_GAME(L"[h2mod] {1} custom gametype turned on!", gametype_it.first);
				gametype_it.second = true; // enable a gametype if substring is found
			}
		}

		if (!b_XboxTick) 
		{
			HitFix::Initialize();
			MPMapFix::Initialize();
			H2Tweaks::setMeleeLungeDistanceFactor(0.33333334f * 2.f);
		}
		else
		{
			xboxTickHandler->preSpawnPlayer->execute();
			H2Tweaks::setMeleeLungeDistanceFactor(0.33333334f);
		}
		
		H2Tweaks::enableAI_MP();

		H2Tweaks::setCrosshairSize(0, false);
		H2Tweaks::disable60FPSCutscenes();
		//H2Tweaks::applyShaderTweaks(); 

		if (GameState == 3)
		{
			// send server map checksums to client
			//MapChecksumSync::SendState();
			//inform players of the current advanced lobby settings
			// TODO: issue #232
			//advLobbySettings->sendLobbySettingsPacket();

			if (b_Infection) {
				infectionHandler->initializer->execute();
			}

			if (b_GunGame) {
				gunGame->initializer->execute();
			}

			H2X::Initialize(b_H2X);

			if (b_Halo2Final)
				h2f->Initialize();
		}

	}

	else if (h2mod->GetMapType() == MapType::SINGLE_PLAYER_MAP)
	{
		//if anyone wants to run code on map load single player
		addDebugText("Map type: Singleplayer");
		H2X::Initialize(true);
		H2Tweaks::setMeleeLungeDistanceFactor(0.33333334f * 2.f);

		H2Tweaks::enable60FPSCutscenes();
	}

	return result;
}

typedef bool(__cdecl *spawn_player)(int a1);
spawn_player p_spawn_player;

bool __cdecl OnPlayerSpawn(int a1)
{
	//I cant find somewhere to put this where it actually works (only needs to be done once on map load). It's only a few instructions so it shouldn't take long to execute.
	H2Tweaks::toggleKillVolumes(!AdvLobbySettings_disable_kill_volumes);

	//once players spawn we aren't in lobby anymore ;)
	isLobby = false;
	//LOG_TRACE_GAME("OnPlayerSpawn(a1: %08X)", a1);
	int PlayerIndex = a1;


	if (b_Infection) {
		infectionHandler->preSpawnPlayer->setPlayerIndex(PlayerIndex & 0x0000FFFF);
		infectionHandler->preSpawnPlayer->execute();
	}

	if (b_GunGame) {
		gunGame->preSpawnPlayer->setPlayerIndex(PlayerIndex & 0x0000FFFF);
		gunGame->preSpawnPlayer->execute();
	}

	if (b_XboxTick) {
		xboxTickHandler->preSpawnPlayer->execute();
	}

	bool ret = p_spawn_player(a1);

	if (b_Infection) {
		infectionHandler->spawnPlayer->setPlayerIndex(PlayerIndex & 0x0000FFFF);
		infectionHandler->spawnPlayer->execute();
	}

	if (b_GunGame) {
		gunGame->spawnPlayer->setPlayerIndex(PlayerIndex & 0x0000FFFF);
		gunGame->spawnPlayer->execute();
	}

	return ret;
}

typedef int(__cdecl *object_p_hook)(int s_object_placement_data, int a2, signed int a3, int a4);
object_p_hook object_p_hook_method;

int __cdecl objectPHook(int s_object_placement_data, int object_definition_index, int object_owner, int unk) {
	if (h2mod->hookedObjectDefs.find(object_definition_index) == h2mod->hookedObjectDefs.end()) {
		//ingame only
		wchar_t* mapName = h2mod->GetAddress<wchar_t*>(0x97737C);
		LOG_TRACE_GAME(L"MapName={0}, object_definition_index: {1:x}", mapName, object_definition_index);
		h2mod->hookedObjectDefs.insert(object_definition_index);
	}
	return object_p_hook_method(s_object_placement_data, object_definition_index, object_owner, unk);
}

typedef BOOL(__stdcall *is_debugger_present)();
is_debugger_present is_debugger_present_method;

BOOL __stdcall isDebuggerPresent() {
	return false;
}

typedef void*(__stdcall *tload_wgit)(void* thisptr, int a2, int a3, int a4, unsigned short a5);
tload_wgit pload_wgit;

void* __stdcall OnWgitLoad(void* thisptr, int a2, int a3, int a4, unsigned short a5) {
	int wgit = a2;

	//char NotificationPlayerText[20];
	//sprintf(NotificationPlayerText, "WGIT ID: %d", a2);
	//addDebugText(NotificationPlayerText);
	//MessageBoxA(NULL, NotificationPlayerText, "WGITness", MB_OK);

	pload_wgit(thisptr, wgit, a3, a4, a5);
	return thisptr;
}

typedef void(__cdecl *change_team)(int a1, int a2);
change_team p_change_local_team;

void __cdecl changeTeam(int localPlayerIndex, int teamIndex) {
	wchar_t* variant_name = h2mod->GetLobbyGameVariantName();
	if (StrStrIW(variant_name, L"rvb") != NULL && teamIndex != 0 && teamIndex != 1) {
		//rvb mode enabled, don't change teams
		return;
	}
	p_change_local_team(localPlayerIndex, teamIndex);
}

void H2MOD::set_local_team_index(int local_player_index, int team_index)
{
	// we only use player index 0 due to no splitscreen support but whatever
	typedef void(__cdecl update_player_profile)(int local_player_index);
	auto p_update_player_profile = h2mod->GetAddress<update_player_profile*>(0x206A97);

	p_change_local_team(local_player_index, team_index);
	p_update_player_profile(local_player_index); // fixes infection handicap glitch
}

void __cdecl print_to_console(char *output)
{
	const static std::string prefix = "[HSC Print] ";
	commands->display(prefix + output);
}

void DuplicateDataBlob(DATA_BLOB  *pDataIn, DATA_BLOB  *pDataOut)
{
	pDataOut->cbData = pDataIn->cbData;
	pDataOut->pbData = static_cast<BYTE*>(LocalAlloc(LMEM_FIXED, pDataIn->cbData));
	CopyMemory(pDataOut->pbData, pDataIn->pbData, pDataIn->cbData);
}

BOOL WINAPI CryptProtectDataHook(
	_In_       DATA_BLOB                 *pDataIn,
	_In_opt_   LPCWSTR                   szDataDescr,
	_In_opt_   DATA_BLOB                 *pOptionalEntropy,
	_Reserved_ PVOID                     pvReserved,
	_In_opt_   CRYPTPROTECT_PROMPTSTRUCT *pPromptStruct,
	_In_       DWORD                     dwFlags,
	_Out_      DATA_BLOB                 *pDataOut
)
{
	DuplicateDataBlob(pDataIn, pDataOut);

	return TRUE;
}

BOOL WINAPI CryptUnprotectDataHook(
	_In_       DATA_BLOB                 *pDataIn,
	_Out_opt_  LPWSTR                    *ppszDataDescr,
	_In_opt_   DATA_BLOB                 *pOptionalEntropy,
	_Reserved_ PVOID                     pvReserved,
	_In_opt_   CRYPTPROTECT_PROMPTSTRUCT *pPromptStruct,
	_In_       DWORD                     dwFlags,
	_Out_      DATA_BLOB                 *pDataOut
)
{
	if (CryptUnprotectData(pDataIn, ppszDataDescr, pOptionalEntropy, pvReserved, pPromptStruct, dwFlags, pDataOut) == FALSE) {
		DuplicateDataBlob(pDataIn, pDataOut); // if decrypting the data fails just assume it's unencrypted
	}

	return TRUE;
}

char filo_write__encrypted_data_hook(filo *file_ptr, DWORD nNumberOfBytesToWrite, LPVOID lpBuffer)
{
	DWORD file_size = GetFileSize(file_ptr->handle, NULL);

	if (file_size > nNumberOfBytesToWrite) // clear the file as unencrypted data is shorter then encrypted data.
		FiloInterface::change_size(file_ptr, 0);
	return FiloInterface::write(file_ptr, lpBuffer, nNumberOfBytesToWrite);
}

//int static_lod_state = static_lod::cinematic;
DWORD calculate_model_lod;
DWORD calculate_model_lod_detour_end;
__declspec(naked) void calculate_model_lod_detour()
{
	__asm
	{
		// replaced code
		// todo check if this is needed when using a static LOD, might save on some processor time
		call calculate_model_lod

		cmp H2Config_static_lod_state, 0
		jz END_DETOUR

		mov eax, H2Config_static_lod_state
		sub eax, 1 // convert setting to in-game model LOD value (0 - 5, L1 - L6)

		END_DETOUR:
		jmp calculate_model_lod_detour_end
	}
}

typedef void(__cdecl *on_custom_map_change)(const void* a1);
on_custom_map_change on_custom_map_change_method;

void __cdecl onCustomMapChange(const void* a1) {
	on_custom_map_change_method(a1);
}

typedef char(__stdcall *intercept_map_load)(LPCRITICAL_SECTION* thisx, const void *a2);
intercept_map_load intercept_map_load_method;

char __stdcall interceptMapLoad(LPCRITICAL_SECTION* thisx, const void *a2) {
	LPCRITICAL_SECTION *v2; // ebx@1
	struct _RTL_CRITICAL_SECTION *v3; // ebp@1
	char result; // al@2

	LOG_TRACE_GAME("[h2mod] Intercepted map load - crash function");

	typedef char(__thiscall* map_filetime_check)(LPCRITICAL_SECTION* thisx, int a2, unsigned int a3);
	auto map_filetime_check_method = h2mod->GetAddress<map_filetime_check>(0xC1E01);

	typedef char(__thiscall* map_touch)(LPCRITICAL_SECTION* thisx, int a2);
	auto map_touch_method = h2mod->GetAddress<map_touch>(0xC2541);

	typedef char(unknown_function)();
	auto unknown_function_method = h2mod->GetAddress<unknown_function*>(0x4541);

	typedef char(__stdcall* unknown_function2)(int a1);
	auto unknown_function_method2 = h2mod->GetAddress<unknown_function2>(0xC2069);

	typedef int(__thiscall* map_limit_touch)(int thisx, int a2);
	auto map_limit_touch_method = h2mod->GetAddress<map_limit_touch>(0xC1FA6);

	v2 = thisx;
	v3 = *thisx;

	LOG_TRACE_GAME("[h2mod] Intercepted map load - about to enter critical section");
	EnterCriticalSection(*thisx);
	LOG_TRACE_GAME("[h2mod] Intercepted map load - in critical section");
	if (a2
		&& *((WORD *)v2 + 74008) < 0x32u
		&& map_filetime_check_method(v2, (int)a2, 0xB90u)
		&& !map_touch_method(v2, (int)a2))
	{
		if (!unknown_function_method())
			unknown_function_method2((int)a2);

		map_limit_touch_method((int)v2, (int)a2);
		LOG_TRACE_GAME("[h2mod] Intercepted map load - memcpy");
		memcpy(&v2[740 * (*((WORD *)v2 + 0x12118))++ + 4], a2, 0xB90u);
		LeaveCriticalSection(v3);
		LOG_TRACE_GAME("[h2mod] Intercepted map load - left critical section");
		result = 1;
	}
	else
	{
		LeaveCriticalSection(v3);
		LOG_TRACE_GAME("[h2mod] Intercepted map load - left critical section");
		result = 0;
	}
	return result;
}

typedef bool(__cdecl *tfn_c000bd114)(int);
tfn_c000bd114 pfn_c000bd114;
bool __cdecl fn_c000bd114_IsSkullEnabled(int skull_index)
{
	//bool result = pfn_c000bd114(skull_index);
	//return result;

	bool* var_c004d8320 = h2mod->GetAddress<bool*>(0x4d8320);
	bool(*fn_c00049833_IsEngineModeCampaign)() = (bool(*)())h2mod->GetAddress(0x49833);

	bool result = false;
	if (skull_index <= 0xE && fn_c00049833_IsEngineModeCampaign())
		result = var_c004d8320[skull_index];
	if (skull_index == 0xA && AdvLobbySettings_mp_sputnik)
		result = true;
	else if (skull_index == 0x1 && AdvLobbySettings_mp_grunt_bday_party)
		result = true;
	else if (skull_index == 0x6 && AdvLobbySettings_mp_blind)
		result = true;
	return result;
}

bool GrenadeChainReactIsEngineMPCheck() {
	if (AdvLobbySettings_grenade_chain_react)
		return false;
	return h2mod->GetMapType() == MapType::MULTIPLAYER_MAP;
}

bool BansheeBombIsEngineMPCheck() {
	if (AdvLobbySettings_banshee_bomb)
		return false;
	return h2mod->GetMapType() == MapType::MULTIPLAYER_MAP;
}

bool FlashlightIsEngineSPCheck() {
	if (AdvLobbySettings_flashlight)
		return true;
	return h2mod->GetMapType() == MapType::SINGLE_PLAYER_MAP;
}

typedef bool(__cdecl* verify_game_version_on_join)(int executable_version, int build_version, int build_version2);
verify_game_version_on_join p_verify_game_version_on_join;

bool __cdecl VerifyGameVersionOnJoin(int executable_version, int build_version, int build_version2)
{
	return executable_version == EXECUTABLE_VERSION && build_version >= GAME_BUILD && build_version2 <= GAME_BUILD;
}

typedef bool(__cdecl* verify_executable_version)(int executable_version);
verify_executable_version p_verify_executable_version;

bool __cdecl VerifyExecutableVersion(int executable_version)
{
	return executable_version == EXECUTABLE_VERSION; // will not display servers that don't match this in server list
}

typedef void(__cdecl *get_game_version)(DWORD *executable_version, DWORD *build_version, DWORD *build_version2);
get_game_version p_get_game_version;

void __cdecl GetGameVersion(DWORD *executable_version, DWORD *build_version, DWORD *build_version2)
{
	*executable_version = EXECUTABLE_VERSION;
	*build_version = GAME_BUILD;
	*build_version2 = GAME_BUILD;
}

void GivePlayerWeaponDatum(DatumIndex unit_datum, DatumIndex weapon_datum)
{
	if (unit_datum != -1 && unit_datum != 0)
	{
		ObjectPlacementData nObject;

		call_object_placement_data_new(&nObject, weapon_datum, unit_datum, 0);

		int object_index = call_object_new(&nObject);
		if (object_index != -1)
		{
			call_unit_reset_equipment(unit_datum);
			call_assign_equipment_to_unit(unit_datum, object_index, 1);
		}
	}
}

//This is used for maps with 'shops' where the device_acceleration_scale is an indicator that they're using the control device as a 'shop'
float get_device_acceleration_scale(DatumIndex device_datum)
{
	DWORD tag_data = (DWORD)tags::get_tag_data();
	DWORD tag_instances = (DWORD)tags::get_tag_instances();
	DWORD game_state_objects_header_table = *(DWORD*)((BYTE*)game_state_objects_header + 0x44);

	int device_gamestate_offset = device_datum.Index + device_datum.Index * 2;
	DWORD device_gamestate_datum_pointer = *(DWORD*)((BYTE*)game_state_objects_header_table + device_gamestate_offset * 4 + 8);
	DWORD device_control_datum = *(DWORD*)((BYTE*)device_gamestate_datum_pointer);

	__int16 device_control_index = device_control_datum & 0xFFFF;
	device_control_index = device_control_index << 4;

	DWORD device_control_tag_offset = *(DWORD*)((BYTE*)device_control_index + tag_instances + 8);
	float acceleration_scale = *(float*)((BYTE*)device_control_tag_offset + tag_data + 0x14);

	return acceleration_scale;

}

typedef int(__cdecl *tdevice_touch)(DatumIndex device_datum, DatumIndex unit_datum);
tdevice_touch pdevice_touch;

bool device_active = true;
//This happens whenever a player activates a device control.
int __cdecl device_touch(DatumIndex device_datum, DatumIndex unit_datum)
{
	if (h2mod->GetMapType() == MapType::MULTIPLAYER_MAP)
	{
		//We check this to see if the device control is a 'shopping' device, if so send a request to buy an item to the DeviceShop.
		if (get_device_acceleration_scale(device_datum) == 999.0f)
		{
			if (device_shop->BuyItem(device_datum, unit_datum)) // If the purchase was successful we won't execute the original device control action.
			{
				if (device_active == false)
					return pdevice_touch(device_datum, unit_datum);

				device_active = true;
				return 0;
			}
			// If the purchase fails (they don't have enough points), or the device is not a shopping device return normally.
			// In general's map returning normally will turn the point display red indicating the user has no points, we do not indicate that the purchase failed in any other way.
			if (device_active == false)
				return 0;

			device_active = false;
		}
	}

	return pdevice_touch(device_datum, unit_datum);
}

void H2MOD::ApplyUnitHooks()
{
	DWORD dwBack;

	BYTE packet_sz = 0x30;
	WriteBytes(h2mod->GetAddress(0x1F8029, 0x1E1D8F), &packet_sz, 1);

	//This encodes the unit creation packet, only gets executed on host.
	pc_simulation_unit_entity_definition_encode = (tc_simulation_unit_entity_definition_creation_encode)DetourClassFunc(h2mod->GetAddress<BYTE*>(0x1F8503, 0x1E2269), (BYTE*)c_simulation_unit_entity_definition_creation_encode, 10);
	VirtualProtect(pc_simulation_unit_entity_definition_encode, 4, PAGE_EXECUTE_READWRITE, &dwBack);

	//This decodes the unit creation packet, only gets executed on client.
	pc_simulation_unit_entity_definition_decode = (tc_simulation_unit_entity_definition_creation_decode)DetourClassFunc(h2mod->GetAddress<BYTE*>(0x1F8557, 0x1E22BD), (BYTE*)c_simulation_unit_entity_definition_creation_decode, 11);
	VirtualProtect(pc_simulation_unit_entity_definition_decode, 4, PAGE_EXECUTE_READWRITE, &dwBack);

	pdevice_touch = (tdevice_touch)DetourFunc(h2mod->GetAddress<BYTE*>(0x163420, 0x158EE3), (BYTE*)device_touch, 10);
	VirtualProtect(pdevice_touch, 4, PAGE_EXECUTE_READWRITE, &dwBack);

	//Only patch the object_new call on host during AI_Place function, no reason to hook all object_new calls.
	PatchCall(h2mod->GetAddress(0x318DEC, 0x2C3B56), object_new_hook);

	//We update creation data here which is used later on to add data to the packet
	PatchCall(h2mod->GetAddress(0x1F807A, 0x1E1DE0), set_unit_creation_data_hook);
	pset_unit_creation_data = (tset_unit_creation_data)h2mod->GetAddress(0x1F24ED, 0x1DD586);

	// Hooks a call within the creat_unit property on the client side in order to set their permutation index before spawn.
	PatchCall(h2mod->GetAddress(0x1F9E6C, 0x1E3BD4), create_unit_hook);
	pcreate_unit_hook = (tcreate_unit_hook)h2mod->GetAddress(0x1F32DB, 0x1DE374);

	// Hooks the part of the unit spawn from simulation that handles setting their color data in order to ensure AI do not have their color overridden
	PatchCall(h2mod->GetAddress(0x1F9E34, 0x1E3B9C), set_unit_color_data_hook);
	pset_unit_color_data = (tset_unit_color_data)h2mod->GetAddress(0x6E5C3, 0x6D1BF);
}

void H2MOD::ApplyHooks() {
	/* Should store all offsets in a central location and swap the variables based on h2server/halo2.exe*/
	/* We also need added checks to see if someone is the host or not, if they're not they don't need any of this handling. */

	LOG_TRACE_GAME("Applying hooks...");

	DWORD dwBack;
	/* Labeled "AutoPickup" handler may be proximity to vehicles and such as well */
	PatchCall(h2mod->GetAddress(0x58789, 0x60C81), OnAutoPickUpHandler);

	// hook to initialize stuff before game start
	p_map_cache_load = (map_cache_load)DetourFunc(h2mod->GetAddress<BYTE*>(0x8F62, 0x1F35C), (BYTE*)OnMapLoad, 11);
	VirtualProtect(p_map_cache_load, 4, PAGE_EXECUTE_READWRITE, &dwBack);

	// player spawn hook
	p_spawn_player = (spawn_player)DetourFunc(h2mod->GetAddress<BYTE*>(0x55952, 0x5DE4A), (BYTE*)OnPlayerSpawn, 6);
	VirtualProtect(p_spawn_player, 4, PAGE_EXECUTE_READWRITE, &dwBack);

	// game version hook
	p_get_game_version = (get_game_version)DetourFunc(h2mod->GetAddress<BYTE*>(0x1B4BF5, 0x1B0043), (BYTE*)GetGameVersion, 8);
	VirtualProtect(p_get_game_version, 4, PAGE_EXECUTE_READWRITE, &dwBack);

	pplayer_death = (player_death)DetourFunc(h2mod->GetAddress<BYTE*>(0x17B674, 0x152ED4), (BYTE*)OnPlayerDeath, 9);
	VirtualProtect(pplayer_death, 4, PAGE_EXECUTE_READWRITE, &dwBack);

	pupdate_player_score = (update_player_score)DetourClassFunc(h2mod->GetAddress<BYTE*>(0xD03ED, 0x8C84C), (BYTE*)OnPlayerScore, 12);
	VirtualProtect(pupdate_player_score, 4, PAGE_EXECUTE_READWRITE, &dwBack);

	//on_custom_map_change_method = (on_custom_map_change)DetourFunc(h2mod->GetAddress<BYTE*>(0x32176, 0x25738), (BYTE*)onCustomMapChange, 5);
	//VirtualProtect(on_custom_map_change_method, 4, PAGE_EXECUTE_READWRITE, &dwBack);

	// disable part of custom map tag verification
	NopFill(GetAddress(0x4FA0A, 0x56C0A), 6);

	// disables profiles/game saves encryption
	PatchWinAPICall(GetAddress(0x9B08A, 0x85F5E), CryptProtectDataHook);
	PatchWinAPICall(GetAddress(0x9AF9E, 0x352538), CryptUnprotectDataHook);
	PatchCall(GetAddress(0x9B09F, 0x85F73), filo_write__encrypted_data_hook);

	ApplyUnitHooks();
	mapManager->applyGamePatches();

	// bellow hooks applied to specific executables
	if (this->Server == false) {

		LOG_TRACE_GAME("Applying client hooks...");
		/* These hooks are only built for the client, don't enable them on the server! */

		p_verify_game_version_on_join = (verify_game_version_on_join)DetourFunc(h2mod->GetAddress<BYTE*>(0x1B4C14), (BYTE*)VerifyGameVersionOnJoin, 5);
		VirtualProtect(p_verify_game_version_on_join, 4, PAGE_EXECUTE_READWRITE, &dwBack);

		p_verify_executable_version = (verify_executable_version)DetourFunc(h2mod->GetAddress<BYTE*>(0x1B4C32), (BYTE*)VerifyExecutableVersion, 8);
		VirtualProtect(p_verify_executable_version, 4, PAGE_EXECUTE_READWRITE, &dwBack);

		//pload_wgit = (tload_wgit)DetourClassFunc(h2mod->GetAddress<BYTE*>(0x2106A2), (BYTE*)OnWgitLoad, 13);
		//VirtualProtect(pload_wgit, 4, PAGE_EXECUTE_READWRITE, &dwBack);

		intercept_map_load_method = (intercept_map_load)DetourClassFunc(h2mod->GetAddress<BYTE*>(0xC259B), (BYTE*)interceptMapLoad, 13);
		VirtualProtect(intercept_map_load_method, 4, PAGE_EXECUTE_READWRITE, &dwBack);

		show_error_screen_method = (show_error_screen)DetourFunc(h2mod->GetAddress<BYTE*>(0x20E15A), (BYTE*)showErrorScreen, 8);
		VirtualProtect(show_error_screen_method, 4, PAGE_EXECUTE_READWRITE, &dwBack);

		//TODO: turn on if you want to debug halo2.exe from start of process
		//is_debugger_present_method = (is_debugger_present)DetourFunc(h2mod->GetAddress<BYTE*>(0x39B394), (BYTE*)isDebuggerPresent, 5);
		//VirtualProtect(is_debugger_present_method, 4, PAGE_EXECUTE_READWRITE, &dwBack);

		//TODO: use for object spawn hooking
		//0x132163
		//object_p_hook_method = (object_p_hook)DetourFunc(h2mod->GetAddress<BYTE*>(0x132163), (BYTE*)objectPHook, 6);
		//VirtualProtect(object_p_hook_method, 4, PAGE_EXECUTE_READWRITE, &dwBack);

		//TODO: expensive, use for debugging/searching
		//string_display_hook_method = (string_display_hook)DetourFunc(h2mod->GetAddress<BYTE*>(0x287AB5), (BYTE*)stringDisplayHook, 5);
		//VirtualProtect(string_display_hook_method, 4, PAGE_EXECUTE_READWRITE, &dwBack);

		//pResetRound=(ResetRounds)DetourFunc(h2mod->GetAddress<BYTE*>(0x6B1C8), (BYTE*)OnNextRound, 7);
		//VirtualProtect(pResetRound, 4, PAGE_EXECUTE_READWRITE, &dwBack);

		/*
		WritePointer(h2mod->GetAddress<BYTE*>(0x1F0B3A), player_add_packet_handler);
		WritePointer(h2mod->GetAddress<BYTE*>(0x1F0B80), player_remove_packet_handler);
		*/

		// Patch out the code that displays the "Invalid Checkpoint" error
		// Start
		NopFill(GetAddress(0x30857), 0x41);
		// Respawn
		NopFill(GetAddress(0x8BB98), 0x2B);

		p_change_local_team = (change_team)DetourFunc(h2mod->GetAddress<BYTE*>(0x2068F2), (BYTE*)changeTeam, 8);
		VirtualProtect(p_change_local_team, 4, PAGE_EXECUTE_READWRITE, &dwBack);

		// hook the print command to redirect the output to our console
		PatchCall(GetAddress(0xE9E50), print_to_console);

		calculate_model_lod = GetAddress(0x19CA3E);
		calculate_model_lod_detour_end = GetAddress(0x19CDA3 + 5);
		WriteJmpTo(GetAddress(0x19CDA3), calculate_model_lod_detour);

		// set max model quality to L6
		WriteValue(GetAddress(0x190B38 + 1), 5);

		pfn_c000bd114 = (tfn_c000bd114)DetourFunc(h2mod->GetAddress<BYTE*>(0xbd114), (BYTE*)fn_c000bd114_IsSkullEnabled, 5);
		PatchCall(GetAddress(0x182d6d), GrenadeChainReactIsEngineMPCheck);
		PatchCall(GetAddress(0x92C05), BansheeBombIsEngineMPCheck);
		PatchCall(GetAddress(0x13ff75), FlashlightIsEngineSPCheck);

		Initialise_tag_loader();
		TagInterface::GlobalTagInterface.Init();
	}
	else {

		LOG_TRACE_GAME("Applying dedicated server hooks...");

		ServerConsole::ApplyHooks();

		//TODO: turn on later
		//std::thread t1(&MapManager::startListeningForClients, mapManager);
		//t1.detach();
	}
}

VOID CALLBACK UpdateDiscordStateTimer(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	update_player_count();
}

void H2MOD::Initialize()
{
	if (!h2mod->Server)
	{
		if (H2Config_raw_input)
			Mouseinput::Initialize();

		if (H2Config_discord_enable && H2GetInstanceId() == 1) {
			// Discord init
			DiscordInterface::SetDetails("Startup");
			DiscordInterface::Init();
			SetTimer(NULL, 0, 5000, UpdateDiscordStateTimer);
		}
	}

	LOG_TRACE_GAME("H2MOD - Initialized v0.5a");
	LOG_TRACE_GAME("H2MOD - BASE ADDR {:x}", this->Base);

	h2mod->ApplyHooks();
}

void H2MOD::Deinitialize() {

}

void H2MOD::TeamPlayerIndicatorVisibility(bool toggle)
{
	// makes toggles between jz and jmp
	WriteValue<BYTE>(h2mod->GetAddress(0x111197), toggle ? 0x74 : 0xEB);
}
