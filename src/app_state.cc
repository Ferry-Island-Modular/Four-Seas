#include "daisy.h"
#include "app_state.h"

using namespace fourseas;

void AppState::Init() {}

AppState AppState::Default()
{
    AppState defaultState = {};

    defaultState.interpolate_waves = false;
    defaultState.mod_state_1       = PHASE_MOD;
    defaultState.mod_state_2       = PHASE_MOD;
    defaultState.lfo_state_1       = false;
    defaultState.lfo_state_2       = false;
    defaultState.sync_mode_1       = HARD;
    defaultState.sync_mode_2       = HARD;

    return defaultState;
}
