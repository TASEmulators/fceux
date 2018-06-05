/* FCE Ultra Network Play Server
 *
 * Copyright notice for this file:
 *  Copyright (C) 2004 Xodnizel
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/param.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>

#include <exception>

#include "types.h"
#include "md5.h"
#include "throttle.h"

#define VERSION "0.0.5"
#define DEFAULT_PORT 4046
#define DEFAULT_MAX 100
#define DEFAULT_TIMEOUT 5
#define DEFAULT_FRAMEDIVISOR 1
#define DEFAULT_CONFIG "/etc/fceux-server.conf"

// MSG_NOSIGNAL and SOL_TCP have been depreciated on osx
#if defined (__APPLE__) || defined(BSD)
#define MSG_NOSIGNAL SO_NOSIGPIPE
#define SOL_TCP IPPROTO_TCP
#endif

typedef struct {
	uint32 id; /* mainly for faster referencing when pointed to from the Games
	              entries.
	           */
	char *nickname;
	int TCPSocket;
	void *game;         /* Pointer to the game this player is in. */
	int localplayers;   /* The number of local players, 1-4 */

	time_t timeconnect; /* Time the client made the connection. */

	/* Variables to handle non-blocking TCP reads. */
	uint8 *nbtcp;
	uint32 nbtcphas, nbtcplen;
	uint32 nbtcptype;
} ClientEntry;

typedef struct
{
	uint8 id[16];            /* Unique 128-bit identifier for this game session. */
	uint8 joybuf[5];         /* 4 player data + 1 command byte */
	int MaxPlayers;          /* Maximum players for this game */
	ClientEntry *Players[4]; /* Pointers to player data. */
	int IsUnique[4];         /* Set to 1 if player is unique client, 0
	                            if it's a duplicated entry to handle multiple players
	                            per client.
	                         */
	uint8 ExtraInfo[64];     /* Expansion information to be used in future versions
	                            of FCE Ultra.
	                         */
} GameEntry;

typedef struct
{
	unsigned int MaxClients;     /* The maximum number of clients to allow. */
	                             /* You should expect each client to use
	                                65-70Kbps(not including save state loads) while
	                                connected(and logged in), when using the default
	                                FrameDivisor value of 1.
	                             */
	unsigned int ConnectTimeout; /* How long to wait(in seconds) for the client to provide
	                                login data before disconnecting the client.
	                             */
	unsigned int FrameDivisor;   /* The number of updates per second are divided
	                                by this number.  1 = 60 updates/sec(approx),
	                                2 = 30 updates/sec, etc. */
	unsigned int Port;           /* The port to listen on. */
	uint8 *Password;             /* The server password. */
} CONFIG;

CONFIG ServerConfig;

int LoadConfigFile(char *fn)
{
	FILE *fp;
	ServerConfig.Port = ServerConfig.MaxClients = ServerConfig.ConnectTimeout = ServerConfig.FrameDivisor = ~0;
	if(fp=fopen(fn,"rb"))
	{
		char buf[256];
		while(fgets(buf, 256, fp) != NULL)
		{
			if(!strncasecmp(buf,"maxclients",strlen("maxclients")))
				sscanf(buf,"%*s %d",&ServerConfig.MaxClients);
			else if(!strncasecmp(buf,"connecttimeout",strlen("connecttimeout")))
				sscanf(buf,"%*s %d",&ServerConfig.ConnectTimeout);
			else if(!strncasecmp(buf,"framedivisor",strlen("framedivisor")))
				sscanf(buf,"%*s %d",&ServerConfig.FrameDivisor);
			else if(!strncasecmp(buf,"port",strlen("port")))
				sscanf(buf,"%*s %d",&ServerConfig.Port);
			else if(!strncasecmp(buf,"password",strlen("password")))
			{
				char *pass = 0;
				sscanf(buf,"%*s %as",&pass);
				if(pass)
				{
					struct md5_context md5;
					ServerConfig.Password = (uint8 *)malloc(16);
					md5_starts(&md5);
					md5_update(&md5,(uint8*)pass,strlen(pass));
					md5_finish(&md5,ServerConfig.Password);
					puts("Password required to log in.");
				}
			}
		}
	}
	else
	{
		printf("Cannot load configuration file %s.\n", fn);
		return(0);
	}

	if(~0 == (ServerConfig.Port & ServerConfig.MaxClients & ServerConfig.ConnectTimeout & ServerConfig.FrameDivisor))
	{
		puts("Incomplete configuration file");
		return(0);
	}
	//printf("%d,%d,%d\n",ServerConfig.MaxClients,ServerConfig.ConnectTimeout,ServerConfig.FrameDivisor);
	printf("Using configuration file located at %s\n", fn);
	return(1);
}

