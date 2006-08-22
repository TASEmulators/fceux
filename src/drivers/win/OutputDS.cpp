#include "oakra.h"

#include "dsound.h"
#include <vector>

#define LATENCY_MS (50)


int voltbl[] = {
-10000,-8750,-8018,-7499,-7097,-6768,-6490,-6250,-6037,-5847,-5675,-5518,-5374,-5240,-5116,-5000,
-4890,-4787,-4690,-4597,-4509,-4425,-4345,-4268,-4195,-4124,-4056,-3990,-3927,-3866,-3807,-3749,
-3694,-3640,-3588,-3537,-3488,-3440,-3393,-3347,-3303,-3259,-3217,-3175,-3135,-3095,-3056,-3018,
-2981,-2945,-2909,-2874,-2840,-2806,-2773,-2740,-2708,-2677,-2646,-2616,-2586,-2557,-2528,-2500,
-2472,-2444,-2417,-2390,-2364,-2338,-2312,-2287,-2262,-2238,-2213,-2190,-2166,-2143,-2120,-2097,
-2075,-2053,-2031,-2009,-1988,-1967,-1946,-1925,-1905,-1885,-1865,-1845,-1826,-1806,-1787,-1768,
-1750,-1731,-1713,-1695,-1677,-1659,-1641,-1624,-1607,-1590,-1573,-1556,-1539,-1523,-1506,-1490,
-1474,-1458,-1443,-1427,-1412,-1396,-1381,-1366,-1351,-1336,-1321,-1307,-1292,-1278,-1264,-1250,
-1235,-1222,-1208,-1194,-1180,-1167,-1153,-1140,-1127,-1114,-1101,-1088,-1075,-1062,-1050,-1037,
-1025,-1012,-1000,-988,-976,-963,-951,-940,-928,-916,-904,-893,-881,-870,-858,-847,
-836,-825,-814,-803,-792,-781,-770,-759,-748,-738,-727,-717,-706,-696,-685,-675,
-665,-655,-645,-635,-625,-615,-605,-595,-585,-576,-566,-556,-547,-537,-528,-518,
-509,-500,-490,-481,-472,-463,-454,-445,-436,-427,-418,-409,-400,-391,-383,-374,
-365,-357,-348,-340,-331,-323,-314,-306,-298,-289,-281,-273,-265,-256,-248,-240,
-232,-224,-216,-208,-200,-193,-185,-177,-169,-162,-154,-146,-139,-131,-123,-116,
-108,-101,-93,-86,-79,-71,-64,-57,-49,-42,-35,-28,-21,-14,-7,0};

class DSVoice : public OAKRA_Voice {
public:
	OAKRA_Module_OutputDS *driver;
	OAKRA_Format format;
	int formatShift;
	IDirectSoundBuffer *ds_buf;
	int buflen;
	unsigned int cPlay;
	int vol,pan;

	virtual void setPan(int pan) {
		this->pan = pan;
		if(pan==0) ds_buf->SetPan(0);
		else if(pan>0) ds_buf->SetPan(-voltbl[255-pan]);
		else ds_buf->SetPan(voltbl[255+pan]);
	}
	virtual void setVol(int vol) {
		this->vol = vol;
		ds_buf->SetVolume(voltbl[vol]);
	}
	virtual int getVol() { return vol; }
	virtual int getPan() { return pan; }
	void setSource(OAKRA_Module *source) {
		this->source = source;
	}
	virtual ~DSVoice() {
		driver->freeVoiceInternal(this,true);
		ds_buf->Release();
	}
	DSVoice(OAKRA_Module_OutputDS *driver, OAKRA_Format &format, IDirectSound *ds_dev, bool global) { 
		this->driver = driver;
		vol = 255;
		pan = 0;
		source = 0;
		this->format = format;
		formatShift = getFormatShift(format);
		buflen = (format.rate * LATENCY_MS / 1000);

		WAVEFORMATEX wfx;
		memset(&wfx, 0, sizeof(wfx));
		wfx.wFormatTag      = WAVE_FORMAT_PCM;
		wfx.nChannels       = format.channels;
		wfx.nSamplesPerSec  = format.rate;
		wfx.nAvgBytesPerSec = format.rate * format.size;
		wfx.nBlockAlign     = format.size;
		wfx.wBitsPerSample  = (format.format==OAKRA_S16?16:8);
		wfx.cbSize          = sizeof(wfx);

		DSBUFFERDESC dsbd;
		memset(&dsbd, 0, sizeof(dsbd));
		dsbd.dwSize        = sizeof(dsbd);
		dsbd.dwFlags       = DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_LOCSOFTWARE | DSBCAPS_CTRLVOLUME ;
		if(global) dsbd.dwFlags |= DSBCAPS_GLOBALFOCUS ;
		dsbd.dwBufferBytes = buflen  * format.size;
		dsbd.lpwfxFormat   = &wfx;
		
		HRESULT hr = ds_dev->CreateSoundBuffer(&dsbd,&ds_buf,0);
		cPlay = 0;

		hr = ds_buf->Play(0,0,DSBPLAY_LOOPING);
		int xxx=9;
	}
	
	//not supported
	virtual void volFade(int start, int end, int ms) {}

