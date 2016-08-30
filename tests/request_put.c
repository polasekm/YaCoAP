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

static int handle_request_put_response(const coap_resource_t *resource,
                                       const coap_packet_t *reqpkt,
                                       coap_packet_t *rsppkt)
{

    (void) resource;
    (void) reqpkt;
    printf("handle_request_put_response\n");
    if (rsppkt->hdr.t == COAP_TYPE_ACK) {
        printf("  ACK\n");
        return COAP_RDY;
    }
    printf("  INVALID\n");
    return COAP_ACK_WAIT;
}

coap_resource_t resources[] =
{
    {COAP_RDY, COAP_METHOD_PUT, COAP_TYPE_CON,
        handle_request_put_response, NULL,
        COAP_SET_CONTENTTYPE(COAP_CONTENTTYPE_TXT_PLAIN)},
    {(coap_state_t)0, (coap_method_t)0, (coap_msgtype_t)0,
        NULL, NULL,
        COAP_SET_CONTENTTYPE(COAP_CONTENTTYPE_NONE)}
};

int main(int argc, char *argv[])
{
    int fd;
    struct addrinfo hints, *dstinfo, *p;
    int rv;

    if (argc != 4) {
        fprintf(stderr, "USAGE: %s hostname path content\n", argv[0]);
        return 1;
    }

    coap_resource_path_t path_request_put;
    path_request_put.count = 1;
    path_request_put.items[0] = &argv[2][0];
    for (size_t c = 0; c < strlen(argv[2]); ++c) {
        if (argv[2][c] == '/')
            ++path_request_put.count;
        if (path_request_put.count > COAP_MAX_PATHITEMS) {
            fprintf(stderr, "path has to many elements: %d\n", path_request_put.count);
            return 1;
        }
        path_request_put.items[path_request_put.count] = &argv[2][c+1];
    }

    resources[0].path = &path_request_put;

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
    printf("coap_make_request\n");
    coap_make_request(msgid, NULL, &resources[0], (uint8_t *)argv[3], strlen(argv[3]), &req);
    uint8_t buf[1024];
    size_t buflen = sizeof(buf);
    if ((rc = coap_build(&req, buf, &buflen)) > COAP_ERR) {
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
        for (int state = COAP_ACK_WAIT; state != COAP_RDY; ) {
            n = recvfrom(fd, buf, sizeof(buf), 0, (struct sockaddr *)&cliaddr, &len);
            printf("received message of %d bytes\n", n);
            if ((rc = coap_parse(buf, n, &rsp)) > COAP_ERR) {
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
