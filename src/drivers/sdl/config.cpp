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

int   frameskip;
char *cpalette;
char *soundrecfn;
int   ntsccol, ntschue, ntsctint;
char *DrBaseDirectory;

DSETTINGS Settings;
CFGSTRUCT DriverConfig[]={
	#ifdef OPENGL
	AC(_stretchx),
	AC(_stretchy),
	AC(_opengl),
	AC(_openglip),
	#endif
	AC(Settings.special),
	AC(Settings.specialfs),
	AC(_doublebuf),
	AC(_xscale),
	AC(_yscale),
	AC(_xscalefs),
	AC(_yscalefs),
	AC(_bpp),
	AC(_efx),
	AC(_efxfs),
	AC(_fullscreen),
        AC(_xres),
	AC(_yres),
        ACS(netplaynick),
	AC(netlocalplayers),
	AC(tport),
	ACS(netpassword),
	ACS(netgamekey),
        ENDCFGSTRUCT
};

//-fshack x       Set the environment variable SDL_VIDEODRIVER to \"x\" when
//                entering full screen mode and x is not \"0\".

char *DriverUsage=
"-xres   x	Set horizontal resolution to x for full screen mode.\n\
-yres   x       Set vertical resolution to x for full screen mode.\n\
-xscale(fs) x	Multiply width by x(Real numbers >0 with OpenGL, otherwise integers >0).\n\
-yscale(fs) x	Multiply height by x(Real numbers >0 with OpenGL, otherwise integers >0).\n\
-bpp(fs) x	Bits per pixel for SDL surface(and video mode in fs). 8, 16, 32.\n\
-opengl x	Enable OpenGL support if x is 1.\n\
-openglip x	Enable OpenGL linear interpolation if x is 1.\n\
-doublebuf x	\n\
-special(fs) x	Specify special scaling filter.\n\
-stretch(x/y) x	Stretch to fill surface on x or y axis(fullscreen, only with OpenGL).\n\
-efx(fs) x	Enable special effects.  Logically OR the following together:\n\
		 1 = scanlines(for yscale>=2).\n\
		 2 = TV blur(for bpp of 16 or 32).\n\
-fs	 x      Select full screen mode if x is non zero.\n\
-connect s      Connect to server 's' for TCP/IP network play.\n\
-netnick s	Set the nickname to use in network play.\n\
-netgamekey s 	Use key 's' to create a unique session for the game loaded.\n\
-netpassword s	Password to use for connecting to the server.\n\
-netlocalplayers x	Set the number of local players.\n\
-netport x      Use TCP/IP port x for network play.";

ARGPSTRUCT DriverArgs[]={
	#ifdef OPENGL
	 {"-opengl",0,&_opengl,0},
	 {"-openglip",0,&_openglip,0},
	 {"-stretchx",0,&_stretchx,0},
	 {"-stretchy",0,&_stretchy,0},
	#endif
	 {"-special",0,&Settings.special,0},
	 {"-specialfs",0,&Settings.specialfs,0},
	 {"-doublebuf",0,&_doublebuf,0},
	 {"-bpp",0,&_bpp,0},
	 {"-xscale",0,&_xscale,2},
	 {"-yscale",0,&_yscale,2},
	 {"-efx",0,&_efx,0},
         {"-xscalefs",0,&_xscalefs,2},
         {"-yscalefs",0,&_yscalefs,2},
         {"-efxfs",0,&_efxfs,0},
	 {"-xres",0,&_xres,0},
         {"-yres",0,&_yres,0},
         {"-fs",0,&_fullscreen,0},
         //{"-fshack",0,&_fshack,0x4001},
         {"-connect",0,&netplayhost,0x4001},
         {"-netport",0,&tport,0},
	 {"-netlocalplayers",0,&netlocalplayers,0},
	 {"-netnick",0,&netplaynick,0x4001},
	 {"-netpassword",0,&netpassword,0x4001},
         {0,0,0,0}
};