	void update() {
	    DWORD play, write;
		HRESULT hr = ds_buf->GetCurrentPosition(&play, &write);
		play >>= formatShift;
		write >>= formatShift;
		
		int todo;
		if(play<cPlay) todo = play + buflen - cPlay;
		else todo = play - cPlay;

		if(!todo) return;
		

		void* buffer1;
		void* buffer2;
		DWORD buffer1_length;
		DWORD buffer2_length;
		hr = ds_buf->Lock(
			cPlay<<formatShift,todo<<formatShift,
			&buffer1, &buffer1_length,
			&buffer2, &buffer2_length,0
			);

		buffer1_length >>= formatShift;
		buffer2_length >>= formatShift;
		int done = 0;
		if(source) {
			done = source->generate(buffer1_length,buffer1);
			if(done != buffer1_length) {
				generateSilence(buffer1_length - done,(char *)buffer1 + (done<<formatShift),format.size);
				generateSilence(buffer2_length,buffer2,format.size);
				die();
			} else {
				if(buffer2_length) {
					done = source->generate(buffer2_length,buffer2);
					if(done != buffer2_length) {
						generateSilence(buffer2_length - done,(char *)buffer2 + (done<<formatShift),format.size);
						die();
					}
				}
			}
		}

		ds_buf->Unlock(
			buffer1, buffer1_length,
			buffer2, buffer2_length);

		cPlay = play;
	}
};

class Data {
public:
	bool global;
	IDirectSound* ds_dev;
	std::vector<DSVoice *> voices;
	CRITICAL_SECTION criticalSection;
};


class ThreadData {
public:
	ThreadData() { kill = dead = false; }
	OAKRA_Module_OutputDS *ds;
	bool kill,dead;
};

OAKRA_Module_OutputDS::OAKRA_Module_OutputDS() {
	data = new Data();
	((Data *)data)->global = false;
	InitializeCriticalSection(&((Data *)data)->criticalSection);
}

OAKRA_Module_OutputDS::~OAKRA_Module_OutputDS() {
	//ask the driver to shutdown, and wait for it to do so
	((ThreadData *)threadData)->kill = true;
	while(!((ThreadData *)threadData)->dead) Sleep(1);

	////kill all the voices
	std::vector<DSVoice *> voicesCopy = ((Data *)data)->voices;
	int voices = (int)voicesCopy.size();
	for(int i=0;i<voices;i++)
		delete voicesCopy[i];

	////free other resources
	DeleteCriticalSection(&((Data *)data)->criticalSection);
	((Data *)data)->ds_dev->Release();
	delete (Data *)data;
	delete (ThreadData *)threadData;
}

OAKRA_Voice *OAKRA_Module_OutputDS::getVoice(OAKRA_Format &format, OAKRA_Module *source) {
	DSVoice *dsv = (DSVoice *)getVoice(format);
	dsv->setSource(source);
	return dsv;
}

OAKRA_Voice *OAKRA_Module_OutputDS::getVoice(OAKRA_Format &format) {
	DSVoice *voice = new DSVoice(this,format,((Data *)data)->ds_dev,((Data *)data)->global);
	((Data *)data)->voices.push_back(voice);
	return voice;
}
void OAKRA_Module_OutputDS::freeVoice(OAKRA_Voice *voice) {
	freeVoiceInternal(voice,false);
}

void OAKRA_Module_OutputDS::freeVoiceInternal(OAKRA_Voice *voice, bool internal) {
	lock();
	Data *data = (Data *)this->data;
	int j = -1;
	for(int i=0;i<(int)data->voices.size();i++)
		if(data->voices[i] == voice) j = i;
	if(j!=-1)
		data->voices.erase(data->voices.begin()+j);
	if(!internal)
		delete voice;
	unlock();
}



void OAKRA_Module_OutputDS::start(void *hwnd) {
	HRESULT hr = CoInitialize(NULL);
	IDirectSound* ds_dev;
	hr = CoCreateInstance(CLSID_DirectSound,0,CLSCTX_INPROC_SERVER,IID_IDirectSound,(void**)&ds_dev);
	
	if(!hwnd) {
		hwnd = GetDesktopWindow();
		((Data *)data)->global = true;
	}
	
	//use default device
	hr = ds_dev->Initialize(0); 
	hr = ds_dev->SetCooperativeLevel((HWND)hwnd, DSSCL_NORMAL);

	((Data *)data)->ds_dev = ds_dev;
}


DWORD WINAPI updateProc(LPVOID lpParameter) {
	ThreadData *data = (ThreadData *)lpParameter;
	for(;;) {
		if(data->kill) break;
		data->ds->update();
		Sleep(1);
	}
	data->dead = true;
	return 0;
}

void OAKRA_Module_OutputDS::beginThread() {
	DWORD updateThreadId;
	threadData = new ThreadData();
	((ThreadData *)threadData)->ds = this;
	HANDLE updateThread = CreateThread(0,0,updateProc,threadData,0,&updateThreadId);
	SetThreadPriority(updateThread,THREAD_PRIORITY_TIME_CRITICAL);
	//SetThreadPriority(updateThread,THREAD_PRIORITY_HIGHEST);
}

void OAKRA_Module_OutputDS::endThread() {
	((ThreadData *)threadData)->kill = true;
}

void OAKRA_Module_OutputDS::update() {
	lock();
	int voices = (int)((Data *)data)->voices.size();

	//render all the voices
	for(int i=0;i<voices;i++)
		((Data *)data)->voices[i]->update();

	//look for voices that are dead
	std::vector<DSVoice *> deaders;
	for(int i=0;i<voices;i++)
		if(((Data *)data)->voices[i]->dead)
			deaders.push_back(((Data *)data)->voices[i]);

	//unlock the driver before killing voices!
	//that way, the voice's death callback won't occur within the driver lock
	unlock();

	//kill those voices
	for(int i=0;i<(int)deaders.size();i++) {
		deaders[i]->callbackDied();
		freeVoice(deaders[i]);
	}
}

void OAKRA_Module_OutputDS::lock() {
	EnterCriticalSection(  &((Data *)this->data)->criticalSection );
}

void OAKRA_Module_OutputDS::unlock() {
	LeaveCriticalSection(  &((Data *)this->data)->criticalSection );
}

