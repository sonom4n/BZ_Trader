# BZ_Trader — Admin manual

> **Traveling_Trader on Wheels.** The idea of the traveling trader from FUBAR's Traveling Trader Expansion, put in a vehicle that BZ AutoDrive drives. At each stop the traders get off, walk to their stall, sell, walk back and board, and the vehicle drives on.
> *Manual 1.0 (24 Oct 2026) — DayZ 1.30 · BZ AutoDrive 1.2.4 or later · DayZ Expansion 1.9.79.*

---

## Contents

1. What it does
2. Requirements
3. Your first traveling trader
4. What a trader sells: the Expansion Market
5. The editor
6. How a stop works
7. Vehicles, seats and doors
8. The service file
9. Changes without restarting
10. Reading the server log
11. Problems and known limits
12. Credits and license

---

## 1. What it does

BZ_Trader uses two mods and adds the part between them. It does not replace either of them.

| Job | Done by |
|---|---|
| Driving the route, holding at a stop, driving on, removing the vehicle | BZ AutoDrive |
| Getting off, walking to the stall, selling, walking back, boarding, signals, the horn, deciding when to leave, and protecting the traders | BZ_Trader |
| What each trader sells, at what price, with what stock | Expansion Market (your files) |

### What comes from Traveling Trader Expansion

BZ_Trader does not need Traveling Trader Expansion (TTE), but it grew out of it: it is the idea of FUBAR's traveling trader, put in a vehicle. From TTE it takes:

