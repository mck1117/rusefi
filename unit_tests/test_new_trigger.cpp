#include "engine_test_helper.h"

#include "TriggerDecoder.h"

void testMissingTooth()
{
    MissingToothTriggerDecoder mt(10, 1);

    efitime_t t = 0;

    for(int i = 0; i < 20; i++)
    {
        t += 100;

        auto result = mt.HandleTriggerEdge(SHAFT_PRIMARY_FALLING, t);

        assertEquals(false, result->synced);
    }

    // missing tooth gap!
    t += 200;

    auto result = mt.HandleTriggerEdge(SHAFT_PRIMARY_FALLING, t);

    assertEquals(true, result->synced);

    // Give it a few more turns of the crank
    for(int n = 0; n < 10; n++)
    {
        for(int i = 0; i < 8; i++)
        {
            t += 100;

            auto result = mt.HandleTriggerEdge(SHAFT_PRIMARY_FALLING, t);

            assertEquals(true, result->synced);
        }

        // missing tooth gap!
        t += 200;

        auto result = mt.HandleTriggerEdge(SHAFT_PRIMARY_FALLING, t);

        assertEquals(true, result->synced);
    }
}

void testMissingTooth2()
{
    MissingToothTriggerDecoder mt(10, 3);

    
}








void testNewDecoder()
{
    testMissingTooth();
}