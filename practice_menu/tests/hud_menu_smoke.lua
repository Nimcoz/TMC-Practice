local frame = 0
local phase = "boot"
local phase_frame = 0
local gameplay_frames = 0
local last_task = -1
local task_frames = 0
local out_dir = "C:/Users/user/Documents/Codex/2026-08-30/files-mentioned-by-the-user-tmc/practice_menu/test_results/hud_menu_improvements/"
local log_path = out_dir .. "hud_menu_smoke.log"
local done_path = out_dir .. "hud_menu_smoke.done"

local MAIN = 0x03001000
local INPUT = 0x03000FF0
local PRACTICE = 0x0203D000
local BG0 = 0x02034CB0

local KEY_A = 0x001
local KEY_SELECT = 0x004
local KEY_START = 0x008
local KEY_DOWN = 0x080
local KEY_R = 0x100
local KEY_L = 0x200
local HOTKEY = KEY_L + KEY_R + KEY_SELECT

local PAGE_ROOT = 0
local PAGE_PLAYER = 2
local PAGE_FLAGS = 11
local PAGE_ELEMENTS = 15

local function u32(value)
    if value < 0 then return value + 4294967296 end
    return value
end

local function log(tag)
    local f = io.open(log_path, "a")
    if not f then return end
    f:write(string.format(
        "%s frame=%d phase=%s pc=%08X task=%02X state=%02X sub=%02X held=%04X pressed=%04X magic=%08X menu=%02X page=%02X timer=%u debug=%02X tileT=%04X tileA=%04X\n",
        tag, frame, phase, u32(emu:readRegister("pc")),
        emu:read8(MAIN + 2), emu:read8(MAIN + 3), emu:read8(MAIN + 4),
        emu:read16(INPUT), emu:read16(INPUT + 2), u32(emu:read32(PRACTICE)),
        emu:read8(PRACTICE + 13), emu:read8(PRACTICE + 23),
        u32(emu:read32(PRACTICE + 8)), emu:read8(PRACTICE + 22),
        emu:read16(BG0 + (10 * 2)), emu:read16(BG0 + ((5 * 32 + 20) * 2))))
    f:close()
end

local function set_phase(next_phase)
    phase = next_phase
    phase_frame = frame
    log("PHASE")
end

local function finish(result)
    log(result)
    local f = io.open(done_path, "w")
    if f then
        f:write(result .. "\n")
        f:close()
    end
    phase = "done"
    emu:setKeys(0)
end

