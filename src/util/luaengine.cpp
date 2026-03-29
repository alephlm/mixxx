#include "util/luaengine.h"

#include <lauxlib.h>
#include <lualib.h>

#include <iostream>

#include "control/controlproxy.h"

static int l_setValue(lua_State* L);
static int l_getValue(lua_State* L);
static int l_sendShortMsg(lua_State* L);

LuaEngine::LuaEngine() {
    m_L = luaL_newstate();
    luaL_openlibs(m_L);

    lua_pushlightuserdata(m_L, this);
    lua_setglobal(m_L, "__engine");

    lua_register(m_L, "setValue", l_setValue);
    lua_register(m_L, "getValue", l_getValue);
    lua_register(m_L, "sendShortMsg", l_sendShortMsg);
    std::cout << "[LuaEngine] Initialized" << std::endl;
}

LuaEngine::~LuaEngine() {
    if (m_L) {
        lua_close(m_L);
    }
}

bool LuaEngine::executeString(const char* code) {
    if (luaL_dostring(m_L, code)) {
        std::cerr << "[Lua error] " << lua_tostring(m_L, -1) << std::endl;
        return false;
    }
    return true;
}

bool LuaEngine::executeFile(const char* filename) {
    if (luaL_dofile(m_L, filename)) {
        std::cerr << "[Lua error] " << lua_tostring(m_L, -1) << std::endl;
        return false;
    }
    return true;
}

int LuaEngine::callFunction(const char* name, int a, int b) {
    // Get function
    lua_getglobal(m_L, name);

    if (!lua_isfunction(m_L, -1)) {
        std::cerr << "[Lua error] Function not found: " << name << std::endl;
        lua_pop(m_L, 1);
        return 0;
    }

    // Push arguments
    lua_pushinteger(m_L, a);
    lua_pushinteger(m_L, b);

    // Call function (2 args, 1 return)
    if (lua_pcall(m_L, 2, 1, 0) != LUA_OK) {
        std::cerr << "[Lua error] " << lua_tostring(m_L, -1) << std::endl;
        lua_pop(m_L, 1);
        return 0;
    }

    // Get result
    int result = lua_tointeger(m_L, -1);
    lua_pop(m_L, 1);

    return result;
}

bool LuaEngine::callFunction(const char* name,
        int channel,
        int control,
        int value,
        int status,
        const char* group) {
    lua_getglobal(m_L, name);

    if (!lua_isfunction(m_L, -1)) {
        std::cerr << "[Lua error] Function not found: " << name << std::endl;
        lua_pop(m_L, 1);
        return false;
    }

    lua_pushinteger(m_L, channel);
    lua_pushinteger(m_L, control);
    lua_pushinteger(m_L, value);
    lua_pushinteger(m_L, status);
    lua_pushstring(m_L, group);

    if (lua_pcall(m_L, 5, 0, 0) != LUA_OK) {
        std::cerr << "[Lua error] " << lua_tostring(m_L, -1) << std::endl;
        lua_pop(m_L, 1);
        return false;
    }

    return true;
}

static int l_setValue(lua_State* L) {
    const char* group = luaL_checkstring(L, 1);
    const char* key = luaL_checkstring(L, 2);
    double value = luaL_checknumber(L, 3);

    ControlProxy control(group, key, nullptr, ControlFlag::AllowMissingOrInvalid);
    control.set(value);

    return 0;
}

static int l_getValue(lua_State* L) {
    const char* group = luaL_checkstring(L, 1);
    const char* key = luaL_checkstring(L, 2);

    ControlProxy control(group, key, nullptr, ControlFlag::AllowMissingOrInvalid);
    double value = control.get();

    lua_pushnumber(L, value);
    return 1;
}

static int l_sendShortMsg(lua_State* L) {
    int status = luaL_checkinteger(L, 1);
    int control = luaL_checkinteger(L, 2);
    int value = luaL_checkinteger(L, 3);

    // get LuaEngine instance
    lua_getglobal(L, "__engine");
    LuaEngine* engine = static_cast<LuaEngine*>(lua_touserdata(L, -1));
    lua_pop(L, 1);

    if (engine) {
        engine->sendMidi(status, control, value);
    }

    std::cout << "[Lua MIDI OUT] "
              << status << " "
              << control << " "
              << value << std::endl;

    return 0;
}

void LuaEngine::setSendMidiCallback(SendMidiFn fn) {
    m_sendMidi = fn;
}

void LuaEngine::sendMidi(int status, int control, int value) {
    if (m_sendMidi) {
        m_sendMidi(status, control, value);
    }
}
