/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

// The follower half of a BossPartyHoldRegistry row: wait on the published spot
// while the tank fetches the boss. The leader half is
// DungeonClearAdvanceAction::TryPartyHold; the signal between them is
// DcLeaderSignal::GetPartyHoldSpot.

#include "Log.h"
#include "Map.h"
#include "MotionMaster.h"
#include "Player.h"
#include "PlayerbotAI.h"
#include "Playerbots.h"
#include "Position.h"

#include "Ai/Dungeon/DungeonClear/Action/DcActionShared.h"
#include "Ai/Dungeon/DungeonClear/Action/DungeonClearActions.h"
#include "Ai/Dungeon/DungeonClear/DcValueKeys.h"
#include "Ai/Dungeon/DungeonClear/Trigger/DungeonClearTriggers.h"
#include "Ai/Dungeon/DungeonClear/Util/DcFollowerLifecycle.h"
#include "Ai/Dungeon/DungeonClear/Util/DcLeaderSignal.h"
#include "Ai/Dungeon/DungeonClear/Util/DcMovement.h"
#include "Ai/Dungeon/DungeonClear/Util/DcPullPlanner.h"
#include "Ai/Dungeon/DungeonClear/Util/DcRezRecovery.h"
#include "Ai/Dungeon/DungeonClear/Util/DungeonClearTuning.h"
#include "Ai/Dungeon/DungeonClear/Util/DungeonClearUtil.h"

using namespace DcActionShared;  // DC_PULL_SLOT_RADIUS, DcFaceIfNeeded

namespace
{
    // A follow-tank MoveFollow still installed is the one mover the multiplier
    // cannot zero — it is a MotionMaster generator, not an action — and it walks
    // the follower after the departing tank underneath every rung that yields.
    bool HasFollowGenerator(Player* bot)
    {
        MotionMaster* const mm = bot->GetMotionMaster();
        return mm && mm->GetCurrentMovementGeneratorType() == FOLLOW_MOTION_TYPE;
    }
}

bool DungeonClearPartyHoldTrigger::IsActive()
{
    // Map first: registered on every bot, and everywhere outside a raid it must
    // cost one compare.
    if (!bot || bot->isDead() || bot->IsInCombat())
        return false;
    Map const* const map = bot->GetMap();
    if (!map || !map->IsRaid())
        return false;

    // Resolved once and handed on — the signal would resolve the leader again.
    Player* const leader = DcLeaderSignal::FindLeaderTank(bot);
    if (!leader || leader == bot)
        return false;
    Position spot;
    if (!DcLeaderSignal::GetPartyHoldSpotFrom(leader, spot))
        return false;

    // Off the slot, or still carrying the follow generator: the action has work.
    // On the slot with nothing installed it is inert — the action would only
    // yield, and an inert rung is what lets the rest/buff rungs own the wait.
    if (HasFollowGenerator(bot))
        return true;
    Position const slot = DcPullPlanner::ComputeCampSlot(bot, spot);
    return bot->GetExactDist(&slot) > DC_PULL_SLOT_RADIUS;
}

bool DungeonClearPartyHoldAction::Execute(Event /*event*/)
{
    if (!bot || !botAI)
        return false;
    // The elected rezzer walks to the corpse; every other mover stands down for it
    // (the same rule the camp hold and follow-tank apply).
    if (DcRezRecovery::IsElectedRezzer(bot))
        return false;

    Position spot;
    if (!DcLeaderSignal::GetPartyHoldSpot(bot, spot))
        return false;  // raced away between the trigger and here

    // Tear down the follow-tank generator the once. Follow-tank stands down on
    // the hold signal and so never cancels it itself; left alone it re-asserts on
    // the next motion update and walks the follower out after the tank. Same
    // persistent-generator gotcha as the camp hold.
    if (HasFollowGenerator(bot))
    {
        DcMovement::StopBot(bot, DcMovement::Stop::Hold);
        context->GetValue<ObjectGuid>(DcKey::FollowedTank)->Set(ObjectGuid::Empty);
        DcFollowerLifecycle::UnmarkFollowing(bot->GetGUID());
    }

    // Each follower aims for its own fuzzed slot off the spot, as at a camp, so
    // the raid fans out instead of stacking on one point.
    Position const slot = DcPullPlanner::ComputeCampSlot(bot, spot);
    float const toSlot = bot->GetExactDist(&slot);
    if (toSlot <= DC_PULL_SLOT_RADIUS)
    {
        // Parked: settle, face the tank, and YIELD so the raid rests, eats and
        // buffs here. The multiplier keeps the stock movers off a held follower.
        DcMovement::StopBot(bot, DcMovement::Stop::Soft);
        DcFaceIfNeeded(bot, DcLeaderSignal::FindLeaderTank(bot));
        return false;
    }

    bool const moved =
        DcMoveTo(bot->GetMapId(), slot.GetPositionX(), slot.GetPositionY(), slot.GetPositionZ(),
                 /*idle*/ false, /*react*/ false, /*normal_only*/ false,
                 /*exact_waypoint*/ false, MovementPriority::MOVEMENT_NORMAL);
    DC_PULL_TRACE("[DC:{}] party hold: walking to the wait spot ({:.1f}yd, moved={})",
                  bot->GetName(), toSlot, moved);

    // Backstop — the move was refused, the bot stands still and a stale
    // equal-priority movement block holds it down (the camp hold's lesson).
    if (!moved && !bot->isMoving() && DcMoveDeferred(MovementPriority::MOVEMENT_NORMAL))
    {
        DcMovement::StopBot(bot, DcMovement::Stop::HardPin);
        DcMovement::ClearMovementWait(bot);
        DC_PULL_DEBUG("[DC:{}] party hold: walk refused while standing {:.1f}yd off the "
                      "slot -> cleared the stale movement wait", bot->GetName(), toSlot);
    }
    return true;
}
