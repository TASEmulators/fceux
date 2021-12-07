#include "rainbow_esp.h"

#include "../fceu.h"
#include "../utils/crc32.h"

#include "pping.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <map>
#include <regex>
#include <sstream>
#include <stdexcept>

#if defined(_WIN32) || defined(WIN32)

//Note: do not include UDP networking, mongoose.h should have done it correctly taking care of defining portability macros

// Compatibility hacks
typedef SSIZE_T ssize_t;
#define bzero(b,len) (memset((b), '\0', (len)), (void) 0)
#define cast_network_payload(x) reinterpret_cast<char*>(x)
#define close_sock(x) ::closesocket(x)

#else

// UDP networking
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/udp.h>
#include <unistd.h>

// Compatibility hacks
#define cast_network_payload(x) reinterpret_cast<void*>(x)
#define close_sock(x) ::close(x)

#endif

using easywsclient::WebSocket;

#define RAINBOW_DEBUG 0

#if RAINBOW_DEBUG >= 1
#define UDBG(...) FCEU_printf(__VA_ARGS__)
#else
#define UDBG(...)
#endif

#if RAINBOW_DEBUG >= 2
#define UDBG_FLOOD(...) FCEU_printf(__VA_ARGS__)
#else
#define UDBG_FLOOD(...)
#endif

#if RAINBOW_DEBUG >= 1
#include "../debug.h"
namespace {
uint64_t wall_clock_milli() {
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
};
}
#endif

namespace {

std::vector<uint8> readHostFile(std::string const& file_path) {
	// Open file
	std::ifstream ifs(file_path, std::ifstream::binary);
	if (!ifs) {
		throw std::runtime_error("unable to open file");
	}

	// Get file length
	ifs.seekg(0, ifs.end);
	size_t const file_length = ifs.tellg();
	if (ifs.fail()) {
		throw std::runtime_error("unable to get file length");
	}
	ifs.seekg(0, ifs.beg);

	// Read file
	std::vector<uint8> data(file_length);
	ifs.read(reinterpret_cast<char*>(data.data()), file_length);
	if (ifs.fail()) {
		throw std::runtime_error("error while reading file");
	}

	return data;
}

std::array<std::string, NUM_FILE_PATHS> dir_names = { "SAVE", "ROMS", "USER" };

}

BrokeStudioFirmware::BrokeStudioFirmware() {
	UDBG("RAINBOW BrokeStudioFirmware ctor\n");

	// Get default host/port
	char const* hostname = ::getenv("RAINBOW_SERVER_ADDR");
	if (hostname == nullptr) hostname = "";
	this->server_settings_address = hostname;
	this->default_server_settings_address = hostname;

	char const* port_cstr = ::getenv("RAINBOW_SERVER_PORT");
	if (port_cstr == nullptr) port_cstr = "0";
	std::istringstream port_iss(port_cstr);
	port_iss >> this->server_settings_port;
	this->default_server_settings_port = this->server_settings_port;

	// Start web server
	this->httpd_run = true;
	char const* httpd_port = ::getenv("RAINBOW_WWW_PORT");
	if (httpd_port == nullptr) httpd_port = "8080";
	mg_mgr_init(&this->mgr, reinterpret_cast<void*>(this));
	UDBG("Starting web server on port %s\n", httpd_port);
	this->nc = mg_bind(&this->mgr, httpd_port, BrokeStudioFirmware::httpdEvent);
	if (this->nc == NULL) {
		printf("Failed to create web server\n");
	}else {
		mg_set_protocol_http_websocket(this->nc);
		this->httpd_thread = std::thread([this] {
			while (this->httpd_run) {
				mg_mgr_poll(&this->mgr, 1000);
			}
			mg_mgr_free(&this->mgr);
		});
	}

	// Clear file list
	for (uint8 p = 0; p < NUM_FILE_PATHS; p++)
	{
		for (uint8 f = 0; f < NUM_FILES; f++)
		{
			this->file_exists[p][f] = false;
		}
	}

	// Init fake registered networks
	this->networks = { {
		{"FCEUX_SSID", "FCEUX_PASS"},
		{"", ""},
		{"", ""},
	} };

	// Load file list from save file (if any)
	this->loadFiles();

	// Mark ping result as useless
	this->ping_ready = false;

	// Initialize download system
	this->initDownload();
}

BrokeStudioFirmware::~BrokeStudioFirmware() {
	UDBG("RAINBOW BrokeStudioFirmware dtor\n");

	this->closeConnection();
	if (this->socket_close_thread.joinable()) {
		this->socket_close_thread.join();
	}

	if (this->socket != nullptr) {
		delete this->socket;
		this->socket = nullptr;
	}

	this->httpd_run = false;
	if (this->httpd_thread.joinable()) {
		this->httpd_thread.join();
	}

	this->cleanupDownload();
}

void BrokeStudioFirmware::rx(uint8 v) {
	UDBG_FLOOD("RAINBOW BrokeStudioFirmware rx %02x\n", v);
	if (this->msg_first_byte) {
		this->msg_first_byte = false;
		this->msg_length = v + 1;
	}
	this->rx_buffer.push_back(v);

	if (this->rx_buffer.size() == this->msg_length) {
		this->processBufferedMessage();
		this->msg_first_byte = true;
	}
}

uint8 BrokeStudioFirmware::tx() {
	// Refresh buffer from network
	this->receiveDataFromServer();
	this->receivePingResult();

	// Fill buffer with the next message (if needed)
	if (this->tx_buffer.empty() && !this->tx_messages.empty()) {
		std::deque<uint8> message = this->tx_messages.front();
		// add last received byte first to match hardware behavior
		// it's more of a mapper thing though ...
		// needs a dummy $5000 read when reading data from buffer
		this->tx_buffer.push_back(last_byte_read);
		this->tx_buffer.insert(this->tx_buffer.end(), message.begin(), message.end());
		this->tx_messages.pop_front();
	}

	// Get byte from buffer
	if (!this->tx_buffer.empty()) {
		last_byte_read = this->tx_buffer.front();
		this->tx_buffer.pop_front();
	}

	UDBG_FLOOD("RAINBOW BrokeStudioFirmware tx %02x\n", last_byte_read);
	return last_byte_read;
}

void BrokeStudioFirmware::setGpio4(bool /*v*/) {
}

bool BrokeStudioFirmware::getGpio4() {
	this->receiveDataFromServer();
	this->receivePingResult();
	return !(this->tx_buffer.empty() && this->tx_messages.empty());
}

