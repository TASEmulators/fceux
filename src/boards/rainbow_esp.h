#pragma once
#include "../types.h"
#include "RNBW/bootrom_chr.h"

#define CURL_STATICLIB
#include "curl/curl.h"

#include <assert.h>
#include <array>
#include <atomic>
#include <deque>
#include <thread>
#include <fstream>

using std::array;
using std::atomic;
using std::deque;
using std::ifstream;
using std::max;
using std::min;
using std::ofstream;
using std::pair;
using std::string;
using std::thread;
using std::vector;

#ifdef _WIN32
#include <winsock2.h>
#else
#include <netinet/in.h>
#endif

//////////////////////////////////////
// BrokeStudio's ESP firmware implementation

static const uint8_t NO_WORKING_FILE = 0xff;
static const uint8_t NUM_FILE_PATHS = 3;
static const uint8_t NUM_FILES = 64;

static const uint64_t ESP_FLASH_SIZE = 0x200000; // 2MiB
static const uint64_t SD_CARD_SIZE = 0x80000000; // 2GiB

static const uint8_t NUM_NETWORKS = 3;
static const uint8_t NUM_FAKE_NETWORKS = 5;
static const uint8_t SSID_MAX_LENGTH = 32;
static const uint8_t PASS_MAX_LENGTH = 64;

static const uint8_t DBG_CFG_OFF = 0x00;
static const uint8_t DBG_CFG_DEV_LOG = 0x01;
static const uint8_t DBG_CFG_SERIAL = 0x02;
static const uint8_t DBG_CFG_NETWORK = 0x04;

struct NetworkInfo
{
	string ssid;
	string pass;
	bool active;
};

struct FileConfig
{
	uint8_t access_mode;
	uint8_t drive;
};

struct FileStruct
{
	uint8_t drive;
	string filename;
	vector<uint8_t> data;
};

struct WorkingFile
{
	bool active;
	uint8_t config;
	uint8_t auto_path;
	uint8_t auto_file;
	uint32_t offset;
	FileStruct *file;
};

struct UPDpoolAddress
{
	string ipAddress;
	uint16_t port;
};

class BrokeStudioFirmware
{
public:
	BrokeStudioFirmware();
	~BrokeStudioFirmware();

	void rx(uint8_t v);
	uint8_t tx();

	bool getDataReadyIO();

private:
	// Defined message types from CPU to ESP
	enum class toesp_cmds_t : uint8_t
	{
		// ESP CMDS
		ESP_GET_STATUS = 0,           // 0x00
		DEBUG_GET_LEVEL = 1,          // 0x01
		DEBUG_SET_LEVEL = 2,          // 0x02
		DEBUG_LOG = 3,                // 0x03
		BUFFER_CLEAR_RX_TX = 4,       // 0x04
		BUFFER_DROP_FROM_ESP = 5,     // 0x05
		ESP_GET_FIRMWARE_VERSION = 6, // 0x06
		ESP_FACTORY_RESET = 7,        // 0x07
		ESP_RESTART = 8,              // 0x08

		// WIFI CMDS
		WIFI_GET_STATUS = 9,  // 0x09
		WIFI_GET_SSID = 10,   // 0x0A
		WIFI_GET_IP = 11,     // 0x0B
		WIFI_GET_CONFIG = 12, // 0x0C
		WIFI_SET_CONFIG = 13, // 0x0D

		// ACCESS POINT CMDS
		AP_GET_SSID = 14, // 0x0E
		AP_GET_IP = 15,   // 0x0F

		// RND CMDS
		RND_GET_BYTE = 16,        // 0x10
		RND_GET_BYTE_RANGE = 17,  // 0x11
		RND_GET_WORD = 18,        // 0x12
		RND_GET_WORD_RANGE = 19,  // 0x13

