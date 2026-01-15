#pragma once

namespace fourseas
{
class AppState
{
  public:
    enum MOD_STATES
    {
        PHASE_MOD,
        WAVESHAPING,
        XOR,

        MOD_STATES_LAST,
    };

    enum SYNC_MODES
    {
        HARD,
        SOFT,
        FLIP,

        SYNC_MODES_LAST,
    };

    enum SPREAD_TYPES
    {
        SPREAD_ONE,
        SPREAD_TWO,
        SPREAD_THREE,
        SPREAD_FOUR,
        SPREAD_FIVE,
        SPREAD_SIX,
        SPREAD_SEVEN,
        SPREAD_EIGHT,

        SPREAD_TYPES_LAST,
    };

    enum WAVE_BANKS
    {
        WAVE_BANK_RED,
        WAVE_BANK_GREEN,
        WAVE_BANK_BLUE,
        WAVE_BANK_YELLOW,
        WAVE_BANK_VIOLET,
        WAVE_BANK_AQUA,
        WAVE_BANK_WHITE,
        WAVE_BANK_BLACK,
        WAVE_BANK_LAST,
    };

    bool                 interpolate_waves;
    AppState::MOD_STATES mod_state_1;
    AppState::MOD_STATES mod_state_2;
    bool                 lfo_state_1;
    bool                 lfo_state_2;
    AppState::SYNC_MODES sync_mode_1;
    AppState::SYNC_MODES sync_mode_2;

    AppState() {}
    ~AppState() {}

    void Init();


    AppState Default();
};

inline bool operator==(const AppState& lhs, const AppState& rhs)
{
    return lhs.interpolate_waves == rhs.interpolate_waves
           && lhs.mod_state_1 == rhs.mod_state_1
           && lhs.mod_state_2 == rhs.mod_state_2
           && lhs.lfo_state_1 == rhs.lfo_state_1
           && lhs.lfo_state_2 == rhs.lfo_state_2
           && lhs.sync_mode_1 == rhs.sync_mode_1
           && lhs.sync_mode_2 == rhs.sync_mode_2;
}

inline bool operator!=(const AppState& lhs, const AppState& rhs)
{
    return !(lhs == rhs);
}

} // namespace fourseas