static ClientEntry *Clients;
static GameEntry *Games;

static void en32(uint8 *buf, uint32 morp)
{
	buf[0]=morp;
	buf[1]=morp>>8;
	buf[2]=morp>>16;
	buf[3]=morp>>24;
}

static uint32 de32(uint8 *morp)
{
	return(morp[0]|(morp[1]<<8)|(morp[2]<<16)|(morp[3]<<24));
}

static char *CleanNick(char *nick);
static int NickUnique(ClientEntry *client);
static void AddClientToGame(ClientEntry *client, uint8 id[16], uint8 extra[64]) throw(int);
static void SendToAll(GameEntry *game, int cmd, uint8 *data, uint32 len) throw(int);
static void BroadcastText(GameEntry *game, const char *fmt, ...) throw(int);
static void TextToClient(ClientEntry *client, const char *fmt, ...) throw(int);
static void KillClient(ClientEntry *client);

#define NBTCP_LOGINLEN      0x100
#define NBTCP_LOGIN         0x200
#define NBTCP_COMMANDLEN    0x300
#define NBTCP_COMMAND       0x400

#define NBTCP_UPDATEDATA    0x800

static void StartNBTCPReceive(ClientEntry *client, uint32 type, uint32 len)
{
	client->nbtcp = (uint8 *)malloc(len);
	client->nbtcplen = len;
	client->nbtcphas = 0;
	client->nbtcptype = type;
}

static void RedoNBTCPReceive(ClientEntry *client)
{
	client->nbtcphas = 0;
}

static void EndNBTCPReceive(ClientEntry *client)
{
	free(client->nbtcp);
	client->nbtcplen = client->localplayers;
	client->nbtcphas = 0;
	client->nbtcptype = 0;
	client->nbtcp = 0;
}

static uint8 *MakeMPS(ClientEntry *client)
{
	static uint8 buf[64];
	uint8 *bp = buf;
	int x;
	GameEntry *game = (GameEntry *)client->game;

	for(x=0;x<4;x++)
	{
		if(game->Players[x] == client)
		{
			*bp = '0' + x + 1;
			bp++;
			*bp = ',';
			bp++;
		}
	}
	if(*(bp-1) == ',') bp--;

	*bp = 0;
	return(buf);
}

