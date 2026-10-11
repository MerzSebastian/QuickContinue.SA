# QuickContinue.SA

A **Continue** button for the main menu of GTA San Andreas (v1.0 US, 32-bit), plus a faster start.

- **Continue** at the top of the main menu loads your newest save in one click, instead of
  Start Game > Load Game > slot > Yes. The label follows the game language (English, German, French, Italian, Spanish).
- **AutoLoad** (optional): skip the menu and load the newest save right after the game starts.
  Hold the skip key (default Ctrl) while starting to get the menu instead.
- **Skip intro**: no splash screens, no logo or title video, no copyright fade.
- **Smooth loading screen**: the first loading image is shown at once instead of fading in from black.

If there is no save game, the menu is left alone.

## What it looks like

**The main menu with Continue on top**

![The main menu with the Continue button](docs/media/menu-continue.gif)

**With `AutoLoad=1`: straight from the start into the game**

![AutoLoad: from the start straight into the game](docs/media/autoload.gif)

## Requirements

- GTA San Andreas **1.0 US**, the compact or Hoodlum exe (the one most mods need). The plugin checks the exe and does nothing on any other.
- An **ASI loader** that loads `.asi` files from `scripts\` (for example the Ultimate ASI Loader). It is not included.

## Install

Unpack the zip into the game folder, so that `QuickContinue.SA.asi` and `QuickContinue.SA.ini` end up in `scripts\`.
Start the game. To uninstall, delete the two files.

## Settings

`QuickContinue.SA.ini` (next to the `.asi`):

| Key | Default | |
|---|---|---|
| `ContinueButton` | 1 | Add the Continue entry |
| `AutoLoad` | 0 | Load the newest save at startup, without the menu |
| `SkipKey` | 17 (Ctrl) | Virtual-key code held at startup to skip AutoLoad for that run, 0 = never |
| `SkipIntro` | 1 | Skip splashes, `Logo.mpg` and `GTAtitles.mpg` |
| `SkipTitleFade` | 1 | Skip the fade of the copyright image |
| `SmoothLoadScreen` | 1 | Start the loading screen fully visible |
| `Debug` | 0 | Write extra details to the log (use it for bug reports) |

The log is `QuickContinue.SA.log` in `Documents\GTA San Andreas User Files` (the game folder is often not writable).

"Newest save" is the most recently written, non-empty `GTASAsf1.b` to `GTASAsf8.b` in that folder.

## Examples

Everything goes into `QuickContinue.SA.ini`.

**Start straight into your newest save, without the menu**

```ini
[QuickContinue]
AutoLoad=1
```

Hold **Ctrl** while the game starts to get the menu anyway. With `SkipKey=0` the menu can never be reached this way.

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
