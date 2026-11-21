#ifdef __EMSCRIPTEN__
#include "game.h"

int WebGetKey(int key)
{
    switch (key)
    {
        case kNeed_Forward: return WebGetButton(WEB_UP);
        case kNeed_Backward: return WebGetButton(WEB_DOWN);
        case kNeed_TurnLeft: return WebGetButton(WEB_LEFT);
        case kNeed_TurnRight: return WebGetButton(WEB_RIGHT);
        case kNeed_Jump: return WebGetButton(WEB_JUMP);
        case kNeed_Attack: return WebGetButton(WEB_KICK);
        case kNeed_PickUp: return WebGetButton(WEB_PICKUP);
        case kNeed_JetUp: return WebGetButton(WEB_JET);
        case kNeed_NextWeapon: return WebGetButton(WEB_NEXT_WEAPON);
        case kNeed_PrevWeapon: return WebGetButton(WEB_PREV_WEAPON);
        case kNeed_CameraLeft: return WebGetButton(WEB_CAMERA_LEFT);
        case kNeed_CameraRight: return WebGetButton(WEB_CAMERA_RIGHT);
        case kNeed_ZoomIn: return WebGetButton(WEB_ZOOM_IN);
        case kNeed_ZoomOut: return WebGetButton(WEB_ZOOM_OUT);
        case kNeed_CameraMode: return WebGetButton(WEB_CAMERA_MODE);
        case kNeed_UIUp: return WebGetButton(WEB_UP);
        case kNeed_UIDown: return WebGetButton(WEB_DOWN);
        case kNeed_UILeft: return WebGetButton(WEB_LEFT);
        case kNeed_UIRight: return WebGetButton(WEB_RIGHT);
        case kNeed_UIConfirm: return WebGetButton(WEB_CONFIRM);
        case kNeed_UIBack: return WebGetButton(WEB_PAUSE);
        case kNeed_UIPause: return WebGetButton(WEB_PAUSE);
        default: return 0;
    }
}
float WebGetAnalog(int key)
{
    float x,y; WebGetMovement(&x,&y);
    switch (key)
    {
        case kNeed_TurnLeft: return fmaxf(0, -x);
        case kNeed_TurnRight: return fmaxf(0, x);
        case kNeed_Forward: return fmaxf(0, -y);
        case kNeed_Backward: return fmaxf(0, y);
        default: return WebGetKey(key) ? 1.0f : 0.0f;
    }
}
void WebGameResize(int width, int height) { (void)width; (void)height; QD3D_OnWindowResized(); }
void WebEndFrame(void) { WebPresent(WebIsGameplayActive(), gGamePaused); }
#endif