		// SERVER CMDS
		SERVER_GET_STATUS = 20,             // 0x14
		SERVER_PING = 21,                   // 0x15
		SERVER_SET_PROTOCOL = 22,           // 0x16
		SERVER_GET_SETTINGS = 23,           // 0x17
		SERVER_SET_SETTINGS = 24,           // 0x18
		SERVER_GET_SAVED_SETTINGS = 25,     // 0x19
		SERVER_SET_SAVED_SETTINGS = 26,     // 0x1A
		SERVER_RESTORE_SAVED_SETTINGS = 27, // 0x1B
		SERVER_CONNECT = 28,                // 0x1C
		SERVER_DISCONNECT = 29,             // 0x1D
		SERVER_SEND_MSG = 30,               // 0x1E

		// UDP ADDRESS POOL CMDS
		UDP_ADDR_POOL_CLEAR = 55,     // 0x37
		UDP_ADDR_POOL_ADD = 56,       // 0x38
		UDP_ADDR_POOL_REMOVE = 57,    // 0x39
		UDP_ADDR_POOL_SEND_MSG = 58,  // 0x3A

		// NETWORK CMDS
		NETWORK_SCAN = 31,                   // 0x1F
		NETWORK_GET_SCAN_RESULT = 32,        // 0x20
		NETWORK_GET_SCANNED_DETAILS = 33,    // 0x21
		NETWORK_GET_REGISTERED = 34,         // 0x22
		NETWORK_GET_REGISTERED_DETAILS = 35, // 0x23
		NETWORK_REGISTER = 36,               // 0x24
		NETWORK_UNREGISTER = 37,             // 0x25
		NETWORK_SET_ACTIVE = 38,             // 0x26

		// FILE CMDS
		FILE_OPEN = 39,         // 0x27
		FILE_CLOSE = 40,        // 0x28
		FILE_STATUS = 41,       // 0x29
		FILE_EXISTS = 42,       // 0x2A
		FILE_DELETE = 43,       // 0x2B
		FILE_SET_CUR = 44,      // 0x2C
		FILE_READ = 45,         // 0x2D
		FILE_WRITE = 46,        // 0x2E
		FILE_APPEND = 47,       // 0x2F
		FILE_COUNT = 48,        // 0x30
		FILE_GET_LIST = 49,     // 0x31
		FILE_GET_FREE_ID = 50,  // 0x32
		FILE_GET_FS_INFO = 51,  // 0x33
		FILE_GET_INFO = 52,     // 0x34
		FILE_DOWNLOAD = 53,     // 0x35
		FILE_FORMAT = 54,       // 0x36
	};

	// Defined message types from ESP to CPU
	enum class fromesp_cmds_t : uint8_t
	{
		// ESP CMDS
		READY,
		DEBUG_LEVEL,
		ESP_FIRMWARE_VERSION,
		ESP_FACTORY_RESET,

		// WIFI / AP CMDS
		WIFI_STATUS,
		SSID,
		IP_ADDRESS,
		WIFI_CONFIG,

		// RND CMDS
		RND_BYTE,
		RND_WORD,

		// SERVER CMDS
		SERVER_STATUS,
		SERVER_PING,
		SERVER_SETTINGS,
		MESSAGE_FROM_SERVER,

		// NETWORK CMDS
		NETWORK_SCAN_RESULT,
		NETWORK_SCANNED_DETAILS,
		NETWORK_REGISTERED_DETAILS,
		NETWORK_REGISTERED,

		// FILE CMDS
		FILE_STATUS,
		FILE_EXISTS,
		FILE_DELETE,
		FILE_LIST,
		FILE_DATA,
		FILE_COUNT,
		FILE_ID,
		FILE_FS_INFO,
		FILE_INFO,
		FILE_DOWNLOAD,
	};

	// ESP factory reset result codes
	enum class esp_factory_reset : uint8_t
	{
		SUCCESS = 0,
		ERROR_WHILE_SAVING_CONFIG = 1,
		ERROR_WHILE_DELETING_TWEB = 2,
		ERROR_WHILE_DELETING_WEB = 3
	};

	enum class server_protocol_t : uint8_t
	{
		TCP = 0,
		TCP_SECURED = 1,
		UDP = 2,
		UDP_POOL = 3,
	};