callbacks:add("frame", function()
    frame = frame + 1
    if frame == 1 then
        os.remove(log_path)
        os.remove(done_path)
        log("SCRIPT_START")
    end

    local task = emu:read8(MAIN + 2)
    local state = emu:read8(MAIN + 3)
    local substate = emu:read8(MAIN + 4)
    local magic = u32(emu:read32(PRACTICE))
    local menu_open = emu:read8(PRACTICE + 13)
    local page = emu:read8(PRACTICE + 23)

    if task ~= last_task then
        last_task = task
        task_frames = 0
    else
        task_frames = task_frames + 1
    end

    if frame % 120 == 0 then log("TICK") end
    local keys = 0

    if phase == "boot" then
        if task == 0 and frame >= 650 and ((frame - 650) % 120) < 4 then keys = KEY_START end
        if task == 1 then
            if state == 1 and task_frames >= 120 then emu:write8(MAIN + 3, 2) end
            if task_frames >= 90 and ((task_frames - 90) % 120) < 4 then keys = KEY_A end
        end
        if task == 2 and state == 2 and substate == 2 and magic == 0x504D4331 then
            gameplay_frames = gameplay_frames + 1
            if gameplay_frames >= 45 then set_phase("warm_open_press") end
        end
        if frame > 2400 then finish("FAIL_BOOT_TIMEOUT") end
    elseif phase == "warm_open_press" then
        if frame - phase_frame < 4 then keys = HOTKEY else set_phase("warm_wait_root") end
    elseif phase == "warm_wait_root" then
        if menu_open == 1 and page == PAGE_ROOT then set_phase("warm_close_press")
        elseif frame - phase_frame > 180 then finish("FAIL_WARM_OPEN") end
    elseif phase == "warm_close_press" then
        if frame - phase_frame < 4 then keys = HOTKEY else set_phase("warm_wait_close") end
    elseif phase == "warm_wait_close" then
        if menu_open == 0 and task == 2 and state == 2 and substate == 2 then
            set_phase("baseline_settle")
        elseif frame - phase_frame > 180 then finish("FAIL_WARM_CLOSE") end
    elseif phase == "baseline_settle" then
        if frame - phase_frame >= 45 then
            emu:screenshot(out_dir .. "01_gameplay_no_overlay.png")
            emu:write32(PRACTICE + 8, 3723)
            emu:write8(PRACTICE + 12, 0)
            set_phase("timer_settle")
        end
    elseif phase == "timer_settle" then
        if frame - phase_frame >= 12 then
            log("TIMER_VISIBLE")
            emu:screenshot(out_dir .. "02_gameplay_timer.png")
            emu:write8(PRACTICE + 22, 1)
            set_phase("debug_settle")
        end
    elseif phase == "debug_settle" then
        if frame - phase_frame >= 12 then
            log("DEBUG_VISIBLE")
            emu:screenshot(out_dir .. "03_gameplay_timer_debug.png")
            emu:write32(PRACTICE + 8, 0)
            emu:write8(PRACTICE + 22, 0)
            set_phase("clear_settle")
        end
    elseif phase == "clear_settle" then
        if frame - phase_frame >= 12 then
            log("HUD_CLEARED")
            emu:screenshot(out_dir .. "04_gameplay_overlay_cleared.png")
            set_phase("open_root_press")
        end
    elseif phase == "open_root_press" then
        if frame - phase_frame < 4 then keys = HOTKEY else set_phase("wait_root") end
    elseif phase == "wait_root" then
        if menu_open == 1 and page == PAGE_ROOT then set_phase("root_down_press")
        elseif frame - phase_frame > 180 then finish("FAIL_ROOT_OPEN") end
    elseif phase == "root_down_press" then
        if frame - phase_frame < 3 then keys = KEY_DOWN else set_phase("player_press") end
    elseif phase == "player_press" then
        if frame - phase_frame < 3 then keys = KEY_A else set_phase("wait_player") end
    elseif phase == "wait_player" then
        if menu_open == 1 and page == PAGE_PLAYER then set_phase("close_player_press")
        elseif frame - phase_frame > 90 then finish("FAIL_PLAYER_NAVIGATION") end
    elseif phase == "close_player_press" then
        if frame - phase_frame < 4 then keys = HOTKEY else set_phase("wait_player_close") end
    elseif phase == "wait_player_close" then
        if menu_open == 0 and task == 2 and state == 2 and substate == 2 then set_phase("reopen_player_press")
        elseif frame - phase_frame > 180 then finish("FAIL_PLAYER_CLOSE") end
    elseif phase == "reopen_player_press" then
        if frame - phase_frame < 4 then keys = HOTKEY else set_phase("wait_player_reopen") end
    elseif phase == "wait_player_reopen" then
        if menu_open == 1 then
            if page ~= PAGE_PLAYER then finish("FAIL_PAGE_NOT_PERSISTED")
            else set_phase("player_persist_settle") end
        elseif frame - phase_frame > 180 then finish("FAIL_PLAYER_REOPEN") end
    elseif phase == "player_persist_settle" then
        if frame - phase_frame >= 12 then
            log("PAGE_PERSISTED")
            emu:screenshot(out_dir .. "05_menu_player_persisted.png")
            emu:write8(PRACTICE + 23, PAGE_ELEMENTS)
            set_phase("elements_settle")
        end
    elseif phase == "elements_settle" then
        if frame - phase_frame >= 12 then
            log("ELEMENTS_PAGE")
            emu:screenshot(out_dir .. "06_menu_elements.png")
            emu:write8(PRACTICE + 23, PAGE_FLAGS)
            set_phase("flags_settle")
        end
    elseif phase == "flags_settle" then
        if frame - phase_frame >= 12 then
            log("FLAGS_PAGE")
            emu:screenshot(out_dir .. "07_menu_flags_overview.png")
            finish("PASS")
        end
    end

    emu:setKeys(keys)
end)
