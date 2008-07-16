#include<iostream>
#include<SDL.h>

const int WIDTH = 640;	
const int HEIGHT = 480;
const int BPP = 4;
const int DEPTH = 32;


using namespace std;

int main(int argc, char* argv[])
{
	SDL_WM_SetCaption("Press any key. . .", NULL);
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
/*                    case SDL_JOYAXISMOTION: 
                        if(LastAx[event.jaxis.which][event.jaxis.axis] == 0x100000) {
                            if(abs(event.jaxis.value) < 1000) {
                                LastAx[event.jaxis.which][event.jaxis.axis] = event.jaxis.value;
                            }
                        } else {
                            if(abs(LastAx[event.jaxis.which][event.jaxis.axis] - event.jaxis.value) >= 8192)  {
                                bc->ButtType[wb]  = BUTTC_JOYSTICK;
                                bc->DeviceNum[wb] = event.jaxis.which;
                                bc->ButtonNum[wb] = (0x8000 | event.jaxis.axis |
                                                     ((event.jaxis.value < 0)
                                                      ? 0x4000 : 0));
                                return(1);
                            }
                        }
                        break;
                                case SDL_KEYDOWN:
                        keypress = 1;
                        cout << (int)event.key.keysym.sym;
                        break;
                    case SDL_JOYAXISMOTION:
                        if ( (event.jaxis.value < -3200 ) | | (event.jaxis.value > 3200) )
                        {
                            if (event.jaxis == 0)
                            {
*/                                    
                  
                      
             }
        }
    }   
    return 0;
}

