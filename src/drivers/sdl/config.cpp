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
    char *subs[7]={"fcs","snaps","gameinfo","sav","cheats","movie"};
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
        dir = std::string(home) + "/.fceux";
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
    config->addOption("volume", "SDL.SoundVolume", 150);
    config->addOption("trianglevol", "SDL.TriangleVolume", 256);
    config->addOption("square1vol", "SDL.Square1Volume", 256);
    config->addOption("square2vol", "SDL.Square2Volume", 256);
    config->addOption("noisevol", "SDL.NoiseVolume", 256);
    config->addOption("pcmvol", "SDL.PCMVolume", 256);
    config->addOption("soundrate", "SDL.SoundRate", 11000);
    config->addOption("soundq", "SDL.SoundQuality", 1);
    config->addOption("soundrecord", "SDL.SoundRecordFile", "");
    config->addOption("soundbufsize", "SDL.SoundBufSize", 48);

    // old EOptions
    config->addOption('g', "gamegenie", "SDL.GameGenie", 0);
    config->addOption("lowpass", "SDL.LowPass", 0);
    config->addOption("pal", "SDL.PAL", 0);
    config->addOption("frameskip", "SDL.Frameskip", 0);
    config->addOption("clipsides", "SDL.ClipSides", 0);
    config->addOption("nospritelim", "SDL.DisableSpriteLimit", 1);

    // color control
    config->addOption('p', "palette", "SDL.Palette", "");
    config->addOption("tint", "SDL.Tint", 56);
    config->addOption("hue", "SDL.Hue", 72);
    config->addOption("ntsccolor", "SDL.Color", 0);

    // scanline settings
    config->addOption("slstart", "SDL.ScanLineStart", 0);
    config->addOption("slend", "SDL.ScanLineEnd", 239);

    // video controls
    config->addOption('x', "xres", "SDL.XResolution", 512);
    config->addOption('y', "yres", "SDL.YResolution", 448);
    config->addOption('f', "fullscreen", "SDL.Fullscreen", 0);
    config->addOption('b', "bpp", "SDL.BitsPerPixel", 32);
    config->addOption("doublebuf", "SDL.DoubleBuffering", 0);
    config->addOption("autoscale", "SDL.AutoScale", 1);
    config->addOption("keepratio", "SDL.KeepRatio", 1);
    config->addOption("xscale", "SDL.XScale", 1.0);
    config->addOption("yscale", "SDL.YScale", 1.0);
    config->addOption("xstretch", "SDL.XStretch", 0);
    config->addOption("ystretch", "SDL.YStretch", 0);
    config->addOption("noframe", "SDL.NoFrame", 0);
    config->addOption("special", "SDL.SpecialFilter", 0);

    // OpenGL options
    config->addOption("opengl", "SDL.OpenGL", 0);
    config->addOption("openglip", "SDL.OpenGLip", 0);
    config->addOption("SDL.SpecialFilter", 0);
    config->addOption("SDL.SpecialFX", 0);

    // network play options - netplay is broken
    /*
    config->addOption('n', "net", "SDL.NetworkServer", "");
    config->addOption('u', "user", "SDL.NetworkUsername", "");
    config->addOption('w', "pass", "SDL.NetworkPassword", "");
    config->addOption('k', "netkey", "SDL.NetworkGameKey", "");
    config->addOption( 0, "port", "SDL.NetworkPort", 4046);
    config->addOption('l', "players", "SDL.NetworkNumPlayers", 1);
    */ 
    // input configuration options
    config->addOption("input1", "SDL.Input.0", "GamePad.0");
    config->addOption("input2", "SDL.Input.1", "GamePad.1");
    config->addOption("input3", "SDL.Input.2", "Gamepad.2");
    config->addOption("input4", "SDL.Input.3", "Gamepad.3");

    // allow for input configuration
    config->addOption('i', "inputcfg", "SDL.InputCfg", InputCfg);
    
    // display input
    config->addOption("inputdisplay", "SDL.InputDisplay", 0);
    
    // overwrite the config file?
    config->addOption("no-config", "SDL.NoConfig", 0);
    
    // video playback
    config->addOption("playmov", "SDL.Movie", "");
    
    #ifdef _S9XLUA_H
    // load lua script
    config->addOption("loadlua", "SDL.LuaScript", "");
    #endif
    
    #ifdef CREATE_AVI
    config->addOption("videolog",  "SDL.VideoLog",  "");
    config->addOption("mute", "SDL.MuteCapture", 0);
    #endif
    
    config->addOption("fcmconvert", "SDL.FCMConvert", "");
	
	// enable new PPU core
	config->addOption("newppu", "SDL.NewPPU", "0");

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
    
    // Hotkeys
    prefix = "SDL.Hotkeys.";
    config->addOption(prefix + "CheatMenu", SDLK_F1);
	config->addOption(prefix + "BindState", SDLK_F2);
    #ifdef _S9XLUA_H
    config->addOption(prefix + "LoadLua", SDLK_F3);
    #endif
    config->addOption(prefix + "RenderBG", SDLK_F4);
    config->addOption(prefix + "SaveState", SDLK_F5);
    config->addOption(prefix + "FrameAdvanceLagSkip", SDLK_F6);
    config->addOption(prefix + "LoadState", SDLK_F7);
    config->addOption(prefix + "LagCounterDisplay", SDLK_F8);
    config->addOption(prefix + "MovieToggleFrameDisplay", SDLK_F9);
    config->addOption(prefix + "SubtitleDisplay", SDLK_F10);
    config->addOption(prefix + "Reset", SDLK_F11);
    config->addOption(prefix + "Screenshot", SDLK_F12);
    config->addOption(prefix + "Pause", SDLK_PAUSE);
    config->addOption(prefix + "DecreaseSpeed", SDLK_MINUS);
    config->addOption(prefix + "IncreaseSpeed", SDLK_EQUALS);
    config->addOption(prefix + "FrameAdvance", SDLK_BACKSLASH);
    config->addOption(prefix + "InputDisplay", SDLK_i);
    config->addOption(prefix + "Quit", SDLK_ESCAPE);
    //config->addOption(prefix + "Power", 0);
    
    
    
    
	config->addOption(prefix + "SelectState0", SDLK_0);
	config->addOption(prefix + "SelectState1", SDLK_1);
	config->addOption(prefix + "SelectState2", SDLK_2);
	config->addOption(prefix + "SelectState3", SDLK_3);
	config->addOption(prefix + "SelectState4", SDLK_4);
	config->addOption(prefix + "SelectState5", SDLK_5);
	config->addOption(prefix + "SelectState6", SDLK_6);
	config->addOption(prefix + "SelectState7", SDLK_7);
	config->addOption(prefix + "SelectState8", SDLK_8);
	config->addOption(prefix + "SelectState9", SDLK_9);
	
    

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

    //Not used anymore.
    //config->getOption("SDL.SnapName", &flag);
    //FCEUI_SetSnapName(flag ? true : false);

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
