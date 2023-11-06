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

using std::pair;
using std::array;
using std::vector;
using std::min;
using std::max;
using std::ifstream;
using std::ofstream;
using std::string;
using std::atomic;
using std::thread;
using std::deque;

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
		ESP_GET_STATUS,
		DEBUG_GET_LEVEL,
		DEBUG_SET_LEVEL,
		DEBUG_LOG,
		BUFFER_CLEAR_RX_TX,
		BUFFER_DROP_FROM_ESP,
		ESP_GET_FIRMWARE_VERSION,
		ESP_FACTORY_SETTINGS,
		ESP_RESTART,

		// WIFI CMDS
		WIFI_GET_STATUS,
		WIFI_GET_SSID,
		WIFI_GET_IP,
		WIFI_GET_CONFIG,
		WIFI_SET_CONFIG,

		// AP CMDS
		AP_GET_SSID,
		AP_GET_IP,

		// RND CMDS
		RND_GET_BYTE,
		RND_GET_BYTE_RANGE, // ; min / max
		RND_GET_WORD,
		RND_GET_WORD_RANGE, // ; min / max

		// SERVER CMDS
		SERVER_GET_STATUS,
		SERVER_PING,
		SERVER_SET_PROTOCOL,
		SERVER_GET_SETTINGS,
		SERVER_SET_SETTINGS,
		SERVER_GET_SAVED_SETTINGS,
		SERVER_SET_SAVED_SETTINGS,
		SERVER_RESTORE_SAVED_SETTINGS,
		SERVER_CONNECT,
		SERVER_DISCONNECT,
		SERVER_SEND_MSG,

		// NETWORK CMDS
		NETWORK_SCAN,
		NETWORK_GET_DETAILS,
		NETWORK_GET_REGISTERED,
		NETWORK_GET_REGISTERED_DETAILS,
		NETWORK_REGISTER,
		NETWORK_UNREGISTER,
		NETWORK_SET_ACTIVE,

		// FILE CMDS
		FILE_OPEN,
		FILE_CLOSE,
		FILE_STATUS,
		FILE_EXISTS,
		FILE_DELETE,
		FILE_SET_CUR,
		FILE_READ,
		FILE_WRITE,
		FILE_APPEND,
		FILE_COUNT,
		FILE_GET_LIST,
		FILE_GET_FREE_ID,
		FILE_GET_FS_INFO,
		FILE_GET_INFO,
		FILE_DOWNLOAD,
		FILE_FORMAT,
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
		NETWORK_COUNT,
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
		ERROR_WHILE_RESETTING_CONFIG = 1,
		ERROR_WHILE_DELETING_TWEB = 2,
		ERROR_WHILE_DELETING_WEB = 3
	};

	enum class server_protocol_t : uint8_t
	{
		TCP,
		TCP_SECURED,
		UDP,
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
	void sendTcpDataToServer(I begin, I end);
	void receiveDataFromServer();

	void closeConnection();
	void openConnection();

	void pingRequest(uint8_t n);
	void receivePingResult();

	pair<bool, sockaddr> resolve_server_address();
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
