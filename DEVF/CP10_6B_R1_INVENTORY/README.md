# DEVF CP10.6B-R1 — runtime JPA inventory, DIAGNOSTIC (Expansion 0.8.3)

`BokoblinExpansion_0.8.3_DIAG_TEST.dusk`, mod id `dev.bokoblin.expansion`. It **replaces** 0.8.2 for one test round (never install both; 0.8.2 and 0.8.1 stay as rollbacks). Provider 0.10.0, editor 0.10.2, `encounters.json` and schema are untouched.

**Purpose:** the 0.8.2 candidate asked the engine for the particle `0x0046` (WindLine), which the user's real log showed is **not registered** in the common bank. Choosing another id by its catalogue name would repeat that mistake. This build only **reads and logs which JPA resources the game really registered** (common bank, slot 0; scene bank, slot 1), with a technical shortlist of existing stroke-like resources, so the next candidate is chosen from evidence.

**What it does not do:** no aura, no particle emitter of any kind (the code that created/modified/destroyed emitters is removed, and CI statically checks it), no write to any engine object, no new import (the DLL imports fewer game symbols than 0.8.2), no change to the Bokoblin logic (identity, HP, regeneration, damage, Blue group bonus, Black fury, approved Black texture, purple Black eyes are the unchanged code). **No visual aura is expected in this build; its absence is the correct result.**

**Look for in the log** (`DEVF_CP10_6B_R1_INVENTORY`): `INVT_BEGIN / INVT_IDS / INVT_END` for bank 0 and bank 1, `INVENTORY_READY_COMMON`, `INVENTORY_READY_SCENE`, `INVENTORY_STATS`, `INVENTORY_SHORTLIST`, `INVENTORY_COMPLETE`, or `INVENTORY_PARTIAL` / `INVENTORY_FAILED` with a reason. Status: SOURCE / CI per the pull request; **HUMAN_RUNTIME_INVENTORY: PENDING**.
