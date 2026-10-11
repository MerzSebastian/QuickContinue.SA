# QuickContinue.SA

A **Continue** button for the main menu of GTA San Andreas (v1.0 US, 32-bit), plus a faster start.

- **Continue** at the top of the main menu loads your newest save in one click, instead of
  Start Game > Load Game > slot > Yes. The label follows the game language, has 10 built-in
  languages (see Settings) and can be changed to any wording in the ini.
- **Load latest save** in the pause menu, right below Resume: start over from your newest save when something went wrong.
  The game first asks "All unsaved progress in your current game will be lost" (can be switched off), and the entry can be hidden.
- **AutoLoad** (optional): skip the menu and load the newest save right after the game starts.
  Hold the skip key (default Ctrl) while starting to get the menu instead.
- **Skip intro**: no splash screens, no logo or title video, no copyright fade.
- **Smooth loading screen**: the first loading image is shown at once instead of fading in from black.

If there is no save game, the menu is left alone.

## What it looks like

**Continue in the main menu, and Load latest save in the pause menu**

![Continue in the main menu and Load latest save in the pause menu](docs/media/menu-continue.gif)

The second thing the plugin is made for is **AutoLoad**: it skips the main menu on the first start and goes straight into your
newest save. Switch it on under *Options > Quick start* in the game, or with `AutoLoad=1` in the ini.

## Requirements