void BrokeStudioFirmware::processBufferedMessage() {
	assert(this->rx_buffer.size() >= 2); // Buffer must contain exactly one message, minimal message is two bytes (length + type)
	uint8 const message_size = this->rx_buffer.front();
	assert(message_size >= 1); // minimal payload is one byte (type)
	assert(this->rx_buffer.size() == static_cast<std::deque<uint8>::size_type>(message_size) + 1); // Buffer size must match declared payload size

	// Process the message in RX buffer
	switch (static_cast<toesp_cmds_t>(this->rx_buffer.at(1))) {
		
		// ESP CMDS

		case toesp_cmds_t::ESP_GET_STATUS:
			UDBG("RAINBOW BrokeStudioFirmware received message ESP_GET_STATUS\n");
			this->tx_messages.push_back({1, static_cast<uint8>(fromesp_cmds_t::READY)});
			break;
		case toesp_cmds_t::DEBUG_GET_LEVEL:
			UDBG("RAINBOW BrokeStudioFirmware received message DEBUG_GET_LEVEL\n");
			this->tx_messages.push_back({
				2,
				static_cast<uint8>(fromesp_cmds_t::DEBUG_LEVEL),
				static_cast<uint8>(this->debug_config)
			});
			break;
		case toesp_cmds_t::DEBUG_SET_LEVEL:
			UDBG("RAINBOW BrokeStudioFirmware received message DEBUG_SET_LEVEL\n");
			if (message_size == 2) {
				this->debug_config = this->rx_buffer.at(2);
			}
			FCEU_printf("DEBUG LEVEL SET TO: %u\n", this->debug_config);
			break;
		case toesp_cmds_t::DEBUG_LOG:
			UDBG("RAINBOW DEBUG/LOG\n");
			if (RAINBOW_DEBUG > 0 || (this->debug_config & 1)) {
				for (std::deque<uint8>::const_iterator cur = this->rx_buffer.begin() + 2; cur < this->rx_buffer.end(); ++cur) {
					FCEU_printf("%02x ", *cur);
				}
				FCEU_printf("\n");
			}
			break;
		case toesp_cmds_t::BUFFER_CLEAR_RX_TX:
			UDBG("RAINBOW BrokeStudioFirmware received message BUFFER_CLEAR_RX_TX\n");
			this->receiveDataFromServer();
			this->receivePingResult();
			this->tx_buffer.clear();
			this->tx_messages.clear();
			this->rx_buffer.clear();
			break;
		case toesp_cmds_t::BUFFER_DROP_FROM_ESP:
			UDBG("RAINBOW BrokeStudioFirmware received message BUFFER_DROP_FROM_ESP\n");
			if (message_size == 3) {
				uint8 const message_type = this->rx_buffer.at(2);
				uint8 const n_keep = this->rx_buffer.at(3);

				size_t i = 0;
				for (
					std::deque<std::deque<uint8>>::iterator message = this->tx_messages.end();
					message != this->tx_messages.begin();
				)
				{
					--message;
					if (message->at(1) == message_type) {
						++i;
						if (i > n_keep) {
							UDBG("RAINBOW BrokeStudioFirmware erase message: index=%d\n", message - this->tx_messages.begin());
							message = this->tx_messages.erase(message);
						}else {
							UDBG("RAINBOW BrokeStudioFirmware keep message: index=%d - too recent\n", message - this->tx_messages.begin());
						}
					}else {
						UDBG("RAINBOW BrokeStudioFirmware keep message: index=%d - bad type\n", message - this->tx_messages.begin());
					}
				}
			}
			break;
		case toesp_cmds_t::ESP_GET_FIRMWARE_VERSION:
			UDBG("RAINBOW BrokeStudioFirmware received message ESP_GET_FIRMWARE_VERSION\n");
			this->tx_messages.push_back({ 7, static_cast<uint8>(fromesp_cmds_t::ESP_FIRMWARE_VERSION), 5, 'F', 'C', 'E', 'U', 'X' });
			break;


		// WIFI CMDS
		case toesp_cmds_t::WIFI_GET_STATUS:
			UDBG("RAINBOW BrokeStudioFirmware received message WIFI_GET_STATUS\n");
			this->tx_messages.push_back({ 2, static_cast<uint8>(fromesp_cmds_t::WIFI_STATUS), 3 }); // Simple answer, wifi is ok
			break;
		case toesp_cmds_t::WIFI_GET_SSID:
			UDBG("RAINBOW BrokeStudioFirmware received message WIFI_GET_SSID\n");
			this->tx_messages.push_back({ 12, static_cast<uint8>(fromesp_cmds_t::SSID), 10, 'F', 'C', 'E', 'U', 'X', '_', 'S', 'S', 'I', 'D' });
			break;
		case toesp_cmds_t::WIFI_GET_IP:
			UDBG("RAINBOW BrokeStudioFirmware received message WIFI_GET_ID\n");
			this->tx_messages.push_back({ 14, static_cast<uint8>(fromesp_cmds_t::IP_ADDRESS), 12, '1', '9', '2', '.', '1', '6', '8', '.', '1', '.', '1', '0' });
			break;

		// AP CMDS
		case toesp_cmds_t::AP_GET_SSID:
			UDBG("RAINBOW BrokeStudioFirmware received message AP_GET_SSID\n");
			this->tx_messages.push_back({ 12, static_cast<uint8>(fromesp_cmds_t::SSID), 10, 'F', 'C', 'E', 'U', 'X', '_', 'S', 'S', 'I', 'D' });
			break;
		case toesp_cmds_t::AP_GET_IP:
			UDBG("RAINBOW BrokeStudioFirmware received message AP_GET_ID\n");
			this->tx_messages.push_back({ 16, static_cast<uint8>(fromesp_cmds_t::IP_ADDRESS), 14, '1', '2', '7', '.', '0', '.', '0', '.', '1', ':', '8', '0', '8', '0' });
			break;

		// RND CMDS

		case toesp_cmds_t::RND_GET_BYTE:
			UDBG("RAINBOW BrokeStudioFirmware received message RND_GET_BYTE\n");
			this->tx_messages.push_back({
				2,
				static_cast<uint8>(fromesp_cmds_t::RND_BYTE),
				static_cast<uint8>(rand() % 256)
			});
			break;
		case toesp_cmds_t::RND_GET_BYTE_RANGE: {
			UDBG("RAINBOW BrokeStudioFirmware received message RND_GET_BYTE_RANGE\n");
			if (message_size < 3) {
				break;
			}
			int const min_value = this->rx_buffer.at(2);
			int const max_value = this->rx_buffer.at(3);
			int const range = max_value - min_value;
			this->tx_messages.push_back({
				2,
				static_cast<uint8>(fromesp_cmds_t::RND_BYTE),
				static_cast<uint8>(min_value + (rand() % range))
			});
			break;
		}
		case toesp_cmds_t::RND_GET_WORD:
			UDBG("RAINBOW BrokeStudioFirmware received message RND_GET_WORD\n");
			this->tx_messages.push_back({
				3,
				static_cast<uint8>(fromesp_cmds_t::RND_WORD),
				static_cast<uint8>(rand() % 256),
				static_cast<uint8>(rand() % 256)
			});
			break;
		case toesp_cmds_t::RND_GET_WORD_RANGE: {
			UDBG("RAINBOW BrokeStudioFirmware received message RND_GET_WORD_RANGE\n");
			if (message_size < 5) {
				break;
			}
			int const min_value = (static_cast<int>(this->rx_buffer.at(2)) << 8) + this->rx_buffer.at(3);
			int const max_value = (static_cast<int>(this->rx_buffer.at(4)) << 8) + this->rx_buffer.at(5);
			int const range = max_value - min_value;
			int const rand_value = min_value + (rand() % range);
			this->tx_messages.push_back({
				3,
				static_cast<uint8>(fromesp_cmds_t::RND_WORD),
				static_cast<uint8>(rand_value >> 8),
				static_cast<uint8>(rand_value & 0xff)
			});
			break;
		}

		// SERVER CMDS

		case toesp_cmds_t::SERVER_GET_STATUS: {
			UDBG("RAINBOW BrokeStudioFirmware received message SERVER_GET_STATUS\n");
			uint8 status;
			switch (this->active_protocol) {
			case server_protocol_t::WEBSOCKET:
			case server_protocol_t::WEBSOCKET_SECURED:
				status = (this->socket != nullptr); // Server connection is ok if we succeed to open it
				break;
			case server_protocol_t::TCP:
				//TODO actually check connection state
				status = (this->tcp_socket != -1); // Considere server connection ok if we created a socket
				break;
			case server_protocol_t::TCP_SECURED:
				//TODO
				status = 0;
				break;
			case server_protocol_t::UDP:
				status = (this->udp_socket != -1); // Considere server connection ok if we created a socket
				break;
			default:
				status = 0; // Unknown active protocol, connection certainly broken
			}

			this->tx_messages.push_back({
				2,
				static_cast<uint8>(fromesp_cmds_t::SERVER_STATUS),
				status
			});
			break;
		}
		case toesp_cmds_t::SERVER_PING:
			UDBG("RAINBOW BrokeStudioFirmware received message SERVER_PING\n");
			if (!this->ping_thread.joinable()) {
				if (this->server_settings_address.empty()) {
					this->tx_messages.push_back({
						1,
						static_cast<uint8>(fromesp_cmds_t::SERVER_PING)
					});
				}else if (message_size >= 1) {
					assert(!this->ping_thread.joinable());
					this->ping_ready = false;
					uint8 n = (message_size == 1 ? 0 : this->rx_buffer.at(2));
					if (n == 0) {
						n = 4;
					}
					this->ping_thread = std::thread(&BrokeStudioFirmware::pingRequest, this, n);
				}
			}
			break;
		case toesp_cmds_t::SERVER_SET_PROTOCOL: {
			UDBG("RAINBOW BrokeStudioFirmware received message SERVER_SET_PROTOCOL\n");
			if (message_size == 2) {
				server_protocol_t const requested_protocol = static_cast<server_protocol_t>(this->rx_buffer.at(2));
				if (requested_protocol > server_protocol_t::UDP) {
					UDBG("RAINBOW BrokeStudioFirmware SET_SERVER_PROTOCOL: unknown protocol (%d)\n", requested_protocol);
				}else {
					this->active_protocol = requested_protocol;
				}
			}
			break;
		}
		case toesp_cmds_t::SERVER_GET_SETTINGS: {
			UDBG("RAINBOW BrokeStudioFirmware received message SERVER_GET_SETTINGS\n");
			if (this->server_settings_address.empty() && this->server_settings_port == 0) {
				this->tx_messages.push_back({
					1,
					static_cast<uint8>(fromesp_cmds_t::SERVER_SETTINGS)
				});
			}else {
				std::deque<uint8> message({
					static_cast<uint8>(1 + 2 + this->server_settings_address.size()),
					static_cast<uint8>(fromesp_cmds_t::SERVER_SETTINGS),
					static_cast<uint8>(this->server_settings_port >> 8),
					static_cast<uint8>(this->server_settings_port & 0xff)
				});
				message.insert(message.end(), this->server_settings_address.begin(), this->server_settings_address.end());
				this->tx_messages.push_back(message);
			}
			break;
		}
		case toesp_cmds_t::SERVER_GET_CONFIG_SETTINGS: {
			UDBG("RAINBOW BrokeStudioFirmware received message SERVER_GET_CONFIG_SETTINGS\n");
			if (this->default_server_settings_address.empty() && this->default_server_settings_port == 0) {
				this->tx_messages.push_back({
					1,
					static_cast<uint8>(fromesp_cmds_t::SERVER_SETTINGS)
				});
			}else {
				std::deque<uint8> message({
					static_cast<uint8>(1 + 2 + this->default_server_settings_address.size()),
					static_cast<uint8>(fromesp_cmds_t::SERVER_SETTINGS),
					static_cast<uint8>(this->default_server_settings_port >> 8),
					static_cast<uint8>(this->default_server_settings_port & 0xff)
				});
				message.insert(message.end(), this->default_server_settings_address.begin(), this->default_server_settings_address.end());
				this->tx_messages.push_back(message);
			}
			break;
		}
		case toesp_cmds_t::SERVER_SET_SETTINGS:
			UDBG("RAINBOW BrokeStudioFirmware received message SERVER_SET_SETTINGS\n");
			if (message_size >= 3) {
				this->server_settings_port =
					(static_cast<uint16_t>(this->rx_buffer.at(2)) << 8) +
					(static_cast<uint16_t>(this->rx_buffer.at(3)))
				;
				this->server_settings_address = std::string(this->rx_buffer.begin()+4, this->rx_buffer.end());
			}
			break;
		case toesp_cmds_t::SERVER_RESTORE_SETTINGS:
			UDBG("RAINBOW BrokeStudioFirmware received message SERVER_RESTORE_SETTINGS\n");
			this->server_settings_address = this->default_server_settings_address;
			this->server_settings_port = this->default_server_settings_port;
			break;
		case toesp_cmds_t::SERVER_CONNECT:
			UDBG("RAINBOW BrokeStudioFirmware received message SERVER_CONNECT\n");
			this->openConnection();
			break;
		case toesp_cmds_t::SERVER_DISCONNECT:
			UDBG("RAINBOW BrokeStudioFirmware received message SERVER_DISCONNECT\n");
			this->closeConnection();
			break;
		case toesp_cmds_t::SERVER_SEND_MSG: {
			UDBG("RAINBOW BrokeStudioFirmware received message SERVER_SEND_MSG\n");
			uint8 const payload_size = this->rx_buffer.size() - 2;
			std::deque<uint8>::const_iterator payload_begin = this->rx_buffer.begin() + 2;
			std::deque<uint8>::const_iterator payload_end = payload_begin + payload_size;

			switch (this->active_protocol) {
			case server_protocol_t::WEBSOCKET:
			case server_protocol_t::WEBSOCKET_SECURED:
				this->sendMessageToServer(payload_begin, payload_end);
				break;
			case server_protocol_t::TCP:
				this->sendTcpDataToServer(payload_begin, payload_end);
				break;
			case server_protocol_t::TCP_SECURED:
				//TODO
				break;
			case server_protocol_t::UDP:
				this->sendUdpDatagramToServer(payload_begin, payload_end);
				break;
			default:
				UDBG("RAINBOW BrokeStudioFirmware active protocol (%d) not implemented\n", this->active_protocol);
			};
			break;
		}

		// NETWORK CMDS
		// network commands are not relevant here, so we'll just use test/fake data
		case toesp_cmds_t::NETWORK_SCAN:
			UDBG("RAINBOW BrokeStudioFirmware received message NETWORK_SCAN\n");
			if (message_size == 1) {
				this->tx_messages.push_back({
					2,
					static_cast<uint8>(fromesp_cmds_t::NETWORK_COUNT),
					NUM_FAKE_NETWORKS
				});
			}
			break;
		case toesp_cmds_t::NETWORK_GET_DETAILS:
			UDBG("RAINBOW BrokeStudioFirmware received message NETWORK_GET_DETAILS\n");
			if (message_size == 2) {
				uint8 networkItem = this->rx_buffer.at(2);
				if (networkItem > NUM_FAKE_NETWORKS-1) networkItem = NUM_FAKE_NETWORKS-1;
				this->tx_messages.push_back({
					21,
					static_cast<uint8>(fromesp_cmds_t::NETWORK_SCANNED_DETAILS),
					4, // encryption type
					0x47, // RSSI
					0x00,0x00,0x00,0x01, // channel
					0, // hidden?
					12, // SSID length
					'F','C','E','U','X','_','S','S','I','D','_',static_cast<uint8>(networkItem + '0') // SSID
				});
			}
			break;
		case toesp_cmds_t::NETWORK_GET_REGISTERED:
			UDBG("RAINBOW BrokeStudioFirmware received message NETWORK_GET_REGISTERED\n");
			if (message_size == 1) {
				this->tx_messages.push_back({
					NUM_NETWORKS+1,
					static_cast<uint8>(fromesp_cmds_t::NETWORK_REGISTERED),
					static_cast<uint8>((this->networks[0].ssid != "") ? 1 : 0),
					static_cast<uint8>((this->networks[1].ssid != "") ? 1 : 0),
					static_cast<uint8>((this->networks[2].ssid != "") ? 1 : 0)
				});
			}
			break;
		case toesp_cmds_t::NETWORK_GET_REGISTERED_DETAILS:
			UDBG("RAINBOW BrokeStudioFirmware received message NETWORK_GET_REGISTERED_DETAILS\n");
			if (message_size == 2) {
				uint8 networkItem = this->rx_buffer.at(2);
				if (networkItem > NUM_NETWORKS - 1) networkItem = NUM_NETWORKS - 1;
				std::deque<uint8> message({
					static_cast<uint8>(2 + this->networks[networkItem].ssid.length() + 1 + this->networks[networkItem].pass.length()),
					static_cast<uint8>(fromesp_cmds_t::NETWORK_REGISTERED_DETAILS),
					static_cast<uint8>(this->networks[networkItem].ssid.length())
				});
				message.insert(message.end(), this->networks[networkItem].ssid.begin(), this->networks[networkItem].ssid.end());
				message.insert(message.end(), this->networks[networkItem].pass.length());
				message.insert(message.end(), this->networks[networkItem].pass.begin(), this->networks[networkItem].pass.end());
				this->tx_messages.push_back(message);
			}
			break;
		case toesp_cmds_t::NETWORK_REGISTER:
			UDBG("RAINBOW BrokeStudioFirmware received message NETWORK_REGISTER\n");
			if (message_size > 6) {
				uint8 const networkItem = this->rx_buffer.at(2);
				if (networkItem > NUM_NETWORKS - 1) break;
				uint8 SSIDlength = std::min(SSID_MAX_LENGTH, this->rx_buffer.at(3));
				uint8 PASSlength = std::min(PASS_MAX_LENGTH, this->rx_buffer.at(4 + SSIDlength));
				this->networks[networkItem].ssid = std::string(this->rx_buffer.begin() + 4, this->rx_buffer.begin() + 4 + SSIDlength);
				this->networks[networkItem].pass = std::string(this->rx_buffer.begin() + 4 + SSIDlength + 1, this->rx_buffer.begin() + 4 + SSIDlength + 1 + PASSlength);

			}
			break;
		case toesp_cmds_t::NETWORK_UNREGISTER:
			UDBG("RAINBOW BrokeStudioFirmware received message NETWORK_UNREGISTER\n");
			if (message_size == 2) {
				uint8 const networkItem = this->rx_buffer.at(2);
				if (networkItem > NUM_NETWORKS - 1) break;
				this->networks[networkItem].ssid = "";
				this->networks[networkItem].pass = "";
			}
			break;

		// FILE CMDS

		case toesp_cmds_t::FILE_OPEN: {
			UDBG("RAINBOW BrokeStudioFirmware received message FILE_OPEN\n");
			if (message_size >= 4) {
				uint8 config = this->rx_buffer.at(2);
				uint8 access_mode = config & static_cast<uint8>(file_config_flags_t::ACCESS_MODE);

				if (access_mode == static_cast<uint8>(file_config_flags_t::AUTO_ACCESS_MODE)) {
					uint8 const path = this->rx_buffer.at(3);
					uint8 const file = this->rx_buffer.at(4);
					if (path < NUM_FILE_PATHS && file < NUM_FILES) {
						this->file_exists[path][file] = true;
						this->working_path = path;
						this->working_file = file;
						this->working_file_config = config;
						this->file_offset = 0;
						this->saveFiles();
					}
				}else {
					//TODO manual mode
				}
			}
			break;
		}
		case toesp_cmds_t::FILE_CLOSE:
			UDBG("RAINBOW BrokeStudioFirmware received message FILE_CLOSE\n");
			this->working_file = NO_WORKING_FILE;
			this->saveFiles();
			break;
		case toesp_cmds_t::FILE_STATUS: {
			UDBG("RAINBOW BrokeStudioFirmware received message FILE_STATUS\n");

			uint8 access_mode = this->working_file_config & static_cast<uint8>(file_config_flags_t::ACCESS_MODE);

			if (access_mode == static_cast<uint8>(file_config_flags_t::AUTO_ACCESS_MODE)) {
				if (this->working_file == NO_WORKING_FILE) {
					this->tx_messages.push_back({
						2,
						static_cast<uint8>(fromesp_cmds_t::FILE_STATUS),
						0
					});
				}else {
					this->tx_messages.push_back({
						5,
						static_cast<uint8>(fromesp_cmds_t::FILE_STATUS),
						1,
						static_cast<uint8>(this->working_file_config),
						static_cast<uint8>(this->working_path),
						static_cast<uint8>(this->working_file),
					});
				}
			}
			else {
				//TODO manual mode
			}
			break;
		}
		case toesp_cmds_t::FILE_EXISTS: {
			UDBG("RAINBOW BrokeStudioFirmware received message FILE_EXISTS\n");

			if (message_size < 2) {
				break;
			}
			uint8 config = this->rx_buffer.at(2);
			uint8 access_mode = config & static_cast<uint8>(file_config_flags_t::ACCESS_MODE);

			if (access_mode == static_cast<uint8>(file_config_flags_t::AUTO_ACCESS_MODE)) {
				if (message_size == 4) {
					uint8 const path = this->rx_buffer.at(3);
					uint8 const file = this->rx_buffer.at(4);
					if (path < NUM_FILE_PATHS && file < NUM_FILES) {
						this->tx_messages.push_back({
							2,
							static_cast<uint8>(fromesp_cmds_t::FILE_EXISTS),
							static_cast<uint8>(this->file_exists[path][file] ? 1 : 0)
						});
					}
				}
			}else {
				//TODO manual mode
			}
			break;
		}
		case toesp_cmds_t::FILE_DELETE: {
			UDBG("RAINBOW BrokeStudioFirmware received message FILE_DELETE\n");

			if (message_size < 2) {
				break;
			}
			uint8 config = this->rx_buffer.at(2);
			uint8 access_mode = config & static_cast<uint8>(file_config_flags_t::ACCESS_MODE);

			if (access_mode == static_cast<uint8>(file_config_flags_t::AUTO_ACCESS_MODE)) {
				if (message_size == 4) {
					uint8 const path = this->rx_buffer.at(3);
					uint8 const file = this->rx_buffer.at(4);
					if (path < NUM_FILE_PATHS && file < NUM_FILES) {
						if (this->file_exists[path][file]) {
							// File exists, let's delete it
							this->files[path][file].clear();
							this->file_exists[path][file] = false;
							this->tx_messages.push_back({
								2,
								static_cast<uint8>(fromesp_cmds_t::FILE_DELETE),
								static_cast<uint8>(file_delete_results_t::SUCCESS)
							});
							this->saveFiles();
						}else {
							// File does not exist
							this->tx_messages.push_back({
								2,
								static_cast<uint8>(fromesp_cmds_t::FILE_DELETE),
								static_cast<uint8>(file_delete_results_t::FILE_NOT_FOUND)
							});
						}
					}else {
						// Error while deleting the file
						this->tx_messages.push_back({
							2,
							static_cast<uint8>(fromesp_cmds_t::FILE_DELETE),
							static_cast<uint8>(file_delete_results_t::INVALID_PATH_OR_FILE)
						});
					}
				}
			}
			else {
				//TODO manual mode
			}
			break;
		}
		case toesp_cmds_t::FILE_SET_CUR:
			UDBG("RAINBOW BrokeStudioFirmware received message FILE_SET_CUR\n");
			if (2 <= message_size && message_size <= 5) {
				this->file_offset = this->rx_buffer.at(2);
				this->file_offset += static_cast<uint32>(message_size >= 3 ? this->rx_buffer.at(3) : 0) << 8;
				this->file_offset += static_cast<uint32>(message_size >= 4 ? this->rx_buffer.at(4) : 0) << 16;
				this->file_offset += static_cast<uint32>(message_size >= 5 ? this->rx_buffer.at(5) : 0) << 24;
			}
			break;
		case toesp_cmds_t::FILE_READ:
			UDBG("RAINBOW BrokeStudioFirmware received message FILE_READ\n");
			if (message_size == 2) {
				if (this->working_file != NO_WORKING_FILE) {
					uint8 const n = this->rx_buffer.at(2);
					this->readFile(this->working_path, this->working_file, n, this->file_offset);
					this->file_offset += n;
					UDBG("working file offset: %u (%x)\n", this->file_offset, this->file_offset);
					UDBG("file size: %lu bytes\n", this->files[this->working_path][this->working_file].size());
					if (this->file_offset > this->files[this->working_path][this->working_file].size()) {
						this->file_offset = this->files[this->working_path][this->working_file].size();
					}
				}else {
					this->tx_messages.push_back({2, static_cast<uint8>(fromesp_cmds_t::FILE_DATA), 0});
				}
			}
			break;
		case toesp_cmds_t::FILE_WRITE:
			UDBG("RAINBOW BrokeStudioFirmware received message FILE_WRITE\n");
			if (message_size >= 3 && this->working_file != NO_WORKING_FILE) {
				this->writeFile(this->working_path, this->working_file, this->file_offset, this->rx_buffer.begin() + 2, this->rx_buffer.begin() + message_size + 1);
				this->file_offset += message_size - 1;
			}
			break;
		case toesp_cmds_t::FILE_APPEND:
			UDBG("RAINBOW BrokeStudioFirmware received message FILE_APPEND\n");
			if (message_size >= 3 && this->working_file != NO_WORKING_FILE) {
				this->writeFile(this->working_path, this->working_file, this->files[working_path][working_file].size(), this->rx_buffer.begin() + 2, this->rx_buffer.begin() + message_size + 1);
			}
			break;
		case toesp_cmds_t::FILE_COUNT: {
			UDBG("RAINBOW BrokeStudioFirmware received message FILE_COUNT\n");

			if (message_size < 2) {
				break;
			}
			uint8 config = this->rx_buffer.at(2);
			uint8 access_mode = config & static_cast<uint8>(file_config_flags_t::ACCESS_MODE);

			if (access_mode == static_cast<uint8>(file_config_flags_t::AUTO_ACCESS_MODE)) {
				if (message_size == 3) {
					uint8 const path = this->rx_buffer.at(3);
					if (path >= NUM_FILE_PATHS) {
						this->tx_messages.push_back({
							2,
							static_cast<uint8>(fromesp_cmds_t::FILE_COUNT),
							0
						});
					}else {
						uint8 nb_files = 0;
						for (bool exists : this->file_exists[path]) {
							if (exists) {
								++nb_files;
							}
						}
						this->tx_messages.push_back({
							2,
							static_cast<uint8>(fromesp_cmds_t::FILE_COUNT),
							nb_files
							});
						UDBG("%u files found in path %u\n", nb_files, path);
					}
				}
			}else {
				//TODO manual mode
			}

			break;
		}case toesp_cmds_t::FILE_GET_LIST: {
			UDBG("RAINBOW BrokeStudioFirmware received message FILE_GET_LIST\n");

			if (message_size < 2) {
				break;
			}
			uint8 config = this->rx_buffer.at(2);
			uint8 access_mode = config & static_cast<uint8>(file_config_flags_t::ACCESS_MODE);

			if (access_mode == static_cast<uint8>(file_config_flags_t::AUTO_ACCESS_MODE)) {
				if (message_size >= 3) {
					std::vector<uint8> existing_files;
					uint8 const path = this->rx_buffer.at(3);
					uint8 page_size = NUM_FILES;
					uint8 current_page = 0;
					if (message_size == 5) {
						page_size = this->rx_buffer.at(4);
						current_page = this->rx_buffer.at(5);
					}
					uint8 page_start = current_page * page_size;
					uint8 page_end = current_page * page_size + page_size;
					uint8 nFiles = 0;
					if (page_end > this->file_exists[path].size())
						page_end = this->file_exists[path].size();
					for (uint8 i = 0; i < NUM_FILES; ++i) {
						if (this->file_exists[path][i]) {
							if (nFiles >= page_start && nFiles < page_end) {
								existing_files.push_back(i);
							}
							nFiles++;
						}
						if (nFiles >= page_end) break;
					}
					std::deque<uint8> message({
						static_cast<uint8>(existing_files.size() + 2),
						static_cast<uint8>(fromesp_cmds_t::FILE_LIST),
						static_cast<uint8>(existing_files.size())
						});
					message.insert(message.end(), existing_files.begin(), existing_files.end());
					this->tx_messages.push_back(message);
				}
			}
			else {
				//TODO manual mode
			}
			break;
		}
		case toesp_cmds_t::FILE_GET_FREE_ID:
			UDBG("RAINBOW BrokeStudioFirmware received message FILE_GET_FREE_ID\n");
			if (message_size == 2) {
				uint8 const file_id = this->getFreeFileId(this->rx_buffer.at(2));
				if (file_id != 128) {
					// Free file ID found
					this->tx_messages.push_back({
						2,
						static_cast<uint8>(fromesp_cmds_t::FILE_ID),
						file_id,
					});
				}else {
					// Free file ID not found
					this->tx_messages.push_back({
						1,
						static_cast<uint8>(fromesp_cmds_t::FILE_ID)
					});
				}
			}
			break;
		case toesp_cmds_t::FILE_GET_INFO: {
			UDBG("RAINBOW BrokeStudioFirmware received message FILE_GET_INFO\n");

			if (message_size < 2) {
				break;
			}
			uint8 config = this->rx_buffer.at(2);
			uint8 access_mode = config & static_cast<uint8>(file_config_flags_t::ACCESS_MODE);

			if (access_mode == static_cast<uint8>(file_config_flags_t::AUTO_ACCESS_MODE)) {
				if (message_size == 4) {
					uint8 const path = this->rx_buffer.at(3);
					uint8 const file = this->rx_buffer.at(4);
					if (path < NUM_FILE_PATHS && file < NUM_FILES && this->file_exists[path][file]) {
						// Compute info
						uint32 file_crc32;
						file_crc32 = CalcCRC32(0L, this->files[path][file].data(), this->files[path][file].size());

						uint32 file_size = this->files[path][file].size();

						// Send info
						this->tx_messages.push_back({
							9,
							static_cast<uint8>(fromesp_cmds_t::FILE_INFO),

							static_cast<uint8>((file_crc32 >> 24) & 0xff),
							static_cast<uint8>((file_crc32 >> 16) & 0xff),
							static_cast<uint8>((file_crc32 >> 8) & 0xff),
							static_cast<uint8>(file_crc32 & 0xff),

							static_cast<uint8>((file_size >> 24) & 0xff),
							static_cast<uint8>((file_size >> 16) & 0xff),
							static_cast<uint8>((file_size >> 8) & 0xff),
							static_cast<uint8>(file_size & 0xff)
						});
					}else {
						// File not found or path/file out of bounds
						this->tx_messages.push_back({
							1,
							static_cast<uint8>(fromesp_cmds_t::FILE_INFO)
						});
					}
				}
			}else {
				//TODO manual mode
			}

			break;
		}
		case toesp_cmds_t::FILE_DOWNLOAD: {
			UDBG("RAINBOW BrokeStudioFirmware received message FILE_DOWNLOAD\n");

			if (message_size < 2) {
				break;
			}
			uint8 config = this->rx_buffer.at(2);
			uint8 access_mode = config & static_cast<uint8>(file_config_flags_t::ACCESS_MODE);

			if (access_mode == static_cast<uint8>(file_config_flags_t::AUTO_ACCESS_MODE)) {
				if (message_size > 6) {
					// Parse
					uint8 const urlLength = this->rx_buffer.at(3);
					if (message_size != urlLength + 5) {
						break;
					}
					std::string const url(this->rx_buffer.begin() + 4, this->rx_buffer.begin() + 4 + urlLength);

					uint8 const path = this->rx_buffer.at(4 + urlLength);
					uint8 const file = this->rx_buffer.at(4 + 1 + urlLength);

					// Delete existing file
					if (path < NUM_FILE_PATHS && file < NUM_FILES) {
						if (this->file_exists[path][file]) {
							// File exists, let's delete it
							this->files[path][file].clear();
							this->file_exists[path][file] = false;
							this->saveFiles();
						}
					}else {
						// Invalid path / file
						this->tx_messages.push_back({
							4,
							static_cast<uint8>(fromesp_cmds_t::FILE_DOWNLOAD),
							static_cast<uint8>(file_download_results_t::INVALID_DESTINATION),
							0,
							0
						});
						break;
					}

					// Download new file
					this->downloadFile(url, path, file);
				}
			}else {
				//TODO manual mode
			}
			break;
		}
		case toesp_cmds_t::FILE_FORMAT:
			UDBG("RAINBOW BrokeStudioFirmware received message FILE_FORMAT\n");
			if (message_size == 1) {
				// Clear file list
				for (uint8 p = 0; p < NUM_FILE_PATHS; p++) {
					for (uint8 f = 0; f < NUM_FILES; f++) {
						this->file_exists[p][f] = false;
					}
				}
			}
			break;
		default:
			UDBG("RAINBOW BrokeStudioFirmware received unknown message %02x\n", this->rx_buffer.at(1));
			break;
	};

	// Remove processed message
	this->rx_buffer.clear();
}

