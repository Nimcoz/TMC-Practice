-- Local synthetic read model is not FPGA emulation. No runtime memory repairs.
-- Menu hotkey/A toggle and directional inputs exercise the actual user path.
local H=assert(loadfile(assert(os.getenv('TMC_HARDWARE_HARNESS'))))()
local enabled=false
local paths={move=0,explore=0,oldMove=0,oldExplore=0,auto=0}
local iterator={entries=0,exits=0,bad=0,sp=nil}
local function reg(n) return emu:readRegister(n)&0xFFFFFFFF end
for _,p in ipairs({{'move',0x09070064},{'explore',0x0907008C},
 {'oldMove',0x09010064},{'oldExplore',0x0901008C},{'auto',0x09020000}}) do
 emu:setBreakpoint(function() if enabled then paths[p[1]]=paths[p[1]]+1 end end,p[2])
end
local function violation(s)
 iterator.bad=iterator.bad+1
 if iterator.bad<12 then H.log('ITERATOR VIOLATION '..s) end
end
emu:setBreakpoint(function()
 if not enabled then return end
 if iterator.sp then violation(string.format('nested entry outerSP=%08X innerSP=%08X LR=%08X',iterator.sp,reg('sp'),reg('lr'))) end
 iterator.sp=reg('sp');iterator.entries=iterator.entries+1
end,0x03005F40)
emu:setBreakpoint(function()
 if not enabled then return end
 if iterator.sp and reg('sp')+36~=iterator.sp then violation('exit SP does not match native 36-byte frame') end
 iterator.sp=nil;iterator.exits=iterator.exits+1
end,0x03005FAC)
H.run(function()
 H.log('FIXTURE '..(os.getenv('TMC_RELOCATED_FIXTURE') or 'production ROM')..'; synthetic aperture model is NOT FPGA emulation')
 -- Dispatch literal offsets move when the module is rebuilt. Below, actual
 -- execution breakpoints verify both targets without binding to one layout.
 H.check(H.r32(0x09070000)==0x1C04B510,'relocated payload first word intact')
 if os.getenv('TMC_RELOCATED_FIXTURE') then
  H.check(H.r32(0x09000000)==1 and H.r32(0x09010000)==1 and H.r32(0x0901FFFC)==1,'synthetic old aperture reads observed word 00000001')
 end
 local base=emu:saveStateBuffer()
 local dirs={{'right',0x10},{'left',0x20},{'up',0x40},{'down',0x80},
 {'up_right',0x50},{'up_left',0x60},{'down_right',0x90},{'down_left',0xA0}}
 local cases=0
 for on=0,1 do for speed=0,2 do for _,dir in ipairs(dirs) do
  enabled=false;iterator.sp=nil
  emu:loadStateBuffer(base);emu:setKeys(0);H.wait(3)
  H.menu(9,0);if on==1 then H.press(1) end
  H.menu(9,1);for n=1,speed do H.press(1) end
  H.check(H.r8(H.PM+14)==on and H.r8(H.PM+H.L.speedMode)==speed,'native menu selection NoClip='..on..' speed='..speed)
  local m,e=paths.move,paths.explore
  local before=H.r16(H.M+12)
  enabled=true;H.close();H.wait(120,dir[2]);H.wait(8)
  enabled=false;iterator.sp=nil;cases=cases+1
  local tag='nc'..on..'_speed'..speed..'_'..dir[1]
  local nm,ne=paths.move-m,paths.explore-e
  H.log(string.format('CASE %02d %s ticks=%04X->%04X room=%02X/%02X xy=%d,%d move=%d explore=%d iterator=%d/%d bad=%d',cases,tag,before,H.r16(H.M+12),H.r8(H.R+4),H.r8(H.R+5),H.r16(H.P+46),H.r16(H.P+50),nm,ne,iterator.entries,iterator.exits,iterator.bad))
  H.check(H.r16(H.M+12)~=before,tag..' native frames continue')
  H.check((on==0 and nm==0 and ne==0) or (on==1 and nm>0 and ne>0),tag..' correct OFF/ON payload routing')
  H.check(paths.oldMove==0 and paths.oldExplore==0 and paths.auto==0 and iterator.bad==0,tag..' no old payload/AutoTest execution or nested native iterator/SP hazard')
 end end end
 H.check(cases==48,'completed OFF/ON x three speeds x eight directions')
 H.log(string.format('TOTAL cases=%d relocatedMove=%d relocatedExplore=%d oldMove=%d oldExplore=%d auto=%d iteratorEntries=%d iteratorExits=%d iteratorViolations=%d',cases,paths.move,paths.explore,paths.oldMove,paths.oldExplore,paths.auto,iterator.entries,iterator.exits,iterator.bad))
 H.shot('relocated_payload_final')
end)
