# Changelog

## 1.0.0

First release.

- "Continue" entry at the top of the main menu: loads the newest save game in one click. Label in English, German,
  French, Italian and Spanish.
- Optional AutoLoad: loads the newest save right after startup, without the menu. A held key (default Ctrl) skips it.
- Skips the splash screens, the logo and title videos and the copyright fade at startup.
- Shows the first loading image at once instead of fading it in from black.
- "Load latest save" in the pause menu (below Resume), with the game's confirmation question; can be hidden (`PauseMenuLoadLatest=0`) or
  loaded at once (`PauseMenuConfirm=0`).
- `Language=` picks one of 10 built-in languages (for language mods); own wording for all entries and the settings page in `[Text]` for any other.
- Options > Quick start: switch the main settings (Continue button, AutoLoad, pause menu entry and question) in the game; saved to `QuickContinue.SA.cfg` next to the saves (the ini holds the defaults), off with `SettingsMenu=0`.
- Works together with plugins that move the frontend screen table (the address is read from the game's code).
- No log file by default; `Debug=1` in the ini writes `Documents\GTA San Andreas User Files\QuickContinue.SA.log`.

Known limits: only GTA San Andreas 1.0 US.
