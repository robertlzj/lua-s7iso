--[[
		[Homepage](https://snap7.sourceforge.net/)
		document:
			[snap7-full-x.y.z.7z] \ snap7-full-x.y.z \ doc - Snap7-refman.pdf
]]

local s7 = require('lua-s7iso')

local ip, rack, slot = '192.168.0.2', 0, 1

-- create TS7Client
local plc = s7.TS7Client.new()

local IsoErrors={--from "module_enums.cpp"
	[s7.IsoErrors.CONNECT]="errIsoConnect",	--"Connection error"
																					--	from "snap7.h"
	[s7.IsoErrors.DISCONNECT]="errIsoDisconnect",--"Disconnect error"
	--	...
}

-- connect to plc, print error in case of error
local ret,err = plc:connectTo(ip, rack, slot)
if not plc:isConnected() then
    print(ret, err)
		--	"10065	 TCP : Unreachable peer"
		--		from "s7_text.cpp"
    return
else
	local CpuStatus={--from "module_enums.cpp"
		[s7.CpuStatus.UNKNOWN]="S7CpuStatusUnknown",
		[s7.CpuStatus.RUN]="S7CpuStatusRun",
		[s7.CpuStatus.STOP]="S7CpuStatusStop",
	}
	print("PLC Status:",CpuStatus[plc:plcStatus()])
end

-- read a byte from MB1000 (unsigned per default)
value,err = plc:read('MB1000')
if value == nil then
    print("Error:",err)
end
print("Success:",value)

-- read a byte from MB1000, signed
value,err = plc:read('MB1000', s7.FormatHint.SIGNED)
if value == nil then
    print("Error:",err)
end
print("Success:",value)

-- read multi (3) bytes from MB1000, signed
values = {plc:read('MB1000', s7.FormatHint.SIGNED,3)}
if values[1] == nil then
    print("Error:",values[2])
end
print("Success:",table.unpack(values))

-- read a unsigned int (32bit) from MD1000
value,err = plc:read('MD1000', s7.FormatHint.UNSIGNED)
if value == nil then
    print("Error:",err)
end
print("Success:",value)

-- read a float (32bit) from MD1000
value,err = plc:read('MD1000', s7.FormatHint.FLOAT)
if value == nil then
    print("Error:",err)
end
print("Success:",value)

-- write 1 to byte at MB1000
-- FormatHint isn't necessary here, because this info is taken
-- directly from the lua data type
ret,err = plc:write('MB1000', 1)
if not ret then
    print(ret,err)
end
-- write 3 to byte at MB1002
ret,err = plc:write('MB1002', 3)
if not ret then
    print(ret,err)
end

-- disconnect from plc
plc:disconnect()