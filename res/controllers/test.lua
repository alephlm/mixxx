-- Test Lua script for Mixxx controller
function testMidiFunction(channel, control, value, status, group)
    print("Lua script called: channel=" .. channel .. ", control=" .. control .. ", value=" .. value .. ", status=" .. status .. ", group=" .. group)
end