void BrokeStudioFirmware::readFile(uint8 path, uint8 file, uint8 n, uint32 offset) {
	assert(path < NUM_FILE_PATHS);
	assert(file < NUM_FILES);

	// Get data range
	std::vector<uint8> const& f = this->files[path][file];
	std::vector<uint8>::const_iterator data_begin;
	std::vector<uint8>::const_iterator data_end;
	if (offset >= f.size()) {
		data_begin = f.end();
		data_end = data_begin;
	}else {
		data_begin = f.begin() + offset;
		data_end = f.begin() + std::min(static_cast<std::vector<uint8>::size_type>(offset) + n, f.size());
	}
	std::vector<uint8>::size_type const data_size = data_end - data_begin;

	// Write response
	std::deque<uint8> message({
		static_cast<uint8>(data_size + 2),
		static_cast<uint8>(fromesp_cmds_t::FILE_DATA),
		static_cast<uint8>(data_size)
	});
	message.insert(message.end(), data_begin, data_end);
	this->tx_messages.push_back(message);
}

template<class I>
void BrokeStudioFirmware::writeFile(uint8 path, uint8 file, uint32 offset, I data_begin, I data_end) {
	std::vector<uint8>& f = this->files[path][file];
	auto const data_size = data_end - data_begin;
	uint32 const offset_end = offset + data_size;
	if (offset_end > f.size()) {
		f.resize(offset_end, 0);
	}

	for (std::vector<uint8>::size_type i = offset; i < offset_end; ++i) {
		f[i] = *data_begin;
		++data_begin;
	}
	this->file_exists[path][file] = true;
}

