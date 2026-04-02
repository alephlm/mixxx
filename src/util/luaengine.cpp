#include "util/luaengine.h"

#include <lauxlib.h>
#include <lualib.h>

#include <QString>
#include <iostream>

#include "control/controlproxy.h"

class LuaValueChangedCallback : public QObject {
  public:
    LuaValueChangedCallback(lua_State* L, int ref, const char* group, const char* name)
            : m_L(L),
              m_ref(ref),
              m_group(group),
              m_name(name) {
    }

    ~LuaValueChangedCallback() {
        if (m_ref != LUA_REFNIL) {
            luaL_unref(m_L, LUA_REGISTRYINDEX, m_ref);
        }
    }

    void onValueChanged(double value) {
        lua_rawgeti(m_L, LUA_REGISTRYINDEX, m_ref);
        if (!lua_isfunction(m_L, -1)) {
            lua_pop(m_L, 1);
            return;
        }
        lua_pushnumber(m_L, value);
        lua_pushstring(m_L, m_group.c_str());
        lua_pushstring(m_L, m_name.c_str());
        if (lua_pcall(m_L, 3, 0, 0) != LUA_OK) {
            std::cerr << "[Lua error] " << lua_tostring(m_L, -1) << std::endl;
            lua_pop(m_L, 1);
        }
    }

  private:
    lua_State* m_L;
    int m_ref;
    std::string m_group;
    std::string m_name;
};

static int l_setValue(lua_State* L);
static int l_getValue(lua_State* L);
static int l_sendShortMsg(lua_State* L);
static int l_makeConnection(lua_State* L);

LuaEngine::LuaEngine() {
    m_L = luaL_newstate();
    luaL_openlibs(m_L);

    lua_pushlightuserdata(m_L, this);
    lua_setglobal(m_L, "__engine");

    lua_register(m_L, "setValue", l_setValue);
    lua_register(m_L, "getValue", l_getValue);
    lua_register(m_L, "sendShortMsg", l_sendShortMsg);
    lua_register(m_L, "makeConnection", l_makeConnection);

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

bool LuaEngine::callFunction(const char* name) {
    lua_getglobal(m_L, name);

    if (!lua_isfunction(m_L, -1)) {
        lua_pop(m_L, 1);
        return false;
    }

    if (lua_pcall(m_L, 0, 0, 0) != LUA_OK) {
        std::cerr << "[Lua error] " << lua_tostring(m_L, -1) << std::endl;
        lua_pop(m_L, 1);
        return false;
    }
    return true;
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
    int value = luaL_checknumber(L, 3); // Accept number and convert to int

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

bool LuaEngine::makeConnection(const char* group, const char* name, int luaCallbackRef) {
    if (group == nullptr || name == nullptr) {
        return false;
    }

    auto proxy = std::make_unique<ControlProxy>(QString::fromUtf8(group),
            QString::fromUtf8(name),
            nullptr,
            ControlFlag::AllowMissingOrInvalid);

    if (!proxy->valid()) {
        return false;
    }

    LuaValueChangedCallback* callbackHolder =
            new LuaValueChangedCallback(m_L, luaCallbackRef, group, name);
    callbackHolder->setParent(proxy.get());

    if (!proxy->connectValueChanged(callbackHolder, &LuaValueChangedCallback::onValueChanged)) {
        delete callbackHolder;
        return false;
    }

    LuaScriptHandle handle;
    handle.controlProxy = std::move(proxy);

    m_luaConnections.push_back(std::move(handle));

    return true;
}

static int l_makeConnection(lua_State* L) {
    const char* group = luaL_checkstring(L, 1);
    const char* name = luaL_checkstring(L, 2);
    luaL_checktype(L, 3, LUA_TFUNCTION);

    // Create a stable reference to function.
    lua_pushvalue(L, 3);
    int callbackRef = luaL_ref(L, LUA_REGISTRYINDEX);

    LuaEngine* engine = static_cast<LuaEngine*>(lua_touserdata(L, lua_upvalueindex(1)));
    // We cannot get this from upvalue; instead user object is stored on __engine global.
    lua_getglobal(L, "__engine");
    engine = static_cast<LuaEngine*>(lua_touserdata(L, -1));
    lua_pop(L, 1);

    bool success = false;
    if (engine) {
        success = engine->makeConnection(group, name, callbackRef);
    }
    lua_pushboolean(L, success);
    return 1;
}
