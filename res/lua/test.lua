-- Test Lua script for Mixxx controller
function testMidiFunction(channel, control, value, status, group)
    print("Lua script called: channel=" .. channel .. ", control=" .. control .. ", value=" .. value .. ", status=" .. status .. ", group=" .. group)
end

function playButtonHandler(channel, control, value, status, group)
    if value == 0x7F then
        print("Play button pressed on " .. group)
        local currentState = getValue(group, "play")
        print("Play button pressed on " .. currentState)
        setValue(group, "play", currentState == 0 and 1 or 0)
        local newState = getValue(group, "play")
        local midiChannel = group == "[Channel1]" and 0x91 or 0x92
        sendShortMsg(midiChannel, 0x30, newState == 0 and 0x00 or 0x7F)
    else
        print("Play button released on " .. group)
    end
end
