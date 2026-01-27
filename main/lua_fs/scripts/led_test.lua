-- LED Test Script for lhOS
-- Demonstrates LED control (blink LED on GPIO 2)

print("Starting LED test...")

-- Configure GPIO 48 as output
lhos.led.config(48)
print("LED configured on GPIO 48")
-- Blink LED 5 times
for i = 1, 5 do
    lhos.led.set(2, true)   -- Turn on
    print("LED ON")
    lhos.yield(500)         -- Wait 500ms

    lhos.led.set(2, false)  -- Turn off
    print("LED OFF")
    lhos.yield(500)         -- Wait 500ms
end

-- Check final state
local state = lhos.led.get(2)
print("Final LED state: " .. tostring(state))

print("LED test completed!")