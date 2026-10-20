# Rollback

Close Dusklight. Move the two CP21 bundles out of its mods folder. Restore your backed-up, previously working bundles with the same IDs and restart. Never keep two installed copies of either ID. CP21 does not edit the player's data JSON, host, SDK, game executable, ROM/CISO or saves.

CP20 compiled baseline: `2ce550395ff69d2c9046a2d3e0b635024307ce1e`; documentary closure: `d9f83eaabff549440a10a33c002bf8e2e37945a6`. Source PR #80 and private CI PR #65 remain unchanged. Returning to the historical development build may require its existing external configuration; CP21 embeds its input and requires no such setup.

Removal cleans owned stage edits and restores the existing CP20 hook/state lifecycle. It does not claim to delete already loaded live actors; reload the scene or restart before judging the rollback.
