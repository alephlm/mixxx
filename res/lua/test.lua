-- Test Lua script for Mixxx controller
local toggle = false;

function init()
    -- Optional startup initialization. Do not send MIDI unconditionally here
    toggle = false
    print("------- LUA init() ------");
    makeConnection("[Channel1]", "vu_meter", vuMeterPeakDeck );
    makeConnection("[Channel2]", "vu_meter", vuMeterPeakDeck );

end

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
        print("Play button pressed on " .. newState)
        local midiChannel = group == "[Channel1]" and 0x91 or 0x92;
        print("MIDI channel: " .. midiChannel)
        sendShortMsg(midiChannel, 0x30, newState == 0 and 0x00 or 0x7F)
    else
        print("Play button released on " .. group)
    end
end

function vuMeterPeakDeck (value, group, control)
    print(
    "VU Meter Peak Deck: " ..
    tostring(value) ..
    " Group: " ..
    tostring(group) ..
    " Control: " ..
    tostring(control)

)
    value = absoluteLinInverse(value, 0.0, 1.0, 0, 125);
    if group == "[Channel1]" then
        sendShortMsg(0xB1, 0x40, value);
    else
        sendShortMsg(0xB2, 0x40, value);
    end
end

function absoluteLinInverse(value, low, high, min, max)
    min = min or 0
    max = max or 127

    local result = (((value - low) * (max - min)) / (high - low)) + min

    return math.max(min, math.min(max, result))
end
