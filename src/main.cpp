
#include <cstdint>
#include <chrono>

#define WIN32_LEAN_AND_MEAN
#include "Windows.h"

#include "lua.hpp"
#include "MinHook.h"

constexpr uintptr_t Offset_LuaVM_Construct = 0x054a1e0;
constexpr uint64_t MicrosecondsPerSecond = 1000000;
constexpr uint64_t MaxLuaCallbackMicroseconds = MicrosecondsPerSecond * 5;	// 5 seconds

enum class LuaEnvType {
	Game,
	Terrain
};

struct LuaVM {
	lua_State* L;
};


// Utility functions

static uint64_t GetMicros() {
	auto now = std::chrono::high_resolution_clock::now();
	auto us = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
	return static_cast<uint64_t>(us);
}


// Global state

static lua_State* g_thisLuaState = nullptr;
static bool g_mhInit = false;
static bool g_mhHooksEnabled = false;
static bool g_watchdogEnabled = false;
static uint64_t g_watchdogStartTimeUs = 0;

// See lua_sethook
static void LuaHook(lua_State* L, lua_Debug* ar) {
	if ( g_watchdogEnabled )
		if ( (GetMicros() - g_watchdogStartTimeUs) >= MaxLuaCallbackMicroseconds )
			luaL_error(L, "LUA WATCHDOG: Callback execution time exceeded %i seconds", int(MaxLuaCallbackMicroseconds / MicrosecondsPerSecond));
}


// Hooks

static int (*O_lua_pcall)(lua_State*, int, int, int) = nullptr;
static int H_lua_pcall(lua_State* L, int nArgs, int nResults, int errFunc) {
	if ( L == g_thisLuaState && !g_watchdogEnabled ) {
		g_watchdogEnabled = true;
		g_watchdogStartTimeUs = GetMicros();
		int res = O_lua_pcall(L, nArgs, nResults, errFunc);
		g_watchdogEnabled = false;
		return res;
	}
	return O_lua_pcall(L, nArgs, nResults, errFunc);
}

static LuaVM* (*O_LuaVM_Construct)(LuaVM*, void*, LuaEnvType) = nullptr;
static LuaVM* H_LuaVM_Construct(LuaVM* self, void* ptr, LuaEnvType env) {
	O_LuaVM_Construct(self, ptr, env);
	if ( env == LuaEnvType::Game ) {
		g_thisLuaState = self->L;
		lua_sethook(self->L, &LuaHook, LUA_MASKLINE, 0);
	}
	return self;
}



static void DllAttach() {
	if ( MH_Initialize() != MH_OK )
		return (void)MessageBoxA(nullptr, "LuaWatchdog failed to init MinHook", "LuaWatchdog Error", MB_OK);
	g_mhInit = true;

	uintptr_t base = uintptr_t(GetModuleHandle(0));

	if ( MH_CreateHook((void*)(base + Offset_LuaVM_Construct), H_LuaVM_Construct, (LPVOID*)&O_LuaVM_Construct) != MH_OK )
		return (void)MessageBoxA(nullptr, "LuaWatchdog failed to hook LuaVM", "LuaWatchdog Error", MB_OK);
	if ( MH_CreateHookApi(L"lua51.dll", "lua_pcall", H_lua_pcall, (LPVOID*)&O_lua_pcall) != MH_OK )
		return (void)MessageBoxA(nullptr, "LuaWatchdog failed to hook lua_pcall", "LuaWatchdog Error", MB_OK);

	if ( MH_EnableHook(MH_ALL_HOOKS) != MH_OK )
		return (void)MessageBoxA(nullptr, "LuaWatchdog failed to enable hooks", "LuaWatchdog Error", MB_OK);
	g_mhHooksEnabled = true;
}

static void DllDetach() {
	if ( g_mhInit ) {
		if ( g_mhHooksEnabled )
			MH_DisableHook(MH_ALL_HOOKS);
		MH_Uninitialize();
	}
}

BOOL WINAPI DllMain( HINSTANCE hInstance, DWORD fdwReason, LPVOID lpvReserved ) {
	switch( fdwReason ) {
		case DLL_PROCESS_ATTACH: {
			DllAttach();
			break;
		}
		case DLL_PROCESS_DETACH: {
			DllDetach();
			break;
		}
	}
	return TRUE;
}
