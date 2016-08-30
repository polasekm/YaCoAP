#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdbool.h>
#include <strings.h>

#include "coap.h"
#include "coap_dump.h"

extern void resource_setup(const coap_resource_t *resources);
extern coap_resource_t resources[];

int main(void)
{
    int fd;
#ifdef IPV6
    struct sockaddr_in6 servaddr, cliaddr;
#else /* IPV6 */
    struct sockaddr_in servaddr, cliaddr;
#endif /* IPV6 */
    uint8_t buf[1024];

#ifdef IPV6
    fd = socket(AF_INET6,SOCK_DGRAM,0);
#else /* IPV6 */
    fd = socket(AF_INET,SOCK_DGRAM,0);
#endif /* IPV6 */

    bzero(&servaddr,sizeof(servaddr));
#ifdef IPV6
    servaddr.sin6_family = AF_INET6;
    servaddr.sin6_addr = in6addr_any;
    servaddr.sin6_port = htons(COAP_DEFAULT_PORT);
#else /* IPV6 */
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(COAP_DEFAULT_PORT);
#endif /* IPV6 */
    bind(fd,(struct sockaddr *)&servaddr, sizeof(servaddr));

    resource_setup(resources);

    while(1)
    {
        int n, rc;
        socklen_t len = sizeof(cliaddr);
        coap_packet_t pkt;

        n = recvfrom(fd, buf, sizeof(buf), 0, (struct sockaddr *)&cliaddr, &len);
#ifdef YACOAP_DEBUG
        printf("Received: ");
        coap_dump(buf, n, true);
        printf("\n");
#endif

        if (0 != (rc = coap_parse(buf, n, &pkt)))
            printf("Bad packet rc=%d\n", rc);
        else
        {
            size_t buflen = sizeof(buf);
            coap_packet_t rsppkt;
#ifdef YACOAP_DEBUG
            coap_dump_packet(&pkt);
#endif
            coap_handle_request(resources, &pkt, &rsppkt);

            if (0 != (rc = coap_build(&rsppkt, buf, &buflen)))
                printf("coap_build failed rc=%d\n", rc);
            else
            {
#ifdef YACOAP_DEBUG
                printf("Sending: ");
                coap_dump(buf, buflen, true);
                printf("\n");
#endif
#ifdef YACOAP_DEBUG
                coap_dump_packet(&rsppkt);
#endif

                sendto(fd, buf, buflen, 0, (struct sockaddr *)&cliaddr, sizeof(cliaddr));
            }
        }
    }
}
