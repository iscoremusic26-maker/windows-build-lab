# Catalog

45 entries. Parameters, profiles and sentinels come from Dawnlight `b5e9046`, checked by `ci/catalog_check.py` against `ci/dawnlight_reference.json`. The order and the indices did not change in FIX01; only the labels became English.

| Group | Entries |
|---|---|
| Bokoblin | E_OC native, E_OC2 native, Blue (Expansion), Red (Expansion), **Black (Expansion): integration unavailable** |
| Humanoids | Darknut, Lizalfos, Dynalfos, Bulblin, Stalfos, Stalchild, Stalhound (night only), Gibdo, Puppet |
| Animals and others | White Wolfos, Rat, Bomskit, Tektite, Dodongo, Skulltula, Helmasaur, Helmasaurus, Armos |
| Ice | Chilfos, Freezard, Mini Freezard |
| Flying (+200) | Keese (normal, fire, ice), Bubble (normal, fire, ice), Aeralfos, Kargarok, Guay |
| Plants | Baba Serpent, Big Baba, Deku Baba |
| Toadpoli | Water and fire, with Dawnlight's adaptation to stay on the chosen ground |
| Chu | Red, blue, yellow, purple, black |

**Not offered:** Goron (an NPC) and Staltroop (it removes itself without Stallord, and Dawnlight replaces its AI).

**Depends on the environment, no guarantee:**
- Stalhound only appears at night.
- Outside its dungeon, Dodongo's fire may not be visible: the game creates no particle when the scene lacks it, and the native code tolerates that.
- Aeralfos, Armos and Big Baba belong to special rooms: they are created with Dawnlight's no-arena parameters.
