local out=assert(os.getenv('TMC_TEST_OUTPUT'))
local H=assert(loadfile(out..'../../tests/gameplay_harness.lua'))()
H.run(function()
 emu:write8(H.PM+16,1);emu:write8(0x2002C9E,H.r8(0x2002C9E)|0x20)
 for _,room in ipairs({{0x49,0,120,180,'chuchu'},{0x51,0,160,160,'gleerok'},{0x58,0x16,184,80,'mazaal'}}) do
  H.option('freezeEnemies',0);H.warp(room[1],room[2],room[3],room[4],1100);H.wait(400)
  -- Advance genuine boss-intro dialogue with real input, not action-byte forcing.
  for t=1,8 do H.press(1);H.wait(30) end
  if room[5]=='mazaal' then
   local found=false
   for i=0,71 do local p=0x30015A0+i*136;if H.r32(p+4)~=0 and H.r8(p+8)==3 then found=true end end
   if not found then
    H.log('FIXTURE raw-spawn Mazaal root in native Mazaal room')
    H.menu(26,1);H.press(1);H.option('actorRawKind',3);H.option('actorRawId',0x36)
    H.option('actorRawType',0);H.option('actorRawType2',0);H.option('actorRawFlags',0)
    H.menu(29,9);H.press(1);H.press(0x80);H.press(0x301);H.close();H.wait(120)
   end
  end
  local enemies={}
  for i=0,71 do local p=0x30015A0+i*136
   if H.r32(p+4)~=0 and H.r8(p+8)==3 then
    enemies[#enemies+1]=p;H.log(string.format('%s native actor %08X id=%02X act=%d flags=%02X enemyflags=%02X draw=%d',room[5],p,H.r8(p+9),H.r8(p+12),H.r8(p+16),H.r8(p+109),H.r8(p+24)&3))
   end
  end
  H.check(#enemies>0,room[5]..' native boss actors loaded')
  H.option('freezeEnemies',1);H.wait(12);local states={}
  for _,p in ipairs(enemies) do states[p]={H.r32(p+8),emu:readRange(p+12,4),emu:readRange(p+44,12)} end
  H.wait(120);H.shot(room[5]..'_frozen')
  for _,p in ipairs(enemies) do local s=states[p]
   H.check(H.r32(p+8)==s[1] and emu:readRange(p+12,4)==s[2] and emu:readRange(p+44,12)==s[3],room[5]..' boss component held '..string.format('%08X',p))
  end
  H.option('freezeEnemies',0);H.wait(180);H.shot(room[5]..'_resumed')
  local changed=false;for _,p in ipairs(enemies) do local s=states[p]
   if H.r32(p+8)~=s[1] or emu:readRange(p+12,4)~=s[2] or emu:readRange(p+44,12)~=s[3] then changed=true end
  end
  H.check(changed,room[5]..' native behavior resumes after unfreeze')
 end
end)
