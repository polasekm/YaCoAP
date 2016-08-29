#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdbool.h>
#include <strings.h>

#include "coap.h"

const uint16_t rsplen = 128;
static char rsp[128] = "";

static const coap_resource_path_t path_well_known_core = {2, {".well-known", "core"}};
static int handle_get_well_known_core(const coap_resource_t *resource,
                                      const coap_packet_t *inpkt,
                                      coap_packet_t *pkt)
{
    printf("handle_get_well_known_core\n");
    return coap_make_response(inpkt->hdr.id, &inpkt->tok,
                              COAP_TYPE_ACK, COAP_RSPCODE_CONTENT,
                              resource->content_type,
                              (const uint8_t *)rsp, strlen(rsp),
                              pkt);
}

static const coap_resource_path_t path_piggyback = {1, {"piggyback"}};
static int handle_get_piggyback(const coap_resource_t *resource,
                                const coap_packet_t *inpkt,
                                coap_packet_t *pkt)
{
    printf("handle_get_piggyback\n");
    return coap_make_response(inpkt->hdr.id, &inpkt->tok,
                              COAP_TYPE_ACK, COAP_RSPCODE_CONTENT,
                              resource->content_type,
                              (const uint8_t *)path_piggyback.items[0], 9,
                              pkt);
}

static const coap_resource_path_t path_separate = {1, {"separate"}};
static int handle_get_separate(const coap_resource_t *resource,
                               const coap_packet_t *inpkt,
                               coap_packet_t *pkt)
{
    printf("handle_get_separate\n");
    return coap_make_response(inpkt->hdr.id, &inpkt->tok,
                              resource->msg_type, COAP_RSPCODE_CONTENT,
                              resource->content_type,
                              (const uint8_t *)path_separate.items[0], 8,
                              pkt);
}

coap_resource_t resources[] =
{
    {COAP_RDY, COAP_METHOD_GET, COAP_TYPE_ACK,
        handle_get_well_known_core, &path_well_known_core,
        COAP_SET_CONTENTTYPE(COAP_CONTENTTYPE_APP_LINKFORMAT)},
    {COAP_RDY, COAP_METHOD_GET, COAP_TYPE_ACK,
        handle_get_piggyback, &path_piggyback,
        COAP_SET_CONTENTTYPE(COAP_CONTENTTYPE_TXT_PLAIN)},
    {COAP_RDY, COAP_METHOD_GET, COAP_TYPE_NONCON,
        handle_get_separate, &path_separate,
        COAP_SET_CONTENTTYPE(COAP_CONTENTTYPE_TXT_PLAIN)},
    {(coap_state_t)0, (coap_method_t)0, (coap_msgtype_t)0,
        NULL, NULL,
        COAP_SET_CONTENTTYPE(COAP_CONTENTTYPE_NONE)}
};

int main(void)
{
    int fd;
    struct sockaddr_in6 servaddr, cliaddr;
    uint8_t buf[1024];

    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin6_family = AF_INET6;
    servaddr.sin6_addr = in6addr_any;
    servaddr.sin6_port = htons(COAP_DEFAULT_PORT);

    fd = socket(AF_INET6,SOCK_DGRAM,0);
    bind(fd,(struct sockaddr *)&servaddr, sizeof(servaddr));

    coap_make_link_format(resources, rsp, rsplen);

    while(1)
    {
        int n, rc;
        socklen_t len = sizeof(cliaddr);
        coap_packet_t pkt;

        n = recvfrom(fd, buf, sizeof(buf), 0, (struct sockaddr *)&cliaddr, &len);
        printf("received message of %d bytes\n", n);

        if (0 != (rc = coap_parse(buf, n, &pkt))) {
            printf("Bad packet rc=%d\n", rc);
        }
        else {
            for (int state = COAP_RSP_WAIT; state != COAP_RSP_SEND; ) {
                size_t buflen = sizeof(buf);
                coap_packet_t rsppkt;
                state = coap_handle_request(resources, &pkt, &rsppkt);

                if (0 != (rc = coap_build(&rsppkt, buf, &buflen))) {
                    printf("coap_build failed rc=%d\n", rc);
                    break;
                }
                else {
                    printf("send response\n");
                    sendto(fd, buf, buflen, 0, (struct sockaddr *)&cliaddr, sizeof(cliaddr));
                }
            }
        }
    }
    return 0;
}
