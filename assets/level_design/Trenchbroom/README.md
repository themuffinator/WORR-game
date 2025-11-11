# WORR TrenchBroom Support

This directory bundles a self-contained TrenchBroom game configuration for WORR. Copy the `games/WORR` folder into your TrenchBroom user configuration directory (for example, `~/Library/Application Support/TrenchBroom` on macOS or `%APPDATA%\TrenchBroom` on Windows) and drop the official WORR icon into the same folder if you want it to appear in the launcher.

* `GameConfig.cfg` selects the WORR entity definitions and points the editor at the `worr` search path.
* `WORR.fgd` describes WORR’s expanded entity catalogue, spawn flags, and key/value metadata. It now incorporates the lighting helpers and legacy aliases that previously lived in the auxiliary FGDs.

The FGDs mirror the exhaustive entity coverage documented in `docs/wiki/level-design.md`, including Domination, ProBall, headhunter receptacles, TrenchBroom-ready spawn filters, and the extended worldspawn metadata (ruleset overrides, arena counts, spawn pad toggles, etc.).
