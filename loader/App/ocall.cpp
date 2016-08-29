#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <sys/stat.h>
#include <netdb.h>
#include <sys/time.h>
#include <errno.h>

clock_t ocall_sgx_clock()
{
    return clock();
}

time_t ocall_sgx_time(time_t *timep)
{
    return time(timep);
}

struct tm *ocall_sgx_localtime(const time_t *timep)
{
    return localtime(timep);
}

struct tm *ocall_sgx_gmtime(const time_t *timep)
{
    return gmtime(timep);
}

time_t ocall_sgx_mktime(struct tm *timeptr)
{
    return mktime(timeptr);
}

int ocall_sgx_gettimeofday(struct timeval *tv, struct timezone *tz)
{
    return gettimeofday(tv, tz);
}

int ocall_sgx_puts(const char *str)
{
    return puts(str);
}

// skip sgx_push_gadget

int ocall_sgx_open(const char *pathname, int flags, unsigned mode)
{
    return open(pathname, flags, mode);
}

int ocall_sgx_close(int fd)
{
    return close(fd);
}

ssize_t ocall_sgx_read(int fd, char * buf, size_t buf_len)
{
    return read(fd, buf, buf_len);
}

ssize_t ocall_sgx_write(int fd, const char *buf, size_t n)
{
    int w = write(fd, buf, n);
    if(w < 0) {
        printf("Error write!: errno = %d\n", errno);
    }
    return w;
}

off_t ocall_sgx_lseek(int fildes, off_t offset, int whence)
{
    return lseek(fildes, offset, whence);
}

//--------------- network --------------

#define SOCKET int
#define SOCKET_ERROR -1

int ocall_sgx_socket(int af, int type, int protocol)
{
    return socket(af, type, protocol);
}

int ocall_sgx_bind(int s, unsigned long addr, size_t addr_size)
{
    return bind((SOCKET)s, (const struct sockaddr *)addr, addr_size);
}

int ocall_sgx_connect(int s, const struct sockaddr *addr, size_t addrlen)
{
    return connect((SOCKET)s, addr, addrlen);
}

int ocall_sgx_listen(int s, int backlog)
{
    return listen((SOCKET)s, backlog);
}

int ocall_sgx_accept(int s, struct sockaddr *addr, unsigned addr_size, socklen_t *addrlen)
{
    return (SOCKET)accept((SOCKET)s, addr, (socklen_t *)addrlen);
}

int ocall_sgx_fstat(int fd, struct stat *buf)
{
    return fstat(fd, buf);
}

ssize_t ocall_sgx_send(int s, const void *buf, size_t len, int flags)
{
    return send((SOCKET)s, buf, len, flags);
}

ssize_t ocall_sgx_recv(int s, void *buf, size_t len, int flags)
{
    return recv((SOCKET)s, buf, len, flags);
}

ssize_t ocall_sgx_sendto(int sockfd, const void *buf, size_t len, int flags,
        const struct sockaddr *dest_addr, size_t addrlen)
{
    return sendto(sockfd, buf, len, flags, dest_addr, addrlen);
}

ssize_t ocall_sgx_recvfrom(int s, void *buf, size_t len, int flags,
        struct sockaddr *dest_addr, unsigned alen, socklen_t* addrlen)
{
    return recvfrom(s, buf, len, flags, dest_addr, addrlen);
}

int ocall_sgx_gethostname(char *name, size_t namelen)
{
    return gethostname(name, namelen);
}

int ocall_sgx_getaddrinfo(const char *node, const char *service,
        const struct addrinfo *hints, unsigned long *res) {
    // TODO: copy res to enclave mem region
    return getaddrinfo(node, service, hints, (struct addrinfo **)res);
}

char *ocall_sgx_getenv(const char *env)
{
    // TODO: copy res to enclave mem region
    return getenv(env);
}

int ocall_sgx_getsockname(int s, struct sockaddr *name, unsigned nlen, socklen_t *addrlen)
{
    return getsockname((SOCKET)s, name, addrlen);
}

int ocall_sgx_getsockopt(int s, int level, int optname, void *optval, unsigned len,
        socklen_t* optlen)
{
    int ret = getsockopt((SOCKET)s, level, optname, optval, &len);
    *optlen = len;
    return ret;
}

struct servent *ocall_sgx_getservbyname(const char *name, const char *proto)
{
    // TODO: copy res to enclave mem region
    return getservbyname(name, proto);
}

struct protoent *ocall_sgx_getprotobynumber(int proto)
{
    return getprotobynumber(proto);
}

int ocall_sgx_setsockopt(int s, int level, int optname, const void *optval, size_t optlen)
{
    return setsockopt(s, level, optname, optval, optlen);
}

#include <arpa/inet.h>

unsigned short ocall_sgx_htons(unsigned short hostshort)
{
    return htons(hostshort);
}

unsigned long ocall_sgx_htonl(unsigned long hostlong)
{
    return htonl(hostlong);
}

unsigned short ocall_sgx_ntohs(unsigned short netshort)
{
    return ntohs(netshort);
}

unsigned long ocall_sgx_ntohl(unsigned long netlong)
{
    return ntohl(netlong);
}

void sgx_signal_handle_caller(int signum)
{
    printf("Error: signum = %d\n", signum);
}

#include <signal.h>
sighandler_t ocall_sgx_signal(int signum, sighandler_t a)
{
    return signal(signum, sgx_signal_handle_caller);
}

int ocall_sgx_shutdown(int sockfd, int how)
{
    return shutdown(sockfd, how);
}
