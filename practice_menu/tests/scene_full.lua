local out=assert(os.getenv('TMC_TEST_OUTPUT'))
local H=assert(loadfile(out..'../../tests/gameplay_harness.lua'))()
local UI,MENU,FADE=0x2032EC0,0x2000080,0x3000FD0
local function mode() return H.r8(H.PM+H.L.sceneReplay) end
H.run(function()
 for _,row in ipairs({5,6}) do
  H.menu(8,row);H.press(1);H.press(1)
  for i=1,250 do if mode()~=0 then break end;H.wait(1) end
  assert(mode()~=0,'scene start missing')
  local save=emu:readRange(H.L.sceneBackup,0x4B4)
  local seen={};local credits=false;local count=0;local creditMenu=false
  for i=1,60000 do
   if mode()==0 then break end
   local task,state,sub=H.r8(H.M+2),H.r8(H.M+3),H.r8(H.M+4)
   local key=string.format('%d/%d/%d room%02X/%02X ui%d overlay%d',task,state,sub,H.r8(H.R+4),H.r8(H.R+5),H.r8(UI+2),H.r8(MENU+6))
   if not seen[key] then H.log('NATIVE '..key);seen[key]=true end
   if task==4 then
    if not credits then H.shot('full_ending_credits');credits=true end
    if state==1 and H.r8(FADE)==0 and not creditMenu then
     H.wait(40);H.press(H.hotkey());H.wait(8)
     H.check(H.r8(H.PM+H.L.modalMode)==1,'Practice opens during replay credits')
     H.shot('replay_credits_menu');H.press(H.hotkey());H.wait(8);creditMenu=true
     H.check(emu:readRange(H.L.sceneBackup,0x4B4)==save,'modal graphics do not overwrite replay save backup')
    end
    assert(not(state==2 and H.r8(MENU+6)==1),'replay entered native end-save prompt')
    -- Accelerate waiting only, not native entries, scripts or scene indices.
    local kind,overlay=H.r8(MENU+5),H.r8(MENU+6)
    if state==1 and (kind==0 or kind==3 or (kind==2 and overlay==2)) and H.r16(MENU+8)>2 then emu:write16(MENU+8,2) end
   end
   H.wait(1,(i%16<5) and 1 or 0);count=i
  end
  H.check(mode()==0,'full native replay auto-returns '..row..' frames='..count)
  H.check(emu:readRange(0x2002A40,0x4B4)==save,'full replay restores every save byte '..row)
  if row==6 then H.check(credits,'full ending naturally reaches credits') end
  H.shot('full_return_'..row);H.wait(35)
 end
end)
