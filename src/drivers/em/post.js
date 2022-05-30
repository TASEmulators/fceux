(function() {
  Module._defaultConfig = {
    'video-system': 'auto',
    'video-ntsc': false,
    'video-tv': false,
    'video-brightness': 0,
    'video-contrast': 0,
    'video-color': 0,
    'video-sharpness': 0.2,
    'video-gamma': 0,
    'video-noise': 0.3,
    'video-convergence': 0.4,
    'video-scanlines': 0.1,
    'video-glow': 0.2,
    'system-port-2': 'zapper',
  };

  Module.deleteSaveFiles = function() {
    const dir = '/tmp/sav';
    FS.unmount(dir);
    FS.mount(MEMFS, {}, dir);
  };
  Module.exportSaveFiles = function() {
    Module._saveGameSave();
    const dir = '/tmp/sav/';
    const result = {};
    for (let name of FS.readdir(dir)) {
      if (FS.isFile(FS.stat(dir + name).mode)) {
        result[name] = FS.readFile(dir + name);
      }
    }
    return result;
  };
  Module.importSaveFiles = function(states) {
    Module.deleteSaveFiles();
    const dir = '/tmp/sav/';
    for (let name in states) {
      FS.writeFile(dir + name, states[name]);
    }
    Module._loadGameSave();
  };

  Module.eventListeners = {};
  Module.addEventListener = function(name, listener) {
    Module.eventListeners[name] = listener;
  };
  Module.removeEventListener = function(listener) {
    for (let k in Module.eventListeners) {
      if (Module.eventListeners[k] == listener) {
        delete Module.eventListeners[k];
      }
    }
  };

  Module.loadGame = function(uint8Array) {
    FS.writeFile('/tmp/rom', uint8Array);
    Module._startGame();
  };
  Module.downloadGame = function(url, init) {
    fetch(url, init).then(response => {
      if (response.ok) {
        response.arrayBuffer().then(buffer => {
          Module.loadGame(new Uint8Array(buffer));
        });
      } else {
        throw Error;
      }
    });
  };
})();