/* Returns 1 if we are back to normal game mode, 0 if more data is yet to arrive. */
static int CheckNBTCPReceive(ClientEntry *client) throw(int)
{
	if(!client->nbtcplen)
		throw(1); /* Should not happen. */

	int l;

	while(l = recv(client->TCPSocket, client->nbtcp + client->nbtcphas, client->nbtcplen  - client->nbtcphas, MSG_NOSIGNAL))
	{
		if(l == -1)
		{
			if(errno == EAGAIN || errno == EWOULDBLOCK)
				break;
			throw(1); /* Die now.  NOW. */
		}
		client->nbtcphas += l;

		//printf("Read: %d, %04x, %d, %d\n",l,client->nbtcptype,client->nbtcphas, client->nbtcplen);

		/* We're all full.  Yippie. */
		if(client->nbtcphas == client->nbtcplen)
		{
			uint32 len;

			switch(client->nbtcptype & 0xF00)
			{
			case NBTCP_UPDATEDATA:
				{
					GameEntry *game = (GameEntry *)client->game;
					int x, wx;
					if(client->nbtcp[0] == 0xFF)
					{
						EndNBTCPReceive(client);
						StartNBTCPReceive(client, NBTCP_COMMANDLEN, 5);
						return(1);
					}
					for(x=0,wx=0; x < 4; x++)
					{
						if(game->Players[x] == client)
						{
							game->joybuf[x] = client->nbtcp[wx];
							wx++;
						}
					}
					RedoNBTCPReceive(client);
				}
				return(1);
			case NBTCP_COMMANDLEN:
				{
					uint8 cmd = client->nbtcp[4];
					len = de32(client->nbtcp);
					if(len > 200000) /* Sanity check. */
						throw(1);

					//printf("%02x, %d\n",cmd,len);
					if(!len && !(cmd&0x80))
					{
						SendToAll((GameEntry*)client->game, client->nbtcp[4], 0, 0);
						EndNBTCPReceive(client);
						StartNBTCPReceive(client,NBTCP_UPDATEDATA,client->localplayers);
					}
					else if(client->nbtcp[4]&0x80)
					{
						EndNBTCPReceive(client);
						if(len)
						{
							StartNBTCPReceive(client,NBTCP_COMMAND | cmd,len);
						}
						else
						{
							/* Woops.  Client probably tried to send a text message of 0 length.
							   Or maybe a 0-length cheat file?  Better be safe! */
							StartNBTCPReceive(client,NBTCP_UPDATEDATA,client->localplayers);
						}
					}
					else throw(1);
					return(1);
				}
			case NBTCP_COMMAND:
				{
					len = client->nbtcplen;
					uint32 tocmd = client->nbtcptype & 0xFF;

					if(tocmd == 0x90) /* Text */
					{
						char *ma, *ma2;

						ma = (char *) malloc(len + 1);
						memcpy(ma, client->nbtcp, len);
						ma[len] = 0;
						asprintf(&ma2, "<%s> %s",client->nickname,ma);
						free(ma);
						ma = ma2;
						len=strlen(ma);
						SendToAll((GameEntry*)client->game, tocmd, (uint8 *)ma, len);
						free(ma);
					}
					else
					{
						SendToAll((GameEntry*)client->game, tocmd, client->nbtcp, len);
					}
					EndNBTCPReceive(client);
					StartNBTCPReceive(client,NBTCP_UPDATEDATA,client->localplayers);
					return(1);
				}
			case NBTCP_LOGINLEN:
				len = de32(client->nbtcp);
				if(len > 1024 || len < (16 + 16 + 64 + 1))
				throw(1);
				EndNBTCPReceive(client);
				StartNBTCPReceive(client,NBTCP_LOGIN,len);
				return(1);
			case NBTCP_LOGIN:
				{
					uint32 len;
					uint8 gameid[16];
					uint8 *sexybuf;
					uint8 extra[64];

					len = client->nbtcplen;
					sexybuf = client->nbtcp;

					/* Game ID(MD5'd game MD5 and password on client side). */
					memcpy(gameid, sexybuf, 16);
					sexybuf += 16;
					len -= 16;

					if(ServerConfig.Password)
					if(memcmp(ServerConfig.Password,sexybuf,16))
					{
						TextToClient(client,"Invalid server password.");
						throw(1);
					}
					sexybuf += 16;
					len -= 16;

					memcpy(extra, sexybuf, 64);
					sexybuf += 64;
					len -= 64;

					client->localplayers = *sexybuf;
					if(client->localplayers < 1 || client->localplayers > 4)
					{
						TextToClient(client,"Invalid number(%d) of local players!",client->localplayers);
						throw(1);
					}
					sexybuf++;
					len -= 1;

					AddClientToGame(client, gameid, extra);
					/* Get the nickname */
					if(len)
					{
						client->nickname = (char *)malloc(len + 1);
						memcpy(client->nickname, sexybuf, len);
						client->nickname[len] = 0;
						if((client->nickname = CleanNick(client->nickname)))
						if(!NickUnique(client)) /* Nickname already exists */
						{
							free(client->nickname);
							client->nickname = 0;
						}
					}
					uint8 *mps = MakeMPS(client);

					if(!client->nickname)
						asprintf(&client->nickname,"*Player %s",mps);

					printf("Client %d assigned to game %d as player %s <%s>\n",client->id,(GameEntry*)client->game - Games,mps, client->nickname);

					int x;
					GameEntry *tg=(GameEntry *)client->game;

					for(x=0; x<tg->MaxPlayers; x++)
					{
						if(tg->Players[x] && tg->IsUnique[x])
						{
							if(tg->Players[x] != client)
							{
								try
								{
									TextToClient(tg->Players[x], "* Player %s has just connected as: %s",MakeMPS(client),client->nickname);
								}
								catch(int i)
								{
									KillClient(tg->Players[x]);
								}
								TextToClient(client, "* Player %s is already connected as: %s",MakeMPS(tg->Players[x]),tg->Players[x]->nickname);
							}
							else
								TextToClient(client, "* You(Player %s) have just connected as: %s",MakeMPS(client),client->nickname);
						}
					}
				}
				EndNBTCPReceive(client);
				StartNBTCPReceive(client,NBTCP_UPDATEDATA,client->localplayers);
				return(1);
			}
		}
	}
	return(0);
}