- GTA San Andreas **1.0 US**, the compact or Hoodlum exe (the one most mods need). The plugin checks the exe and does nothing on any other.
- An **ASI loader** that loads `.asi` files from `scripts\` (for example the Ultimate ASI Loader). It is not included.

## Install

Unpack the zip into the game folder, so that `QuickContinue.SA.asi` and `QuickContinue.SA.ini` end up in `scripts\`.
Start the game. To uninstall, delete the two files.

## Settings

`QuickContinue.SA.ini` (next to the `.asi`):

**Changeable in the game** (Options > Quick start). The value in the ini is the default; a change made in the game is saved to `QuickContinue.SA.cfg` and used instead:

| Key | Default | |
|---|---|---|
| `ContinueButton` | 1 | Add the Continue entry |
| `AutoLoad` | 0 | Load the newest save at startup, without the menu |
| `PauseMenuLoadLatest` | 1 | Add "Load latest save" to the pause menu; 0 hides it |
| `PauseMenuConfirm` | 1 | Ask "All unsaved progress will be lost" before loading from the pause menu; 0 loads at once |

**Only in the ini** (the game menu never changes these):

| Key | Default | |
|---|---|---|
| `SkipKey` | 17 (Ctrl) | Virtual-key code held at startup to skip AutoLoad for that run, 0 = never |
| `SkipIntro` | 1 | Skip splashes, `Logo.mpg` and `GTAtitles.mpg` |
| `SkipTitleFade` | 1 | Skip the fade of the copyright image |
| `SmoothLoadScreen` | 1 | Start the loading screen fully visible |
| `SettingsMenu` | 1 | Add Options > Quick start to change these settings in the game (see below); 0 = no page |
| `Language` | empty | Language of the entries: `en de fr it es pt nl sv fi hu`; empty = follow the game language |
| `Debug` | 0 | 0 = no log file at all; 1 = write `QuickContinue.SA.log` (use it for bug reports) |
| `[Text]` | empty | Your own wording for the entries and the settings page (see Languages); empty = built-in text |

**Languages.** By default the plugin follows the game language. The game has five language slots, and the entries and the
settings page are translated for those: English, German, French, Italian, Spanish.

Five more translations are built in, but they are my own short wording and not checked by native speakers: Portuguese (`pt`),
Dutch (`nl`), Swedish (`sv`), Finnish (`fi`) and Hungarian (`hu`). They are never chosen automatically: language mods
(Polish, Russian, Czech ...) replace one of the five slots, so the plugin cannot tell which language is shown. To use one, set it
yourself with `Language=pt` (any code above, plus `en de fr it es`). That turns the automatic choice off.

Languages whose letters the game font does not have (Polish, Czech, Turkish, Romanian ...) are not built in. For those, and for
any other wording, write the texts yourself under `[Text]`; they win over `Language=`. All keys are optional, an empty key uses
the built-in text:

| `[Text]` key | What it is |
|---|---|
| `Continue` | The entry in the main menu |
| `LoadLatest` | The entry in the pause menu |
| `PageName` | Name of the settings page (the entry in the Options list shows it in capitals) |
| `ContinueButton`, `AutoLoad`, `PauseLoadLatest`, `PauseAskFirst` | The four switches on the settings page |
| `RestartNote` | The note "changes apply after restarting" |
| `ResetButton` | "Reset to defaults" |

The built-in texts only use letters of the game font. In `[Text]` only the characters of the game's font (or of your language
mod's font) are shown, and the ini must be saved in the Windows code page of that language.

There is no log by default. With `Debug=1` the log is `QuickContinue.SA.log` in `Documents\GTA San Andreas User Files` (the game folder is often not writable).

"Newest save" is the most recently written, non-empty `GTASAsf1.b` to `GTASAsf8.b` in that folder.

## Settings in the game

With `SettingsMenu=1` (default) there is a **Quick start** page in *Options* (Schnellstart, Démarrage rapide, Avvio rapido, Inicio rápido) with an on/off switch for the four
settings players change most: `ContinueButton`, `AutoLoad`, `PauseMenuLoadLatest` and `PauseMenuConfirm`. A change is saved
at once to `QuickContinue.SA.cfg` in `Documents\GTA San Andreas User Files`, next to your save games, and applies the next
time the game starts. All other settings (`SkipIntro`, `SkipTitleFade`, `SmoothLoadScreen`, `SkipKey`, `Language`, `[Text]`,
`Debug`) are only in the ini.

- The ini holds the defaults. For the four settings on the page, the cfg file wins over the ini; a key that is not in the cfg
  falls back to the ini.
- "Reset to defaults" on the page (or deleting `QuickContinue.SA.cfg`) goes back to the values of the ini.
- With `SettingsMenu=0` there is no page, and the cfg file is neither read nor written, so the ini is the only source.
- The page borrows the game's unused "game loaded" screen (a console leftover). If the menu tables are not what the plugin
  expects, for example because another plugin changed Options, the page is skipped (the log says so, with `Debug=1`).

## Examples

Everything goes into `QuickContinue.SA.ini`.

**Start straight into your newest save, without the menu**

```ini
[QuickContinue]
AutoLoad=1
```

Hold **Ctrl** while the game starts to get the menu anyway. With `SkipKey=0` the menu can never be reached this way.

**No "Load latest save" in the pause menu**

```ini
[QuickContinue]
PauseMenuLoadLatest=0
```

**Reload at once, without the question**

```ini
[QuickContinue]
PauseMenuConfirm=0
```

**A built-in language that the game does not offer (for example Portuguese with a Portuguese language mod)**

```ini
[QuickContinue]
Language=pt
```

**Your own wording**

```ini
[Text]
Continue=Weiterspielen
LoadLatest=Zurueck zum letzten Spielstand
```

**Only the Continue button, leave the start-up as the game has it**

```ini
[QuickContinue]
ContinueButton=1
AutoLoad=0
SkipIntro=0
SkipTitleFade=0
SmoothLoadScreen=0
```

**Only the faster start-up, no Continue button**

```ini
[QuickContinue]
ContinueButton=0
```

**Report a problem**

Set `Debug=1`, start the game, reproduce the problem and attach `QuickContinue.SA.log` to the bug report.

## Build

Needs Visual Studio with the "Desktop development with C++" workload (x86 tools). Run `build.bat`;
the result is `out\QuickContinue.SA.asi` and `out\QuickContinue.SA.ini`.

## How it works

- The plugin hooks the two calls to `CMenuManager::DrawFrontEnd`. Other plugins redirect these calls, so any call
  target is accepted and chained to.
- On the first main-menu frame it reads the address of the frontend screen table from the code of `DrawStandardMenus`
  and `JumpToGenericMessageScreen`. Some plugins move the table to the heap and rewrite those operands, so the
  original address `0x8CE008` is not always the table in use. It inserts the item there. The item leads to
  `SCREEN_LOAD_FIRST_SAVE`, the screen the game reaches after you confirm "load this game?", so the game's own code
  does the loading.
- The `CText::Get` calls inside the menu code are redirected, so the key `QC_CONT` returns the label.
- Intro skip: the two state-machine stores `gGameState = 2` and `= 4` become `3` and `5` (next state without
  playing the video), and `VideoPlayer::Play` is disabled.
- `DoPCTitleFadeIn/Out` also set the loading screen state, so their calls are redirected to stubs that set only that
  state. Removing the calls crashed the in-game loading screen.
- Going back from Start Game, Options or Quit selects items by a fixed index in the game; the plugin adds one so the
  cursor stays on the right entry.

The `CMenuManager` layout is checked by a `static_assert` against its known size (`0x1B78`).

## License

Copyright (C) 2026 Sebastian Merz. Free software under the **GNU General Public License, version 3 or (at your
option) any later version** - see [LICENSE](LICENSE). The plugin comes with no warranty. It is a fan project and not
affiliated with Rockstar Games or Take-Two Interactive.