uint8 BrokeStudioFirmware::getFreeFileId(uint8 path) const {
	uint8 const NOT_FOUND = 128;
	if (path >= NUM_FILE_PATHS) {
		return NOT_FOUND;
	}
	std::array<bool, NUM_FILES> const& existing_files = this->file_exists.at(path);
	for (size_t i = 0; i < existing_files.size(); ++i) {
		if (!existing_files[i]) {
			return i;
		}
	}
	return NOT_FOUND;
}

void BrokeStudioFirmware::saveFiles() const {
	char const* filesystem_file_path = ::getenv("RAINBOW_FILESYSTEM_FILE");
	if (filesystem_file_path == NULL) {
		return;
	}

	std::ofstream ofs(filesystem_file_path);
	for(std::array<bool, NUM_FILES> const& path: this->file_exists) {
		for(bool exists: path) {
			ofs << exists << ' ';
		}
		ofs << '\n';
	}
	ofs << '\n';

	for (std::array<std::vector<uint8>, NUM_FILES> const& path: this->files) {
		for (std::vector<uint8> const& file: path) {
			ofs << file.size() << '\n';
			for (uint8 byte: file) {
				ofs << (uint16_t)byte << ' ';
			}
			ofs << '\n';
		}
	}
}

void BrokeStudioFirmware::loadFiles() {
	char const* filesystem_file_path = ::getenv("RAINBOW_FILESYSTEM_FILE");
	if (filesystem_file_path == NULL) {
		return;
	}

	std::ifstream ifs(filesystem_file_path);
	if (ifs.fail()) {
		return;
	}

	for(std::array<bool, NUM_FILES>& path: this->file_exists) {
		for(bool& exists: path) {
			ifs >> exists;
		}
	}

	for (std::array<std::vector<uint8>, NUM_FILES>& path: this->files) {
		for (std::vector<uint8>& file: path) {
			size_t file_size;
			ifs >> file_size;

			file.clear();
			file.reserve(file_size);
			for (size_t i = 0; i < file_size; ++i) {
				uint16_t byte;
				ifs >> byte;
				if (byte > 255) throw std::runtime_error("invalid filesystem file");
				file.push_back(byte);
			}
		}
	}
}