int ListenSocket;

static char *CleanNick(char *nick)
{
	int x;

	for(x=0; x<strlen(nick); x++)
		if(nick[x] == '<' || nick[x] == '>' || nick[x] == '*' || (nick[x] < 0x20))
			nick[x] = 0;

	if(!strlen(nick))
	{
		free(nick);
		nick = 0;
	}

	return(nick);
}

static int NickUnique(ClientEntry *client)
{
	int x;
	GameEntry *game = (GameEntry *)client-> game;

	for(x=0; x<game->MaxPlayers; x++)
		if(game->Players[x] && client != game->Players[x])

	if(!strcasecmp(client->nickname, game->Players[x]->nickname))
		return(0);

	return(1);
}

static int MakeSendTCP(ClientEntry *client, uint8 *data, uint32 len) throw(int)
{
	if(send(client->TCPSocket, data, len, MSG_NOSIGNAL) != len)
		throw(1);

	return(1);
}

static void SendToAll(GameEntry *game, int cmd, uint8 *data, uint32 len) throw(int)
{
	uint8 poo[5];
	int x;

	poo[4] = cmd;

	for(x=0;x<game->MaxPlayers;x++)
	{
		if(!game->Players[x] || !game->IsUnique[x]) continue;

		try
		{
			if(cmd & 0x80)
				en32(poo, len);
			MakeSendTCP(game->Players[x],poo,5);

			if(cmd & 0x80)
			{
				MakeSendTCP(game->Players[x], data, len);
			}
		}
		catch(int i)
		{
			KillClient(game->Players[x]);
		}
	}
}

static void TextToClient(ClientEntry *client, const char *fmt, ...) throw(int)
{
	char *moo;
	va_list ap;

	va_start(ap,fmt);
	vasprintf(&moo, fmt, ap);
	va_end(ap);


	uint8 poo[5];
	uint32 len;

	poo[4] = 0x90;
	len = strlen(moo);
	en32(poo, len);

	MakeSendTCP(client, poo, 5);
	MakeSendTCP(client, (uint8*)moo, len);
	free(moo);
}

static void BroadcastText(GameEntry *game, const char *fmt, ...) throw(int)
{
	char *moo;
	va_list ap;

	va_start(ap,fmt);
	vasprintf(&moo, fmt, ap);
	va_end(ap);

	SendToAll(game, 0x90,(uint8 *)moo,strlen(moo));
	free(moo);
}

static void KillClient(ClientEntry *client)
{
	GameEntry *game;
	uint8 *mps;
	char *bmsg;

	game = (GameEntry *)client->game;
	if(game)
	{
		int w;
		int tc = 0;

		for(w=0;w<game->MaxPlayers;w++)
			if(game->Players[w]) tc++;

		for(w=0;w<game->MaxPlayers;w++)
			if(game->Players[w])
				if(game->Players[w] == client)
					game->Players[w] = NULL;

		time_t curtime = time(0);
		printf("Player <%s> disconnected from game %d on %s",client->nickname,game-Games,ctime(&curtime));
		asprintf(&bmsg, "* Player %s <%s> left.",MakeMPS(client),client->nickname);
		if(tc == client->localplayers) /* If total players for this game = total local
		                                  players for this client, destroy the game.
		                               */
		{
			printf("Game %d destroyed.\n",game-Games);
			memset(game, 0, sizeof(GameEntry));
			game = 0;
		}
	}
	else
	{
		time_t curtime = time(0);
		printf("Unassigned client %d disconnected on %s",client->id, ctime(&curtime));
	}

	if(client->nbtcp)
		free(client->nbtcp);

	if(client->nickname)
		free(client->nickname);

	if(client->TCPSocket != -1)
		close(client->TCPSocket);

	memset(client, 0, sizeof(ClientEntry));
	client->TCPSocket = -1;

	if(game)
		BroadcastText(game,"%s",bmsg);
}

