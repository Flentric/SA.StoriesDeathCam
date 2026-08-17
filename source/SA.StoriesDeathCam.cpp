#include "plugin.h"
#include "common.h"
#include "CPlayerPed.h"
#include "CWorld.h"
#include "CCamera.h"
#include "CCam.h"
#include "CTimer.h"
#include "CSprite2d.h"
#include "CMenuManager.h"
#include <windows.h>
#include <string>
#include <cmath>

using namespace plugin;

// replaces CCam::ProcessPedsDeadBaby (SA's death cam)
static const uintptr_t PROCESS_PED_DEAD_BABY = 0x519250;
static const float kPI = 3.14159265f;

class DeathCam {
public:
    static bool active;
    static unsigned int startTime;
    static unsigned int lastCallTime;
    static float startAngle;
    static CVector lastGoodPos;
    static CVector lastGoodForward;
    static std::string iniPath;
    static int reloadTimer;

    static float radius, radiusGrow, heightStart, riseSpeed, spinSpeed, lookHeight, fov;
    static float washDelay, washInTime;
    static int fadeR, fadeG, fadeB, maxWash;
    static int wastedDelayMs;

    static std::string AsiFolder() {
        char path[MAX_PATH] = { 0 };
        HMODULE hm = NULL;
        GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            (LPCSTR)&AsiFolder, &hm);
        GetModuleFileNameA(hm, path, MAX_PATH);
        std::string p(path);
        size_t slash = p.find_last_of("\\/");
        return (slash != std::string::npos) ? p.substr(0, slash + 1) : "";
    }

    static float ReadF(const char* key, float def) {
        char buf[64];
        GetPrivateProfileStringA("StoriesDeathCam", key, "", buf, sizeof(buf), iniPath.c_str());
        return buf[0] ? (float)atof(buf) : def;
    }

    static void LoadConfig() {
        radius      = ReadF("Radius", 0.0f);
        radiusGrow  = ReadF("RadiusGrow", 0.0f);
        heightStart = ReadF("HeightStart", 2.0f);
        riseSpeed   = ReadF("RiseSpeed", 2.5f);
        spinSpeed   = ReadF("SpinSpeed", 55.0f);
        lookHeight  = ReadF("LookHeight", 0.3f);
        fov         = ReadF("FOV", 70.0f);
        wastedDelayMs = (int)ReadF("WastedDelayMs", 2000.0f);
        washDelay   = ReadF("WashDelay", 2.3f);
        washInTime  = ReadF("WashInTime", 1.2f);
        fadeR = (int)ReadF("FadeR", 110.0f);
        fadeG = (int)ReadF("FadeG", 112.0f);
        fadeB = (int)ReadF("FadeB", 110.0f);
        maxWash = (int)ReadF("MaxWash", 255.0f);
    }

    static void Normalise(CVector& v) {
        float l = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
        if (l > 0.0001f) { v.x /= l; v.y /= l; v.z /= l; }
    }

    static float Elapsed() {
        return (CTimer::m_snTimeInMilliseconds - startTime) / 1000.0f;
    }

    // replacement for CCam::ProcessPedsDeadBaby (thiscall -> __fastcall)
    static void __fastcall ProcessHook(CCam* self, void*) {
        unsigned int now = CTimer::m_snTimeInMilliseconds;
        bool reset = (!active || now - lastCallTime > 250);
        lastCallTime = now;
        active = true;

        CVector target = lastGoodPos;
        if (CPlayerPed* ped = FindPlayerPed()) {
            CVector pp = ped->GetPosition();
            if (pp.x != 0.0f || pp.y != 0.0f) target = pp;
        }

        if (reset) {
            startTime = now;
            startAngle = atan2f(-lastGoodForward.y, -lastGoodForward.x);   // start behind the player
        }

        float t = Elapsed();
        float ang = startAngle + spinSpeed * (kPI / 180.0f) * t;
        float r = radius + radiusGrow * t;
        float h = heightStart + riseSpeed * t;

        CVector look(target.x, target.y, target.z + lookHeight);
        CVector cam(target.x + cosf(ang) * r, target.y + sinf(ang) * r, target.z + h);

        CVector front(look.x - cam.x, look.y - cam.y, look.z - cam.z);
        Normalise(front);
        // perp to front, orbit-tangent fallback when near top-down
        CVector right(front.y, -front.x, 0.0f);
        float rl = sqrtf(right.x * right.x + right.y * right.y);
        if (rl > 0.01f) { right.x /= rl; right.y /= rl; }
        else { right.x = -sinf(ang); right.y = cosf(ang); right.z = 0.0f; }
        CVector up(right.y * front.z - right.z * front.y,
                   right.z * front.x - right.x * front.z,
                   right.x * front.y - right.y * front.x);
        Normalise(up);

        self->m_vecSource = cam;
        self->m_vecFront = front;
        self->m_vecUp = up;
        self->m_vecTargetCoorsForFudgeInter = look;
        self->m_fFOV = fov;
    }

    static void Process() {
        if (--reloadTimer <= 0) { LoadConfig(); reloadTimer = 100; }

        if (CPlayerPed* ped = FindPlayerPed()) {
            if (ped->m_fHealth > 0.0f) {
                lastGoodPos = ped->GetPosition();
                lastGoodForward = ped->GetForward();
            }
        }

        if (active && CTimer::m_snTimeInMilliseconds - lastCallTime > 250)
            active = false;   // leave the fade colour grey for the hospital fade-in
        if (active)
            TheCamera.SetFadeColour((unsigned char)fadeR, (unsigned char)fadeG, (unsigned char)fadeB);
    }

    static void DrawWash() {
        if (!active) return;
        if (FrontEndMenuManager.m_bMenuActive) return;

        float k = washInTime > 0.0f ? (Elapsed() - washDelay) / washInTime : 1.0f;
        if (k < 0.0f) k = 0.0f;
        if (k > 1.0f) k = 1.0f;
        int a = (int)(k * maxWash);
        if (a <= 0) return;

        float w = (float)RsGlobal.maximumWidth;
        float h = (float)RsGlobal.maximumHeight;
        CSprite2d::DrawRect(CRect(0.0f, 0.0f, w, h),
            CRGBA((unsigned char)fadeR, (unsigned char)fadeG, (unsigned char)fadeB, (unsigned char)a));
    }
};

