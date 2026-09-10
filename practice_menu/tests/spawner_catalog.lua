local out=assert(os.getenv('TMC_TEST_OUTPUT'))
local H=assert(loadfile(out..'../../tests/gameplay_harness.lua'))()
local specs={
 {3,0,0},{3,8,0},{3,0x2C,0},{6,0,0x5F},{6,0,0x54},
 {3,1,0},{3,2,0},{3,0x0B,0},{3,0x12,0},{3,0x1E,0},{3,0x26,0},
 {3,0x30,0},{3,0x31,0},{3,0x32,0},{3,0x34,0},{3,0x22,0},{3,5,0},
 {6,0,0x55},{6,0,0x56},{6,0,0x57},{6,0,0x58},{6,0,0x5D},{6,0,0x5E},
 {6,0,0x61},{6,0,0x6C},{6,0,0x6E},
}
local rooms={overworld={0,0,504,504},house={0x22,0x10,88,88},temple={0x48,0,120,120},castle={0x80,0,216,256}}
local label=out:match('catalog_(%a+)/$') or 'overworld'
local room=assert(rooms[label])
local function floor(crow,leever)
 local cp,ap=H.r32(0x800024C),H.r32(0x800027C)
 local ox,oy,w,h=H.r16(H.R+6),H.r16(H.R+8),H.r16(H.R+30),H.r16(H.R+32)
 local best,bestx,besty=-1,nil,nil
 local sine=H.r32(0x8002978) -- native literal gSineTable
 local function s16(a) local v=H.r16(a);return v>=32768 and v-65536 or v end
 local function rays(x,y)
  local count=0
  for a=0,31 do
   local sx,sy=s16(sine+a*16)*2048,s16(sine+(a*8+64)*2)*2048
   local wx,wy=x*65536,y*65536;local valid=true
   for i=1,8 do
    wx=wx+sx;wy=wy-sy
    local px,py=wx//65536-ox,wy//65536-oy
    if px<0 or py<0 or px>=w or py>=h then valid=false;break end
    local p=(py//16)*64+px//16;local act=H.r8(ap+p)
    if H.r8(cp+p)~=0 or act<9 or act>12 then valid=false;break end
   end
   if valid then count=count+1 end
  end
  return count
 end
 for ty=2,h//16-2 do for tx=2,w//16-3 do
  local ok=true
  -- Feet at tile Y+12 keep the full -9..0 footprint inside one tile.
  -- Sample both Link and target, matching ordinary/clone-surface policy.
  for _,dx in ipairs({0,2}) do local p=ty*64+tx+dx
   local surface=H.surface(H.r8(ap+p))
   if H.r8(cp+p)~=0 or (surface~=0 and surface~=0x15) then ok=false end
  end
  if crow then
   local p=(ty+1)*64+tx+2
   local surface=H.surface(H.r8(ap+p))
   if H.r8(cp+p)~=0 or (surface~=0 and surface~=0x15) then ok=false end
  end
  local x,y=ox+tx*16+8,oy+ty*16+12
  for i=0,71 do local p=0x30015A0+i*136
   if H.r32(p+4)~=0 and math.abs(H.r16(p+46)-(x+32))<18 and math.abs(H.r16(p+50)-y)<18 then ok=false end
  end
  if ok then
   if not leever then bestx,besty=x,y;break end
   local score=rays(x,y)
   if score>best then best,bestx,besty=score,x,y end
  end
 end
 if bestx and not leever then break end
 end
 if bestx then
  if leever then H.log('FIXTURE Leever eligible native 8-step rays='..best..'/32') end
  emu:write32(H.P+44,bestx*65536);emu:write32(H.P+48,besty*65536);emu:write8(H.P+20,2);H.wait(10);return best
 end
 H.log(string.format('FLOOR unavailable layer=%d collision=%08X act=%08X size=%d,%d xy=%d,%d',H.r8(H.P+56),cp,ap,w,h,H.r16(H.P+46),H.r16(H.P+50)))
 H.shot('floor_unavailable')
 error('no ordinary spawn floor')
end
local function freezeActive(e)
 return H.r8(e+12)>0 and (H.r8(e+16)&0x80)~=0 and (H.r8(e+24)&3)~=0 and
 H.r32(e+80)==0 and H.r32(e+84)==0 and (H.r8(e+0x6D)&0x71)==0
end
H.run(function()
 H.check(H.L.actorSpawnCount==#specs,'all declared catalog entries match independent count')
 H.action(14,0);emu:write8(H.PM+16,1)
 for index,s in ipairs(specs) do
  local tag=string.format('%s_%02d_%02X',label,index-1,s[2])
  H.warp(room[1],room[2],room[3],room[4]);H.wait(30)
  local paths=floor(s[1]==3 and s[2]==0x31,s[1]==3 and s[2]==2)
  -- Test-only resource setup ensures every pickup has space; no native save.
  emu:write8(0x2002AEA,8);emu:write8(0x2002AEC,1);emu:write8(0x2002AED,1)
  emu:write16(0x2002B00,10);emu:write16(0x2002B02,10)
  local flags=emu:readRange(0x2002C9C,512)
  H.menu(29,1);H.option('actorSpawn',index-1);H.wait(2);H.shot(tag..'_menu');H.press(1);H.wait(75);H.close()
  local e=H.r32(H.PM+H.L.actorSelected)
  H.check(e>=0x30015A0 and e<0x3003BE0,tag..' allocated')
  if e<0x30015A0 or e>=0x3003BE0 then
   H.log(string.format('SPAWN DIAGNOSTIC status=%s xy=%d,%d anim=%d action=%d flags=%08X message=%d',emu:readRange(H.PM+H.L.status,28),H.r16(H.P+46),H.r16(H.P+50),H.r8(H.P+20),H.r8(H.P+12),H.r32(H.S+0x30),H.r8(0x2000050)))
   H.shot(tag..'_spawn_failed')
  end
  if e>=0x30015A0 and e<0x3003BE0 then
   local identity=H.r32(e+8)
   H.check(H.r8(e+8)==s[1] and H.r8(e+9)==s[2] and H.r8(e+10)==s[3],tag..' exact native kind/id/type')
   H.check(H.r8(e+12)>0 and (H.r8(e+16)&1)~=0,tag..' native initialization complete')
   if s[1]==3 then
    -- Crow's native idle action only launches when Link is below it (>8px).
    -- Place Link on the verified floor below; do not mutate the enemy action.
    if s[2]==0x31 then
     emu:write32(H.P+44,H.r32(e+44));emu:write32(H.P+48,H.r32(e+48)+16*65536)
    end
    local visible=false
    for t=1,720 do
     if H.r32(e+4)==0 or H.r32(e+8)~=identity then break end
     if freezeActive(e) then visible=true;break end
     H.wait(1)
    end
    local buried=s[2]==2 and paths==0
    if buried then
     H.check(not visible and H.r8(e+12)==1 and (H.r8(e+24)&3)==0 and (H.r8(e+16)&0x80)==0,tag..' no forced Leever emergence without native path')
    else H.check(visible,tag..' visible collidable independent actor') end
    H.shot(tag..'_native')
    local m=H.PM+H.L.actorMarks+(e-0x30015A0)//136
    if visible then
     H.menu(28,0);H.press(1);H.close();H.wait(10)
     local x,y,t=H.r32(e+44),H.r32(e+48),H.r8(e+14)
     H.wait(60)
     H.check((H.r8(m)&1)==1 and H.r32(e+44)==x and H.r32(e+48)==y and H.r8(e+14)==t,tag..' individual freeze60')
     H.shot(tag..'_frozen')
    end
    if visible or buried then
     H.menu(28,1);H.press(1)
     H.check(H.r8(H.PM+H.L.page)==30,tag..' guarded removal available')
     if H.r8(H.PM+H.L.page)==30 then H.press(0x80);H.press(0x301);H.wait(45);H.close()
      H.check(H.r32(H.PM+H.L.actorSelected)==0 and H.r8(m)==0,tag..' native cleanup and handle invalidation')
     else H.close() end
    end
   else
    H.shot(tag..'_native')
    local expected={[0x5F]={0x2002AEA,16,1},[0x54]={0x2002B00,11,2},[0x55]={0x2002B00,15,2},
     [0x56]={0x2002B00,30,2},[0x57]={0x2002B00,60,2},[0x58]={0x2002B00,110,2},
     [0x5D]={0x2002AEC,6,1},[0x5E]={0x2002AED,6,1},[0x61]={0x2002B02,11,2},
     [0x6C]={0x2002AEC,11,1},[0x6E]={0x2002AED,11,1}}
    local effect=assert(expected[s[3]])
    emu:write32(H.P+44,H.r32(e+44));emu:write32(H.P+48,H.r32(e+48));H.wait(140)
    local value=effect[3]==1 and H.r8(effect[1]) or H.r16(effect[1])
    H.check(value==effect[2],tag..' native pickup result '..value..' expected '..effect[2])
    H.check(H.r32(H.PM+H.L.actorSelected)==0,tag..' native pickup deletes selected handle')
    H.shot(tag..'_collected')
    -- A first pickup can open the native item-get text. Dismiss with real A
    -- input before asking for another spawn; scene guards must stay enabled.
    for t=1,60 do
     if (H.r8(0x2000050)&0x7F)==0 and H.r8(H.S+0x8B)==0 then break end
     H.press(1);H.wait(8)
    end
    H.wait(30)
    H.check((H.r8(0x2000050)&0x7F)==0 and H.r8(H.S+0x8B)==0,tag..' native item-get dialog closed by input')
    -- A second instance verifies guarded native removal, independently of pickup.
    floor();H.menu(29,1);H.option('actorSpawn',index-1);H.press(1);H.wait(75);H.close()
    local p=H.r32(H.PM+H.L.actorSelected)
    H.check(p>=0x30015A0 and p<0x3003BE0,tag..' second pickup allocated')
    if p>=0x30015A0 and p<0x3003BE0 then
     local m=H.PM+H.L.actorMarks+(p-0x30015A0)//136
     H.menu(28,1);H.press(1)
     H.check(H.r8(H.PM+H.L.page)==30,tag..' pickup removal confirmation')
     if H.r8(H.PM+H.L.page)==30 then H.press(0x80);H.press(0x301);H.wait(45);H.close()
      H.check(H.r32(H.PM+H.L.actorSelected)==0 and H.r8(m)==0,tag..' pickup native cleanup')
     else H.close() end
    end
   end
   H.check(emu:readRange(0x2002C9C,512)==flags,tag..' story/local flags unchanged')
   H.check(H.r8(H.R+4)==room[1] and H.r8(H.R+5)==room[2],tag..' no artificial room change')
  end
  H.close()
 end
end)
