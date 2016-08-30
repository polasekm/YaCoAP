#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "coap.h"
#include "coap_dump.h"

#if YACOAP_DEBUG

static void _dump_header(const coap_header_t *hdr);
static void _dump_options(const coap_option_t *opts, const size_t numopt);

/* --- PRIVATE -------------------------------------------------------------- */
static void _dump_header(const coap_header_t *hdr)
{
    printf("Header:\n");
    printf("  ver  0x%02X\n", hdr->ver);
    printf("  t    0x%02X\n", hdr->t);
    printf("  tkl  0x%02X\n", hdr->tkl);
    printf("  code 0x%02X\n", hdr->code);
    printf("  id   0x%04X\n", hdr->id);
}

static void _dump_options(const coap_option_t *opts, const size_t numopt)
{
    printf(" Options:\n");
    for (size_t i = 0; i < numopt; ++i) {
        printf("  0x%02X [ ", opts[i].num);
        coap_dump(opts[i].buf.p, opts[i].buf.len, true);
        printf(" ]\n");
    }
}

/* --- PUBLIC --------------------------------------------------------------- */
void coap_dump(const uint8_t *buf, size_t buflen, bool bare)
{
    if (bare) {
        while(buflen--) {
            printf("%02X%s", *buf++, (buflen > 0) ? " " : "");
        }
    }
    else {
        printf("Dump: ");
        while(buflen--) {
            printf("%02X%s", *buf++, (buflen > 0) ? " " : "");
        }
        printf("\n");
    }
}

void coap_dump_packet(const coap_packet_t *pkt)
{
    _dump_header(&pkt->hdr);
    _dump_options(pkt->opts, pkt->numopts);
    printf("Payload: ");
    coap_dump(pkt->payload.p, pkt->payload.len, true);
    printf("\n");
}
#endif /* YACOAP_DEBUG */