template<class I>
void BrokeStudioFirmware::sendMessageToServer(I begin, I end) {
#if RAINBOW_DEBUG >= 1
	FCEU_printf("RAINBOW message to send: ");
	for (I cur = begin; cur < end; ++cur) {
		FCEU_printf("%02x ", *cur);
	}
	FCEU_printf("\n");
#endif

	if (this->socket != nullptr) {
		size_t message_size = end - begin;
		std::vector<uint8> aggregated;
		aggregated.reserve(message_size);
		aggregated.insert(aggregated.end(), begin, end);
		this->socket->sendBinary(aggregated);
		this->socket->poll();
	}
}

template<class I>
void BrokeStudioFirmware::sendUdpDatagramToServer(I begin, I end) {
#if RAINBOW_DEBUG >= 1
	FCEU_printf("RAINBOW %lu udp datagram to send", wall_clock_milli());
#	if RAINBOW_DEBUG >= 2
	FCEU_printf(": ");
	for (I cur = begin; cur < end; ++cur) {
		FCEU_printf("%02x ", *cur);
	}
#	endif
	FCEU_printf("\n");
#endif

	if (this->udp_socket != -1) {
		size_t message_size = end - begin;
		std::vector<uint8> aggregated;
		aggregated.reserve(message_size);
		aggregated.insert(aggregated.end(), begin, end);

		ssize_t n = sendto(
			this->udp_socket, cast_network_payload(aggregated.data()), aggregated.size(), 0,
			reinterpret_cast<sockaddr*>(&this->server_addr), sizeof(sockaddr)
		);
		if (n == -1) {
			UDBG("RAINBOW UDP send failed: %s\n", strerror(errno));
		}else if (n != message_size) {
			UDBG("RAINBOW UDP sent partial message\n");
		}
	}
}

template<class I>
void BrokeStudioFirmware::sendTcpDataToServer(I begin, I end) {
#if RAINBOW_DEBUG >= 1
	FCEU_printf("RAINBOW %lu tcp data to send", wall_clock_milli());
#	if RAINBOW_DEBUG >= 2
	FCEU_printf(": ");
	for (I cur = begin; cur < end; ++cur) {
		FCEU_printf("%02x ", *cur);
	}
#	endif
	FCEU_printf("\n");
#endif

	if (this->tcp_socket != -1) {
		size_t message_size = end - begin;
		std::vector<uint8> aggregated;
		aggregated.reserve(message_size);
		aggregated.insert(aggregated.end(), begin, end);

		ssize_t n = ::send(
			this->tcp_socket,
			cast_network_payload(aggregated.data()), aggregated.size(),
			0
		);
		if (n == -1) {
			UDBG("RAINBOW TCP send failed: %s\n", strerror(errno));
		}else if (n != message_size) {
			UDBG("RAINBOW TCP sent partial message\n");
		}
	}
}

std::deque<uint8> BrokeStudioFirmware::read_socket(int socket) {
	fd_set rfds;
	FD_ZERO(&rfds);
	FD_SET(socket, &rfds);

	timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 0;

	int n_readable = ::select(socket+1, &rfds, NULL, NULL, &tv);
	if (n_readable == -1) {
		UDBG("RAINBOW failed to check sockets for data: %s\n", strerror(errno));
	}else if (n_readable > 0) {
		if (FD_ISSET(socket, &rfds)) {
			size_t const MAX_MSG_SIZE = 254;
			std::vector<uint8> data;
			data.resize(MAX_MSG_SIZE);

			sockaddr_in addr_from;
			socklen_t addr_from_len = sizeof(addr_from);
			ssize_t msg_len = ::recvfrom(
				socket, cast_network_payload(data.data()), MAX_MSG_SIZE, 0,
				reinterpret_cast<sockaddr*>(&addr_from), &addr_from_len
			);

			if (msg_len == -1) {
				UDBG("RAINBOW failed to read socket: %s\n", strerror(errno));
			}else if (msg_len <= MAX_MSG_SIZE) {
				UDBG("RAINBOW %lu received message of size %zd", wall_clock_milli(), msg_len);
#if RAINBOW_DEBUG >= 2
				UDBG_FLOOD(": ");
				for (auto it = data.begin(); it != data.begin() + msg_len; ++it) {
					UDBG_FLOOD("%02x", *it);
				}
#endif
				UDBG("\n");
				std::deque<uint8> message({
					static_cast<uint8>(msg_len+1),
					static_cast<uint8>(fromesp_cmds_t::MESSAGE_FROM_SERVER)
				});
				message.insert(message.end(), data.begin(), data.begin() + msg_len);
				return message;
			}else {
				UDBG("RAINBOW received a bigger message than expected\n");
				//TODO handle it like Rainbow's ESP handle it
			}
		}
	}
	return std::deque<uint8>();
}

