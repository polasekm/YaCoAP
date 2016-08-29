#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <arpa/inet.h>

#include "coap.h"

/* --- PRIVATE -------------------------------------------------------------- */
static coap_state_t _parse_token(const uint8_t *buf,
                                 const size_t buflen,
                                 coap_packet_t *pkt);
static coap_state_t _parse_header(const uint8_t *buf,
                                  const size_t buflen,
                                  coap_header_t *hdr);
static coap_state_t _parse_options_payload(const uint8_t *buf,
                                           const size_t buflen,
                                           coap_packet_t *pkt);
static coap_state_t _parse_option(const uint8_t **buf,
                                  const size_t buflen,
                                  coap_option_t *option,
                                  uint16_t *running_delta);

static coap_state_t _parse_header(const uint8_t *buf,
                                  const size_t buflen,
                                  coap_header_t *hdr)
{
    if (buflen < sizeof(coap_raw_header_t)) {
        return COAP_ERR_HEADER_TOO_SHORT;
    }
    coap_raw_header_t *r = (coap_raw_header_t*)buf;
    /* parse header from raw buffer */
    hdr->ver = r->hdr.ver;
    hdr->t = r->hdr.t;
    hdr->tkl = r->hdr.tkl;
    hdr->code = r->hdr.code;
    hdr->id = ntohs(r->hdr.id);
    if (hdr->ver != 1) {
        return COAP_ERR_VERSION_NOT_1;
    }
    return COAP_SUCCESS;
}

static coap_state_t _parse_token(const uint8_t *buf,
                                 const size_t buflen,
                                 coap_packet_t *pkt)
{
    coap_buffer_t *tok = &pkt->tok;
    int toklen = pkt->hdr.tkl;
    /* validate the token length */
    if (sizeof(coap_raw_header_t) + toklen > buflen || toklen > 8) {
        return COAP_ERR_TOKEN_TOO_SHORT;
    }
    tok->len = toklen;
    if (!toklen) {
        tok->p = NULL;
    }
    else {
        tok->p = buf + sizeof(coap_raw_header_t);
    }
    return COAP_SUCCESS;
}

static coap_state_t _parse_option(const uint8_t **buf,
                                  const size_t buflen,
                                  coap_option_t *option,
                                  uint16_t *running_delta)
{
    const uint8_t *p = *buf;
    uint8_t headlen = 1;
    uint16_t len, delta;

    if (buflen < headlen) {
        return COAP_ERR_OPTION_TOO_SHORT_FOR_HEADER;
    }
    delta = (p[0] & 0xF0) >> 4;
    len = p[0] & 0x0F;

    /* FIXME: untested and may be buggy */
    if (delta == 13) {
        headlen++;
        if (buflen < headlen)
            return COAP_ERR_OPTION_TOO_SHORT_FOR_HEADER;
        delta = p[1] + 13;
        p++;
    }
    else if (delta == 14) {
        headlen += 2;
        if (buflen < headlen) {
            return COAP_ERR_OPTION_TOO_SHORT_FOR_HEADER;
        }
        delta = ((p[1] << 8) | p[2]) + 269;
        p+=2;
    }
    else if (delta == 15) {
        return COAP_ERR_OPTION_DELTA_INVALID;
    }

    if (len == 13) {
        headlen++;
        if (buflen < headlen) {
            return COAP_ERR_OPTION_TOO_SHORT_FOR_HEADER;
        }
        len = p[1] + 13;
        p++;
    }
    else if (len == 14) {
        headlen += 2;
        if (buflen < headlen) {
            return COAP_ERR_OPTION_TOO_SHORT_FOR_HEADER;
        }
        len = ((p[1] << 8) | p[2]) + 269;
        p += 2;
    }
    else if (len == 15) {
        return COAP_ERR_OPTION_LEN_INVALID;
    }

    if ((p + 1 + len) > (*buf + buflen)) {
        return COAP_ERR_OPTION_TOO_BIG;
    }
    /* set option header */
    option->num = delta + *running_delta;
    option->buf.p = p+1;
    option->buf.len = len;
    /* advance buffer cursor */
    *buf = p + 1 + len;
    *running_delta += delta;

    return COAP_SUCCESS;
}

// http://tools.ietf.org/html/rfc7252#section-3.1
static coap_state_t _parse_options_payload(const uint8_t *buf,
                                           const size_t buflen,
                                           coap_packet_t *pkt)
{
    size_t optionIndex = 0;
    uint16_t delta = 0;
    const uint8_t *p = buf + sizeof(coap_raw_header_t) + pkt->hdr.tkl;
    const uint8_t *end = buf + buflen;
    int rc;
    if (p > end) {
        return COAP_ERR_OPTION_OVERRUNS_PACKET;
    }

    /* Note: 0xFF is payload marker */
    while ((optionIndex < COAP_MAX_OPTIONS) && (p < end) && (*p != 0xFF)) {
        rc = _parse_option(&p, end - p, &pkt->opts[optionIndex], &delta);
        if(rc) {
            return rc;
        }
        optionIndex++;
    }
    pkt->numopts = optionIndex;

    if ((p + 1) < end && *p == 0xFF) {
        pkt->payload.p = p + 1;
        pkt->payload.len = end - (p + 1);
    }
    else {
        pkt->payload.p = NULL;
        pkt->payload.len = 0;
    }
    return COAP_SUCCESS;
}

/* --- PUBLIC --------------------------------------------------------------- */
coap_state_t coap_parse(const uint8_t *buf,
                        const size_t buflen,
                        coap_packet_t *pkt)
{
    int rc;
    /* parse header, token, options, and payload */
    rc = _parse_header(buf, buflen, &pkt->hdr);
    if(rc) {
        return rc;
    }
    rc = _parse_token(buf, buflen, pkt);
    if(rc) {
        return rc;
    }
    pkt->numopts = COAP_MAX_OPTIONS;
    rc = _parse_options_payload(buf, buflen, pkt);
    if(rc) {
        return rc;
    }
    return COAP_SUCCESS;
}
