#ifndef COAP_DUMP_H
#define COAP_DUMP_H 1

#ifdef __cplusplus
extern "C" {
#endif

#include "coap.h"

#if YACOAP_DEBUG

/**
 * Dumps the content of \p buf as hexadecimal.
 *
 * @param[in] buf The buffer to be dumped.
 * @param[in] buflen The length of \p buf in bytes.
 * @param[in] bare If true, "Dump:" and a the newline character are printed
 * before the actual values.
 */
void coap_dump(const uint8_t *buf, size_t buflen, bool bare);

/**
 * Dumps all values of a CoAP packet (including payload) as hexadecimal.
 *
 * @param[in] pkt Pointer to the packet whose content is to be dumped.
 */
void coap_dump_packet(const coap_packet_t *pkt);

#endif /* YACOAP_DEBUG */

#ifdef __cplusplus
}
#endif

#endif
