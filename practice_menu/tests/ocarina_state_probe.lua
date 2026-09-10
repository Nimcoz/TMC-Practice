local out=assert(os.getenv('TMC_TEST_OUTPUT'))
local f=assert(io.open(out..'trace.log','w'))
local n=0
local function log(tag)
 f:write(string.format('%s frame=%d flags=%08X ef=%02X priority=%02X/%02X pause=%d action=%d sub=%d anim=%04X frame=%02X z=%08X vz=%08X field27=%02X floor=%02X last=%02X keep=%02X mobility=%02X active=%02X/%02X\n',tag,n,emu:read32(0x3003FB0),emu:read8(0x3001170),emu:read8(0x3003DC0),emu:read8(0x3001171),emu:read8(0x2034490),emu:read8(0x300116C),emu:read8(0x300116D),emu:read16(0x3003F88),emu:read8(0x30011B8),emu:read32(0x3001194),emu:read32(0x3001180),emu:read8(0x3003FA7),emu:read8(0x3003F92),emu:read8(0x3003F93),emu:read8(0x3003F8B),emu:read8(0x3003F9A),emu:read8(0x3000B80),emu:read8(0x3000B81)))
end
callbacks:add('frame',function()
 n=n+1;local k=0
 if n==1 then assert(emu:loadStateFile('C:/Users/user/Documents/Codex/2026-08-30/files-mentioned-by-the-user-tmc/practice_menu/test_results/ocarina_native_house/native_glitch.ss0'));log('LOADED_NATIVE_STAGE1') end
 if n==2 then
  local d=assert(io.open(out..'stage1_player.bin','wb'));d:write(emu:readRange(0x3001160,0x88));d:close()
 end
 if n>=10 and n<=12 then k=1 end
 if n%10==0 then log('PLAY_OCARINA_TO_CANCEL') end
 if n==220 then
  emu:screenshot(out..'after_native_cancel.png');log('COMPLETE');f:close()
  local d=assert(io.open(out..'done.txt','w'));d:write('NATIVE_CANCEL_CAPTURED');d:close();n=-100000
 end
 emu:setKeys(k)
end)
