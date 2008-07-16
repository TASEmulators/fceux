// main.cpp
// get_fceu_key
//   this program opens an SDL window and gets an input event from the user.
//   it than prints the values that are needed in the fceux config
//   this was written to be used with gfceux, the GTK frontend for gfceu

// Lukas Sabota
// Licensed under the GPL v2
// July 16, 2008

#include<iostream>
#include<SDL.h>

const int WIDTH = 300;	
const int HEIGHT = 300;
const int BPP = 4;
const int DEPTH = 32;

using namespace std;

int main(int argc, char* argv[])
{
    // the caption doesn't set on intrepid
    // TODO: test on ubuntu 8.04
	SDL_WM_SetCaption("Press any key. . .", 0);
	
	SDL_Surface *screen;
	SDL_Event event;
	
	// open any joysticks
	for(int i = 0; i < SDL_NumJoysticks(); i++)
		SDL_JoystickOpen(i);

	if (!(screen = SDL_SetVideoMode(WIDTH, HEIGHT, DEPTH, SDL_SWSURFACE)))
    {
        SDL_Quit();
        return 1;
    }

	 while(1) 
     {
         while(SDL_PollEvent(&event)) 
         {      
              switch (event.type) 
              {
                    case SDL_QUIT:
                        return 0;
                    // this code was modified from drivers/sdl/input.cpp in fceu so 
                    //   that the same values will be written in the config as fceux --inputcfg
	                case SDL_KEYDOWN:
                        cout << "BUTTC_KEYBOARD" << endl;
                        cout << 0 << endl;
                        cout << event.key.keysym.sym << endl;
                        return 1;
                    case SDL_JOYBUTTONDOWN:
                        cout << "BUTTC_JOYSTICK" << endl;
                        cout << event.jbutton.which << endl;
                        cout << event.jbutton.button << endl; 
                        return 1;
                    case SDL_JOYHATMOTION:
                        if(event.jhat.value != SDL_HAT_CENTERED) {
                            cout << "BUTTC_JOYSTICK" << endl;
                            cout << event.jhat.which << endl;
                            cout <<  (0x2000 | ((event.jhat.hat & 0x1F) << 8) |
                                                 event.jhat.value) << endl;
                            return 1;
                        }
                        break;
                }
            }
        }   
    return 0;
}

