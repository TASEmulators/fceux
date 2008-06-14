#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "main.h"
#include "throttle.h"
#include "config.h"

#include "../common/cheat.h"

#include "input.h"
#include "dface.h"

#include "sdl.h"
#include "sdl-video.h"
#include "unix-netplay.h"

#ifdef WIN32
#include <windows.h>
#endif

/**
 * Read a custom pallete from a file and load it into the core.
 */
static void
LoadCPalette(const std::string &file)
{
    uint8 tmpp[192];
    FILE *fp;

    if(!(fp = FCEUD_UTF8fopen(file.c_str(), "rb"))) {
        printf(" Error loading custom palette from file: %s\n", file.c_str());
        return;
    }
    fread(tmpp, 1, 192, fp);
    FCEUI_SetPaletteArray(tmpp);
    fclose(fp);
}

/**
 * Creates the subdirectories used for saving snapshots, movies, game
 * saves, etc.  Hopefully obsolete with new configuration system.
 */
static void
CreateDirs(const std::string &dir)
{
    char *subs[7]={"fcs","fcm","snaps","gameinfo","sav","cheats","movie"};
    std::string subdir;
    int x;

#ifdef WIN32
    mkdir(dir.c_str());
    for(x = 0; x < 6; x++) {
        subdir = dir + PSS + subs[x];
        mkdir(subdir.c_str());
    }
#else
    mkdir(dir.c_str(), S_IRWXU);
    for(x = 0; x < 6; x++) {
        subdir = dir + PSS + subs[x];
        mkdir(subdir.c_str(), S_IRWXU);
    }
#endif
}

/**
 * Attempts to locate FCEU's application directory.  This will
 * hopefully become obsolete once the new configuration system is in
 * place.
 */
static void
GetBaseDirectory(std::string &dir)
{
    char *home = getenv("HOME");
    if(home) {
        dir = std::string(home) + "/.fceultra";
    } else {
#ifdef WIN32
        home = new char[MAX_PATH + 1];
        GetModuleFileName(NULL, home, MAX_PATH + 1);

        char *lastBS = strrchr(home,'\\');
        if(lastBS) {
            *lastBS = 0;
        }

        dir = std::string(home);
        delete[] home;
#else
        dir = "";
#endif
    }
}


