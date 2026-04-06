local colorMapper = nil;

function init()
    print("------- LUA init() ------");
    makeConnection("[Channel1]", "vu_meter", vuMeterPeakDeck );
    makeConnection("[Channel2]", "vu_meter", vuMeterPeakDeck );
    for i = 1, 8 do
        local index = i  -- closure safety

        _G["hotcueButton" .. index] = function(channel, control, value, status, group)
            hotcueButtonHandler(index, channel, control, value, status, group)
        end
    end
    for i = 1, 8 do
        makeConnection("[Channel1]", "hotcue_" .. i .. "_color", setHotCueColor );
        makeConnection("[Channel1]", "hotcue_" .. i .. "_enabled", hotcueClear );
    end
    for i = 1, 8 do
        makeConnection("[Channel2]", "hotcue_" .. i .. "_color", setHotCueColor );
        makeConnection("[Channel2]", "hotcue_" .. i .. "_enabled", hotcueClear );
    end

    -- Test ColorMapper
    print("Creating ColorMapper...");
    PadColorMapper = ColorMapper({
        [0xFF0000] = 0x60,
        [0xFFFF00] = 0x7C,
        [0x00FF00] = 0x1C,
        [0x00FFFF] = 0x1F,
        [0x0000FF] = 0x03,
        [0xFF00FF] = 0x42,
        [0xFF88FF] = 0x63,
        [0xFFFFFF] = 0x7F,
        [0x000088] = 0x02,
        [0x008800] = 0x10,
        [0x008888] = 0x12,
        [0x228800] = 0x30,
        [0x880000] = 0x40,
        [0x882200] = 0x4C,
        [0x888800] = 0x50,
        [0x888888] = 0x52,
        [0x88FF00] = 0x5C,
        [0xFF8800] = 0x74,
    });
    print("ColorMapper created!");

    -- Test nearest color
    local nearestColor = PadColorMapper:getNearestColor(0xFFFFFF);
    print("Nearest color to white (0xFFFFFF) is: 0x" .. string.format("%06X", nearestColor));

    -- Test value for nearest color
    local value = PadColorMapper:getValueForNearestColor(0xFF0000);
    print("MIDI value for nearest red (0xFF0000) is: " .. value);

end

function hotcueButtonHandler(index, channel, control, value, status, group)
    print("👉 Pad " .. index .. " pressed")
    setValue(group, "hotcue_" .. index .. "_activate", value);
end

function playButtonHandler(channel, control, value, status, group)
    if value == 0x7F then
        print("Play button pressed on " .. group)
        local currentState = getValue(group, "play")
        print("Play button pressed on " .. currentState)
        setValue(group, "play", currentState == 0 and 1 or 0)
        local newState = getValue(group, "play")
        print("Play button pressed on " .. newState)
        local midiChannel = group == "[Channel1]" and 1 or 2;
        print("MIDI channel: " .. midiChannel)
        sendShortMsg(midiChannel, 0x30, newState == 0 and 0x00 or 0x7F)
        local colorKey = "hotcue_2_color"
        local color_code = getValue(group, colorKey)
        print("Current color code for " .. colorKey .. ": 0x" .. string.format("%06X", color_code))
        local nearestColorValue = PadColorMapper:getValueForNearestColor();

        sendShortMsg(0x95 + midiChannel, 0x01, math.random(0, 6) + math.random(0, 9));

    else
        print("Play button released on " .. group)
    end
end

function vuMeterPeakDeck (value, group, control)
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

hotcueClear = function(value, group, control)
    print("Hotcue Clear: " .. value .. " Group: " .. group .. " Control: " .. control)
    if value == 0 then
        local midiChannel = group == "[Channel1]" and 1 or 2;
        local hotcueIndex = string.match(control, "hotcue_(%d+)_enabled")
        if not hotcueIndex then
            print("Error: Could not parse hotcue index from control: " .. control)
            return
        end
        hotcueIndex = tonumber(hotcueIndex) - 1
        sendShortMsg(0x95 + midiChannel, hotcueIndex, 0x00);
    else
        -- get hotcue color and resend to update the pad color after clearing
        color = getValue(group, control:gsub("_enabled", "_color")) -- Ensure the value is up to date
        print("COLOR " .. color)
        setHotCueColor(color, group, control:gsub("_enabled", "_color")) -- Clear the hotcue color
    end
end

function setHotCueColor(value, group, control)
        local midiChannel = group == "[Channel1]" and 1 or 2;
    -- Extract hotcue index from control name (e.g., "hotcue_1_color" -> 1)
    local hotcueIndex = string.match(control, "hotcue_(%d+)_color")
    if not hotcueIndex then
        print("Error: Could not parse hotcue index from control: " .. control)
        return
    end
    hotcueIndex = tonumber(hotcueIndex) - 1
    local nearestColorValue = PadColorMapper:getValueForNearestColor(value);
    print("nearestColorValue: " .. nearestColorValue)
    sendShortMsg(0x95 + midiChannel, hotcueIndex, nearestColorValue);
end
