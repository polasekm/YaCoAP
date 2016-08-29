#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include "coap.h"

#define DSTPORT     "5683"

static const coap_resource_path_t path_well_known_core = {2, {".well-known", "core"}};
static int handle_get_well_known_core(const coap_resource_t *resource,
                                      const coap_packet_t *reqpkt,
                                      coap_packet_t *rsppkt)
{

    (void) resource;
    (void) reqpkt;
    printf("handle_get_well_known_core\n");
    printf("  content: %.*s\n", (int)rsppkt->payload.len, (char *)rsppkt->payload.p);
    return COAP_RDY;
}

coap_resource_t resources[] =
{
    {COAP_RDY, COAP_METHOD_GET, COAP_TYPE_ACK,
        handle_get_well_known_core, &path_well_known_core,
        COAP_SET_CONTENTTYPE(COAP_CONTENTTYPE_APP_LINKFORMAT)},
    {(coap_state_t)0, (coap_method_t)0, (coap_msgtype_t)0,
        NULL, NULL,
        COAP_SET_CONTENTTYPE(COAP_CONTENTTYPE_NONE)}
};

int main(int argc, char *argv[])
{
    int fd;
    struct addrinfo hints, *dstinfo, *p;
    int rv;

    if (argc != 2) {
        fprintf(stderr, "USAGE: %s hostname\n", argv[0]);
        return 1;
    }
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    if ((rv = getaddrinfo(argv[1], DSTPORT, &hints, &dstinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    for (p = dstinfo; p != NULL; p = p->ai_next) {
        if ((fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("socket");
            continue;
        }
        break;
    }
    if (p == NULL) {
        fprintf(stderr, "failed to bind socket\n");
        return 2;
    }

    int n, rc;
    struct sockaddr_storage cliaddr;
    socklen_t len = sizeof(cliaddr);
    coap_packet_t req, rsp;
    uint16_t msgid = 42;
    printf(" + coap_make_request\n");
    coap_make_request(msgid, NULL, &resources[0], NULL, 0, &req);
    uint8_t buf[1024];
    size_t buflen = sizeof(buf);
    if (0 != (rc = coap_build(&req, buf, &buflen))) {
        printf("coap_build failed rc=%d\n", rc);
        return 1;
    }
    else {
        printf("send request\n");
        if ((n = sendto(fd, buf, buflen, 0, p->ai_addr, p->ai_addrlen)) == -1) {
            perror("sendto");
            return 1;
        }
        printf("wait for response ...\n");
        for (int state = COAP_RSP_WAIT; state != COAP_RDY; ) {
            n = recvfrom(fd, buf, sizeof(buf), 0, (struct sockaddr *)&cliaddr, &len);
            printf("received message of %d bytes\n", n);
            if (0 != (rc = coap_parse(buf, n, &rsp))) {
                printf("Bad packet rc=%d\n", rc);
                return 1;
            }
            state = coap_handle_response(resources, &req, &rsp);
        }
    }
    // cleanup and exit
    freeaddrinfo(dstinfo);
    close(fd);
    return 0;
}
