#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <linux/tcp.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>

/* TODO: read configuration from environment */

#ifndef TARGET_PORT_LOW
#error "TARGET_PORT_LOW not defined"
#endif

#ifndef TARGET_PORT_HIGH
#error "TARGET_PORT_HIGH not defined"
#endif

#ifndef TARGET_IP_ADDRESS
#error "TARGET_IP_ADDRESS not defined"
#endif

#ifndef TARGET_IP_ADDRESS_MASK
#error "TARGET_IP_ADDRESS_MASK not defined"
#endif

#ifndef TIMEOUT_MILLISECONDS
#error "TIMEOUT_MILLISECONDS not defined"
#endif

static int matched_port(const struct sockaddr_in *addr_in)
{
    uint16_t port = ntohs(addr_in->sin_port);
    return TARGET_PORT_LOW <= port && TARGET_PORT_HIGH >= port;
}

static int matched_ip_address(const struct sockaddr_in *addr_in)
{
    assert(addr_in->sin_family == AF_INET);

    /* TODO: move parsing of constants into initializer having __attribute__((constructor)) */

    struct in_addr target_addr = { 0 };
    int target_addr_res =
        inet_pton(AF_INET, TARGET_IP_ADDRESS, &target_addr);
    if (1 != target_addr_res) {
        if (target_addr_res == 0) {
            fprintf(stderr, "inet_pton: address not valid: %s\n",
                    TARGET_IP_ADDRESS);
        } else if (target_addr_res == -1) {
            perror("inet_pton");
        }
        exit(EXIT_FAILURE);
    }

    struct in_addr target_addr_mask = { 0 };
    int target_addr_mask_res =
        inet_pton(AF_INET, TARGET_IP_ADDRESS_MASK, &target_addr_mask);
    if (1 != target_addr_mask_res) {
        if (target_addr_mask_res == 0) {
            fprintf(stderr, "inet_pton: address not valid: %s\n",
                    TARGET_IP_ADDRESS_MASK);
        } else if (target_addr_mask_res == -1) {
            perror("inet_pton");
        }
        exit(EXIT_FAILURE);
    }

    return (target_addr.s_addr & target_addr_mask.s_addr) ==
        (addr_in->sin_addr.s_addr & target_addr_mask.s_addr);
}

static int matched(const struct sockaddr_in *addr_in)
{
    return matched_port(addr_in) && matched_ip_address(addr_in);
}

static void intercept_message(const struct sockaddr_in *addr_in)
{
#ifdef DEBUG
    assert(addr_in->sin_family == AF_INET);

    uint16_t port = ntohs(addr_in->sin_port);
    char buf[INET_ADDRSTRLEN + 1] = { 0 };
    if (NULL ==
        inet_ntop(addr_in->sin_family, &(addr_in->sin_addr), buf,
                  INET_ADDRSTRLEN)) {
        perror("inet_ntop");
        exit(EXIT_FAILURE);
    }

    fprintf(stderr,
            "intercepted connect() to %s:%d; setting timeout\n", buf,
            port);
#endif
}

static void set_timeout(int sockfd)
{
    unsigned int timeout = TIMEOUT_MILLISECONDS;
    socklen_t timeout_len = sizeof(timeout);

    if (0 !=
        setsockopt(sockfd, IPPROTO_TCP, TCP_USER_TIMEOUT, &timeout,
                   timeout_len)) {
        /* ENOPROTOOPT means the socket was not a TCP socket */
        if (ENOPROTOOPT != errno) {
            perror("setsockopt");
            exit(EXIT_FAILURE);
        }
    }
}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    dlerror();
    int (*next_connect) (int, const struct sockaddr *, socklen_t) =
        dlsym(RTLD_NEXT, "connect");
    char *error = dlerror();

    if (NULL != error) {
        fprintf(stderr, "dlsym: %s\n", error);
        exit(EXIT_FAILURE);
    }

    if (AF_INET == addr->sa_family) {
        const struct sockaddr_in *addr_in =
            (const struct sockaddr_in *) addr;

        if (matched(addr_in)) {
            intercept_message(addr_in);
            set_timeout(sockfd);
        }
    }
    /* TODO: support IPv6 or IPv4-mapped IPv6 addresses (RFC 4291 2.5.5) */

    return next_connect(sockfd, addr, addrlen);
}