bool DeathCam::active = false;
unsigned int DeathCam::startTime = 0;
unsigned int DeathCam::lastCallTime = 0;
float DeathCam::startAngle = 0.0f;
CVector DeathCam::lastGoodPos;
CVector DeathCam::lastGoodForward(1.0f, 0.0f, 0.0f);
std::string DeathCam::iniPath;
int DeathCam::reloadTimer = 0;
float DeathCam::radius, DeathCam::radiusGrow, DeathCam::heightStart, DeathCam::riseSpeed, DeathCam::spinSpeed, DeathCam::lookHeight, DeathCam::fov;
float DeathCam::washDelay, DeathCam::washInTime;
int DeathCam::fadeR, DeathCam::fadeG, DeathCam::fadeB, DeathCam::maxWash;
int DeathCam::wastedDelayMs;

// CGameLogic::Update wasted/busted fade delays (cmp reg,3000); imm32 of each
// compare, rewritten to WastedDelayMs to shorten the death sequence
static const uintptr_t g_wastedTimerImms[] = {
    0x442dc2, 0x442dd4, 0x4430d7, 0x4430e7, 0x443468, 0x44347a,
};

// CHud::DrawAfterFade = last draw each frame (HUD + "wasted" text over the fade);
// wrap its call site so the wash paints on top of it
static const uintptr_t DRAW_AFTER_FADE = 0x58D490;

static void __cdecl HookDrawAfterFade() {
    reinterpret_cast<void(__cdecl*)()>(DRAW_AFTER_FADE)();
    DeathCam::DrawWash();
}

// hospital fade-in's SetFadeColour(0,0,0) call - redirect to force grey
static const uintptr_t HOSPITAL_FADE_CALL = 0x4436e7;

static void __fastcall HookHospitalFadeColour(void*, void*, int, int, int) {
    TheCamera.SetFadeColour((unsigned char)DeathCam::fadeR,
                            (unsigned char)DeathCam::fadeG,
                            (unsigned char)DeathCam::fadeB);
}

static uintptr_t FindCallTo(uintptr_t target, uintptr_t start, uintptr_t end) {
    for (uintptr_t a = start; a < end; ++a) {
        if (*(unsigned char*)a == 0xE8) {
            int32_t rel = *(int32_t*)(a + 1);
            if (a + 5 + (uintptr_t)rel == target)
                return a;
        }
    }
    return 0;
}

class StoriesDeathCamPlugin {
public:
    StoriesDeathCamPlugin() {
        DeathCam::iniPath = DeathCam::AsiFolder() + "SA.StoriesDeathCam.ini";
        DeathCam::LoadConfig();

        if (DeathCam::wastedDelayMs > 0) {
            unsigned int ms = DeathCam::wastedDelayMs;
            if (ms < 250) ms = 250;
            if (ms > 3000) ms = 3000;
            for (uintptr_t a : g_wastedTimerImms)
                patch::SetUInt(a, ms);
        }

        patch::RedirectJump(PROCESS_PED_DEAD_BABY, DeathCam::ProcessHook);
        Events::gameProcessEvent += [] { DeathCam::Process(); };

        uintptr_t call = FindCallTo(DRAW_AFTER_FADE, 0x401000, 0x7C0000);
        if (call)
            patch::RedirectCall(call, HookDrawAfterFade);
        else
            Events::drawHudEvent += [] { DeathCam::DrawWash(); };   // fallback (may draw under text)

        patch::RedirectCall(HOSPITAL_FADE_CALL, HookHospitalFadeColour);
    }
} storiesDeathCamPlugin;