#include "usage.h"

void
SetDefaults(void)
{
    Settings.special=Settings.specialfs=0;
    _bpp=8;
    _xres=640;
    _yres=480;
    _fullscreen=0;
    _xscale=2.50;
    _yscale=2;
    _xscalefs=_yscalefs=2;
    _efx=_efxfs=0;
    //_fshack=_fshacksave=0;
#ifdef OPENGL
    _opengl=1;
    _stretchx=1; 
    _stretchy=0;
    _openglip=1;
#endif
}


/**
 * Unimplemented.
 */
void DoDriverArgs(void)
{

}

/**
 * Handles arguments passed to FCEU.  Hopefully obsolete with new
 * configuration system.
 */
static void
DoArgs(int argc,
       char *argv[])
{
    int x;

    static ARGPSTRUCT FCEUArgs[]={
        {"-soundbufsize",0,&soundbufsize,0},
        {"-soundrate",0,&soundrate,0},
        {"-soundq",0,&soundq,0},
#ifdef FRAMESKIP
        {"-frameskip",0,&frameskip,0},
#endif
        {"-sound",0,&_sound,0},
        {"-soundvol",0,&soundvol,0},
        {"-cpalette",0,&cpalette,0x4001},
        {"-soundrecord",0,&soundrecfn,0x4001},

        {"-ntsccol",0,&ntsccol,0},
        {"-pal",0,&eoptions,0x8000|EO_PAL},

        {"-lowpass",0,&eoptions,0x8000|EO_LOWPASS},
        {"-gg",0,&eoptions,0x8000|EO_GAMEGENIE},
        {"-no8lim",0,&eoptions,0x8001},
        {"-snapname",0,&eoptions,0x8000|EO_SNAPNAME},
        {"-nofs",0,&eoptions,0x8000|EO_NOFOURSCORE},
        {"-clipsides",0,&eoptions,0x8000|EO_CLIPSIDES},
        {"-nothrottle",0,&eoptions,0x8000|EO_NOTHROTTLE},
        {"-slstart",0,&srendlinev[0],0},{"-slend",0,&erendlinev[0],0},
        {"-slstartp",0,&srendlinev[1],0},{"-slendp",0,&erendlinev[1],0},
        {0,(int *)InputArgs,0,0},
        {0,(int *)DriverArgs,0,0},
        {0,0,0,0}
    };

    ParseArguments(argc, argv, FCEUArgs);
    if(cpalette) {
        if(cpalette[0] == '0') {
            if(cpalette[1] == 0) {
                free(cpalette);
                cpalette=0;
            }
        }
    }

    FCEUI_SetVidSystem((eoptions&EO_PAL)?1:0);
    FCEUI_SetGameGenie((eoptions&EO_GAMEGENIE)?1:0);
    FCEUI_SetLowPass((eoptions&EO_LOWPASS)?1:0);

    FCEUI_DisableSpriteLimitation(eoptions&1);
    FCEUI_SetSnapName(eoptions&EO_SNAPNAME);

    for(x = 0; x < 2; x++) {
        if(srendlinev[x]<0 || srendlinev[x]>239) srendlinev[x]=0;
        if(erendlinev[x]<srendlinev[x] || erendlinev[x]>239) erendlinev[x]=239;
    }

    FCEUI_SetRenderedLines(srendlinev[0],erendlinev[0],srendlinev[1],erendlinev[1]);
    DoDriverArgs();
}

/**
 * Read a custom pallete from a file and load it into the core.
 */
static void
LoadCPalette()
{
    uint8 tmpp[192];
    FILE *fp;

    if(!(fp = FCEUD_UTF8fopen(cpalette, "rb"))) {
        printf(" Error loading custom palette from file: %s\n", cpalette);
        return;
    }
    fread(tmpp, 1, 192, fp);
    FCEUI_SetPaletteArray(tmpp);
    fclose(fp);
}