Config *
InitConfig()
{
    std::string dir, prefix;
    Config *config;

    GetBaseDirectory(dir);

    FCEUI_SetBaseDirectory(dir.c_str());
    CreateDirs(dir);

    config = new Config(dir);

    // sound options
    config->addOption('s', "sound", "SDL.Sound", 1);
    config->addOption("volume", "SDL.SoundVolume", 100);
    config->addOption("soundrate", "SDL.SoundRate", 48000);
    config->addOption("soundq", "SDL.SoundQuality", 1);
    config->addOption("soundrecord", "SDL.SoundRecordFile", "");
#ifdef WIN32
    config->addOption("soundbufsize", "SDL.SoundBufSize", 52);
#else
    config->addOption("soundbufsize", "SDL.SoundBufSize", 24);
#endif

    // old EOptions
    config->addOption('g', "gamegenie", "SDL.GameGenie", 0);
    config->addOption("lowpass", "SDL.LowPass", 0);
    config->addOption("pal", "SDL.PAL", 0);
    config->addOption("frameskip", "SDL.Frameskip", 0);
    config->addOption("snapname", "SDL.SnapName", 0);
    config->addOption("clipsides", "SDL.ClipSides", 0);
    config->addOption("nothrottle", "SDL.NoThrottle", 0);
    config->addOption("no8lim", "SDL.DisableSpriteLimit", 0);

    // color control
    config->addOption('p', "palette", "SDL.Palette", "");
    config->addOption("tint", "SDL.Tint", 56);
    config->addOption("hue", "SDL.Hue", 72);
    config->addOption("color", "SDL.Color", 0);

    // scanline settings
    config->addOption("slstart", "SDL.ScanLineStart", 0);
    config->addOption("slend", "SDL.ScanLineEnd", 239);

    // video controls
    config->addOption('x', "xres", "SDL.XResolution", 512);
    config->addOption('y', "yres", "SDL.YResolution", 448);
    config->addOption('f', "fullscreen", "SDL.Fullscreen", 0);
    config->addOption('b', "bpp", "SDL.BitsPerPixel", 32);
    config->addOption("doublebuf", "SDL.DoubleBuffering", 0);
    config->addOption("xscale", "SDL.XScale", 1.0);
    config->addOption("yscale", "SDL.YScale", 1.0);
    config->addOption("xstretch", "SDL.XStretch", 0);
    config->addOption("ystretch", "SDL.YStretch", 0);
    config->addOption("noframe", "SDL.NoFrame", 0);

    // OpenGL options
    config->addOption("opengl", "SDL.OpenGL", 0);
    config->addOption("openglip", "SDL.OpenGLip", 0);
    config->addOption("SDL.SpecialFilter", 0);
    config->addOption("SDL.SpecialFX", 0);

    // network play options
    config->addOption('n', "net", "SDL.NetworkServer", "");
    config->addOption('u', "user", "SDL.NetworkUsername", "");
    config->addOption('w', "pass", "SDL.NetworkPassword", "");
    config->addOption('k', "netkey", "SDL.NetworkGameKey", "");
    config->addOption('p', "port", "SDL.NetworkPort", 0xFCE);
    config->addOption('l', "players", "SDL.NetworkNumPlayers", 1);

    // input configuration options
    config->addOption("SDL.Input.0", "GamePad.0");
    config->addOption("SDL.Input.1", "GamePad.1");
    config->addOption("SDL.Input.2", "None");

    // Allow for input configuration
    config->addOption('i', "inputcfg", "SDL.InputCfg", InputCfg);

    // GamePad 0 - 3
    for(unsigned int i = 0; i < GAMEPAD_NUM_DEVICES; i++) {
        char buf[64];
        snprintf(buf, 20, "SDL.Input.GamePad.%d.", i);
        prefix = buf;

        config->addOption(prefix + "DeviceType", DefaultGamePadDevice[i]);
        config->addOption(prefix + "DeviceNum",  0);
        for(unsigned int j = 0; j < GAMEPAD_NUM_BUTTONS; j++) {
            config->addOption(prefix + GamePadNames[j], DefaultGamePad[i][j]);
        }
    }

    // PowerPad 0 - 1
    for(unsigned int i = 0; i < POWERPAD_NUM_DEVICES; i++) {
        char buf[64];
        snprintf(buf, 20, "SDL.Input.PowerPad.%d.", i);
        prefix = buf;

        config->addOption(prefix + "DeviceType", DefaultPowerPadDevice[i]);
        config->addOption(prefix + "DeviceNum",  0);
        for(unsigned int j = 0; j < POWERPAD_NUM_BUTTONS; j++) {
            config->addOption(prefix +PowerPadNames[j], DefaultPowerPad[i][j]);
        }
    }

    // QuizKing
    prefix = "SDL.Input.QuizKing.";
    config->addOption(prefix + "DeviceType", DefaultQuizKingDevice);
    config->addOption(prefix + "DeviceNum", 0);
    for(unsigned int j = 0; j < QUIZKING_NUM_BUTTONS; j++) {
        config->addOption(prefix + QuizKingNames[j], DefaultQuizKing[j]);
    }

    // HyperShot
    prefix = "SDL.Input.HyperShot.";
    config->addOption(prefix + "DeviceType", DefaultHyperShotDevice);
    config->addOption(prefix + "DeviceNum", 0);
    for(unsigned int j = 0; j < HYPERSHOT_NUM_BUTTONS; j++) {
        config->addOption(prefix + HyperShotNames[j], DefaultHyperShot[j]);
    }

    // Mahjong
    prefix = "SDL.Input.Mahjong.";
    config->addOption(prefix + "DeviceType", DefaultMahjongDevice);
    config->addOption(prefix + "DeviceNum", 0);
    for(unsigned int j = 0; j < MAHJONG_NUM_BUTTONS; j++) {
        config->addOption(prefix + MahjongNames[j], DefaultMahjong[j]);
    }

    // TopRider
    prefix = "SDL.Input.TopRider.";
    config->addOption(prefix + "DeviceType", DefaultTopRiderDevice);
    config->addOption(prefix + "DeviceNum", 0);
    for(unsigned int j = 0; j < TOPRIDER_NUM_BUTTONS; j++) {
        config->addOption(prefix + TopRiderNames[j], DefaultTopRider[j]);
    }

    // FTrainer
    prefix = "SDL.Input.FTrainer.";
    config->addOption(prefix + "DeviceType", DefaultFTrainerDevice);
    config->addOption(prefix + "DeviceNum", 0);
    for(unsigned int j = 0; j < FTRAINER_NUM_BUTTONS; j++) {
        config->addOption(prefix + FTrainerNames[j], DefaultFTrainer[j]);
    }

    // FamilyKeyBoard
    prefix = "SDL.Input.FamilyKeyBoard.";
    config->addOption(prefix + "DeviceType", DefaultFamilyKeyBoardDevice);
    config->addOption(prefix + "DeviceNum", 0);
    for(unsigned int j = 0; j < FAMILYKEYBOARD_NUM_BUTTONS; j++) {
        config->addOption(prefix + FamilyKeyBoardNames[j],
                          DefaultFamilyKeyBoard[j]);
    }

    // All mouse devices
    config->addOption("SDL.OekaKids.0.DeviceType", "Mouse");
    config->addOption("SDL.OekaKids.0.DeviceNum", 0);

    config->addOption("SDL.Arkanoid.0.DeviceType", "Mouse");
    config->addOption("SDL.Arkanoid.0.DeviceNum", 0);

    config->addOption("SDL.Shadow.0.DeviceType", "Mouse");
    config->addOption("SDL.Shadow.0.DeviceNum", 0);

    config->addOption("SDL.Zapper.0.DeviceType", "Mouse");
    config->addOption("SDL.Zapper.0.DeviceNum", 0);

    return config;
}

