#ifndef PPING_H
#define PPING_H

/* Portable Ping, pinging made easy
 *
 * PPing is a C library allowing to make ping requests. It aims at being
 * and avoid requiring non-necessary privileges. As such, you can use the same
 * simple interface on Linux, Windows and OSX without requiring root or
 * administrator permissions.
 *
 * It also comes without dependencies outside of standard system libraries,
 * no include path, nor library to install.
 *
 * If you want a header-only version, just include the .c
 *
 * Author: Sylvain Gadrat
 * License: WTFPL v2
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Ping an address, return after receiving the response
 * @return 0: success ; 1 failure ; 2 timeout
 *
 * Can resolve domain name, beware that it will take significant time. Avoid
 * using it (especially with a domain name) to measure round trip time.
 */
int pping_easy_ping(char const* addr);

/* More complete API
 *
 * Usage example:
 *   void my_ping_func() {
 *     struct pping_s* pping;
 *     int result;
 *
 *     pping = pping_init("example.com");
 *
 *     r = pping_ping(pping);
 *     if (r != 0) printf("lost first packet\n");
 *     r = pping_ping(pping);
 *     if (r != 0) printf("lost second packet\n");
 *     // ...
 *
 *     pping_free(pping);
 *   }
 *
 * Notes:
 *   Time consuming tasks (like DNS resolution) are done in pping_init().
 *   Time spent in pping_ping() is near the round trip time
 */
struct pping_s;
struct pping_s* pping_init(const char* addr);
int pping_ping(struct pping_s* pping);
void pping_free(struct pping_s* pping);

#ifdef __cplusplus
}
#endif

#endif
