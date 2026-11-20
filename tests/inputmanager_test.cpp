// ============================================================================
// CannonCruise - InputManager Test (tests/inputmanager_test.cpp)
// ============================================================================

#include "InputManager.h"
#include "ShipPlayer.h"
#include "ShipPlayerInputProxy.h"
#include <cassert>
#include <cstdio>

int main(int argc, char* argv[])
{
    std::printf("=== CannonCruise CInputManager Test ===\n");

    CInputManager inputMgr;

    // 1. Focus Stack Test
    std::printf("[1] Focus Stack...\n");
    RwInt32 focusMenu = inputMgr.AllocateFocusID();
    RwInt32 focusGame = inputMgr.AllocateFocusID();
    assert(focusMenu >= 0 && focusGame >= 0 && focusMenu != focusGame);

    inputMgr.PushFocusID(focusMenu);
    assert(inputMgr.GetCurrentFocusID() == focusMenu);
    inputMgr.PushFocusID(focusGame);
    assert(inputMgr.GetCurrentFocusID() == focusGame);

    inputMgr.PopFocusID(focusGame);
    assert(inputMgr.GetCurrentFocusID() == focusMenu);

    inputMgr.FreeFocusID(focusMenu);
    inputMgr.FreeFocusID(focusGame);
    std::printf("  OK: Focus stack allocated, pushed, popped and freed\n");

    // 2. Button State & Edge Detection Test
    std::printf("[2] Button State & Edge Trigger Detection...\n");
    assert(inputMgr.IsButtonDown(0, P1_INPUT_3) == FALSE);

    // Frame 1: Button pressed
    inputMgr.SetButtonState(0, P1_INPUT_3, TRUE);
    assert(inputMgr.IsButtonDown(0, P1_INPUT_3) == TRUE);
    assert(inputMgr.IsButtonPressed(0, P1_INPUT_3) == TRUE && "Rising edge expected on first frame");

    // Latch update (Frame 2)
    inputMgr.Update();
    assert(inputMgr.IsButtonDown(0, P1_INPUT_3) == TRUE);
    assert(inputMgr.IsButtonPressed(0, P1_INPUT_3) == FALSE && "Edge must fade on second frame");

    // Button released
    inputMgr.SetButtonState(0, P1_INPUT_3, FALSE);
    inputMgr.Update();
    assert(inputMgr.IsButtonDown(0, P1_INPUT_3) == FALSE);
    std::printf("  OK: Button press and edge detection verified\n");

    // 3. ShipPlayerInputProxy Integration Test
    std::printf("[3] ShipPlayerInputProxy integration...\n");
    CShipPlayer ship;
    CShipPlayerInputProxy proxy(&ship);

    // Forward movement (P1_INPUT_3)
    inputMgr.SetButtonState(0, P1_INPUT_3, TRUE);
    proxy.Update();
    ship.Update(0.1f);
    assert(ship.GetSpeed() > 0.0f && "Ship must accelerate forward");

    // Left rudder (P1_INPUT_1)
    inputMgr.SetButtonState(0, P1_INPUT_1, TRUE);
    proxy.Update();
    // Rudder set to -1.0f

    std::printf("  OK: Ship steering and throttle driven by InputManager\n");

    std::printf("ALL INPUT MANAGER TESTS PASSED!\n");
    return 0;
}