static CFGSTRUCT fceuconfig[]= {
	AC(soundrate),
	AC(soundq),
	AC(_sound),
	AC(soundvol),
	AC(soundbufsize),
	ACS(cpalette),
	AC(ntsctint),
	AC(ntschue),
	AC(ntsccol),
	AC(eoptions),
	ACA(srendlinev),
	ACA(erendlinev),
	ADDCFGSTRUCT(InputConfig),
	ADDCFGSTRUCT(DriverConfig),
	ENDCFGSTRUCT
};

/**
 * Wrapper for SaveFCEUConfig() that sets the path.  Hopefully
 * obsolete with new configuration system.
 */
void
SaveConfig()
{	
    char tdir[2048];
    sprintf(tdir,"%s"PSS"fceu98.cfg", DrBaseDirectory);
    FCEUI_GetNTSCTH(&ntsctint, &ntschue);
    SaveFCEUConfig(tdir, fceuconfig);
}

/**
 * Wrapper for LoadFCEUConfig() that sets the path.  Hopefully
 * obsolete with the new configuration system.
 */
static void
LoadConfig()
{
    char tdir[2048];
    sprintf(tdir,"%s"PSS"fceu98.cfg",DrBaseDirectory);

    /* Get default settings for if no config file exists. */
    FCEUI_GetNTSCTH(&ntsctint, &ntschue);	
    LoadFCEUConfig(tdir,fceuconfig);
    InputUserActiveFix();
}

/**
 * Creates the subdirectories used for saving snapshots, movies, game
 * saves, etc.  Hopefully obsolete with new configuration system.
 */
static void
CreateDirs(void)
{
    char *subs[7]={"fcs","fcm","snaps","gameinfo","sav","cheats","movie"};
    char tdir[2048];
    int x;

#ifdef WIN32
    mkdir((char *)DrBaseDirectory);
    for(x = 0; x < 6; x++) {
        sprintf(tdir,"%s"PSS"%s",DrBaseDirectory,subs[x]);
        mkdir(tdir);
    }
#else
    mkdir((char *)DrBaseDirectory,S_IRWXU);
    for(x = 0; x < 6; x++) {
        sprintf(tdir,"%s"PSS"%s",DrBaseDirectory,subs[x]);
        mkdir(tdir,S_IRWXU);
    }
#endif
}

/**
 * Attempts to locate FCEU's application directory.  This will
 * hopefully become obsolete once the new configuration system is in
 * place.
 */
static uint8 *
GetBaseDirectory()
{
    uint8 *ol;
    uint8 *ret; 

    ol=(uint8 *)getenv("HOME");

    if(ol) {
        ret=(uint8 *)malloc(strlen((char *)ol)+1+strlen("./fceultra"));
        strcpy((char *)ret,(char *)ol);
        strcat((char *)ret,"/.fceultra");
    } else {
#ifdef WIN32
        char *sa;

        ret=(uint8*)malloc(MAX_PATH+1);
        GetModuleFileName(NULL,(char*)ret,MAX_PATH+1);

        sa=strrchr((char*)ret,'\\');
        if(sa)
            *sa = 0; 
#else
        ret=(uint8 *)malloc(sizeof(uint8));
        ret[0]=0;
#endif
        printf("%s\n",ret);
    }
    return(ret);
}


int
InitConfig(int argc,
           char **argv)
{
    DrBaseDirectory = (char *)GetBaseDirectory();
    FCEUI_SetBaseDirectory(DrBaseDirectory);

    CreateDirs();

    if(argc<=1)  {
        ShowUsage(argv[0]);
        return -1;
    }
    LoadConfig();
    DoArgs(argc - 2, &argv[1]);

    FCEUI_SetNTSCTH(ntsccol, ntsctint, ntschue);
    if(cpalette) {
        LoadCPalette();
    }

    return 0;
}
