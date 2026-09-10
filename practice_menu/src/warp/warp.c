#include "practice.h"

#define AREA_COUNT 0x90u
#define ROOM_LIMIT 0x40u
#include "warp_metadata.h"

s32 Warp_FavoriteIndex(void) {
    u32 i;
    for(i=0;i<MAX_WARP_FAVORITES;i++)
        if(gPracticeState.favorites[i][0]==gPracticeState.selectedArea &&
           gPracticeState.favorites[i][1]==gPracticeState.selectedRoom) return i;
    return -1;
}

void Warp_ToggleFavorite(void) {
    s32 index=Warp_FavoriteIndex(); u32 i;
    if(index>=0) {
        gPracticeState.favorites[index][0]=gPracticeState.favorites[index][1]=0xFF;
        PracticeRuntime_SetStatus("FAVORITE REMOVED - SAVE SETUP"); return;
    }
    if(!Warp_IsValid(gPracticeState.selectedArea,gPracticeState.selectedRoom)) {
        PracticeRuntime_SetStatus("INVALID FAVORITE"); return;
    }
    for(i=0;i<MAX_WARP_FAVORITES;i++) if(gPracticeState.favorites[i][0]==0xFF) {
        gPracticeState.favorites[i][0]=gPracticeState.selectedArea;
        gPracticeState.favorites[i][1]=gPracticeState.selectedRoom;
        PracticeRuntime_SetStatus("FAVORITE ADDED - SAVE SETUP"); return;
    }
    PracticeRuntime_SetStatus("8 FAVORITES FULL - REMOVE ONE");
}

const char* Warp_AreaName(u32 area) {
    return area < AREA_COUNT && sAreaNames[area] ? sAreaNames[area] : "UNNAMED AREA";
}

const WarpRoomInfo* Warp_RoomInfo(u32 area, u32 room) {
    u32 i;
    for (i = 0; i < ARRAY_COUNT(sRoomNames); i++)
        if (sRoomNames[i].area == area && sRoomNames[i].room == room) return &sRoomNames[i];
    return 0;
}

static const TmcRoomHeader* area_headers(u32 area) {
    const TmcRoomHeader* const* table = (const TmcRoomHeader* const*)ADDR_AREA_ROOM_HEADERS;
    uptr pointer;
    if (area >= AREA_COUNT) return 0;
    pointer = (uptr)table[area];
    if (pointer < 0x08000000u || pointer >= 0x0A000000u) return 0;
    return (const TmcRoomHeader*)pointer;
}

const TmcRoomHeader* Warp_GetHeader(u32 area, u32 room) {
    const TmcRoomHeader* headers = area_headers(area);
    u32 index;
    if (headers == 0 || room >= ROOM_LIMIT) return 0;
    for (index = 0; index <= room; index++) {
        if (headers[index].mapX == 0xFFFFu) return 0;
    }
    if (headers[room].width == 0 || headers[room].height == 0) return 0;
    return &headers[room];
}

u32 Warp_IsValid(u32 area, u32 room) {
    return Warp_GetHeader(area, room) != 0;
}

static u32 area_has_room(u32 area) {
    u32 room;
    for (room = 0; room < ROOM_LIMIT; room++) {
        if (Warp_IsValid(area, room)) return 1;
    }
    return 0;
}

void Warp_AdjustArea(s32 direction) {
    s32 area = gPracticeState.selectedArea;
    u32 attempts;
    for (attempts = 0; attempts < AREA_COUNT; attempts++) {
        area += direction;
        if (area < 0) area = AREA_COUNT - 1;
        if (area >= (s32)AREA_COUNT) area = 0;
        if (area_has_room((u32)area)) {
            gPracticeState.selectedArea = (u8)area;
            gPracticeState.selectedRoom = 0;
            while (!Warp_IsValid(gPracticeState.selectedArea, gPracticeState.selectedRoom)) {
                gPracticeState.selectedRoom++;
            }
            return;
        }
    }
}

void Warp_AdjustRoom(s32 direction) {
    s32 room = gPracticeState.selectedRoom;
    u32 attempts;
    for (attempts = 0; attempts < ROOM_LIMIT; attempts++) {
        room += direction;
        if (room < 0) room = ROOM_LIMIT - 1;
        if (room >= (s32)ROOM_LIMIT) room = 0;
        if (Warp_IsValid(gPracticeState.selectedArea, (u32)room)) {
            gPracticeState.selectedRoom = (u8)room;
            return;
        }
    }
}
