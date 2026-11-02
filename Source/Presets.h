#pragma once

#include <vector>

// Factory presets. Values are in real units (Hz, ms, dB, semitones, mode index, 0/1, or 0..1 amounts).
// Any parameter a preset does not list goes back to its default.
namespace Presets
{
struct Value { const char* id; float value; };

struct Preset
{
    const char* category;
    const char* name;
    const char* description;
    std::vector<Value> values;
};

inline const char* const categories[] = { "Trap", "R&B", "Rage", "FX" };

inline const std::vector<Preset>& all()
{
    static const std::vector<Preset> list {
        { "Trap", "Astro Space",
          "Ad-libs psychedeliques : octave grave en couche, saturation tape, delay ping-pong et grosse reverb.",
          { { "clean_low", 180 }, { "punch_amount", 0.6f },
            { "pitch_on", 1 }, { "pitch", -12 }, { "pitch_mix", 0.5f },
            { "color_on", 1 }, { "color_mode", 0 }, { "dirt", 0.35f },
            { "mod_on", 1 }, { "mod_mode", 1 }, { "mod_depth", 0.5f }, { "mod_mix", 0.4f },
            { "delay_on", 1 }, { "delay_mode", 4 }, { "delay_feedback", 0.45f }, { "delay_mix", 0.5f },
            { "reverb_on", 1 }, { "reverb_mode", 2 }, { "reverb_size", 0.8f }, { "reverb_mix", 0.55f },
            { "space", 0.85f }, { "width", 0.7f } } },

        { "Trap", "Demon Low",
          "Voix demoniaque seche, une octave plus bas, collee au beat.",
          { { "clean_low", 120 }, { "punch_amount", 0.7f },
            { "pitch_on", 1 }, { "pitch", -12 }, { "pitch_mix", 1.0f },
            { "color_on", 1 }, { "color_mode", 1 }, { "dirt", 0.2f },
            { "space", 0.1f }, { "width", 0.3f } } },

        { "Trap", "Throw It Back",
          "Le delay ne part que sur la fin des mots. Parfait pour les \"yeah\" et \"skrt\".",
          { { "clean_low", 200 }, { "punch_amount", 0.5f },
            { "delay_on", 1 }, { "delay_mode", 1 }, { "delay_feedback", 0.45f }, { "delay_mix", 0.7f },
            { "reverb_on", 1 }, { "reverb_mode", 1 }, { "reverb_size", 0.4f }, { "reverb_mix", 0.3f },
            { "throw", 1 }, { "space", 0.6f }, { "width", 0.6f } } },

        { "R&B", "6AM Toronto",
          "R&B sombre et introspectif : voix feutree, filtree, reverb nocturne.",
          { { "clean_mode", 2 }, { "clean_low", 150 }, { "clean_high", 6000 }, { "punch_amount", 0.4f },
            { "color_on", 1 }, { "color_mode", 1 }, { "dirt", 0.1f }, { "color_tone", 0.3f },
            { "delay_on", 1 }, { "delay_mode", 0 }, { "delay_feedback", 0.3f }, { "delay_tone", 0.35f }, { "delay_mix", 0.3f },
            { "reverb_on", 1 }, { "reverb_mode", 3 }, { "reverb_size", 0.7f }, { "reverb_mix", 0.45f },
            { "space", 0.55f }, { "width", 0.45f } } },

        { "R&B", "Silk Stack",
          "Voix lisse et doublee, tres large, pour les harmonies R&B.",
          { { "clean_low", 150 }, { "punch_amount", 0.5f },
            { "mod_on", 1 }, { "mod_mode", 0 }, { "mod_depth", 0.5f }, { "mod_mix", 0.6f },
            { "delay_on", 1 }, { "delay_mode", 1 }, { "delay_feedback", 0.25f }, { "delay_mix", 0.25f },
            { "reverb_on", 1 }, { "reverb_mode", 1 }, { "reverb_size", 0.6f }, { "reverb_mix", 0.4f },
            { "space", 0.6f }, { "width", 0.85f } } },

        { "R&B", "Queen Choir",
          "Effet chorale ample, avec une octave aigue discrete en dessous.",
          { { "clean_low", 140 }, { "punch_amount", 0.4f },
            { "pitch_on", 1 }, { "pitch", 12 }, { "pitch_mix", 0.2f },
            { "mod_on", 1 }, { "mod_mode", 0 }, { "mod_depth", 0.7f }, { "mod_mix", 0.7f },
            { "reverb_on", 1 }, { "reverb_mode", 2 }, { "reverb_size", 0.75f }, { "reverb_mix", 0.5f },
            { "space", 0.8f }, { "width", 0.9f } } },

        { "Rage", "Rage Scream",
          "Distorsion massive pour les cris rage / hyperpop.",
          { { "clean_low", 250 }, { "punch_mode", 2 }, { "punch_amount", 0.8f },
            { "color_on", 1 }, { "color_mode", 2 }, { "dirt", 0.9f }, { "color_mix", 0.9f },
            { "delay_on", 1 }, { "delay_mode", 2 }, { "delay_feedback", 0.2f }, { "delay_mix", 0.25f },
            { "reverb_on", 1 }, { "reverb_mode", 0 }, { "reverb_size", 0.3f }, { "reverb_mix", 0.25f },
            { "space", 0.3f }, { "width", 0.5f } } },

        { "Rage", "Blown Out",
          "Voix ecrasee a fond et legerement descendue, son de concert explose.",
          { { "clean_mode", 2 }, { "clean_low", 300 }, { "clean_high", 7000 },
            { "punch_mode", 2 }, { "punch_amount", 1.0f },
            { "pitch_on", 1 }, { "pitch", -2 }, { "pitch_mix", 1.0f },
            { "color_on", 1 }, { "color_mode", 3 }, { "dirt", 0.75f },
            { "reverb_on", 1 }, { "reverb_mode", 0 }, { "reverb_size", 0.2f }, { "reverb_mix", 0.2f },
            { "space", 0.2f }, { "width", 0.4f } } },

        { "Rage", "Bullhorn",
          "Voix criee dans un porte-voix, agressive et mono.",
          { { "clean_low", 300 }, { "punch_mode", 2 }, { "punch_amount", 0.7f },
            { "color_on", 1 }, { "color_mode", 6 }, { "dirt", 0.7f },
            { "delay_on", 1 }, { "delay_mode", 2 }, { "delay_feedback", 0.15f }, { "delay_mix", 0.2f },
            { "space", 0.3f }, { "width", 0.1f } } },

        { "FX", "Phone Call",
          "Effet telephone : bande etroite, saturee, mono.",
          { { "clean_low", 300 }, { "punch_amount", 0.6f },
            { "color_on", 1 }, { "color_mode", 4 }, { "dirt", 0.6f }, { "color_tone", 0.5f },
            { "reverb_on", 1 }, { "reverb_mode", 0 }, { "reverb_size", 0.15f }, { "reverb_mix", 0.15f },
            { "space", 0.15f }, { "width", 0.0f } } },

        { "FX", "Radio Days",
          "Vieille radio : voix etroite et grenue, un peu de piece.",
          { { "clean_low", 300 }, { "punch_amount", 0.5f },
            { "color_on", 1 }, { "color_mode", 5 }, { "dirt", 0.5f }, { "color_tone", 0.45f },
            { "reverb_on", 1 }, { "reverb_mode", 0 }, { "reverb_size", 0.25f }, { "reverb_mix", 0.2f },
            { "space", 0.25f }, { "width", 0.15f } } },

        { "FX", "Ghost Reverse",
          "Reverb inversee qui monte avant chaque phrase.",
          { { "clean_low", 200 }, { "punch_amount", 0.3f },
            { "mod_on", 1 }, { "mod_mode", 1 }, { "mod_depth", 0.4f }, { "mod_mix", 0.4f },
            { "reverb_on", 1 }, { "reverb_mode", 4 }, { "reverb_size", 0.6f }, { "reverb_mix", 0.7f },
            { "space", 0.95f }, { "width", 0.75f } } },

        { "FX", "Chipmunk",
          "Voix aigue et rapide, une octave au-dessus.",
          { { "clean_low", 220 }, { "punch_amount", 0.5f },
            { "pitch_on", 1 }, { "pitch", 12 }, { "pitch_mix", 1.0f },
            { "delay_on", 1 }, { "delay_mode", 1 }, { "delay_feedback", 0.3f }, { "delay_mix", 0.3f },
            { "reverb_on", 1 }, { "reverb_mode", 1 }, { "reverb_size", 0.3f }, { "reverb_mix", 0.25f },
            { "space", 0.25f }, { "width", 0.55f } } },
    };
    return list;
}
} // namespace Presets
