-- WiFi Test Script for lhOS
-- Demonstrates WiFi connection and status checking

print("Starting WiFi test...")

-- Initialize WiFi
local ok = lhos.wifi.init()
if not ok then
    print("WiFi init failed")
    return
end

print("WiFi initialized")

-- Connect to network (replace with your SSID and password)
local ssid = "LinuxAcademy"
local password = "m@st3rk3y*12255221!!1234567890**"

ok = lhos.wifi.connect(ssid, password)
if not ok then
    print("WiFi connect failed")
    return
end

print("Connecting to " .. ssid .. "...")

-- Wait a bit and check status
lhos.yield(5000)  -- Wait 5 seconds

local status = lhos.wifi.get_status()
print("WiFi status: " .. status)

if status == "connected" then
    print("WiFi test successful!")
else
    print("WiFi test failed - not connected")
end

-- Disconnect
lhos.wifi.disconnect()
print("WiFi disconnected")