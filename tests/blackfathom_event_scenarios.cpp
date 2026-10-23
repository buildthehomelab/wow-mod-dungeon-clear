
int main()
{
    std::vector<DungeonEvent> events;
    RegisterBlackfathomEvents(events);
    assert(events.size() == 1);
    auto const& event = events.front();
    assert(event.map == 48 && event.id == 1 && event.persistent && event.panelBoss == 4832);
    assert(event.completeGo == 21117 && event.completeState == GO_STATE_ACTIVE);
    Player bot;
    assert(!event.condition(&bot, nullptr));
    DcSettings::enabled = true;
    assert(event.condition(&bot, nullptr));
    bot.kelrisAlive = true; assert(!event.condition(&bot, nullptr)); bot.kelrisAlive = false;
    bot.nearbyDoor = false; assert(!event.condition(&bot, nullptr)); bot.nearbyDoor = true;
    bot.door.state = GO_STATE_ACTIVE; assert(!event.condition(&bot, nullptr)); bot.door.state = GO_STATE_READY;
    bot.map = 0; assert(!event.condition(&bot, nullptr)); bot.map = 48;
    assert(!event.condition(nullptr, nullptr));
    assert(event.steps.size() == 15 && event.steps.front().kind == "move" && event.steps[1].kind == "clear");
    uint32 fire = 21118;
    for (uint32 creature : {4825,4977,4823,4978})
    {
        auto offset = 2 + (fire - 21118) * 3;
        assert(event.steps[offset].kind == "use" && event.steps[offset].entry == fire);
        assert(event.steps[offset+1].kind == "state" && event.steps[offset+1].entry == fire && event.steps[offset+1].state == GO_STATE_ACTIVE);
        assert(event.steps[offset+2].kind == "kill" && event.steps[offset+2].entry == creature && event.steps[offset+2].radius == 120);
        ++fire;
    }
    assert(event.steps.back().entry == 21117 && event.steps.back().state == GO_STATE_ACTIVE);
    std::cout << "Blackfathom event source scenarios passed\n";
}