	// WIFI_CONFIG
	enum class wifi_config_t : uint8_t
	{
		WIFI_ENABLE = 1,
		AP_ENABLE = 2,
		WEB_SERVER_ENABLE = 4
	};

	// FILE_CONFIG
	enum class file_config_flags_t : uint8_t
	{
		ACCESS_MODE_MASK = 1,
		ACCESS_MODE_AUTO = 0,
		ACCESS_MODE_MANUAL = 1,
		DESTINATION_MASK = 2,
		DESTINATION_ESP = 0,
		DESTINATION_SD = 2,
	};

	enum class file_delete_results_t : uint8_t
	{
		SUCCESS,
		ERROR_WHILE_DELETING_FILE,
		FILE_NOT_FOUND,
		INVALID_PATH_OR_FILE,
	};

	enum class file_download_results_t : uint8_t
	{
		SUCCESS,
		INVALID_DESTINATION,
		ERROR_WHILE_DELETING_FILE,
		UNKNOWN_OR_UNSUPPORTED_PROTOCOL,
		NETWORK_ERROR,
		HTTP_STATUS_NOT_IN_2XX,
	};

	enum class file_download_network_error_t : uint8_t
	{
		CONNECTION_FAILED = 255,
		SEND_HEADER_FAILED = 254,
		SEND_PAYLOAD_FILED = 253,
		NOT_CONNECTED = 252,
		CONNECTION_LOST = 251,
		NO_STREAM = 250,
		NO_HTTP_SERVER = 249,
		OUT_OF_RAM = 248,
		ENCODING = 247,
		STREAM_WRITE = 246,
		READ_TIMEOUT = 245,
	};

	void processBufferedMessage();
	FileConfig parseFileConfig(uint8_t config);
	int findFile(uint8_t drive, string filename);
	int findPath(uint8_t drive, string path);
	string getAutoFilename(uint8_t path, uint8_t file);
	void readFile(uint8_t n);
	template <class I>
	void writeFile(I data_begin, I data_end);
	template <class I>
	void appendFile(I data_begin, I data_end);
	void saveFiles();
	void saveFile(uint8_t drive, char const* filename);
	void loadFiles();
	void loadFile(uint8_t drive, char const* filename);
	void clearFiles(uint8_t drive);

	template <class I>
	void sendUdpDatagramToServer(I begin, I end);
	template <class I>
	void sendUdpDatagramToPool(I begin, I end);
	template <class I>
	void sendTcpDataToServer(I begin, I end);
	void receiveDataFromServer();

	void closeConnection();
	void openConnection();

	void pingRequest(uint8_t n);
	void receivePingResult();

	pair<bool, sockaddr> resolve_address(string address, uint16_t port);
	static deque<uint8_t> read_socket(int socket);

	void initDownload();
	static pair<uint8_t, uint8_t> curle_to_net_error(CURLcode curle);
	void downloadFile(string const &url, uint8_t path, uint8_t file);
	void cleanupDownload();

private:
	deque<uint8_t> rx_buffer;
	deque<uint8_t> tx_buffer;
	deque<deque<uint8_t>> tx_messages;

	bool isEspFlashFilePresent = false;
	bool isSdCardFilePresent = false;
	WorkingFile working_file;
	vector<FileStruct> files;

	array<NetworkInfo, NUM_NETWORKS> networks;

	server_protocol_t active_protocol = server_protocol_t::TCP;
	string default_server_settings_address;
	uint16_t default_server_settings_port = 0;
	string server_settings_address;
	uint16_t server_settings_port = 0;

	UPDpoolAddress ipAddressPool[16];

	uint8_t debug_config = 0;
	uint8_t wifi_config = 1;

	uint8_t ping_min = 0;
	uint8_t ping_avg = 0;
	uint8_t ping_max = 0;
	uint8_t ping_lost = 0;
	atomic<bool> ping_ready;
	thread ping_thread;

	int udp_socket = -1;
	sockaddr server_addr;

	int tcp_socket = -1;

	bool msg_first_byte = true;
	uint8_t msg_length = 0;
	uint8_t last_byte_read = 0;

	CURL* curl_handle = nullptr;
};