- **The idea.** A trader who travels the map. In TTE the trader can be killed and looted; BZ_Trader's traders cannot (section 7).
- **How traders move on foot.** Off the vehicle, each trader walks like one of TTE's walkers: along waypoints, as part of its AI group. While selling, the trader halts and faces a player who comes close.
- **The file format.** Where a field means the same as in a TTE route, it has the same name and format: `TraderFile`, `Waypoints` (`WayPointName`, `Position`), `MovementSpeed`, `LootingBehaviour` (with TTE's default) and the map marker (`Markers`). An admin who knows TTE already knows these fields.
- **The protections.** Keeping players from recruiting, handcuffing or treating the traders is adapted from TTE's code, with FUBAR's permission (section 7).

The walking scheme is BZ_Trader's own code. If your server also runs TTE, the two live side by side.

### What you can build

- A drinks seller on a motorbike who sells from the saddle at every stop.
- A pickup with a medic, a grocer and a clothes seller who set up their stalls at each stop.
- A bus that carries a whole market: six traders in a semicircle, with the red smoke of an airdrop so players see it from far away.
- A trade zone that travels with the vehicle and keeps its own stock anywhere on the map.
- A crew nobody can kill, with one of the traders at the wheel, so the vehicle always finishes its route.
- Vehicles locked to players: nobody takes a seat, and players buy at the stalls or through the open doors.

---

## 2. Requirements

| Mod | Notes |
|---|---|
| `@CF` | Community Framework. |
| DayZ Expansion | AI, Market and Navigation (plus Vehicles, for the vehicle lock: section 7; Missions, only for the `airdrop` signal): either `@DayZ-Expansion-Bundle` (with its requirements: Dabs Framework, Expansion Licensed) or those modules on their own. Tested with the Bundle. |
| `@BZ_AutoDrive` | Version 1.2.4 or later. [Workshop 3751815579](https://steamcommunity.com/sharedfiles/filedetails/?id=3751815579) |
| `@BZ_Trader` | Server side only: players do not download it. [Workshop 3822271044](https://steamcommunity.com/sharedfiles/filedetails/?id=3822271044) |

- **Expansion is not a required Workshop item, on purpose.** A server that already runs separate Expansion modules would clash with the Bundle. Subscribe to whatever your server uses.
- **Traveling Trader Expansion is not needed.** If your server already runs it, it can stay: the two live side by side.
- **`@BZ_Trader` goes inside the server folder,** next to `DayZServer_x64.exe`, like the server's other mods. Copy it from where Steam downloads it when you subscribe. The editor must run from there: it reads the server it lives in. Opened from the Workshop folder, it finds no server.
- **First start.** The mod creates `profiles\BZ_Trader\`. If the folder is empty, it writes an example service that is switched off (`Enable: 0`).
- **The editor** is a web page in `@BZ_Trader\tools\editor\`, with nothing to install. Open it with `Editor.bat`, next to it: it reads the server it lives in and saves straight to it, also inside `Program Files` if Windows lets the user write there. Without the `.bat` it opens in Chrome or Edge, and inside `Program Files` it can only read (section 5).

---

## 3. Your first traveling trader

From an empty server to a trader on the road, without restarting. This whole flow was tested in game, with the server and the client running.

1. **Choose what it sells.** Pick an Expansion trader file in `profiles\ExpansionMod\Traders\`, or copy one and change its items. Example: a drinks trader with the water bottle and soda cans. See section 4.
2. **Record the route.** In game, with BZ AutoDrive: **NUMPAD 5** records, and **NUMPAD 4** marks each place where the trader should stop. Drive it the way the trader should drive it. The AutoDrive manual covers recording in detail.
3. **Convert it with the AutoDrive Wizard.** The Wizard writes three route files to `profiles\BZ_AutoDrive\`: `<route>.json`, `<route>_hdr.json` and `<route>_wp.csv`.
4. **Open the editor (`Editor.bat`, in the `@BZ_Trader` inside the server folder) and press Scan server.** Then start a new service from the converted recording, picking it by name. If your server is on a host, see **Server on a host** (section 5).
5. **Say who travels.** For each trader: seat, character, clothes and what they sell. One of them drives (**Who drives**); in a new service, the first trader you add.
6. **Set up each stop.** Who gets off, where each stall is, which way each trader faces, how long they sell, and optional signals.
7. **Review and save.** **Save all** writes the service file, the crew into the route, and the trade zone if it is new.
8. **The server picks it up.** Within 10 seconds the server reads the new service and launches it. No restart is needed, with the exceptions listed in section 9.

---

## 4. What a trader sells: the Expansion Market

BZ_Trader never edits your economy. A traveling trader sells exactly like a fixed Expansion trader, and three Market files decide what and how much.

| File | Where | What it decides |
|---|---|---|
| Trader | `profiles\ExpansionMod\Traders\<Name>.json` | The categories and items this trader handles. In `Items`, the number is the trade mode (`1` = players can buy and sell it). It is not the stock. |
| Category | `profiles\ExpansionMod\Market\<Category>.json` | Prices, `MaxStockThreshold` and `InitStockPercent`. |
| Trade zone | `<mission>\expansion\traderzones\<Zone>.json` | The actual stock in that zone. |

- **A zone that travels.** With `TraderZone.FollowsVehicle: 1` the service's zone moves with the vehicle, so its traders sell from the same stock anywhere on the map. The editor offers to create the zone (named `BZT_<service>`) when it does not exist. When a trader links to a zone, the Market adds the items the zone is missing.
- **Reputation.** If Hardline has `UseReputation: 1` (`profiles\ExpansionMod\Settings\HardlineSettings.json`), items with a rarity need reputation to be bought. The editor warns you.
- **Items the Market lacks.** The Market of Expansion 1.9.79 has no entries for the clothes added in DayZ 1.30. To sell them, add a category file with those items and their prices, and a trader file that lists the category. A new category needs a server restart.
- **The water bottle is sold empty.** That is how the vanilla item works: players fill it. For drinks ready to drink, add soda cans.

---

## 5. The editor

The editor always runs from the copy of `@BZ_Trader` inside the server folder (section 2). The easiest way is `Editor.bat`, in `@BZ_Trader\tools\editor\`. It opens the editor already reading that server, without choosing any folder, and saves straight to it, also inside `Program Files` if Windows lets the user write there (on a Steam server it usually does; if not, it downloads the file and says where it goes). Its window says which server it is for; closing the window ends it. When you close the editor in the browser (all its tabs), the window closes on its own a few seconds later; reloading (F5) does not close it. If the tab is put to sleep for a long time, the window closes too: open `Editor.bat` again. Without the `.bat`: double-click `editor_bz_trader.html`, in Chrome or Edge. The **EN** and **ES** buttons at the top right switch the language at once, and the choice is remembered; **Manual**, next to them, opens this manual as a PDF in the same language. The editor always opens empty: it remembers each server's folder, the seats of each vehicle and the language, but not the last service. The editor works in four steps, top to bottom.

### Step 1 · The server

- The editor says which server it belongs to. **Scan server** reads it whole (the `.bat` files, the profiles and missions, and the recordings and services of each map that loads `@BZ_Trader`) and sums it up in one line. With the `.html`, the first time you choose the server folder (the one that holds `DayZServer_x64.exe`), and it is remembered **per server**. **Choose another folder** is for another server on purpose: the editor warns you and does not remember it.
- The editor reads the server's `.bat` that loads `@BZ_Trader` and takes the profile and the mission from it. Example: a `start_server.bat` with `-profiles=profiles` and `-mission=mpmissions\dayzOffline.chernarusplus` gives that profile and that mission. If the server runs several maps, switch with **Change map**. A `.bat` that sets the profile or the mission in variables (`set`) is read; a variable defined outside the `.bat` is named, and you pick that map by hand under **Change map**.
- Everything else is the standard folder tree, and the editor shows what it found under **Where everything is**. **Advanced** is only for a server laid out differently.
- **Read-only** (without `Editor.bat`). Inside `Program Files`, in Firefox or on a copy of the server, the browser gives no write access: **Scan server** reads the folder without writing (the same as the **Read without write access** button). You still choose the folder, never loose files. Each file the editor needs to save is downloaded, with its name and the folder it goes in.

### Server on a host

The editor runs on your PC, not on the host. It works the same way as BZ AutoDrive's Wizard: on a copy, and then you upload the files.

1. **Make the copy on your PC.** Download from the host the profile folder (the `-profiles` one, with `BZ_AutoDrive`, `BZ_Trader` and `ExpansionMod`) and the mission folder, inside `mpmissions`. Put them in one folder, with the same tree as on the host.
2. **Put `@BZ_Trader` next to them,** in that same folder, and open `@BZ_Trader\tools\editor\Editor.bat`. The editor takes that folder as the server.
3. **Choose the map.** A copy from a host usually has no start `.bat`, so the editor cannot tell which profile and mission the server uses: pick them from the list.
4. **Build the service and save.** **Save all** lists every file it wrote, with its folder.
5. **Upload those files to the host,** into the same folders. The server picks up a new or changed service without restarting; what needs a restart is in section 9.

### Step 2 · A recording or a service

- **New from a converted recording:** pick it by name, and the editor loads its three files.
- **Open a service** that is already on the server: its recording comes from its `Route` field. The service file holds the whole session, so it is also the file you reopen to change anything.
- If a file is missing, the editor says which one and where it looked.

### Step 3 · Traders and stops

The route appears as a strip with **Point A**, one point per NUMPAD 4 mark, and **END**. Click a point to edit it.

- **Point A:** how it stops (at the marks, only when a player waves with F1, or both), and the wave stop with its **Honk before leaving a wave stop**; the vehicle's seats and **Who drives**; each trader's character (`ExpansionTraderAI…`), clothes (a loadout or single items) and what they sell (the faction is always `InvincibleObservers`: section 7); and players near the vehicle.
- **Each mark:** who gets off (the driver too, if you tick them), and the stop plan (where each stall is and which way each trader faces); how long they sell, the limit to get back aboard and **Honk before leaving**; the signals. Section 6 explains a stop in detail.
- **END:** disappears, reappears after a number of seconds, or starts again from A, with an optional wait at the last point before that.

**What it sells** is a list of the Market's trader files. The editor shows each file's categories and items but does not edit them. A file that cannot be used appears greyed out, with the reason.

**Who drives** is a list of the service's traders: the vehicle is always driven by one of them. The one you pick moves to seat 0; if another trader was driving, the two swap seats, and each keeps their own stops. The driver wears the clothes on their own card, like the rest. A new service starts with the first trader at the wheel, who sells from the seat until you tick them to get off at a stop. A service from an older version, whose driver did not sell (`Driver.IsTrader: 0`), cannot be saved until you pick who drives.

**Map marker**. Its own block at point A shows the service's vehicle on the map, so players can tell it from other services or missions. **Show the vehicle on the map** switches it on (it is off by default); then the **Marker text** (empty = the service name), the **Colour** (8 swatches, or **Another colour**) and **Update every (s)** (2 s when you switch it on, at least 1). The marker belongs to Expansion Navigation. While the vehicle moves, BZ_Trader moves that same marker: the icon jumps to the new position every **Update every (s)** seconds if the vehicle moved 1 m or more, never doubled and never disappearing. It cannot glide like a player's arrow: that would need a client mod. At a stop it blinks, so players can tell the traders are selling (**Blink at stops**, on by default, and **Blink: how often it changes (s)**, 2 by default; 0 keeps it fixed). It blinks at a wave stop too; at the start and at the end of the route it stays fixed. Each blink change costs one update, like a move. Each update sends Expansion's map settings to every player: about 40 bytes plus 100 per server marker on the map. At start, the server log says how many there are (`marcadores: K marcadores de servidor en el mapa`); with many, raise **Update every (s)**. With few, as is usual, you can lower it to 1 s for shorter jumps: the cost stays small (with a single server marker, about 140 bytes per player and update). The review warns if the mission has no `expansion\settings\MapSettings.json` (no Navigation) or has `EnableServerMarkers: 0` (players would not see it).

### The stop plan

North is up. The vehicle is drawn turned the way it arrived at the mark, with its left and right labelled, its doors, and where each seat gets off. Drag each trader to their stall, and drag the tip of their arrow (or type the degrees) to set which way they face. While you scroll down through the stop's fields, the route strip stays at the top and the plan stays in view beside them.

| Control | What it does |
|---|---|
| Mouse wheel | Zooms in and out around the pointer. Trader markers keep their size on screen. |
| Drag the background | Moves the view without moving anything. |
| Drag the tip of an arrow | Turns the trader in 5° steps. With **Shift**, 1° steps. |
| **Fit** | Frames the vehicle, the stalls and the signals. Each stop opens like this. |
| **Enlarge** | Fills the window with the plan. **Esc** returns. |
| Arrow keys | Move the selected marker 0.5 m. With **Shift**, 2 m. With **Ctrl**, 0.1 m. |

| On the plan | What it is |
|---|---|
| The circle around a trader | Their halt: this close to a player, they stop and face them while selling (`PlayerHaltM`). |
| Diamond | Where that seat really lets the trader out (measured by the mod). |
| `?` | That seat's exit has not been measured yet: the mod measures it the first time someone gets off there. |
| Square | Where that trader boards on the way back. |
| The line around the vehicle | Its hull, measured by the mod. |

### Step 4 · Review and save

- The review runs the same checks the mod runs when it reads the file, plus what the editor knows about the server: a missing Market file, the zone, reputation, a stall too far from its mark. Errors block saving; warnings do not.
- **Save all** writes the service file, the crew into the route, and the new zone. Every file it overwrites is backed up first in `_bz_trader_backups\`, and the editor lists each file with its folder.
- In the route it changes only the `DriverClass` line and the `Crew` block, as text. Nothing else in the recording is touched.
- With the server scanned, the editor never opens a file dialog: everything comes from its own lists and is written straight to its folder.

---

## 6. How a stop works

1. **Arrival.** The vehicle stops at a NUMPAD 4 mark. If it ends up more than 15 m from the mark (it can happen after AutoDrive frees a stuck vehicle), nobody gets off and it drives on. Stops when a player waves are different: see below.
2. **Getting off.** The traders listed in `WhoGetsOff` get off, and the stop's signals light up. The trader who drives sells from the seat, unless the stop has them get off too: then the vehicle waits at the stop, braked, until they are back at the wheel. The driver of a motorbike never gets off (with nobody on it, it falls over), and neither does a driver whose vehicle is still rolling.
3. **Walking to the stall.** Each trader walks to their stall, around the vehicle if the stall is on the other side, at a walk or a jog.
4. **Selling.** For `TimeS` seconds, facing the direction you set. If `MaxWithPlayersS` is above 0 (it is 0 by default), the sale stretches while a player is within `PlayerRadiusM`, up to that limit. With a player within `PlayerHaltM`, the trader stops and faces them, then turns back to their own direction. There is no such halt while walking to or from the stall.
5. **Boarding.** Each trader walks back and boards from a point outside the vehicle. A trader who is not seated after `BoardTimeoutS` is moved into the seat.
6. **Leaving.** If the stop has **Honk before leaving**, the vehicle gives one short honk and leaves 2 s later. The signals go out and are removed, and the vehicle drives on.

> **The departure rule.** The vehicle never leaves until every living trader is really seated, at the start and at every stop. BZ_Trader re-seats any trader who is not. If it cannot within the limit, it relaunches the lap with a fresh crew instead of leaving with someone half aboard. The trader who drives is waited for like the rest. Players aboard do not count. At the start of a lap the vehicle then waits still a few seconds more: AutoDrive prepares the route only once the traders are seated.

### Honk before leaving

An option on each stop (`Stops[].HornOnDepart`) and another for wave stops (`Wave.HornOnDepart`), both off by default. It warns that the vehicle is about to move: a player standing in front has 2 s to step aside.

- When the stop decides to go, the vehicle gives one short honk and leaves 2 s later.
- If the stop goes back to waiting in those 2 s (a trader still boarding, a player in front, a pause from the panel), the departure is cancelled; when it decides again, it honks again.
- Leaving the start does not honk: it is not a stop.
- A motorbike has no horn: it leaves without one, and the log says so once.
- The horn is also noise for the game's AI: it can draw infected to the stop.

### Pausing from the AutoDrive panel

Pausing the vehicle from the BZ AutoDrive panel (or NUMPAD .) also stops the service's clock.

- **At a stop or while boarding at the start,** the stop freezes: time does not count, the doors stay as they are, the traders stay at their stalls and the vehicle does not leave. Signals are not renewed; smoke already lit burns out on its own.
- **When the pause is lifted,** everything carries on where it was, and the stop lasts at least 10 s more: the player who was waiting has time to open the menu.
- **While driving,** AutoDrive brakes the vehicle and the service does not count time: a pending wave does not expire.
- The log says when each pause starts and ends (section 10).

### Stops when a player waves

With `StopMode: wave_only` or `marks_and_wave`, a player who waves (F1) ahead of the vehicle stops it anywhere on the route. There are no stalls there, and the ground may not suit getting off, so **nobody gets off**: the traders sell from their seats, with their doors open, for `Wave.TimeS` seconds (longer with players nearby only if `MaxWithPlayersS` is above 0), no signals are lit, and the vehicle drives on and closes the doors. A seat that shares its door with another opens it once; on a motorbike no door opens. The doors open, but the vehicle stays locked: nobody gets in or opens its inventory.

- With `marks_and_wave` the vehicle also makes its normal stops at the marks, with the traders at their stalls.
- A wave is noticed up to 150 m from the vehicle (10 × `Wave.RadiusM`), and it holds until the vehicle stops, passes the player or moves away, for at most `Wave.MemoryS` (60 s).
- The player can wave from the road or from the shoulder next to it. The vehicle starts braking early enough to stop with its front about 10-20 m before the player, whatever its speed, so the player does not need to judge it. While a wave is pending it checks four times a second; if it is already too close to stop in time, the wave is ignored.
- A wave stop is an emergency stop: AutoDrive brakes hard. On a straight road the vehicle stays on its line; fast in a curve it can slide a few metres, and AutoDrive brings it back (section 11).
- A wave is ignored if the next stop at a mark is less than `Wave.MinGapM` ahead along the route (500 m; at least 50 m, even with 0): the vehicle is about to stop there.
- After any stop, at a mark or for a wave, the vehicle must drive `Wave.MinGapM` (500 m) before a wave can stop it again. A group waving one after another, or a player who follows the vehicle, gets one stop.
- At most `Wave.MaxPerLap` (3) wave stops per lap. A lap runs from the start to the end of the route, or to a relaunch.
- After any stop, at a mark or for a wave, another wave can stop the vehicle only `Wave.CooldownS` (10 s) after it leaves.
- Each wave that does not stop the vehicle leaves one line in the server log, with the reason (section 10).
- A mark set to `Mode: wave_only` stops only if someone waves near it, and then the traders get off to their stalls, as at any mark.
- Buying from a seated trader from outside was tested in game on a motorbike, and on a Land Rover with its doors open. In a closed vehicle (a bus, an APC) it has not been tested whether the trade option reaches the trader from outside; on the bus, a player standing in the aisle can buy from them (tested in game).

### Which way traders face

- Each stall can have a heading (`HeadingDeg`, DayZ heading: 0 = north, clockwise) or a point to look at (`LookAt: [x, z]`). If both are set, `LookAt` wins. Without either, the trader faces the way they walked in.
- The trader turns on arrival and holds that direction while selling. After facing a nearby player, they turn back.
- Editor shortcuts: **Semicircle facing the centre** (radius, arc, and the centre is the cross on the plan, which you can drag) and **All facing the street**.
- The semicircle puts the traders who get off on the outer side of the arc, each with `LookAt` on the centre. If you move a stall afterwards, that trader still faces the centre.

### Signals

So players can see a stop from far away, for example a long stop at a market you built, each stop can light signals. Every type is tested in game, by day and by night.

| `Type` | What lights up | Lasts |
|---|---|---|
| `auto` | Smoke of the chosen colour by day, a red road flare by night. The server decides each time it lights one. | — |
| `smoke` | Smoke grenade: `green`, `purple`, `red`, `white`, `yellow` or `black`. The game has no blue smoke: `blue` comes out purple. | 90 s |
| `roadflare` | Red road flare on the ground. | 20 min |
| `fireworks` | Fireworks launcher: 16 shots, random colours. | ~75 s |
| `airdrop` | The red smoke and light of an Expansion airdrop (needs Expansion Missions, part of the Bundle). The crate is buried at the signal's place and nobody can open it. No infected, no loot, no marker. Always red. | Until it drives on |

`flaregun`, from an older file, is read as `roadflare`: the game creates a flare gun's projectile only when someone fires it with the gun in hand.

- They light when the traders get off (or when the vehicle stops, if nobody does), each after its `StartDelayS`. Every `EverySeconds` the one that went out is lit again, never two in the same place, so with `EverySeconds` below 75 the fireworks go on for the whole stop. `EverySeconds: 0` lights it once. The airdrop does not go out on its own: it lasts until the vehicle drives on.
- When the vehicle leaves, every signal goes out and is removed. They are never saved in the world, and players cannot pick them up.
- **Coloured smoke and road flares are seen up to about 150 m.** That is how far DayZ sends small objects to players (`networkRangeNear` in `serverDZ.cfg`, 150 m by default); farther away the player's game does not have them. To find the vehicle from far away, there is the map marker (section 5).
- **Fireworks and the airdrop are seen from far away.** The fireworks, like the airdrop smoke. The server sends the airdrop to every player, wherever they are: its smoke was seen from 744 m in a test, and it shows at night too. A player who joins the server during the stop gets it as well.
- Up to 10 per stop. In the editor, **+ Add signal** places one 10 m to the right of the vehicle, and **3 automatic signals around the stop** places green, yellow and purple in a triangle 15 m from the mark, off the route, and replaces any that were there.
- **Placement is yours.** The mod never moves a signal. Smoke beside or under the vehicle works and looks good; inside a building it may not be seen. Move it in the editor if you need to.

### Players nearby, or a fixed sequence

By default a stop lasts the same with or without players (`MaxWithPlayersS: 0`). While selling, a trader stops and faces a nearby player (`PlayerHaltM`); that does not delay the departure, because the selling time keeps running. With `MaxWithPlayersS` above 0 the sale stretches while players are nearby, up to that limit, and then any player can delay the departure. The editor button **Fixed sequence** sets `MaxWithPlayersS: 0` and `PlayerHaltM: 0`: every stop lasts its minimum and nobody stops for players. It does not change who can trade (the Market decides that by distance), waving, or the departure rule.

**A player standing in front.** Off by default (`FrontPlayerWaitS: 0`): the warning that the vehicle is about to move is **Honk before leaving**. If you switch it on, before leaving a stop or the start, with a player right in front of the vehicle (up to 12 m ahead, within its width plus 1 m on each side), the vehicle waits for them to move, for at most `FrontPlayerWaitS` seconds, and then leaves anyway. It exists because pulling away into a player standing in front pushes them, and AutoDrive takes that as a crash. A player standing beside it, or seated in it, does not count.

### Long stops at a market you built

Ten minutes is `TimeS: 600` per trader. A stall or signal more than `MaxStallDistanceM` (25 m by default) from its mark only produces a warning, in the log and in the editor. For a large market, raise it. The trade zone grows to reach the farthest stall by itself.

---

## 7. Vehicles, seats and doors

- Any vehicle BZ AutoDrive can drive works: cars, trucks, buses and, in DayZ 1.30, motorbikes.
- Seat 0 is the driver, and the driver is always one of the traders. The traders are the route's driver and crew (`DriverClass` and `Crew`): AutoDrive creates them, BZ_Trader links each one to its Market file and seats it. The editor writes them into the route for you.
- The first time the mod sees a vehicle class, it writes `profiles\BZ_Trader\_plazas\<class>.json`: the hull, where each seat sits, its door and, once traders get off, where each seat really lets them out. The editor reads it to know how many seats there are and to draw them in place, with the hull, the doors and the exits.
- **A new vehicle takes two passes through the editor.** Before its first launch, the editor draws the seats approximately, and says so: that is enough to choose who goes in each seat. You save, the server launches the service on its own, with no player connected, and measures the vehicle on that launch. Scan the server again and the seats are where they really are. It happens once per vehicle class.
- On foot, each trader starts from where they got off. From inside the hull (a bus aisle) they walk straight to their stall. From outside, with the stall on the other side (an APC's rear hatch), they walk around the nearest corner. To board, they go to a point outside the hull; two seats that share a door board one at a time. Doors are closed after seating and before leaving.

| Vehicle tested | Seats | In game |
|---|---|---|
| Motorbike (DayZ 1.30) | The driver is the trader and sells from the saddle. | Tested |
| `ExpansionUAZRoofless` | A trader drives; three more in seats 1–3. | Tested, with a driver who did not sell (an earlier version) |
| `ExpansionBus` | 22 seats. Every seat lets out onto the aisle, inside the bus, and traders walk straight out to their stalls. | Tested with 6 traders |
| APC with a rear hatch | 6 seats: 0–1 by the side doors, 2–5 by the rear hatch. | Tested: from the hatch, traders walk around the vehicle to a stall on the other side, and sometimes climb over it |

### Players aboard

- **Service vehicles are always locked to players:** nobody takes a seat (neither the wheel nor a passenger seat), opens the doors or uses the inventory; the traders and the driver still board. It is Expansion's vehicle lock (it comes with its **Vehicles** module, part of the Bundle). Without that module the server still denies the seats: the player may see the action, but nothing happens.
- **Riding standing:** in a vehicle where you can ride standing (a truck bed, the roof, a bus interior), a player can still ride along. It is a limitation of the vehicle, and each admin chooses which vehicle to use.
- **A vehicle is never removed with a player aboard** (end of the lap, `Enable: 0`, a relaunch). Someone standing on top or inside is put on the ground beside it and the vehicle is removed. If someone is still seated (they should not be able to), the service goes on with a new vehicle and the old one waits, stopped and locked, until they get off. No chat messages: the server log says it (section 10). If the server shuts down with a player aboard, the vehicle is not removed; it is not saved either, so it does not come back.
- `PlayersCanBoard` is no longer used: if a service has it, the log says so and the editor removes it on save.

### Empty hands

Traders always walk, sell and ride with empty hands, whatever their loadout. Some of Expansion's default loadouts give a shovel, pickaxe or bat on the back, or a pistol in hand. BZ_Trader stops eAI from taking them to hand, and puts away anything already in hand: on the back, on the shoulder, in the inventory or, if it does not fit anywhere, it is deleted.

### Traders cannot be killed

BZ_Trader makes every trader invincible, the driver included, aboard and on foot, and checks it every second. That is on purpose: the vehicle must finish its route, and a dead driver would leave it stopped on the road. Fighting over a vehicle is a job for a convoy mod; BZ_Trader is about trading. (In Traveling Trader Expansion traders can be killed and looted; in BZ_Trader they cannot.)

- The driver also depends on BZ AutoDrive: keep its `DriverInvincible` on (the default), and do not use the route verb `set_driver_mortality` in a BZ_Trader route.
- **If a service ever gets stuck,** for whatever reason, press **Stop** on its vehicle in the BZ AutoDrive panel: that is the red button. The vehicle is removed and BZ_Trader launches the service again, from the start, 60 s later. To take a service off the road, use `Enable: 0` instead (section 9).

### What a player cannot do to them

The protections apply only to BZ_Trader's traders: Expansion's fixed traders and other mods' traders do not change. They are adapted from Traveling Trader Expansion, with FUBAR's permission.

- **They cannot be recruited.** Every trader, the driver included, belongs to the `InvincibleObservers` faction (the one Expansion uses for its own AI traders), whatever the file says. With that faction, **Recruit** does not even show. Dismissing them or opening their inventory needs recruiting them first, so that cannot be done either.
- **They cannot be handcuffed, gagged, hooded, fed or treated:** handcuffs, rope or tape, a hood, food or drink, bandages, injections, blood, saline, CPR, pulse, stitching, splinting, drawing blood, taking their temperature.
- **What the player sees.** With empty hands, the only option on a trader is **Trade** (tested in game): Expansion hides what cannot be done to someone of an invincible faction, **Check pulse** included. With a bandage in hand, the only option is still **Trade** (tested in game): bandaging is offered only to someone who is bleeding, and traders do not bleed. With an item in hand, Expansion also hides feeding, giving drink, drawing or giving blood, injections and the defibrillator. If any other action with an item did show (saline, a hood, handcuffs…), choosing it does nothing: BZ_Trader is a server mod and that protection runs on the server. There is no animation and the item is not used up.
- **AI treats them the same:** infected, animals, bandits, guards and mission AI do not attack them. Because their faction is invincible, eAI does not steer them around gas: if a stall or a path crosses a contaminated area, they walk through it. Being invincible, it does them no harm.
- Trader routes do not use AutoDrive's `guard_stop` verb: with this faction, anyone who got off would not fight.

---

## 8. The service file

One file per service in `profiles\BZ_Trader\`. It is the file the editor writes and reopens; you can also edit it by hand. Keys are in English, and where a field means the same as in a Traveling Trader Expansion route, it has the same name and format. Any missing field takes a safe default, and the server log lists which ones did.

A short example, with two traders and one stop: Mirek drives and sells from the seat, and Denis gets off to sell at a stall:

```json
{
  "Enable": 1,
  "Name": "TRADER_PICKUP_01",
  "Route": "BZBusRoute_TRADER_PICKUP_01.json",
  "VehicleClass": "ExpansionUAZRoofless_LightGreen",
  "StopMode": "marks",
  "Driver": { "IsTrader": 1, "AllCanDrive": 1 },
  "TraderZone": { "Name": "BZT_TRADER_PICKUP_01", "FollowsVehicle": 1, "RadiusM": 5.4 },
  "Traders": [
    { "Seat": 0, "Name": "Mirek", "TraderAIClass": "ExpansionTraderAIMirek",
      "Loadout": "SurvivorLoadout", "TraderFile": "Consumables" },
    { "Seat": 1, "Name": "Denis", "TraderAIClass": "ExpansionTraderAIDenis",
      "Loadout": "SurvivorLoadout", "TraderFile": "Medicals" }
  ],
  "Stops": [
    { "Mark": 1, "Pos": [6381.3, 8085.7], "Mode": "always",
      "WhoGetsOff": [1], "BoardTimeoutS": 20, "HornOnDepart": 1,
      "Traders": [
        { "Seat": 1, "TimeS": 120, "MovementSpeed": "WALK", "HeadingDeg": 125,
          "Waypoints": [ { "WayPointName": "M1-1", "Position": [6382.3, 0, 8090.5] } ] }
      ],
      "Signals": [
        { "Type": "auto", "Color": "green", "Pos": [6370.0, 8095.0], "EverySeconds": 60, "StartDelayS": 0 }
      ] }
  ],
  "OnRouteEnd": { "Action": "restart", "DelayS": 0, "WaitS": 0 }
}
```

### Top level

| Field | Default | Meaning |
|---|---|---|
| `Enable` | 0 | 1 = in service. |
| `Name` | file name | The service's name. It signs its log lines. |
| `Route` | required | The AutoDrive route file, in `profiles\BZ_AutoDrive\`. |
| `VehicleClass` | — | For information: compared with the route's vehicle, with a warning if they differ. |
| `StopMode` | `marks` | `marks`, `wave_only` or `marks_and_wave`: stop at the marks, only when waved at (nobody gets off), or both. |
| `Wave` | off | Stopping when a player waves. See below. |
| `Driver` | — | `IsTrader`: 1 = the trader in seat 0 drives (and sells). 0 is only for files from an older version: the route's driver drives, does not sell, and the log warns. `AllCanDrive` (1): reserved, no effect yet. |
| `TraderZone` | — | `Name` (the Market zone; empty = the zone of the place), `FollowsVehicle` (0), `RadiusM` (at least 5). |
| `Traders[]` | — | Who travels. See below. |
| `Stops[]` | no stops | One per NUMPAD 4 mark. See below. |
| `OnRouteEnd` | `despawn` | `Action`: `despawn` (disappears), `respawn` (comes back after `DelayS`) or `restart` (starts again from A). `WaitS`: optional wait at the last point first. |
| `StartDelayS` | 0 | Seconds before the first launch. |
| `Markers` | no marker | A map marker, in Traveling Trader Expansion's format: `useMapMarker` (0), `MarkerUpdateSeconds` (30, at least 1; the editor sets 2), `MarkerLabel` (the name), `MarkersIcon` (`Trader`), `Expansion3DMarkers` (0), `Red`, `Green`, `Blue`, `Alpha` (`"255"`, as text). BZ_Trader adds `BlinkAtStops` (1: blinks at the stops; 0 = fixed) and `BlinkSeconds` (2: how often it changes; 0 = fixed at stops too). |
| `RespawnSeconds` | 600 | A safety net: if the driver ever dies (traders are invincible), seconds until the service comes back. Below 0 = never. |
| `PlayerRadiusM` | 8 | Distance that counts as a player nearby. |
| `MaxWithPlayersS` | 0 | The longest a stop stretches while players are nearby. 0 = it does not stretch (a player cannot delay the departure). |
| `SeatedWaitS` | 20 | Minimum wait at a stop where nobody gets off (the driver sells from the seat). |
| `PlayerHaltM` | 5 | While selling, how close a player must be for the trader to stop and face them. 0 = never. |
| `FrontPlayerWaitS` | 0 | Before leaving, how long the vehicle waits for a player standing right in front to move (section 6). 0 = it does not wait: the warning is the honk before leaving. |
| `MaxStallDistanceM` | 25 | A stall or signal farther than this from its mark gets a warning. |
| `Editor` | — | What the editor needs to rebuild the session. The mod ignores it. |

### Traders[]

| Field | Default | Meaning |
|---|---|---|
| `Seat` | required | The seat. 0 is the driver (with `Driver.IsTrader: 1`). Without it the entry is ignored. |
| `Name` | seat N trader | The name shown in the log. |
| `TraderAIClass` | — | The character, an `ExpansionTraderAI…` class. AutoDrive creates the bot from the route's `Crew`; this field is checked against it. |
| `Loadout`, `Clothing[]` | — | Clothes: an Expansion loadout, or a list of items. |
| `TraderFile` | — | The Market trader file it sells from, without `.json`. |
| `Faction` | — | Ignored: the faction is always `InvincibleObservers` (section 7), and the log says so if the file asks for another. The editor removes it when it saves and writes `InvincibleObservers` into the route's crew. |
| `LootingBehaviour` | TTE's default | No effect: traders are passive and never pick things up. Kept for compatibility with Traveling Trader Expansion's format. |

The default of `LootingBehaviour` is Traveling Trader Expansion's own: `"BANDAGES | FOOD | CLOTHING | UPGRADE"`.

`Invincible` and `CanBeLooted`, from older files, are ignored: traders are always invincible (section 7). The log says so, and the editor removes them when it saves.

### Stops[]

| Field | Default | Meaning |
|---|---|---|
| `Mark` | list order | Which NUMPAD 4 mark, counting from 1. |
| `Pos` | — | `[x, z]` of the mark, to check the file against the recording. |
| `HeadingDeg`, `DistM` | — | The vehicle's heading on arrival and the distance along the route. Written by the editor. |
| `Mode` | `always` | `always`, `wave_only` or `off`: always stops here, only if waved at within `WaveRadiusM` (20) of the mark, or never. |
| `WhoGetsOff[]` | `[]` | The seats that get off here. It may include 0, the driver: the vehicle waits for them (not on a motorbike). |
| `BoardTimeoutS` | 60 | Seconds to get back in the seat before being moved there directly. At least 5. |
| `HornOnDepart` | 0 | 1 = one honk when leaving this stop, and it leaves 2 s later (section 6). |
| `Traders[]` | — | One entry per trader who gets off. See below. |
| `Signals[]` | none | `Type` (`auto`, `smoke`, `roadflare`, `fireworks` or `airdrop`; `auto` by default), `Color` (`green`), `Pos` `[x, z]` (height from the terrain), `EverySeconds` (60), `StartDelayS` (0). Up to 10 per stop. |

Each entry of `Stops[].Traders[]`:

| Field | Default | Meaning |
|---|---|---|
| `Seat` | — | Which trader. |
| `Waypoints[]` | none | `WayPointName` and `Position` `[x, y, z]`. The last one is the stall. With none, the trader sells where they got off. |
| `TimeS` | 60 | Seconds selling. |
| `MovementSpeed` | `WALK` | `WALK` or `JOG` (`RUN` counts as `JOG`). |
| `HeadingDeg`, `LookAt` | — | Which way the trader faces at the stall (section 6). |

### Wave

| Field | Default | Meaning |
|---|---|---|
| `Enabled` | 0 | 1 = a player's wave (F1) stops the vehicle. Bots' waves do not count. |
| `RadiusM` | 15 | The wave's scale: a wave is noticed within 10 × `RadiusM` of the vehicle, and it ends when the vehicle came within 3 × `RadiusM` and left the player behind. Where the vehicle stops does not depend on it: it brakes to stop before the player. |
| `AheadOnly` | 1 | Only a player ahead of the vehicle. |
| `CooldownS` | 10 | After the vehicle leaves a stop (at a mark or for a wave), seconds before a wave can stop it again. |
| `MinGapM` | 500 | After any stop, the metres the vehicle must drive before a wave can stop it again. A wave is also ignored if the next stop at a mark is this close along the route (at least 50 m). 0 = no minimum. The editor warns when the value leaves no stretch of the route where a wave can count. |
| `MaxPerLap` | 3 | The most wave stops in one lap (from the start to the end of the route, or to a relaunch). 0 = no limit. |
| `TimeS` | 60 | How long a wave stop lasts. The traders sell from their seats, with their doors open. |
| `MemoryS` | 60 | The longest a wave stays valid. It ends sooner when the vehicle stops, passes the player or moves away. |
| `HornOnDepart` | 0 | 1 = one honk when leaving a wave stop, and it leaves 2 s later. |

---

## 9. Changes without restarting

| You change | What happens |
|---|---|
| A service file | The server re-reads them every 10 s. A new one is launched. A changed one applies at that service's next launch, never while traders are off the vehicle. With `Enable: 0`, or deleted, the service retires after the current stop. |
| The route (its crew) | AutoDrive re-reads it at every launch. |
| A new Market trader file whose categories already exist | Loaded without restarting. If players do not get the option to trade, restart. |
| A new trade zone | Loaded without restarting. If a service names a zone that does not exist (a file written by hand, for example), BZ_Trader creates it in the mission with no stock, and the Market fills it with each trader's items. Buying in a zone created this way has not been tested in game. |
| A new Market category, prices, maximum stock, or a trader file already loaded | **Restart the server.** The editor tells you when it saves. |

---

## 10. Reading the server log

Every line starts with `[BZ_Trader] [<service>]`; warnings with `[BZ_Trader][WARN]` and errors with `[BZ_Trader][ERROR]`. To follow one service, search the RPT for its name. The messages themselves are written in Spanish; this table explains the ones you will see most.

| The line says | It means |
|---|---|
| `por defecto (no escritos en …): …` | Fields missing from the file, which took their default. |
| `APAGADO (Enable 0)` | The service is switched off. |
| `embarque: … SALE tras N s` | Every living trader is seated at the start: the vehicle leaves. |
| `soltado hace N s (controlador armado hace N s) y se movio 0 m` | The vehicle was released, but it did not move within 10 s of AutoDrive being ready. Only a diagnosis: if it repeats, read AutoDrive's lines for that vehicle. |
| `soltado hace N s y AutoDrive todavia no armo el controlador (…; tope 60 s)` | AutoDrive was not ready 60 s after the release. Only a diagnosis. |
| `PARADA (…): … despliegue N/M` | Stopped at a stop; N of the M traders got off. |
| `PARADA (…): nadie baja` | Nobody gets off here; the vehicle waits `SeatedWaitS`. |
| `… NO BAJA · …` | A trader did not get off, with the reason. |
| `se lo sienta por TELEPORT` · `SENTADO (teleport)` | A trader did not get back in time (`BoardTimeoutS`) and was moved into the seat. |
| `FANTASMA` | A trader listed in a seat without really sitting in it. BZ_Trader seats it again where it is. |
| `embarcados N de M vivos · muertos K` | How many living traders are back in their seats. |
| `señales: N encendidas` · `… apagadas y borradas` | The stop's signals lit; then removed on leaving. |
| `señal N NO SE CREO` | A signal could not be created. It is retried. |
| `señales: 1 encendidas (humo de airdrop (…) … creado en los clientes)` · `humo de airdrop: N hasta que sigue (no se renueva)` | An `airdrop` signal: the server sent the airdrop smoke to every player. It lasts until the vehicle drives on. |
| `PARADA (…): bocina al salir · suelta en 2 s` | The vehicle honked and leaves 2 s later. |
| `bocina al salir: se cancela (la parada volvio a esperar) · se vuelve a decidir` | The stop went back to waiting in those 2 s: when it decides again, it honks again. |
| `bocina al salir: el vehiculo no es un auto (una moto) … sale sin bocina` | A motorbike has no horn. Once per service. |
| `PAUSA del panel de AutoDrive · … se congela` · `pausa quitada tras … · sigue donde estaba, no antes de 10 s` | An admin paused the vehicle from the AutoDrive panel at a stop or at the start, then lifted the pause (section 6). While driving it says `en ruta: PAUSA del panel de AutoDrive`. |
| `· faccion InvincibleObservers (el archivo dice … : se ignora)` | When a service loads: the file asks for another faction, and it is ignored (section 7). |
| `<trader>: faccion InvincibleObservers (su grupo era Passive; cambia para todo el grupo)` | At every launch: BZ_Trader sets the faction of the driver's group. |
| `jugador <name> (entro hace 2 s): datos de N comerciante(s) enviados` | A player who joins gets the traders' data at 2 s and at 8 s, so **Trade** shows for them. |
| `parada (…): datos de N comerciante(s) reenviados a J jugador(es) a <= 500 m` | At every stop the traders' data is sent again to nearby players, just in case. |
| `COMERCIO en la zona '…'` | A player bought or sold. |
| `cierre: vehiculo cerrado para jugadores (…)` | The vehicle was locked to players when it appeared. |
| `… parado arriba o adentro de <class>: se lo pone en el piso, al costado` | When the vehicle was removed (end of the lap, `Enable: 0`, a relaunch): a player standing on top or inside was put on the ground beside it. |
| `… a bordo -> el vehiculo NO se retira: queda frenado …` · `desalojo: vacio de jugadores tras … -> UnregisterRunner hecho` | Someone was still seated when the vehicle was due to be removed: it waited, stopped and locked, and was removed once empty (section 7). |
| `VEHICULO PERDIDO` | The vehicle is no longer in the world, for example because an admin stopped it from the AutoDrive panel. The service is launched again after 60 s. |
| `PARADA (marca N): el vehiculo esta a X m de la marca … no se baja nadie y sigue` | The vehicle stopped more than 15 m from its mark (it can happen after AutoDrive frees a stuck vehicle): nobody gets off. |
| `PARADA por saludo de <jugador> · … nadie baja: venden sentados` | A player waved: the vehicle stops and the traders sell from their seats. |
| `PARADA (…): puertas abiertas para vender: N (plazas …)` | At a wave stop: how many doors were opened, and for which seats. With the lock on it adds `· con el cierre de Expansion puesto (nadie sube ni toca el inventario)`. A vehicle without seat doors (a bus) says `sin puertas de plaza que abrir (venden sentados igual)`. |
| `saludo de <jugador> ignorado: la proxima parada marcada (marca N) esta a X m (MinGapM …)` · `… (va a parar ahi)` | The next stop at a mark is closer than `MinGapM` (or 50 m): the vehicle stops there instead. |
| `saludo de <jugador> ignorado: paro hace X m (MinGapM …: faltan … m)` | Too soon after the last stop: the vehicle has not driven `MinGapM` yet. |
| `saludo de <jugador> ignorado: ya hizo N paradas por saludo en esta vuelta (MaxPerLap N)` | This lap already had its wave stops. |
| `parada por saludo N de la vuelta M (MaxPerLap N)` | A wave stop, counted against the lap's limit. |
| `vuelta N: K parada(s) por saludo (MaxPerLap N)` | When a new lap starts: how many wave stops the last one had. |
| `sena de <jugador> anotada a X m del vehiculo · …` | A player's wave was noted. It holds until the vehicle stops, passes the player or moves away (`MemoryS` at most). |
| `saludo de <jugador> ignorado: …` | A wave that did not stop the vehicle, with the reason: out of range, the wait between waves, too soon after the last stop (`MinGapM`), the lap's limit (`MaxPerLap`), the vehicle passed the player or moved away, `MemoryS` ran out, the player was not ahead, or the vehicle was already stopped. One line per wave. |
| `saludo de <jugador> atendido por la parada en marca N` | The vehicle stopped at a mark before reaching the player: that stop answered the wave. |
| `SENA de <jugador> a X m · adelante · parada por saludo en wp N (V km/h, frena con Y m de frenado)` | The vehicle started braking for a wave. |
| `parada por saludo: el frente quedo a X m de <jugador>` | Where the front of the vehicle stopped, against the 10 m it aims for (negative = it passed the player). |
| `… ignorado: no alcanza a frenar antes del jugador (a X m, necesita Y m)` | The vehicle was already too close to stop before the player. |
| `jugador <nombre> enfrente a X m: espera que se corra (tope N s)` · `se corrio a los N s` · `sigue enfrente: sale igual tras N s` | A player stood in front when the vehicle was about to leave (only with `FrontPlayerWaitS` above 0). |
| `marcadores: K marcadores de servidor en el mapa (MapSettings) · cada envio ~N bytes por jugador` | At start: how many server markers the map has, and what each map-marker update costs per player. |
| `marcador: N envios en la vuelta (…)` | At the end of a lap: how many map-marker updates were sent. Multiplied by the cost of one update (the `marcadores:` line at start), it is what a lap costs each player. The marker count is the one from the lap's last update. |
| `TraderZone '…': ya cargada por el Market · se asigna` · `no estaba cargada · se carga del archivo` | How the service's trade zone was found. |
| `cierre del server: N señales borradas` | The stop's signals were removed when the server shut down. |
| `Wave.WhoGetsOff ya no se usa` | An old field in the file: it is ignored, because nobody gets off at a wave stop. |
| `Invincible/CanBeLooted ya no se usan` | Fields from an older file: ignored, because traders are always invincible. |
| `PlayersCanBoard ya no se usa` | An old field: ignored, because service vehicles are always locked to players. The editor removes it on save. |
| `Driver.IsTrader = 0: maneja el DriverClass de la ruta, que no vende …` | A file from an older version: the route's driver drives and does not sell. Open it in the editor and pick who drives. |
| `plaza 0 (…): BAJA · … es el que maneja: el vehiculo espera …` | The driver gets off at this stop; the vehicle waits, braked, until they are back. |
| `0(no es un auto: …)` · `0(el vehiculo todavia anda: …)` | The driver was set to get off but stays seated: a motorbike, or the vehicle was still rolling. |
| `marcador: parpadea en la parada (…)` · `marcador: fin del parpadeo (…)` · `fijo otra vez` | The map marker blinks during a stop, then stays fixed again. |
| `manos vacias · <item> al hombro` · `a la espalda` · `al inventario` | Something a trader had in hand was put away: on the back, on the shoulder, in the inventory, or deleted if it did not fit. Once per item. |
| `eAI le VOLVIO a sacar …` | eAI took the item to hand again. It should not happen: report it. |
| `el cierre no quedo puesto (falta el modulo Vehicles de Expansion?)` | Expansion's Vehicles module is missing: players can open the doors and the inventory, but still cannot take a seat. |
| `FIN del servicio · …` | The service ended, with the reason. |
| `no se pudo leer …` | A service file could not be read (usually broken JSON). Fix the file; the server re-reads it within 10 s. |

---

## 11. Problems and known limits

| What you see | Why, and what to do |
|---|---|
| No service starts, and the log shows no BZ_Trader service at all. | In DayZ 1.30, an absolute `-profiles` path keeps the mod from listing its service files. Use a relative one, such as `-profiles=profiles`. |
| The editor (or the `Editor.bat` window) says "BZ_Trader is not in the root of a DayZ server". | You opened it from the folder where Steam downloads it (there the mod's folder is named with a number). Copy `@BZ_Trader` into the server folder, next to `DayZServer_x64.exe`, and open the editor from there (section 2). |
| The editor cannot save. | The server is inside `Program Files`, or the browser is Firefox. Use the read-only mode and copy the downloaded files to the folders it names, or move the server. |
| An item does not show or cannot be bought. | Hardline reputation, or the item is not in any category the trader file lists (section 4). |
| The water bottle arrives empty. | That is how the vanilla item is sold. Add soda cans for ready drinks. |
| A trader takes long to board, or appears in the seat suddenly. | After `BoardTimeoutS` the trader is moved into the seat (`se lo sienta por TELEPORT` in the log). Stalls on the far side of the vehicle from the door make the walk back longer. |
| A signal cannot be seen from far away. | Coloured smoke and road flares are small objects: DayZ sends them only to players within about 150 m (`networkRangeNear`). To be seen from far away, use the `airdrop` or the fireworks; to find the vehicle, the map marker. |
| A player chooses an action with an item in hand on a trader (handcuffs, a bandage) and nothing happens. | Expected: the protections run on the server (section 7). |
| The horn draws infected. | It is noise for the game's AI, like any horn. If it is a problem, switch it off at that stop. |
| A signal cannot be seen (inside a wall or a building). | Signal positions are yours; the mod never moves them. Move it in the editor. Smoke beside or under the vehicle works fine. |
| A player climbs onto the bed, the roof or into the bus and rides along. | Expected: it cannot be prevented. When the vehicle is removed, they are put on the ground beside it (section 7). |
| Players can open the doors or the vehicle's inventory. | Expansion's Vehicles module is missing (it comes with the Bundle). They still cannot take a seat. The log says `el cierre no quedo puesto`. |
| I used the AutoDrive panel (**Stop**, **Respawn**) on a BZ_Trader vehicle and the service came back. | Expected: BZ_Trader relaunches the service on its own. **Stop** is the red button for a stuck service (section 7). To take a service off the road or put it back, use `Enable` in its file (or in the editor). |
| The old vehicle stays braked at the end of the route while the service goes on with a new one. | Someone is still seated in it, which should not happen: seats are denied to players. It is removed once they get off (section 7). |
| `GetTraderFromObject … is NULL!` errors when a service relaunches with a player nearby. | Harmless. The player's game asked for the trader's data a moment before BZ_Trader linked the trader to the Market; BZ_Trader sends the data as soon as it links it, and trading works. |
| At a wave stop, the vehicle slides off its line and takes a moment to drive on. | A wave stop brakes hard (AutoDrive's emergency stop). On straight roads it stays on its line; fast in a curve it can slide a few metres, and AutoDrive brings it back. A smoother wave stop needs a later version of BZ AutoDrive. |
| A warning about a stall that is too far. | Only a warning. For a large market, raise `MaxStallDistanceM`. |

### Not available yet

- **Convoys.** One service drives one vehicle. Several services can share roads, but they do not travel together.
- **The flare gun** as a signal: the game creates its projectile only when someone fires it with the gun in hand. There is no blue smoke in the game either.

---

## 12. Credits and license

- **Traveling trade: FUBAR**, author of [Traveling Trader Expansion](https://steamcommunity.com/sharedfiles/filedetails/?id=3797172077). The idea of a trader who travels the map is his, and the name of this mod is a nod to his. Off the vehicle, our traders walk and stop in front of players like his walkers, and a service file uses the same fields as a TTE route. The protections that keep our traders from being recruited, handcuffed or treated are adapted from TTE's code, with FUBAR's permission. Thanks, FUBAR!
- **DayZ Expansion**: the traders, the Market, the zones, the loadouts, the markers and the airdrop smoke. BZ_Trader uses its API and copies none of its code.
- **BZ AutoDrive**: the driver and the driving.
- BZ_Trader by **Sonom4n** and **Hiperhipo10** — BrigadaZ.

> **License.** BZ_Trader's code is open source (MIT). The trader protections are adapted from Traveling Trader Expansion, by FUBAR, with his permission. DayZ Expansion is a separate dependency under its own license: it may not be re-uploaded or repacked together with this mod.
