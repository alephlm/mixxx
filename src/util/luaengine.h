#pragma once
#include <lua.h>

#include <functional>

class LuaEngine {
  public:
    LuaEngine();
    ~LuaEngine();

    using SendMidiFn = std::function<void(int, int, int)>;
    void setSendMidiCallback(SendMidiFn fn);
    void sendMidi(int status, int control, int value);
    bool executeString(const char* code);
    bool executeFile(const char* filename);
    int callFunction(const char* name, int a, int b);

    // High-level helper for midi callback functions:
    // function callback(channel, control, value, status, group)
    bool callFunction(const char* name,
            int channel,
            int control,
            int value,
            int status,
            const char* group);

    lua_State* getState() const {
        return m_L;
    }

  private:
    lua_State* m_L;
    SendMidiFn m_sendMidi;
};
