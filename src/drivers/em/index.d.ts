interface ConfigMap {
  'system-port-2': string;
  'video-system': string;
  'video-ntsc': number;
  'video-tv': number;
  'video-brightness': number;
  'video-contrast': number;
  'video-color': number;
  'video-sharpness': number;
  'video-gamma': number;
  'video-noise': number;
  'video-convergence': number;
  'video-scanlines': number;
  'video-glow': number;
}

interface EventMap {
  'game-loaded': () => any;
}

interface SaveFiles {
  [filename: string]: Uint8Array;
}

export default function FCEUX(params?): Promise<FceuxModule>;

export class FceuxModule {
  // em.cpp

  /*
  Initialise the instance.
  Must be called before anything else and only once.
  Returns true on success, false otherwise.
  */
  init: (canvasQuerySelector: string) => boolean;

  /*
  Update the instance.
  Updates emulation, render graphics and audio.
  Call in requestAnimationFrame().
  */
  update: () => void;

  /*
  Load amd start a game from an Uint8Array.
  Supported file formats: NES, ZIP, NSF.
  Events: `'game-loaded'` when game is loaded.
  */
  loadGame: (array: Uint8Array) => void;
  /*
  Download game, then load and start it with loadGame().
  Uses `fetch()`.
  */
  downloadGame: (url: RequestInfo, init?: RequestInit) => void;
  /*
  Get MD5 of the loaded game.
  Returns '' if no game is loaded.
  */
  gameMd5: () => string;

  /*
  Reset system.
  Same as pushing the reset button.
  */
  reset: () => void;

  /*
  Set game controllers state (bits).
  Bits 0..7 represent controller 1, bits 8..15 controller 2, etc.
  Buttons bits are in order: A, B, Select, Start, Up, Down, Left, Right.
  For example, bit 0 = controller 1 A button, bit 9 = controller 2 B button.
  */
  setControllerBits: (bits: number) => void;
  /*
  Trigger Zapper.
  Parameters `x` and `y` are in canvas client coordinates
  (`getBoundingClientRect()`).
  */
  triggerZapper: (x: number, y: number) => void;

  /*
  Enable/disable throttling (aka turbo, fast-forward).
  */
  setThrottling: (throttling: boolean) => void;
  /*
  Get throttling enabled/disabled state.
  */
  throttling: () => boolean;

  /*
  Pause/unpause.
  */
  setPaused: (pause: boolean) => void;
  /*
  Get paused/unpaused state.
  */
  paused: () => boolean;

  /*
  Advance a single frame.
  Use this when paused to advance frames.
  */
  advanceFrame: () => void;

  /*
  Set current save state index.
  Allowed range: 0..9.
  Default: 0 (set when game is loaded).
  */
  setState: (index: number) => void;
  /*
  Save save state.
  */
  saveState: () => void;
  /*
  Load save state.
  */
  loadState: () => void;

  /*
  Mute/unmute audio.
  */
  setMuted: (muted: boolean) => void;
  /*
  Get mute/unmute state.
  */
  muted: () => boolean;

  /*
  Set configuration item.
  */
  setConfig: (key: string, value: string | number) => void;

  // post.js

  /*
  Export save files of the currently loaded game.
  */
  exportSaveFiles: () => SaveFiles;
  /*
  Import save files for the currently loaded game.
  */
  importSaveFiles: (saveFiles: SaveFiles) => void;

  /*
  Add event listener.
  */
  addEventListener: <K extends keyof EventMap>(name: K, listener: EventMap[K]) => void;
  /*
  Remove event listener.
  */
  removeEventListener: (listener: any) => void;

  // Properties:
  _audioContext?: AudioContext; // Web Audio context created by init().
  _defaultConfig: ConfigMap; // Default config values.

  // Properties exposed from Emscripten module:
  then: (resolve?, reject?) => FceuxModule;
  FS: any; // TODO: Not needed?
}