void BrokeStudioFirmware::receiveDataFromServer() {
	// Websocket
	if (this->socket != nullptr) {
		this->socket->poll();
		this->socket->dispatchBinary([this] (std::vector<uint8_t> const& data) {
			size_t const msg_len = data.end() - data.begin();
			if (msg_len <= 0xff) {
				UDBG("RAINBOW %lu WebSocket data received... size %02x", wall_clock_milli(), msg_len);
#if RAINBOW_DEBUG >= 2
				UDBG_FLOOD(": ");
				for (uint8_t const c: data) {
					UDBG_FLOOD("%02x ", c);
				}
#endif
				UDBG("\n");
				std::deque<uint8> message({
					static_cast<uint8>(msg_len+1),
					static_cast<uint8>(fromesp_cmds_t::MESSAGE_FROM_SERVER)
				});
				message.insert(message.end(), data.begin(), data.end());
				this->tx_messages.push_back(message);
			}
		});
	}

	// TCP
	if (this->tcp_socket != -1) {
		std::deque<uint8> message = read_socket(this->tcp_socket);
		if (!message.empty()) {
			this->tx_messages.push_back(message);
		}
	}

	// UDP
	if (this->udp_socket != -1) {
		std::deque<uint8> message = read_socket(this->udp_socket);
		if (!message.empty()) {
			this->tx_messages.push_back(message);
		}
	}
}

void BrokeStudioFirmware::closeConnection() {
	//TODO close UDP socket

	// Close TCP socket
	if (this->tcp_socket != - 1) {
		close_sock(this->tcp_socket);
	}

	// Close WebSocket
	if (this->socket != nullptr) {
		// Gently ask for connection closing
		if (this->socket->getReadyState() == WebSocket::OPEN) {
			this->socket->close();
		}

		// Start a thread that waits for the connection to be closed, before deleting the socket
		if (this->socket_close_thread.joinable()) {
			this->socket_close_thread.join();
		}
		WebSocket::pointer ws = this->socket;
		this->socket_close_thread = std::thread([ws] {
			while (ws->getReadyState() != WebSocket::CLOSED) {
				ws->poll(5);
			}
			delete ws;
		});

		// Forget about this connection
		this->socket = nullptr;
	}
}

void BrokeStudioFirmware::openConnection() {
	this->closeConnection();

	if ((this->active_protocol == server_protocol_t::WEBSOCKET) || (this->active_protocol == server_protocol_t::WEBSOCKET_SECURED)) {
		// Create websocket
		std::ostringstream ws_url;
		std::string protocol = "";
		if (this->active_protocol == server_protocol_t::WEBSOCKET) protocol = "ws://";
		if (this->active_protocol == server_protocol_t::WEBSOCKET_SECURED) protocol = "wss://";
		ws_url << protocol << this->server_settings_address << ':' << this->server_settings_port;
		WebSocket::pointer ws = WebSocket::from_url(ws_url.str());
		if (!ws) {
			UDBG("RAINBOW unable to connect to WebSocket server\n");
		}else {
			this->socket = ws;
		}
	}else if (this->active_protocol == server_protocol_t::TCP) {
		// Resolve server's hostname
		std::pair<bool, sockaddr_in> server_addr = this->resolve_server_address();
		if (!server_addr.first) {
			return;
		}

		// Create socket
		this->tcp_socket = ::socket(AF_INET, SOCK_STREAM, 0);
		if (this->tcp_socket == -1) {
			UDBG("RAINBOW unable to create TCP socket: %s\n", strerror(errno));
		}

		// Connect to server
		int connect_res = ::connect(this->tcp_socket, reinterpret_cast<sockaddr*>(&server_addr.second), sizeof(sockaddr));
		if (connect_res == -1) {
			UDBG("RAINBOW unable to connect to TCP server: %s\n", strerror(errno));
			this->tcp_socket = -1;
		}
	}else if (this->active_protocol == server_protocol_t::TCP_SECURED) {
		//TODO
		UDBG("RAINBOW TCP_SECURED not yet implemented");
	}else if (this->active_protocol == server_protocol_t::UDP) {
		// Init UDP socket and store parsed address
		std::pair<bool, sockaddr_in> server_addr = this->resolve_server_address();
		if (!server_addr.first) {
			return;
		}
		this->server_addr = server_addr.second;

		this->udp_socket = ::socket(AF_INET, SOCK_DGRAM, 0);
		if (this->udp_socket == -1) {
			UDBG("RAINBOW unable to connect to UDP server: %s\n", strerror(errno));
		}

		sockaddr_in bind_addr;
		bzero(reinterpret_cast<void*>(&bind_addr), sizeof(bind_addr));
		bind_addr.sin_family = AF_INET;
		bind_addr.sin_port = htons(0);
		bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		bind(this->udp_socket, reinterpret_cast<sockaddr*>(&bind_addr), sizeof(sockaddr));
	}
}

void BrokeStudioFirmware::pingRequest(uint8 n) {
	using std::chrono::time_point;
	using std::chrono::steady_clock;
	using std::chrono::duration_cast;
	using std::chrono::milliseconds;

	uint8 min = 255;
	uint8 max = 0;
	uint32 total_ms = 0;
	uint8 lost = 0;

	pping_s* pping = pping_init(this->server_settings_address.c_str());
	if (pping == NULL) {
		lost = n;
	}else {
		for (uint8 i = 0; i < n; ++i) {
			time_point<steady_clock> begin = steady_clock::now();
			int r = pping_ping(pping);
			time_point<steady_clock> end = steady_clock::now();

			if (r != 0) {
				UDBG("RAINBOW BrokeStudioFirmware ping lost packet\n");
				++lost;
			}else {
				uint32 const round_trip_time_ms = duration_cast<milliseconds>(end - begin).count();
				uint8 const rtt = (round_trip_time_ms + 2) / 4;
				UDBG("RAINBOW BrokeStudioFirmware ping %d ms\n", round_trip_time_ms);
				min = std::min(min, rtt);
				max = std::max(max, rtt);
				total_ms += round_trip_time_ms;
			}

			if (i < n - 1) {
				std::this_thread::sleep_for(std::chrono::seconds(1));
			}
		}
		pping_free(pping);
	}

	this->ping_min = min;
	if (lost < n) {
		this->ping_avg = ((total_ms / (n - lost)) + 2) / 4;
	}
	this->ping_max = max;
	this->ping_lost = lost;
	this->ping_ready = true;
	UDBG("RAINBOW BrokeStudioFirmware ping stored: %d/%d/%d/%d (min/max/avg/lost)\n", this->ping_min, this ->ping_max, this->ping_avg, this->ping_lost);
}

void BrokeStudioFirmware::receivePingResult() {
	if (!this->ping_ready) {
		return;
	}
	assert(this->ping_thread.joinable());

	this->ping_thread.join();
	this->ping_ready = false;

	this->tx_messages.push_back({
		5,
		static_cast<uint8>(fromesp_cmds_t::SERVER_PING),
		this->ping_min,
		this->ping_max,
		this->ping_avg,
		this->ping_lost
	});
}

