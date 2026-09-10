local out=assert(os.getenv('TMC_TEST_OUTPUT'))
local region=emu:read8(0x080000AF)==80 and 'EU' or 'JP'
local H=assert(loadfile(out..'../../build/regions/'..region..'/tests/gameplay_harness.lua'))()
H.boot=function()
 local nameKeys=0
 for i=1,8000 do
  local t,s,b=H.r8(H.M+2),H.r8(H.M+3),H.r8(H.M+4)
  local k=0
  if i%120==0 then
   if t==0 and i>650 then k=8 end
   if t==1 then
    local ui,kind=H.r8(0x2032EC2),H.r8(0x2000085)
    if ui==1 and kind==1 then
     k=nameKeys==0 and 1 or 8;nameKeys=nameKeys+1
     H.log('NATIVE new-file name input '..nameKeys)
    elseif ui==0 and H.r8(0x2019EE6)>0 then k=0x40
    else k=1 end
   end
  end
  if t==2 and s==2 and b==7 and H.r8(0x2032EC0)==2 and H.r8(0x2032EC2)==5 and H.r8(0x2000086)==2 and H.r8(0x3000FD0)==0 then
   H.log('NATIVE blank EEPROM -> name entry -> new-game story intro');return
  end
  if i%500==0 then H.log(string.format('BOOT %d/%d/%d ui=%d type=%d',t,s,b,H.r8(0x2032EC2),H.r8(0x2000085))) end
  H.wait(1,k)
 end
 H.shot('new_file_timeout');error('native new-file boot timeout')
end
H.run(function()
 H.wait(65);H.shot('new_game_intro')
 H.check(H.r8(0x2002A41)==1,'native new save initialized')
 local save=emu:readRange(0x2002A40,0x4B4)
 H.menu(0,0);H.shot('new_game_practice')
 H.check(H.r8(H.PM+H.L.modalMode)==1,'Practice opens during brand-new game intro')
 H.check(H.r8(H.PM+H.L.modalSaveEdits)==0,'fresh intro rejects save mutation')
 H.press(H.hotkey());H.wait(12)
 H.check(H.r8(H.PM+H.L.modalMode)==0,'new-game intro resumes')
 H.check(emu:readRange(0x2002A40,0x4B4)==save,'new-game intro save state preserved')
 H.shot('new_game_restored')
end)
