# BZ_Trader — Traveling_Trader on Wheels

**Steam Workshop:** https://steamcommunity.com/sharedfiles/filedetails/?id=3822271044 · *Español: [README.es.md](README.es.md)*

A vehicle drives a recorded route with one or more Expansion traders aboard. At each stop they
**get off, walk to their stall, sell and get back on**, and the vehicle drives on. **BZ_AutoDrive** drives the
vehicle. Trading is the **Expansion Market**'s; the traders' protections are adapted from **Traveling Trader Expansion**
(FUBAR).

> **Server-side mod:** players do not download it. Requires BZ_AutoDrive 1.2.4 or later; tested on DayZ 1.30 with Expansion 1.9.79.
> The full manual is next to this file: `MANUAL_BZ_Trader.md` / `MANUAL_BZ_Trader.pdf` (Spanish: `.es.md` / `.es.pdf`).

## Credits

- **Traveling trade: FUBAR**, author of [Traveling Trader Expansion](https://steamcommunity.com/sharedfiles/filedetails/?id=3797172077). The idea of a trader who travels the map is his, and the name of this mod is a nod to his. Off the vehicle, our traders walk and stop in front of players like his walkers, and a service file uses the same fields as a TTE route. The protections that keep our traders from being recruited, handcuffed or treated are adapted from TTE's code, with FUBAR's permission. Thanks, FUBAR!
- **DayZ Expansion** (AI, Market, Navigation): the traders, the trading, the zones, the loadouts and the markers.
  Its API is used; its code is not copied (Expansion is CC BY-NC-ND 4.0).
- **BZ_AutoDrive** (Sonom4n, Hiperhipo10): the driver, the driving and the recipe for seating the bots.
- BZ_Trader: Sonom4n and Hiperhipo10.

## How it works

| | who |
|---|---|
| drive, stop (HOLD), drive on, disappear | BZ_AutoDrive |
| get off, walk to the stall, sell, walk back and board | BZ_Trader |
| what each trader sells (stock and prices) | the Market trader file |

- **The traders are the crew (`Crew`) of the AutoDrive route**: AutoDrive creates them, BZ_Trader links each one to
  its Market file (`LoadTrader`) and seats it in its seat.
- **The vehicle does not leave until every living trader is really seated**, neither at launch nor at any stop.
  BZ_Trader re-seats anyone who is not. If it cannot within the limit, it relaunches the lap with a new crew: it never
  leaves with a trader half aboard.
- **One of the traders drives** (seat 0: the editor's «Who drives»), dressed as their card says. At each mark they
  sell from the seat, or get off to a stall like the others if the stop says so: AutoDrive keeps the vehicle stopped
  (HOLD) and BZ_Trader does not drive on until they are back at the wheel. Not on a motorbike (with nobody on it, it
  falls over). An old file with `Driver.IsTrader: 0` (the route's driver drives and does not sell) still works, with
  a line in the server log.
- **Traders are always invincible**, aboard and on foot, the driver too. The invincibility is BZ_Trader's
  (`SetAllowDamage(false)`, re-applied every second): TTE's own traders can always be killed and looted. BZ_Trader does
  not use that: on a vehicle that follows a route they are too easy a target, even on PvP, and the
  service would turn into an armed convoy. `Traders[].Invincible` and `CanBeLooted` are no longer read: if a file has
  them, the server log says so once and the editor removes them when saving.
- **Nobody can recruit, handcuff or treat them** (bandages, CPR, pulse, food, blood, saline, head cover…): only our
  traders; Expansion's fixed traders do not change. Their eAI faction is always `InvincibleObservers`, which is
  invincible, and Expansion hides what it does not allow on an invincible one: with empty hands, the only option on a
  trader is «Trade» («Recruit» and «Check pulse» do not show). With an item in hand, Expansion also hides feeding and
  giving a drink, taking or giving blood, injections and the defibrillator; with a bandage in hand, the only option is
  still «Trade» (tested in the game). If any other action with an item showed up (saline, head cover, handcuffs…),
  choosing it does nothing: that guard runs on the server.
- **Traders are passive for eAI**: they do not look for targets, so they do not fight or wander off to pick things up
  on the way to their stall. That is why `LootingBehaviour` has no effect in BZ_Trader; it stays for compatibility with TTE.
- **Empty hands, always:** whatever a loadout puts in a trader's hands goes to its slot (the back, `Melee`, or the
  shoulder), else into the inventory, else it is deleted, with a log line. eAI is stopped from drawing a weapon for
  our traders only (other bots are untouched).
- **On foot they walk around the vehicle:** each trader walks from where they stood after getting off (the **measured
  exit**). Traders who got off inside the hull (a bus aisle) go straight to their stall; if they got off outside and
  the stall is on the other side (the rear hatch of an APC), they walk round the nearest corner. On the way back they
  board from a point **outside** the hull: their exit, if it was outside; if their seat is inside (the bus), the
  nearest side at the height of the seat; otherwise 1 m outside their door. Two seats sharing a door board one at a time.
- **Service vehicles are always locked to players:** nobody takes a seat (neither the wheel nor a passenger seat),
  opens the doors or uses the inventory; the traders and the driver still board. It is Expansion's vehicle lock (it
  comes with its Vehicles module, part of the Bundle); without that module the server still denies the seats: the
  player may see the action, but nothing happens. In a vehicle where you can ride standing (a truck bed, the roof, a
  bus interior), a player can still ride: it is a limitation of the vehicle, and each admin chooses which vehicle to
  use. A `PlayersCanBoard` in an old file is ignored (the log says so and the editor removes it on save).
- **A vehicle is never removed with a player aboard** (end of the lap, `Enable` 0, a service relaunched…): a player
  standing on top or inside (in any vehicle: a truck bed, the roof, the bus interior) is put on the ground beside it,
  and the vehicle is removed. With a seated player (since DayZ 1.28 the server cannot take a player out of a seat),
  the service goes on with a new vehicle and the old one waits, stopped and locked (nobody else gets in; getting out
  works), until they get off; only then is it removed. No chat messages: the server log says it. When the server
  shuts down, a vehicle with a player aboard is not removed (it is not saved, so it does not come back).
- **A player in a trader's seat:** the trader does not sit on top of them; it waits next to the vehicle (no message to
  the player: the log says it). If they do not move, the stop waits up to three times its `BoardTimeoutS` and then the
  lap is relaunched (the old vehicle waits, as above, until the player gets off).
