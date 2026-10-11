# Changelog

## 1.1.1

- Fix: "Load latest save" and Continue are in the menu from its very first frame. Before, the pause menu showed without the
  entry the first time it was opened, and the entry appeared one frame later.

## 1.1.0

- **Pause menu:** "Load latest save" below Resume, with the game's confirmation question. Hide it with
  `PauseMenuLoadLatest=0`, load at once with `PauseMenuConfirm=0`.
- **Options > Quick start:** a settings page for the Continue button, AutoLoad and the two pause-menu options. Changes are
  saved to `QuickContinue.SA.cfg` next to the save games and used instead of the ini, which now holds the defaults.
  "Reset to defaults" goes back to the ini. `SettingsMenu=0` removes the page and ignores the cfg file.
- **Languages:** English, German, French, Italian and Spanish follow the game language. Portuguese, Dutch, Swedish,
  Finnish and Hungarian can be chosen with `Language=`. Every text (entries, settings page) can be replaced under `[Text]`.
- **Loading screen:** the loading bar and the changing pictures work with `SmoothLoadScreen=1` (the bar was missing in 1.0.0).
- No log file by default; `Debug=1` writes `Documents\GTA San Andreas User Files\QuickContinue.SA.log`.

## 1.0.0

First release.

- "Continue" entry at the top of the main menu: loads the newest save game in one click.
- Optional AutoLoad: loads the newest save right after startup, without the menu. A held key (default Ctrl) skips it.
- Skips the splash screens, the logo and title videos and the copyright fade at startup.
- Shows the first loading image at once instead of fading it in from black.
- Works together with plugins that move the frontend screen table (the address is read from the game's code).

Known limits: only GTA San Andreas 1.0 US.