std::pair<bool, sockaddr_in> BrokeStudioFirmware::resolve_server_address() {
	sockaddr_in addr;

	hostent *he = gethostbyname(this->server_settings_address.c_str());
	if (he == NULL) {
		UDBG("RAINBOW unable to resolve server's hostname\n");
		return std::make_pair(false, addr);
	}

	bzero(reinterpret_cast<void*>(&addr), sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(this->server_settings_port);
	addr.sin_addr = *((in_addr*)he->h_addr);

	return std::make_pair(true, addr);
}

void BrokeStudioFirmware::httpdEvent(mg_connection *nc, int ev, void *ev_data) {
	BrokeStudioFirmware* self = reinterpret_cast<BrokeStudioFirmware*>(nc->mgr->user_data);
	auto send_message = [&] (int status_code, char const * body, char const * mime) {
		std::string header = std::string("Content-Type: ") + mime + "\r\nConnection: close\r\n";
		mg_send_response_line(nc, status_code, header.c_str());
		mg_printf(nc, body);
		nc->flags |= MG_F_SEND_AND_CLOSE;
	};
	auto send_generic_error = [&] {
		send_message(200, "{\"success\":\"false\"}\n", "application/json");
	};
	if (ev == MG_EV_HTTP_REQUEST) {
		UDBG("http request event \n");
		struct http_message *hm = (struct http_message *) ev_data;
		UDBG("  uri: %.*s\n", hm->uri.len, hm->uri.p);
		if (std::string("/api/config") == std::string(hm->uri.p, hm->uri.len)) {
			if (mg_vcasecmp(&hm->method, "GET") == 0) {
				mg_send_response_line(nc, 200, "Content-Type: application/json\r\nConnection: close\r\n");
				mg_printf(nc, "{\"server\":{\"host\":\"%s\", \"port\":\"%u\"}}\n", self->server_settings_address.c_str(), self->server_settings_port);
				nc->flags |= MG_F_SEND_AND_CLOSE;
			}
		}
		else if (std::string("/api/config/update") == std::string(hm->uri.p, hm->uri.len)) {
			if (mg_vcasecmp(&hm->method, "POST") == 0) {
				char var_name[100], file_name[100];
				const char *chunk;
				size_t chunk_len, n1, n2;
				n1 = n2 = 0;
				while ((n2 = mg_parse_multipart(hm->body.p + n1, hm->body.len - n1, var_name, sizeof(var_name), file_name, sizeof(file_name), &chunk, &chunk_len)) > 0) {
					if (strcmp(var_name, "server_host") == 0) {
						self->server_settings_address = std::string(chunk, (int)chunk_len);
					}
					if (strcmp(var_name, "server_port") == 0) {
						self->server_settings_port = std::atoi(chunk);
					}
					n1 += n2;
				}
				send_message(200, "{\"success\":\"true\"}\n", "application/json");
			}
			else {
				send_generic_error();
			}
		}
		else if (std::string("/api/esp/debugconfig") == std::string(hm->uri.p, hm->uri.len)) {
			if (mg_vcasecmp(&hm->method, "POST") == 0) {
				char var_name[100], file_name[100];
				const char *chunk;
				size_t chunk_len, n1, n2;
				n1 = n2 = 0;
				while ((n2 = mg_parse_multipart(hm->body.p + n1, hm->body.len - n1, var_name, sizeof(var_name), file_name, sizeof(file_name), &chunk, &chunk_len)) > 0) {
					if (strcmp(var_name, "debugConfig") == 0) {
						self->debug_config = *chunk - '0';
						send_message(200, "{\"success\":\"true\"}\n", "application/json");
						break;
					}
					n1 += n2;
				}
			}else {
				mg_send_response_line(nc, 200, "Content-Type: application/json\r\nConnection: close\r\n");
				mg_printf(nc, "{\"debugConfig\":\"%u\"}", self->debug_config);
				nc->flags |= MG_F_SEND_AND_CLOSE;
			}
		}
		else if (std::string("/api/file/list") == std::string(hm->uri.p, hm->uri.len)) {
			char path[256];
			int const path_len = mg_get_http_var(&hm->query_string, "path", path, 256);
			if (path_len < 0) {
				send_generic_error();
				return;
			}
			mg_send_response_line(nc, 200, "Content-Type: application/json\r\nConnection: close\r\n");
			mg_printf(nc, "[");
			if (path_len == 0) {
				// Send three paths
				for (uint8_t path_index = 0; path_index < NUM_FILE_PATHS; ++path_index) {
					if (path_index != 0)
					{
						mg_printf(nc, ",");
					}
					uint32_t path_size = 0L;
					for (uint8_t file_index = 0; file_index < NUM_FILES; ++file_index) {
						if (self->file_exists[path_index][file_index])
							path_size += self->files[path_index][file_index].size();
					}
					mg_printf(nc, "{\"id\":\"%d\",\"type\":\"dir\",\"name\":\"%s\",\"size\":\"%d\"}", path_index, dir_names[path_index].c_str(), path_size);
				}
			}
			else {
				// Send path content
				int path_index = path[0] - '0';
				mg_printf(nc, "{\"id\":\"\",\"type\":\"dir\",\"name\":\"..\",\"size\":\"-1\"}");
					for (uint8_t file_index = 0; file_index < self->file_exists[path_index].size(); ++file_index) {
						if (self->file_exists[path_index][file_index]) {
							mg_printf(nc, ",");
							mg_printf(nc, "{\"id\":\"%d\",\"type\":\"file\",\"name\":\"file%d.bin\",\"size\":\"%d\"}", file_index, file_index, static_cast<int>(self->files[path_index][file_index].size()));
						}
					}
			}
			mg_printf(nc, "]");
			nc->flags |= MG_F_SEND_AND_CLOSE;
		}
		else if (std::string("/api/file/free") == std::string(hm->uri.p, hm->uri.len)) {
			char path[256];
			int const path_len = mg_get_http_var(&hm->query_string, "path", path, 256);
			if (path_len <= 0) {
				send_generic_error();
				return;
			}
			mg_send_response_line(nc, 200, "Content-Type: application/json\r\nConnection: close\r\n");
			mg_printf(nc, "[");
			int path_index = std::atoi(path);
			bool found = false;
			for (uint8_t file_index = 0; file_index < NUM_FILES; ++file_index) {
				if (!self->file_exists[path_index][file_index]) {
					if (found) {
						mg_printf(nc, ",");
					}
					mg_printf(nc, "{\"id\":\"%d\",\"name\":\"%d\"}", file_index, file_index);
					found = true;
				}
			}
			mg_printf(nc, "]");
			nc->flags |= MG_F_SEND_AND_CLOSE;
		}else if (std::string("/api/file/delete") == std::string(hm->uri.p, hm->uri.len)) {
			char path[256];
			char file[256];
			int const path_len = mg_get_http_var(&hm->query_string, "path", path, 256);
			int const file_len = mg_get_http_var(&hm->query_string, "file", file, 256);

			if (path_len <= 0 || file_len <= 0) {
				send_generic_error();
				return;
			}

			uint8 path_index = std::atoi(path);
			uint8 file_index = std::atoi(file);

			if (path_index >= NUM_FILE_PATHS || file_index >= NUM_FILES || !self->file_exists[path_index][file_index]) {
				send_generic_error();
				return;
			}

			UDBG("RAINBOW Web(self=%p) deleting file %d/%d\n", self, path_index, file_index);
			self->file_exists[path_index][file_index] = false;
			self->files[path_index][file_index].clear();
			self->saveFiles();
			send_message(200, "{\"success\":\"true\"}\n", "application/json");

		}else if (std::string("/api/file/rename") == std::string(hm->uri.p, hm->uri.len)) {
			char path[256];
			char file[256];
			char new_path[256];
			char new_file[256];
			int const path_len = mg_get_http_var(&hm->query_string, "path", path, 256);
			int const file_len = mg_get_http_var(&hm->query_string, "file", file, 256);
			int const new_path_len = mg_get_http_var(&hm->query_string, "newPath", new_path, 256);
			int const new_file_len = mg_get_http_var(&hm->query_string, "newFile", new_file, 256);

			if (path_len <= 0 || file_len <= 0 || new_path_len <= 0 || new_file_len <= 0) {
				send_generic_error();
				return;
			}

			uint8 path_index = std::atoi(path);
			uint8 file_index = std::atoi(file);
			uint8 new_path_index = std::atoi(new_path);
			uint8 new_file_index = std::atoi(new_file);
			UDBG("%d %d %d %d\n", path_index, file_index, new_path_index, new_file_index);

			if (path_index >= NUM_FILE_PATHS || file_index >= NUM_FILES || new_path_index >= NUM_FILE_PATHS || new_file_index >= NUM_FILES|| !self->file_exists[path_index][file_index]) {
				send_generic_error();
				return;
			}

			self->files[new_path_index][new_file_index] = self->files[path_index][file_index];
			self->file_exists[new_path_index][new_file_index] = self->file_exists[path_index][file_index];
			self->file_exists[path_index][file_index] = false;
			self->files[path_index][file_index].clear();
			self->saveFiles();

			send_message(200, "{\"success\":\"true\"}\n", "application/json");
		}else if (std::string("/api/file/download") == std::string(hm->uri.p, hm->uri.len)) {
			char path[256];
			char file[256];
			int const path_len = mg_get_http_var(&hm->query_string, "path", path, 256);
			int const file_len = mg_get_http_var(&hm->query_string, "file", file, 256);

			if (path_len <= 0 ||file_len <= 0) {
				send_generic_error();
				return;
			}

			uint8 path_index = std::atoi(path);
			uint8 file_index = std::atoi(file);

			if (path_index >= NUM_FILE_PATHS || file_index >= NUM_FILES || !self->file_exists[path_index][file_index]) {
				send_generic_error();
				return;
			}
			mg_send_response_line(
				nc, 200,
				"Content-Type: application/octet-stream\r\n"
				"Connection: close\r\n"
			);
			mg_send(nc, self->files[path_index][file_index].data(), self->files[path_index][file_index].size());
			nc->flags |= MG_F_SEND_AND_CLOSE;
		}else if (std::string("/api/file/upload") == std::string(hm->uri.p, hm->uri.len)) {
			// Get boundary for multipart form in HTTP headers
			std::string multipart_boundary;
			{
				mg_str const * content_type = mg_get_http_header(hm, "Content-Type");
				if (content_type == NULL) {
					send_generic_error();
					return;
				}
				static std::regex const content_type_regex("multipart/form-data; boundary=(.*)");
				std::smatch match;
				std::string content_type_str(content_type->p, content_type->len);
				if (!std::regex_match(content_type_str, match, content_type_regex)) {
					send_generic_error();
					return;
				}
				assert(match.size() == 2);
				multipart_boundary = match[1];
			}

			// Parse form parts
			std::map<std::string, std::string> params;
			{
				std::string body_str(hm->body.p, hm->body.len);
				std::string::size_type pos = 0;
				while (pos != std::string::npos) {
					// Find the parameter name
					std::string::size_type found_pos = body_str.find("form-data; name=\"", pos);
					if (found_pos == std::string::npos) {
						break;
					}
					pos = found_pos + 17;
					found_pos = body_str.find('"', pos);
					if (found_pos == std::string::npos) {
						break;
					}
					std::string const param_name = body_str.substr(pos, found_pos - pos);
					pos = found_pos;

					// Find the begining of the body
					found_pos = body_str.find("\r\n\r\n", pos);
					if (found_pos == std::string::npos) {
						break;
					}
					pos = found_pos + 4;

					// Find the begining of the next delimiter
					found_pos = body_str.find("\r\n--" + multipart_boundary, pos);
					if (found_pos == std::string::npos) {
						break;
					}
					std::string const param_value = body_str.substr(pos, found_pos - pos);
					pos = found_pos;

					// Store parsed parameter
					params[param_name] = param_value;
				}
			}

			// Process request
			std::map<std::string, std::string>::const_iterator file_data = params.find("file_data");
			std::map<std::string, std::string>::const_iterator path = params.find("path");
			std::map<std::string, std::string>::const_iterator file = params.find("file");

			if (file_data == params.end() || path == params.end() || file == params.end()) {
				send_generic_error();
				return;
			}

			uint8 path_index = std::atoi(path->second.c_str());
			uint8 file_index = std::atoi(file->second.c_str());

			self->files[path_index][file_index] = std::vector<uint8>(file_data->second.begin(), file_data->second.end());
			self->file_exists[path_index][file_index] = true;
			self->saveFiles();

			UDBG("RAINBOW Web(self=%p) sucessfuly uploaded file %d/%d\n", self, path_index, file_index);

			// Return something webbrowser friendly
			send_message(200, "{\"success\":\"true\"}\n", "application/json");
		}else if (std::string("/api/file/format") == std::string(hm->uri.p, hm->uri.len)) {
			if (mg_vcasecmp(&hm->method, "POST") == 0) {
				// Clear file list
				for (uint8 p = 0; p < NUM_FILE_PATHS; p++)
				{
					for (uint8 f = 0; f < NUM_FILES; f++)
					{
						self->file_exists[p][f] = false;
					}
				}
				UDBG("RAINBOW Web(self=%p) sucessfuly formatted file system %d/%d\n", self);

				// Return something webbrowser friendly
				send_message(200, "{\"success\":\"true\"}\n", "application/json");
			}
			else {
				send_generic_error();
			}
		}else {
			char const* www_root = ::getenv("RAINBOW_WWW_ROOT");
			if (www_root == NULL) {
				std::string upload_form = R"-(<!doctype html><html><head><style>*{margin:2px}body{font-family:Arial}</style></style>)-";
				upload_form += R"-(<body><h1>Upload file to Rainbow:</h1>)-";
				upload_form += R"-(<input id="file_data" name="file_data" type="file"><br/>)-";
				upload_form += R"-(Path: <select id="path" name="path"><option value="0">SAVE</option><option value="1">ROMS</option><option value="2">USER</option></select><br />)-";
				upload_form += R"-(File: <select id="file" name="file">)-";
				for (uint8 i = 0; i < NUM_FILES; i++)
				{
					upload_form += "<option value=\"" + std::to_string(i) + "\">" + std::to_string(i) + "</option>";
				}
				upload_form += "</select><br/>";
				upload_form += R"-(<button id="btnSubmit" type="submit" onclick="handleSubmit()">Upload</button>)-";
				upload_form += "<script>function handleSubmit(){";
				upload_form += R"-(btnSubmit.disabled=true;)-";
				upload_form += R"-(btnSubmit.innerHTML = "Uploading...";)-";
				upload_form += R"-(let formData = new FormData();)-";
				upload_form += R"-(formData.append("file_data", file_data.files[0]);)-";
				upload_form += R"-(let path_index = path.options[path.selectedIndex].value;)-";
				upload_form += R"-(formData.append("path", path.value);)-";
				upload_form += R"-(formData.append("file", file.value);)-";
				upload_form += R"-(let xhr = new XMLHttpRequest();)-";
				upload_form += R"-(xhr.open("POST", "/api/file/upload");)-";
				upload_form += R"-(xhr.onreadystatechange = function () {)-";
				upload_form += R"-(if (this.readyState === XMLHttpRequest.DONE && this.status === 200) {)-";
				upload_form += R"-(var r = JSON.parse(this.responseText);)-";
				upload_form += R"-(if (r.success) alert("File uploaded successfully.");)-";
				upload_form += R"-(else alert('Error while uploading the file.');)-";
				upload_form += R"-(btnSubmit.disabled=false;)-";
				upload_form += R"-(btnSubmit.innerHTML = "Upload";)-";
				upload_form += R"-(})-";
				upload_form += R"-(if (this.readyState === XMLHttpRequest.DONE && this.status === 0) {)-";
				upload_form += R"-(alert('Please check your connection.');)-";
				upload_form += R"-(btnSubmit.disabled=false;)-";
				upload_form += R"-(btnSubmit.innerHTML = "Upload";)-";
				upload_form += R"-(})-";
				upload_form += R"-(};)-";
				upload_form += R"-(xhr.send(formData);)-";
				upload_form += "}</script>";
				upload_form += "</body></html>";
				send_message(200, upload_form.c_str(), "text/html");
			}else {
				// Translate url path to a file path on disk
				assert(hm->uri.len > 0); // Impossible as HTTP header are constructed in such a way that there is always at least one char in path (I think)
				std::string uri(hm->uri.p+1, hm->uri.len-1); // remove leading "/"
				if (uri.empty()) {
					uri = "index.html";
				}

				// Try to serve requested file, if not found, then try to serve index.html
				std::string file_path = "";
				try {
					file_path = std::string(www_root) + uri;
					std::vector<uint8> contents = readHostFile(file_path);
				}catch (std::runtime_error const& e) {
					try {
						file_path = std::string(www_root) + "index.html";
						std::vector<uint8> contents = readHostFile(file_path);
					}catch (std::runtime_error const& e) {
						std::string message(
							"<html><body><h1>404</h1><p>" +
							file_path +
							"</p><p>" +
							e.what() +
							"</p></body></html>"
						);
						send_message(
							404,
							message.c_str(),
							"text/html"
						);
					}
				}

				// Serve file
				std::vector<uint8> contents = readHostFile(file_path);
				// Basic / naive mime type handler
				std::string ext = file_path.substr(file_path.find_last_of(".") + 1);
				std::string mime_type = "";
				if (ext == "html") mime_type = "text/html";
				else if (ext == "css") mime_type = "text/css";
				else if (ext == "js") mime_type = "application/javascript";
				else mime_type = "application/octet-stream";
				std::string content_type(
					"Content-Type: " + mime_type + "\r\n" +
					"Connection: close\r\n"
				);
				mg_send_response_line(
					nc, 200, content_type.c_str()
				);
				mg_send(nc, contents.data(), contents.size());
				nc->flags |= MG_F_SEND_AND_CLOSE;
			}
		}
	}
}

namespace {
size_t download_write_callback(char *ptr, size_t size, size_t nmemb, void *userdata) {
	std::vector<uint8>* data = reinterpret_cast<std::vector<uint8>*>(userdata);
	data->insert(data->end(), reinterpret_cast<uint8*>(ptr), reinterpret_cast<uint8*>(ptr + size * nmemb));
	return size * nmemb;
}
}

void BrokeStudioFirmware::initDownload() {
	this->curl_handle = curl_easy_init();
	curl_easy_setopt(this->curl_handle, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(this->curl_handle, CURLOPT_WRITEFUNCTION, &download_write_callback);
    curl_easy_setopt(this->curl_handle, CURLOPT_FAILONERROR, 1L);
}

std::pair<uint8, uint8> BrokeStudioFirmware::curle_to_net_error(CURLcode curle) {
	static std::map<CURLcode, std::pair<uint8, uint8>> const resolution = {
		{
			CURLE_UNSUPPORTED_PROTOCOL, std::pair<uint8, uint8>(
				static_cast<uint8>(BrokeStudioFirmware::file_download_results_t::UNKNOWN_OR_UNSUPPORTED_PROTOCOL),
				static_cast<uint8>(BrokeStudioFirmware::file_download_network_error_t::CONNECTION_LOST)
			)
		},
		{
			CURLE_WRITE_ERROR, std::pair<uint8, uint8>(
				static_cast<uint8>(BrokeStudioFirmware::file_download_results_t::NETWORK_ERROR),
				static_cast<uint8>(BrokeStudioFirmware::file_download_network_error_t::STREAM_WRITE)
			)
		},
		{
			CURLE_OUT_OF_MEMORY, std::pair<uint8, uint8>(
				static_cast<uint8>(BrokeStudioFirmware::file_download_results_t::NETWORK_ERROR),
				static_cast<uint8>(BrokeStudioFirmware::file_download_network_error_t::OUT_OF_RAM)
			)
		},
	};

	auto entry = resolution.find(curle);
	if (entry != resolution.end()) {
		return entry->second;
	}
	return std::pair<uint8, uint8>(
		static_cast<uint8>(BrokeStudioFirmware::file_download_results_t::NETWORK_ERROR),
		static_cast<uint8>(BrokeStudioFirmware::file_download_network_error_t::CONNECTION_FAILED)
	);
}

void BrokeStudioFirmware::downloadFile(std::string const& url, uint8 path, uint8 file) {
	UDBG("RAINBOW BrokeStudioFirmware download %s -> (%u,%u)\n", url.c_str(), (unsigned int)path, (unsigned int)file);
	//TODO asynchronous download using curl_multi_* (and maybe a thread, or regular ticks on rx/tx/getGpio4)

	// Directly fail if the curl handle was not properly initialized
	if (this->curl_handle == nullptr) {
		UDBG("RAINBOW BrokeStudioFirmware download failed: no handle\n");
		this->tx_messages.push_back({
			2,
			static_cast<uint8>(fromesp_cmds_t::FILE_DOWNLOAD),
			static_cast<uint8>(file_download_results_t::NETWORK_ERROR),
			0,
			static_cast<uint8>(file_download_network_error_t::NOT_CONNECTED)
		});
		return;
	}

	// Download file
	std::vector<uint8> data;
	curl_easy_setopt(this->curl_handle, CURLOPT_URL, url.c_str());
	curl_easy_setopt(this->curl_handle, CURLOPT_WRITEDATA, (void*)&data);
	CURLcode res = curl_easy_perform(this->curl_handle);

	// Store data and write result message
	if (res != CURLE_OK) {
		UDBG("RAINBOW BrokeStudioFirmware download failed\n");
		std::pair<uint8, uint8> rainbow_error = curle_to_net_error(res);
		this->tx_messages.push_back({
			4,
			static_cast<uint8>(fromesp_cmds_t::FILE_DOWNLOAD),
			rainbow_error.first,
			0,
			rainbow_error.second
		});
	}else {
		UDBG("RAINBOW BrokeStudioFirmware download success\n");
		// Store data
		this->files[path][file] = data;
		this->file_exists[path][file] = true;
		this->saveFiles();

		// Write result message
		this->tx_messages.push_back({
			4,
			static_cast<uint8>(fromesp_cmds_t::FILE_DOWNLOAD),
			static_cast<uint8>(file_download_results_t::SUCCESS)
		});
	}
}

void BrokeStudioFirmware::cleanupDownload() {
	curl_easy_cleanup(this->curl_handle);
	this->curl_handle = nullptr;
}