- **AutoDrive's control panel:** **Respawn** (or **RESET** on the runner's card) is a relaunch for BZ_Trader: it cuts
  whatever was going on (a stop, a wave, the terminal), removes that vehicle and relaunches the lap from the start 3 s
  later, with the boarding and the seats as always. **Stop** (on the card) removes the vehicle: BZ_Trader sees it as
  lost and relaunches it 60 s later. To turn a service off, use `Enable` 0 in its file (or the editor), not **Stop**.
- **They face where you want:** each stall can have a heading (`HeadingDeg`) or a point to look at (`LookAt`); on
  arrival the trader turns and holds it while selling. A trader who stops in front of a player faces them and then turns back.
- **Each vehicle's seats:** the first time the mod sees a class, it writes `profiles\BZ_Trader\_plazas\<class>.json`
  (the hull, where each seat is, its door and, once they get off, the measured exit). The editor uses it to know how
  many seats the vehicle has, to draw each seat where it is (in «Who travels» and on each stop's plan), the hull, the
  doors and the exits, and to warn when a stall is on the other side of the vehicle. A file written before this version
  has no seat positions: the editor approximates them from each seat's entry until the next launch of that vehicle,
  which completes the file (keeping the measured exits).
- **Stops:** the marks 4 of the recording (events of the `_hdr.json`), and optionally **a player's wave** (F1) ahead
  of the vehicle. The vehicle **drives on when every seat is taken again**. Every wait has a limit.
  A wave is valid until the vehicle stops, passes the player or moves away (more than 10 × `Wave.RadiusM`), at most
  `Wave.MemoryS` seconds; the server log says why each wave that did not stop the vehicle was ignored (too far, the
  wait between waves, not ahead of the vehicle, a mark just ahead, expired...).
  So that players do not stop it all the time: after any stop (mark or wave) the vehicle drives `Wave.MinGapM`
  (500 m) along the route before stopping for another wave, it does not stop for a wave if the next marked stop is
  that close ahead, and it makes at most `Wave.MaxPerLap` (3) wave stops per lap (back to 0 every time it leaves the
  start). 0 = no limit, for both.
  For a wave the vehicle starts braking at the distance it needs to stop about 10 m before the player (from its speed,
  checked four times a second while a wave is pending); if it can no longer make it, it does not stop on top of them.
  Nobody gets off at a wave stop, but the doors of the seats with a trader open (once per shared door; none on a
  motorbike) so they sell from their seats, and they close before driving on. The lock still denies getting in and
  the inventory.
- **Honk before leaving** (an option of each stop, `Stops[].HornOnDepart`, and of the wave stops, `Wave.HornOnDepart`;
  off by default): when the stop is over, one short honk, and the vehicle drives off 2 s later. If the stop has to
  wait again in those 2 s (a trader, a player in front, a pause), it honks again before leaving. Motorbikes have no
  horn: they leave without it.
- **Before driving on** (from a stop or from the start), if a player stands in the way ahead (in front of the vehicle,
  its width and up to 12 m), it can wait for them to move, at most `FrontPlayerWaitS` seconds, and then leave anyway.
  Off by default (0): the warning is the honk before leaving.
- **The service's trade zone** can follow the vehicle (`TraderZone`), so it sells with the same stock anywhere on the map.
- **At the end of the route:** it disappears, reappears after N s or starts again (the admin decides).

## Requirements

- **BZ_AutoDrive** (1.2.4 or later): required Workshop item. Traveling_Trader_Expansion is no longer needed.
- DayZ Expansion: AI, Market and Navigation, either the Bundle or those modules on their own. Tested with the Bundle. Expansion is not a required Workshop item, on purpose: a server that already runs separate Expansion modules would clash with the Bundle.
- **Community Framework (CF)**, which Expansion needs.
- **`@BZ_Trader` goes inside the server folder**, next to `DayZServer_x64.exe`, like the server's other mods: copy it
  from where Steam downloads it when you subscribe. The editor has to run from there: it reads the server it is in.
- On DayZ 1.30, **`-profiles` has to be relative** (for example `-profiles=profiles`): with an absolute path the mod
  cannot list its service files. Use a relative one.

## Building a trader, step by step

Example: a trader on a motorbike who sells drinks.

| step | what | who |
|---|---|---|
| 1 | **The Market trader**: `profiles\ExpansionMod\Traders\<Name>.json` (for example a copy of an existing one with `"Items": {"waterbottle": 1}`). It is set up as for a fixed trader. | the admin, in the Market |
| 2 | **The service's Market zone** (`<mission>\expansion\traderzones\BZT_<service>.json`), if you want its own stock travelling with the vehicle. | the editor offers it if it does not exist |
| 3 | **Hardline reputation**: with `UseReputation: 1`, items with a rarity require reputation. | the admin (the editor warns) |
| 4 | **Record the route** (NUMPAD 5 records; NUMPAD 4 marks each stop) and **convert it** with the AutoDrive Wizard. | AutoDrive |
| 5 | **The driver and the crew in the route** (`DriverClass` and `Crew` of the `_hdr.json` and the `.json`). | the editor |
| 6 | **The service**: `profiles\BZ_Trader\<service>.json` (for example `TRADER_PICKUP_01.json`). | the editor |

### The editor

It is in the mod's folder: **`@BZ_Trader\tools\editor\`**, nothing to install (it is not inside the PBO). **Open it from
the server's `@BZ_Trader`**, the one next to `DayZServer_x64.exe`, not from the Workshop folder. If `@BZ_Trader` is not
in the root of a server (no `DayZServer_x64.exe` or `mpmissions` next to it), the window of `Editor.bat` and the top of
the page say so, and **Scan server** does not scan.
- **The easy way: `Editor.bat`** (double click). It starts a small local server, only on this PC, and opens the editor in
  your browser already reading **that** server: nothing to choose, and it saves directly, also inside `Program Files`
  if Windows lets your user write there (if not, it downloads the file and says where it goes). Its window says which
  server it is for; close it to stop, and when you close the editor in the browser (all its tabs) the window closes on
  its own. Reloading the page (F5) does not close it.
- **Without the `.bat`:** open **`editor_bz_trader.html`** with a double click in **Chrome or Edge**.

It is in English; the **EN / ES** buttons at the top right switch to Spanish at once, and the choice is remembered.
**Manual** opens the PDF manual in the editor's language. The editor always opens clean: the session is the service
file on the server ("Open a service").

1. **The server**: the editor knows which server it belongs to from its own folder (`<server>\@BZ_Trader\tools\editor`)
   and says so. **Scan server** reads all of it at once: its `.bat` files, profiles and missions, the converted
   recordings and the services of each map that loads `@BZ_Trader`, summed up in one line
   (`server … · 2 map(s) (…) · 5 recording(s) · 3 service(s)`). With `Editor.bat` it reads on its own; with the `.html`,
   the first time the browser asks you to choose that folder (the one with `DayZServer_x64.exe`), and then it remembers
   it, **one folder per server**: the editor of one server never loads another server's folder.
   On a server that has not started with the mod yet, it says what is missing: start the server once with
   `@BZ_Trader` (the mod creates `profiles\BZ_Trader\` with an example service turned off), and a converted recording.
   The editor finds **the map** on its own: the profile and the mission of the `.bat` that loads `@BZ_Trader` (for
   example `start_server.bat` → Chernarus, `profiles`, `mpmissions\dayzOffline.chernarusplus`), also when the `.bat`
   sets them in variables (`set PROFILE=profiles`); variables it cannot read (set outside the `.bat`) are named, and that
   map is chosen by hand. If the server has several maps, switch with one click (**Change map**).
   From there on it is the usual tree (`BZ_AutoDrive`, `BZ_Trader`, `ExpansionMod\Traders`, `Market`, `Loadouts` in
   the profile; `expansion\traderzones` and `expansion\settings` in the mission), and it shows what it found.
   Nothing is typed by hand (**Advanced**, closed, is for a server laid out differently). **Choose another folder** is
   for working on another server on purpose: the editor says it is not its own and does not remember it.
   **If your server is inside `Program Files`** (for example in `steamapps\common`) and you open the `.html`, the
   browser will not let you give it write access: **Scan server** reads it without writing (**Read without write
   access**) and downloads the files for you. With `Editor.bat` that does not happen.
2. **The recording or the service**, each with **a single choice**:
   - **New from a converted recording**: pick it by name, and the editor loads the route's `_hdr.json`, `_wp.csv`
     and `.json`;
   - **Open a service** that is already on the server: its recording comes from `Route`, on its own. The service
     file IS the session.

   If a file is missing, the editor says which one and where it looked. Below, **Where everything is**: the
   recording, the service, each trader's file and the zone.
3. **Traders and stops**:
   - **Point A**: how it stops (at the marks, by wave or both), the wave, the vehicle's seats, who drives (one of the
     traders) and each trader: class, clothes (a loadout or single items), **what they sell** (a Market file: the
     editor shows its categories and items and does not edit them; a file that cannot be used shows in grey with the
     reason), faction;
   - **each mark 4**: who gets off and **where they sell**: drag each token to its stall (north up; the vehicle is
     turned the way it arrived at the mark, with its left and right marked) and **which way they face**: drag the tip
     of the arrow, or type the degrees. Shortcuts: **Semicircle facing the centre** (radius, arc and centre: the cross
     on the plan) and **All facing the street**. The time selling and the limit to get back aboard.
     **The stop's view**: the mouse wheel zooms in and out, dragging the background moves the view, **Fit** frames
     everything and **Enlarge** fills the window (Esc goes back). With the keyboard: arrows 0.5 m, Shift 2 m, Ctrl 0.1 m;
   - **END**: disappears, reappears or starts again.
4. **Review and save**: **Save all** writes the service, the crew in the route and the new zone, with a backup of
   whatever it overwrites (`_bz_trader_backups\`), and lists each file with its folder. The route is changed **as
   text**: only the `DriverClass` line and the `Crew` block. With the server folder chosen, the editor **opens no
   file dialog**: everything comes from its lists and is written straight into its folder.

**Without write access** (a server inside `Program Files`, Firefox or a copy of the server) you also choose **the
folder**, only to read it: **Read without write access** (in Firefox, "Read the server folder"). Single files are
never chosen, because the browser does not let a page read the neighbours of a chosen file (the `_hdr.json` without
its `_wp.csv`). Each file to be written is downloaded with its name and the folder it goes in. The result is the same file.

### Players nearby: the fixed sequence
By default a stop lasts the same with or without players (`MaxWithPlayersS: 0`): nearby players never delay the
departure. With a limit in `MaxWithPlayersS`, the selling (and the seated wait) stretches up to it while players are
near. While selling, the trader stops and faces a player (`PlayerHaltM`); that does not delay the departure, because the
selling time keeps running. The editor's **Fixed sequence** button sets `MaxWithPlayersS: 0` and `PlayerHaltM: 0`:
every stop lasts its minimum and nobody stops. It does not change "Trade" (the Market gives it by
distance), the wave or the rule of not leaving without the living traders seated. It applies without a restart, at
the service's next launch.

### Signals at the stop
So that a stop can be seen from far away (a long stop at a market the admin built, for example), each stop can light
**signals**: coloured smoke, a road flare (`Roadflare`) or fireworks, all vanilla. In the editor, each stop's
**Signals** block: **+ Add signal** puts one 10 m to the right of the vehicle, to be dragged on the plan like a stall;
the shortcut **3 automatic signals around the stop** places three (green, yellow and purple) in a triangle 15 m from
the mark, off the route.
- **Automatic**: smoke of the chosen colour by day, a red flare by night (the server decides each time it lights one).
- They light when the traders get off (or when it stops, if nobody gets off), each after its delay, and every
  `EverySeconds` **the one that went out** is relit (never two in the same place). Smoke lasts 90 s, the flare
  20 min and the fireworks ~1 min (16 shots, random colours). `EverySeconds: 0` = only once.
- The fireworks are seen from far away, like the airdrop smoke.
- When it drives on, they all go out and are deleted. They are not saved in the world, and pick-up actions do not offer them.
- There is no blue smoke in the game (it comes out purple). `flaregun`, from old files, is read as `roadflare`.
- **Airdrop smoke** (`airdrop`, needs Expansion Missions, which comes with the Bundle): the red smoke and light of an
  Expansion airdrop, seen from far away, also at night (the smoke was seen at 744 m): it is sent to every player, it
  does not depend on the network range of items. No infected, no loot and no map marker: there is no container on the server, only the client's
  crate, buried at the signal's place (put it on the mark to have it under the vehicle), and nobody can open it. It
  does not burn out: it lasts until the vehicle drives on. A player who joins during the stop gets it too.

### Without restarting the server

- **The service** (`profiles\BZ_Trader\*.json`): the server rereads it every 10 s. A new one is launched; a changed
  one applies at that service's next launch (never while traders are off the vehicle); with `Enable: 0` or deleted,
  the service is retired after the current stop.
- **The route** (the crew): AutoDrive rereads it at every launch.
- **A new Market trader** whose categories already exist, and **a new zone**: BZ_Trader loads them on its own, without
  a restart (being tested: if a player does not see "Trade", restart). If the service names a zone that does not
  exist (for example, a service written by hand), BZ_Trader creates it in the mission, without stock: the Market adds
  each trader's items when they link.
- **A restart is needed** for a **new Market category**, a change of prices or maximum stock, or a change to a
  Market trader that was already loaded. The editor says so when saving.

## Configuration: one file per service in `$profile:BZ_Trader/*.json`

It is the file the editor writes, and the editor opens it again to adjust it. Keys in English; where the field is
the same as in a TTE route, the same name and format. Anything missing takes a safe value, and the server log says
which fields came from the default. If the folder is empty, the mod writes a disabled example.

| field | what it is | if missing |
|---|---|---|
| `Enable` | 1 = in service | 0 |
| `Name` | name of the service (and its signature in the log) | the file name |
| `Route` | AutoDrive route file, in `$profile:BZ_AutoDrive/` | required |
| `StopMode` | `marks` · `wave_only` (only by wave) · `marks_and_wave` | `marks` |
| `Wave` | the wave: `Enabled`, `RadiusM` (the scale: a wave is noted within 10 × `RadiusM`; with `AheadOnly` 0, a player beside or behind within `RadiusM` also stops it; ahead, the stop starts at the braking distance), `AheadOnly`, `CooldownS` (after leaving a stop, how long before a wave can stop it again; default 10), `TimeS`, `MemoryS` (the limit: a wave is valid until the vehicle stops, passes the player or moves away, at most this many seconds; re-checked every second, default 60), `MinGapM` (after any stop, the metres it drives along the route before stopping for another wave; no wave stop either if the next marked stop is closer than that, or than 50 m; default 500, 0 = no minimum), `MaxPerLap` (wave stops per lap, at most; back to 0 every lap; default 3, 0 = no limit), `HornOnDepart` (1 = one short honk, and it drives off 2 s later; default 0). Only a player's wave counts (not a bot's), and within 10 × `RadiusM` of the vehicle | off |
| `Driver` | `IsTrader` (1 = one of the traders drives: the one with `Seat` 0; the editor always writes 1. 0 = the route's driver, who does not sell: old files, with a log line), `AllCanDrive` (reserved, not available yet) | 0, 1 |
| `TraderZone` | `Name` (Market zone), `FollowsVehicle`, `RadiusM` | the zone of the place |
| `Traders[]` | `Seat` (0 = the one who drives), `Name`, `TraderAIClass`, `Loadout` or `Clothing[]`, `TraderFile`, `LootingBehaviour`. `Faction` is ignored: always `InvincibleObservers` (the editor removes it when saving and writes it that way in the route's `Crew`; trader routes do not use `guard_stop`, the only thing that would read it) | — |
| `Stops[]` | per mark 4: `Mark`, `Pos`, `HeadingDeg` (the DayZ heading on arrival), `DistM`, `Mode` (`always`/`wave_only`/`off`), `WaveRadiusM`, `WhoGetsOff[]` (0 = the driver gets off too), `BoardTimeoutS`, `Traders[{Seat, Waypoints[{WayPointName, Position}], TimeS, MovementSpeed, HeadingDeg, LookAt}]`, `Signals[]`, `HornOnDepart` (1 = one short honk before leaving, and it drives off 2 s later; default 0) | no stop |
| `Stops[].Traders[].HeadingDeg` · `LookAt` | which way the trader faces at their stall while selling: a DayZ heading (0 = north, clockwise) or a point `[x, z]`; with both, `LookAt` wins | faces the way they arrived |
| `Stops[].Signals[]` | the stop's signals (optional): `Type` (`auto`/`smoke`/`roadflare`/`fireworks`/`airdrop`), `Color` (`green`/`purple`/`red`/`white`/`yellow`/`blue`/`black`, the smoke's; the airdrop is always red), `Pos` `[x, z]` (the height comes from the terrain), `EverySeconds`, `StartDelayS`. Up to 10 per stop | none · `auto`, `green`, 60, 0 |
| `OnRouteEnd` | `Action` (`despawn`/`respawn`/`restart`), `DelayS`, `WaitS` | `despawn` |
| `StartDelayS` | seconds before the first launch | 0 |
| `Markers` | map marker (TTE's `ExpansionMarkers` format): the icon follows the vehicle on the move (it jumps to the new position every `MarkerUpdateSeconds`, without duplicating or going off) and blinks at stops; each update sends the map settings to every player (about 40 bytes plus 100 per server marker on the map) | no marker |
| `PlayerRadiusM` · `MaxWithPlayersS` · `SeatedWaitS` | "player nearby", limit of the wait with players (0 = no stretching), seated wait | 8 · 0 · 20 |
| `PlayerHaltM` | while selling, how many metres from a player the trader stops and faces them (TTE's halt); 0 = never. There is no halt while walking | 5 |
| `MaxStallDistanceM` | a stall or a signal farther than this from its mark is warned about (in the log and in the editor). For a market the admin built, more | 25 |
| `FrontPlayerWaitS` | before driving on, with a player in the way ahead: how long it waits for them to move, at most; 0 = no wait (the warning is the honk before leaving, `HornOnDepart`) | 0 |
| `Editor` | what the editor needs to rebuild the session (seats, length, start and end); the mod ignores it | — |

## Licence

BZ_Trader's code is open (MIT). The traders' protections are adapted from Traveling Trader Expansion, by FUBAR, with
his permission. DayZ Expansion is a dependency, subscribed to separately, with its own licence. This licence gives no
rights over it: it may not be re-uploaded or repacked with this mod.
