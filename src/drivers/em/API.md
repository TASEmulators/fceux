# em-fceux API

## init: (canvasQuerySelector: string) => boolean

Initialise the instance.

Must be called before anything else and only once. Returns true on success,
false otherwise.

## update: () => void

Update the instance.

Updates emulation, render graphics and audio. Call in requestAnimationFrame().

## loadGame: (array: Uint8Array, filename?: string) => void

Load and start a game from an Uint8Array.

Optional `filename` is for FCEUX NTSC/PAL region autodetection. Supported file
formats: NES, ZIP, NSF. Events: `'game-loaded'` when game is loaded.

## downloadGame: (url: RequestInfo, init?: RequestInit, filename?: string) => void

Download game, then load and start it with loadGame().

Pass optional `filename` for FCEUX NTSC/PAL region autodetection, otherwise
`url` filename is used. Uses `fetch()`.

## gameMd5: () => string

Get MD5 of the loaded game.

Returns '' if no game is loaded.

## reset: () => void

Reset system.

Same as pushing the reset button.

## setControllerBits: (bits: number) => void

Set game controllers state (bits).

Bits 0..7 represent controller 1, bits 8..15 controller 2, etc. Buttons bits are
in order: A, B, Select, Start, Up, Down, Left, Right. For example, bit 0 =
controller 1 A button, bit 9 = controller 2 B button.

## triggerZapper: (x: number, y: number) => void

Trigger Zapper.

Parameters `x` and `y` are in canvas client coordinates
(`getBoundingClientRect()`).

## setThrottling: (throttling: boolean) => void

Enable/disable throttling (aka turbo, fast-forward).

## throttling: () => boolean

Get throttling enabled/disabled state.

## setPaused: (pause: boolean) => void

Pause/unpause.

## paused: () => boolean

Get paused/unpaused state.

## advanceFrame: () => void

Advance a single frame.

Use this when paused to advance frames.

## setState: (index: number) => void

Set current save state index.

Allowed range: 0..9. Default: 0 (set when game is loaded).

## saveState: () => void

Save save state.

## loadState: () => void

Load save state.

## setMuted: (muted: boolean) => void

Mute/unmute audio.

## muted: () => boolean

Get mute/unmute state.

## setConfig: (key: string, value: string | number) => void

Set configuration item.

## exportSaveFiles: () => SaveFiles

Export save files of the currently loaded game.

## importSaveFiles: (saveFiles: SaveFiles) => void

Import save files for the currently loaded game.

## addEventListener: (name: K, listener: EventMap[K]) => void

Add event listener.

## removeEventListener: (listener: any) => void

Remove event listener.
