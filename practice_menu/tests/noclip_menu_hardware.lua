-- Controller-only No Clip activation regression. Unlike the older movement
-- fixture, never write the noClip flag or menu selection from the host.
local out=assert(os.getenv('TMC_TEST_OUTPUT'))
local harness=os.getenv('TMC_HARDWARE_HARNESS') or out..'../../tests/gameplay_harness.lua'
local H=assert(loadfile(harness))()
H.run(function()
 local function navigate(address,target,key,limit)
  for attempt=1,limit do
   if H.r8(address)==target then return end
   H.press(key)
  end
  assert(H.r8(address)==target,'controller navigation timed out')
 end
 local function menu()
  H.press(H.hotkey()); H.wait(35)
  assert(H.r8(H.PM+13)==1,'menu must open')
  navigate(H.PM+H.L.page,0,2,8)
  navigate(H.PM+H.L.cursor,2,0x80,11)
  H.press(1)
  assert(H.r8(H.PM+H.L.page)==9,'Movement page must open')
  navigate(H.PM+H.L.cursor+9,0,0x40,7)
 end
 H.check(H.r8(H.PM+H.L.noClip)==0,'No Clip defaults OFF on native boot')
 H.shot('before_activation')
 for iteration=1,4 do
  menu(); H.press(1)
  H.check(H.r8(H.PM+H.L.noClip)==1,'No Clip ON via controller '..iteration)
  H.wait(90); H.shot('menu_on_'..iteration)
  H.close(); local ticks=H.r16(H.M+12); H.wait(90)
  H.check(H.r16(H.M+12)~=ticks,'native main-loop ticks advance after closing '..iteration)
  H.check(H.r8(H.M+3)==2 and H.r8(H.M+4)==2 and H.r8(H.PM+13)==0,
          'native gameplay resumes with No Clip ON '..iteration)
  H.shot('resumed_on_'..iteration)
  for _,key in ipairs({0x10,0x20,0x40,0x80}) do H.wait(20,key); H.wait(5) end
  -- The supplied real save starts directly below the courtyard entrance.
  -- Directional input can start a legitimate room transition; do not treat
  -- its temporary menu gate as a frozen game.
  H.wait(240)
  H.check(H.r8(H.M+3)==2 and H.r8(H.M+4)==2 and H.r8(0x03000FD0)==0,
          'native room transition settles before reopening '..iteration)
  menu(); H.press(1)
  H.check(H.r8(H.PM+H.L.noClip)==0,'No Clip OFF via controller '..iteration)
  H.close(); H.wait(30)
 end
 H.check(true,'four native menu open/toggle/close cycles finished')
end)
