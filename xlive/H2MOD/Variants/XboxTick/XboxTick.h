#pragma once
#include "Globals.h"

class XboxTickHandler : public GameClientServerHandler {
public:
private:
};

class XboxTickPreSpawnHandler : public XboxTickHandler {
	// Inherited via GameClientServerHandler
	virtual void onPeerHost() override;
	virtual void onDedi() override;
	virtual void onClient() override;
};

class XboxTickDeinitializer : public XboxTickHandler {
public:
	// Inherited via GameClientServerHandler
	virtual void onPeerHost() override;
	virtual void onDedi() override;
	virtual void onClient() override;
};

class XboxTick : public GameType<XboxTickHandler>
{
public:
	XboxTick();
	void applyHooks();
	static void Initialize();
	static void Deinitialize();
};