static void AddClientToGame(ClientEntry *client, uint8 id[16], uint8 extra[64]) throw(int)
{
	int wg;
	GameEntry *game,*fegame;

retry:

	game = NULL;
	fegame = NULL;

	/* First, find an available game. */
	for(wg=0; wg<ServerConfig.MaxClients; wg++)
	{
		if(!Games[wg].MaxPlayers && !fegame)
		{
			if(!fegame)
				fegame=&Games[wg];
		}
		else if(!memcmp(Games[wg].id,id,16)) /* A match was found! */
		{
			game = &Games[wg];
			break;
		}
	}

	if(!game) /* Hmm, no game found.  Guess we'll have to create one. */
	{
		game=fegame;
		printf("Game %d added\n",game-Games);
		memset(game, 0, sizeof(GameEntry));
		game->MaxPlayers = 4;
		memcpy(game->id, id, 16);
		memcpy(game->ExtraInfo, extra, 64);
	}

	int n;
	for(n = 0; n < game->MaxPlayers; n++)
	if(game->Players[n])
	{
		try
		{
			uint8 b[5];
			b[4] = 0x81;
			MakeSendTCP(game->Players[n], b, 5);
			break;
		}
		catch(int i)
		{
			KillClient(game->Players[n]);
			/* All right, now the client we sent the request to is killed.  I HOPE YOU'RE HAPPY! */
			/* Since killing a client can destroy a game, we'll need to see if the game was trashed.
			   If it was, then "goto retry", and try again.  Ugly, yes.  I LIKE UGLY.
			*/
			if(!game->MaxPlayers)
				goto retry;
		}
	}

	int instancecount = client->localplayers;

	for(n=0; n<game->MaxPlayers; n++)
	{
		if(!game->Players[n])
		{
			game->Players[n] = client;
			if(instancecount == client->localplayers)
				game->IsUnique[n] = 1;
			else
				game->IsUnique[n] = 0;
			instancecount --;
			if(!instancecount)
				break;
		}
	}

	/* Game is full. */
	if(n == game->MaxPlayers)
	{
		for(n=0; n<game->MaxPlayers; n++)
			if(game->Players[n] == client)
				game->Players[n] = 0;
		TextToClient(client, "Sorry, game is full.  %d instance(s) tried, %d available.",client->localplayers,client->localplayers - instancecount);
		throw(1);
	}

	client->game = (void *)game;
}




