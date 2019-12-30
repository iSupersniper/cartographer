#include "stdafx.h"
#include "Globals.h"
#include "XboxTick.h"
#include <shlwapi.h>

XboxTick::XboxTick()
{
	this->preSpawnPlayer = new XboxTickPreSpawnHandler();
	this->deinitializer = new XboxTickDeinitializer();
}

DWORD& MapHeaderType = *(DWORD*)(H2BaseAddr + 0x47CD68 + 0x14C);

int GetTickrate()
{
	int tickrate = 60;
	const wchar_t* OGH2 = L"ogh2";
	if (StrStrIW(h2mod->GetLobbyGameVariantName(), OGH2))
	{
		tickrate = 30;
		WriteValue((DWORD)h2mod->GetAddress(0x264ABB + 1, 0x0), tickrate);

		BYTE mov_al_1_retn[] = { 0xB0, 0x01, 0xC3 };
		WriteBytes((DWORD)h2mod->GetAddress(0x3A938, 0x0), mov_al_1_retn, sizeof(mov_al_1_retn));

		BYTE jne[] = { 0x85 };
		WriteBytes((DWORD)h2mod->GetAddress(0x288BD), jne, sizeof(jne));

		TRACE_GAME("[h2mod] Set the game tickrate to 30");
	}
	else
	{
		tickrate = 60;
		WriteValue((DWORD)h2mod->GetAddress(0x264ABB + 1, 0x0), tickrate);

		BYTE push_ebp_xor_bl_bl[] = { 0x53, 0x32, 0xDB };
		WriteBytes((DWORD)h2mod->GetAddress(0x3A938, 0x0), push_ebp_xor_bl_bl, sizeof(push_ebp_xor_bl_bl));

		BYTE je[] = { 0x84 };
		WriteBytes((DWORD)h2mod->GetAddress(0x288BD), je, sizeof(je));

		TRACE_GAME("[h2mod] Set the game tickrate to 60");
	}

	return tickrate;
}

void XboxTick::applyHooks()
{
	PatchCall(h2mod->GetAddress(0x48D9A, 0x0), GetTickrate);
}

void XboxTickPreSpawnHandler::onClient() {
}

void XboxTickPreSpawnHandler::onDedi() {
}

void XboxTickPreSpawnHandler::onPeerHost() {
}

void XboxTickDeinitializer::onClient()
{
	int tickrate = 60;
	BYTE push_ebp_xor_bl_bl[] = { 0x53, 0x32, 0xDB };
	WriteBytes((DWORD)h2mod->GetAddress(0x3A938, 0x0), push_ebp_xor_bl_bl, sizeof(push_ebp_xor_bl_bl));

	WriteValue((DWORD)h2mod->GetAddress(0x264ABB + 1, 0x0), tickrate);
	WriteValue(*(DWORD*)(h2mod->GetBase() + 0x4C06E4 + 0x2, 0x0), tickrate);
	WriteValue(*(DWORD*)(h2mod->GetBase() + 0x4C06E4 + 0x4, 0x0), (float)(1 / tickrate));

	BYTE je[] = { 0x84 };
	WriteBytes((DWORD)h2mod->GetAddress(0x288BD), je, sizeof(je));

	TRACE_GAME("[h2mod] Set the game tickrate to 60");
}

void XboxTickDeinitializer::onDedi() {
	//Does nothing for dedicated server host
}

void XboxTickDeinitializer::onPeerHost()
{
	int tickrate = 60;
	BYTE push_ebp_xor_bl_bl[] = { 0x53, 0x32, 0xDB };
	WriteBytes((DWORD)h2mod->GetAddress(0x3A938, 0x0), push_ebp_xor_bl_bl, sizeof(push_ebp_xor_bl_bl));

	WriteValue((DWORD)h2mod->GetAddress(0x264ABB + 1, 0x0), tickrate);
	WriteValue(*(DWORD*)h2mod->GetAddress(0x4C06E4 + 0x2, 0x0), tickrate);
	WriteValue(*(DWORD*)h2mod->GetAddress(0x4C06E4 + 0x4, 0x0), (float)(1 / tickrate));

	BYTE je[] = { 0x84 };
	WriteBytes((DWORD)h2mod->GetAddress(0x288BD), je, sizeof(je));

	TRACE_GAME("[h2mod] Set the game tickrate to 60");
}