void
UpdateEMUCore(Config *config)
{
    int ntsccol, ntsctint, ntschue, flag, start, end;
    std::string cpalette;

    config->getOption("SDL.Color", &ntsccol);
    config->getOption("SDL.Tint", &ntsctint);
    config->getOption("SDL.Hue", &ntschue);
    FCEUI_SetNTSCTH(ntsccol, ntsctint, ntschue);

    config->getOption("SDL.Palette", &cpalette);
    if(cpalette.size()) {
        LoadCPalette(cpalette);
    }

    config->getOption("SDL.PAL", &flag);
    FCEUI_SetVidSystem(flag ? 1 : 0);

    config->getOption("SDL.GameGenie", &flag);
    FCEUI_SetGameGenie(flag ? 1 : 0);

    config->getOption("SDL.LowPass", &flag);
    FCEUI_SetLowPass(flag ? 1 : 0);

    config->getOption("SDL.DisableSpriteLimit", &flag);
    FCEUI_DisableSpriteLimitation(flag ? 1 : 0);

    config->getOption("SDL.DisableSpriteLimit", &flag);
    FCEUI_SetSnapName(flag ? 1 : 0);

    config->getOption("SDL.ScanLineStart", &start);
    config->getOption("SDL.ScanLineEnd", &end);

#if DOING_SCANLINE_CHECKS
    for(int i = 0; i < 2; x++) {
        if(srendlinev[x]<0 || srendlinev[x]>239) srendlinev[x]=0;
        if(erendlinev[x]<srendlinev[x] || erendlinev[x]>239) erendlinev[x]=239;
    }
#endif

    FCEUI_SetRenderedLines(start + 8, end - 8, start, end);
}