int main(int argc, char *argv[])
{
	struct sockaddr_in sockin;
	socklen_t sockin_len;
	int i;
	char* pass = 0;
	/* If we can't load the default config file, use some defined values */
	if(!LoadConfigFile(DEFAULT_CONFIG))
	{
		ServerConfig.Port = DEFAULT_PORT;
		ServerConfig.MaxClients = DEFAULT_MAX;
		ServerConfig.ConnectTimeout = DEFAULT_TIMEOUT;
		ServerConfig.FrameDivisor = DEFAULT_FRAMEDIVISOR;
	}
	char* configfile = 0;

	for(i=1; i<argc ;i++)
	{
		if(!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h"))
		{
			printf("Usage: %s [OPTION]...\n" ,argv[0]);
			printf("Begins the FCE Ultra game server with given options.\n");
			printf("This server will first look in %s for options.  If that \nfile does not exist, it will use the defaults given here.  Any argument given \ndirectly will override any default values.\n\n", DEFAULT_CONFIG);

			printf("-h\t--help\t\tDisplays this help message.\n");
			printf("-v\t--version\t\tDisplays the version number and quits.\n");
			printf("-p\t--port\t\tStarts server on given port. (default=%d)\n", DEFAULT_PORT);
			printf("-w\t--password\tSpecifies a password for entry.\n");
			printf("-m\t--maxclients\tSpecifies the maximum amount of clients allowed \n\t\t\tto access the server. (default=%d)\n", DEFAULT_MAX);
			printf("-t\t--timeout\tSpecifies the amount of seconds before the server \n\t\t\ttimes out. (default=%d)\n", DEFAULT_TIMEOUT);
			printf("-f\t--framedivisor\tSpecifies frame divisor.\n\t\t\t(eg: 60 / framedivisor = updates per second)(default=%d)\n", DEFAULT_FRAMEDIVISOR);
			printf("-c\t--configfile\tLoads the given configuration file.\n");
			return -1;
		}
		if(!strcmp(argv[i], "--port") || !strcmp(argv[i], "-p"))
		{
			i++;
			if(argc == i)
			{
				printf("Please specify a port to use.\n");
				return -1;
			}
			ServerConfig.Port = atoi(argv[i]);
			continue;
		}
		if(!strcmp(argv[i], "--password") || !strcmp(argv[i], "-w"))
		{
			i++;
			if(argc == i)
			{
				printf("Please specify a password.\n");
				return -1;
			}
			pass = argv[i];
			struct md5_context md5;
			ServerConfig.Password = (uint8 *)malloc(16);
			md5_starts(&md5);
			md5_update(&md5,(uint8*)pass,strlen(pass));
			md5_finish(&md5,ServerConfig.Password);
			puts("Password required to log in.");
			continue;
		}
		if(!strcmp(argv[i], "--maxclients") || !strcmp(argv[i], "-m"))
		{
			i++;
			if(argc == i)
			{
				printf("Please specify the maximium ammount of clients.\n");
				return -1;
			}
			ServerConfig.MaxClients = atoi(argv[i]);
			continue;
		}
		if(!strcmp(argv[i], "--timeout") || !strcmp(argv[i], "-t"))
		{
			i++;
			if(argc == i)
			{
				printf("Please specify the conenction timeout time in seconds.\n");
				return -1;
			}
			ServerConfig.ConnectTimeout = atoi(argv[i]);
			continue;
		}
		if(!strcmp(argv[i], "--framedivisor") || !strcmp(argv[i], "-f"))
		{
			i++;
			if(argc == i)
			{
				printf("Please specify a framedivisor.\n");
				return -1;
			}
			ServerConfig.FrameDivisor = atoi(argv[i]);
			continue;
		}
		if(!strcmp(argv[i], "--configfile") || !strcmp(argv[i], "-c"))
		{
			i++;
			if(argc == i)
			{
				printf("Please specify a configfile to load.\n");
				return -1;
			}
			if(!LoadConfigFile(argv[i]))
			{
				puts("Error loading configuration file.");
				exit(-1);
			}
			continue;
		}
		if(!strcmp(argv[i], "--version") || !strcmp(argv[i], "-v"))
		{
			printf("FCE Ultra network server version %s\n", VERSION);
			return 0;
		}

		/* If we've gotten here, we don't know what the argument is */
		printf("Invalid parameter: %s\n", argv[i]);
		return -1;
	}

	Games = (GameEntry *)malloc(sizeof(GameEntry) * ServerConfig.MaxClients);
	Clients = (ClientEntry *)malloc(sizeof(ClientEntry) * ServerConfig.MaxClients);

	memset(Games,0,sizeof(GameEntry) * ServerConfig.MaxClients);
	memset(Clients,0,sizeof(ClientEntry) * ServerConfig.MaxClients);

	{
		int x;
		for(x=0; x<ServerConfig.MaxClients; x++)
			Clients[x].TCPSocket = -1;
	}
	RefreshThrottleFPS(ServerConfig.FrameDivisor);

	/* First, we need to create a socket to listen on. */
	ListenSocket = socket(AF_INET, SOCK_STREAM, 0);

	/* Set send buffer size to 262,144 bytes. */
	int sndbufsize = 262144;
	if(setsockopt(ListenSocket, SOL_SOCKET, SO_SNDBUF, &sndbufsize, sizeof(int)))
		printf("Send buffer size set failed: %s",strerror(errno));

	int tcpopt = 1;
	if(setsockopt(ListenSocket, SOL_TCP, TCP_NODELAY, &tcpopt, sizeof(int)))
	{
		printf("Nodelay fail: %s",strerror(errno));
		exit(-1);
	}

	memset(&sockin, 0, sizeof(sockin));
	sockin.sin_family = AF_INET;
	sockin.sin_addr.s_addr = INADDR_ANY;
	sockin_len = sizeof(sockin);
	sockin.sin_port = htons(ServerConfig.Port);

	printf("Binding to port %d... ",ServerConfig.Port);
	if(bind(ListenSocket, (struct sockaddr *)&sockin, sockin_len))
	{
		printf("Error: %s\n",strerror(errno));
		exit(-1);
	}
	puts("Ok");
	printf("Listening on socket... ");
	if(listen(ListenSocket, 8))
	{
		printf("Error: %s",strerror(errno));
		exit(-1);
	}
	puts("Ok");

	/* We don't want to block on accept() */
	fcntl(ListenSocket, F_SETFL, fcntl(ListenSocket, F_GETFL) | O_NONBLOCK);

	/* Now for the BIG LOOP. */
	while(1)
	{
		int n;

		for(n=0; n<ServerConfig.MaxClients; n++)
		{
			if(Clients[n].TCPSocket != -1) continue;
			if((Clients[n].TCPSocket = accept(ListenSocket, (struct sockaddr *)&sockin, &sockin_len)) != -1)
			{
				struct sockaddr_in usockin;
				/* We have a new client.  Yippie. */

				fcntl(Clients[n].TCPSocket, F_SETFL, fcntl(Clients[n].TCPSocket, F_GETFL) | O_NONBLOCK);

				Clients[n].timeconnect = time(0);
				Clients[n].id = n;
				printf("Client %d connecting from %s on %s",n,inet_ntoa(sockin.sin_addr),ctime(&Clients[n].timeconnect));
				{
					uint8 buf[1];

					buf[0] = ServerConfig.FrameDivisor;
					send(Clients[n].TCPSocket,buf,1,MSG_NOSIGNAL);
				}
				StartNBTCPReceive(&Clients[n], NBTCP_LOGINLEN, 4);
			}
		}

		/* Check for users still in the login process(not yet assigned a game). BOING */
		time_t curtime = time(0);
		for(n = 0; n < ServerConfig.MaxClients; n++)
		try
		{
			if(Clients[n].TCPSocket != -1 && !Clients[n].game)
			{
				if((Clients[n].timeconnect + ServerConfig.ConnectTimeout) < curtime)
				{
					KillClient(&Clients[n]);
				}
				else
					while(CheckNBTCPReceive(&Clients[n])) {};
			}
		}
		catch(int i)
		{
			KillClient(&Clients[n]);
		}

		int whichgame;
		for(whichgame = 0; whichgame < ServerConfig.MaxClients; whichgame ++)
		{
			/* Now, the loop to get data from each client.  Meep. */
			for(n = 0; n < Games[whichgame].MaxPlayers; n++)
			{
				try
				{
					ClientEntry *client = Games[whichgame].Players[n];
					if(!client || !Games[whichgame].IsUnique[n]) continue;
					int localplayers = client->localplayers;

					while(CheckNBTCPReceive(client)) {};
				} // catch
				catch(int i)
				{
					KillClient(Games[whichgame].Players[n]);
				}
			} // A games clients

			/* Now we send the data to all the clients. */
			for(n = 0; n < Games[whichgame].MaxPlayers; n++)
			{
				if(!Games[whichgame].Players[n] || !Games[whichgame].IsUnique[n]) continue;
				try
				{
					MakeSendTCP(Games[whichgame].Players[n], Games[whichgame].joybuf, 5);
				}
				catch(int i)
				{
					KillClient(Games[whichgame].Players[n]);
				}
			} // A game's clients
		} // Games

		SpeedThrottle();
	} // while(1)
}
