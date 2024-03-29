# Breakout Lava Lamp for the Game Boy / Analogue Pocket / MegaDuck

A chill, endless breakout-style lava lamp for the Game Boy inspired by this post: https://mastodon.gamedev.place/@vnglst@hachyderm.io/111828811846394449
- Which was apparently inspired by: https://twitter.com/nicolasdnl/status/1749715070928433161
- And perhaps also from: https://twitter.com/CasualEffects/status/1390290306206216196


### Download
The ROM can be downloaded from [itch.io](https://bbbbbr.itch.io/breakout-lava-lamp)


### Save Data
The game board will be saved when starting, stopping or pausing. Once saved it will maintain state after being powered off (save to cart RAM) and can resume or start fresh after being powered on.


#### FAQ
Q: Will the game board ever reach a steady state?
A: In order to make the outcome more interesting (and less symmetrical) the angle of a ball gets slightly modified each time it bounces. So it should never really stabilize.


### Supported consoles:
 - Game Boy / Game Boy Color (.gb)
 - Analogue Pocket (.pocket)
 - MegaDuck / Cougar Boy (.duck)

![Screenshot](info/breakout_lava_lamp_title_screen.png)
![Screenshot](info/breakout_lavalamp_gameplay.gif)


### Title Screen Menu
If no menu cursors or options are changed for about 1 minute then the game board will auto-start.

Actions:
- Continue: Resume the previous game board that was in progress
- Standard: Start a new game board with the standard, fixed random seed value
- Random: Start a new game board with a random seed value derived from user button press timing

Controls:
- D-Pad: Moves cursor on title screen
- Start/A/B: Continue or Start
  - B kept held down when starting: Random ball layout instead of grid

### During Game Board Updates
Controls:
- A/B: Return to Title Screen Menu
- Select:
  - Runs faster while long-pressed
  - Toggles player balls on/off when short-tapped
- Start: Pause/Un-Pause
- Left/Right: Increase / Decreases speed
- Up/Down: Increase / Decrease number of player balls


### Building
Requires GBDK-2020 4.2.0





