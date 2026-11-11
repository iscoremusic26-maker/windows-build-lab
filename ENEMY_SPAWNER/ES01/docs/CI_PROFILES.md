# CI profiles

Selected by the file `ENEMY_SPAWNER/ES01_CI_PROFILE.txt` of the CI repository, branch `ci/enemy-spawner-standalone-v0-1-1`.

| Profile | Runs | `.dusk` |
|---|---|---|
| `fast` | Provenance and pins, source rules with negatives, English audit with negatives, MSVC build, PE audit, core and panel laboratories | No |
| `integration` | Also: catalog against Dawnlight, vendored Expansion code check, adapter laboratory | No |
| `full` | Also: executed mutants, packaging and independent download | Yes |

No test result is cached. Only `full` names and publishes the package, and any later change needs another `full`.
