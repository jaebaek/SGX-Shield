#ifdef LD_DEBUG
//#define abort() exit(1)
#define abort()
#else
#define abort()
#endif
static clock_t sgx_clock()
{
    clock_t retv;
    sgx_status_t sgx_retv;
    if((sgx_retv = ocall_sgx_clock(&retv)) != SGX_SUCCESS) {
        dlog("%s FAILED!, Error code = %d\n", __FUNCTION__, sgx_retv);
        abort();
    }
    return retv;
}
static time_t sgx_time(time_t *timep)
{
    time_t retv;
    sgx_status_t sgx_retv;
    if((sgx_retv = ocall_sgx_time(&retv, timep)) != SGX_SUCCESS) {
        dlog("%s FAILED!, Error code = %d\n", __FUNCTION__, sgx_retv);
        abort();
    }
    return retv;
}
static struct tm localtime;
static struct tm *sgx_localtime(const time_t *timep)
{
    // TODO: copy res to enclave mem region
    struct tm *retv;
    sgx_status_t sgx_retv;
    if((sgx_retv = ocall_sgx_localtime(&retv, timep)) != SGX_SUCCESS) {
        dlog("%s FAILED!, Error code = %d\n", __FUNCTION__, sgx_retv);
        abort();
    }
    cpy((char *)&localtime, (char *)retv, sizeof(struct tm));
    return &localtime;
}
static struct tm gmtime;
static struct tm *sgx_gmtime(const time_t *timep)
{
    // TODO: copy res to enclave mem region
    struct tm *retv;
    sgx_status_t sgx_retv;
    if((sgx_retv = ocall_sgx_gmtime(&retv, timep)) != SGX_SUCCESS) {
        dlog("%s FAILED!, Error code = %d\n", __FUNCTION__, sgx_retv);
        abort();
    }
    cpy((char *)&gmtime, (char *)retv, sizeof(struct tm));
    return &gmtime;
}
static time_t sgx_mktime(struct tm *timeptr)
{
    time_t retv;
    sgx_status_t sgx_retv;
    if((sgx_retv = ocall_sgx_mktime(&retv, timeptr)) != SGX_SUCCESS) {
        dlog("%s FAILED!, Error code = %d\n", __FUNCTION__, sgx_retv);
        abort();
    }
    return retv;
}
static int sgx_gettimeofday(struct timeval *tv, struct timezone *tz)
{
    int retv;
    sgx_status_t sgx_retv;
    if((sgx_retv = ocall_sgx_gettimeofday(&retv, tv, tz)) != SGX_SUCCESS) {
        dlog("%s FAILED!, Error code = %d\n", __FUNCTION__, sgx_retv);
        abort();
    }
    return retv;
}
static int sgx_puts(const char *str)
{
    int retv;
    sgx_status_t sgx_retv;
    if((sgx_retv = ocall_sgx_puts(&retv, str)) != SGX_SUCCESS) {
        dlog("%s FAILED!, Error code = %d\n", __FUNCTION__, sgx_retv);
        abort();
    }
    return retv;
}
static int sgx_open(const char *pathname, int flags, unsigned mode)
{
    int retv;
    sgx_status_t sgx_retv;
    if((sgx_retv = ocall_sgx_open(&retv, pathname, flags, mode)) != SGX_SUCCESS) {
        dlog("%s FAILED!, Error code = %d\n", __FUNCTION__, sgx_retv);
        abort();
    }
    return retv;
}
static int sgx_close(int fd)
{
    int retv;
    sgx_status_t sgx_retv;
    if((sgx_retv = ocall_sgx_close(&retv, fd)) != SGX_SUCCESS) {
        dlog("%s FAILED!, Error code = %d\n", __FUNCTION__, sgx_retv);
        abort();
    }
    return retv;
}
static ssize_t sgx_read(int fd, char * buf, size_t buf_len)
{
    ssize_t retv;
    sgx_status_t sgx_retv;
    if((sgx_retv = ocall_sgx_read(&retv, fd, buf, buf_len)) != SGX_SUCCESS) {
        dlog("%s FAILED!, Error code = %d\n", __FUNCTION__, sgx_retv);
        abort();
    }
    return retv;
}
static ssize_t sgx_write(int fd, const char *buf, size_t n)
{
    ssize_t retv;
    sgx_status_t sgx_retv;
    if((sgx_retv = ocall_sgx_write(&retv, fd, buf, n)) != SGX_SUCCESS) {
        dlog("%s FAILED!, Error code = %d\n", __FUNCTION__, sgx_retv);
        abort();
    }
    return retv;
}
static off_t sgx_lseek(int fildes, off_t offset, int whence)
{
    off_t retv;
    sgx_status_t sgx_retv;
    if((sgx_retv = ocall_sgx_lseek(&retv, fildes, offset, whence)) != SGX_SUCCESS) {
        dlog("%s FAILED!, Error code = %d\n", __FUNCTION__, sgx_retv);
        abort();
    }
    return retv;
}
static int sgx_socket(int af, int type, int protocol)
{
    int retv;
    sgx_status_t sgx_retv;
    if((sgx_retv = ocall_sgx_socket(&retv, af, type, protocol)) != SGX_SUCCESS) {
        dlog("%s FAILED!, Error code = %d\n", __FUNCTION__, sgx_retv);
        abort();
    }
    return retv;
}
static int sgx_bind(int s, const struct sockaddr *addr, size_t addr_size)
{
    int retv;
    sgx_status_t sgx_retv;
    if((sgx_retv = ocall_sgx_bind(&retv, s, (unsigned long)addr, addr_size)) != SGX_SUCCESS) {
        dlog("%s FAILED!, Error code = %d\n", __FUNCTION__, sgx_retv);
        abort();
    }
    return retv;
}
static int sgx_connect(int s, const struct sockaddr *addr, size_t addrlen)
{
    int retv;
    sgx_status_t sgx_retv;
    if((sgx_retv = ocall_sgx_connect(&retv, s, addr, addrlen)) != SGX_SUCCESS) {
        dlog("%s FAILED!, Error code = %d\n", __FUNCTION__, sgx_retv);
        abort();
    }
    return retv;
}
static int sgx_listen(int s, int backlog)
{
    int retv;
    sgx_status_t sgx_retv;
    if((sgx_retv = ocall_sgx_listen(&retv, s, backlog)) != SGX_SUCCESS) {
        dlog("%s FAILED!, Error code = %d\n", __FUNCTION__, sgx_retv);
        abort();
    }
    return retv;
}
static int sgx_accept(int s, struct sockaddr *addr, socklen_t *addrlen)
{
    int retv;
    sgx_status_t sgx_retv;
    if((sgx_retv = ocall_sgx_accept(&retv, s, addr, (size_t)*addrlen, addrlen))
            != SGX_SUCCESS) {
        dlog("%s FAILED!, Error code = %d\n", __FUNCTION__, sgx_retv);
        abort();
    }
    return retv;
}
static int sgx_fstat(int fd, struct stat *buf)
{
    int retv;
    sgx_status_t sgx_retv;
    if((sgx_retv = ocall_sgx_fstat(&retv, fd, buf)) != SGX_SUCCESS) {
        dlog("%s FAILED!, Error code = %d\n", __FUNCTION__, sgx_retv);
        abort();
    }
    return retv;
}
static ssize_t sgx_send(int s, const void *buf, size_t len, int flags)
{
    ssize_t retv;
    sgx_status_t sgx_retv;
    if((sgx_retv = ocall_sgx_send(&retv, s, buf, len, flags)) != SGX_SUCCESS) {
        dlog("%s FAILED!, Error code = %d\n", __FUNCTION__, sgx_retv);
        abort();
    }
    return retv;
}
static ssize_t sgx_recv(int s, void *buf, size_t len, int flags)
{
    ssize_t retv;
    sgx_status_t sgx_retv;
    if((sgx_retv = ocall_sgx_recv(&retv, s, buf, len, flags)) != SGX_SUCCESS) {
        dlog("%s FAILED!, Error code = %d\n", __FUNCTION__, sgx_retv);
        abort();
    }
    return retv;
}
static ssize_t sgx_sendto(int sockfd, const void *buf, size_t len, int flags,
        const struct sockaddr *dest_addr, size_t addrlen)
{
    ssize_t retv;
    sgx_status_t sgx_retv;
    if((sgx_retv = ocall_sgx_sendto(&retv, sockfd, buf, len, flags,
                    dest_addr, addrlen)) != SGX_SUCCESS) {
        dlog("%s FAILED!, Error code = %d\n", __FUNCTION__, sgx_retv);
        abort();
    }
    return retv;
}
static ssize_t sgx_recvfrom(int s, void *buf, size_t len, int flags,
        struct sockaddr *dest_addr, socklen_t* addrlen)
{
    ssize_t retv;
    sgx_status_t sgx_retv;
    if((sgx_retv = ocall_sgx_recvfrom(&retv, s, buf, len, flags,
                    dest_addr, *addrlen, addrlen)) != SGX_SUCCESS) {
        dlog("%s FAILED!, Error code = %d\n", __FUNCTION__, sgx_retv);
        abort();
    }
    return retv;
}
static int sgx_gethostname(char *name, size_t namelen)
{
    int retv;
    sgx_status_t sgx_retv;
    if((sgx_retv = ocall_sgx_gethostname(&retv, name, namelen)) != SGX_SUCCESS) {
        dlog("%s FAILED!, Error code = %d\n", __FUNCTION__, sgx_retv);
        abort();
    }
    return retv;
}

/*
int getaddrinfo(const char *node, const char *service,
        const struct addrinfo *hints,
        struct addrinfo **res);
        */
#define INFO_MAX 64
static addrinfo addrinfoarr[INFO_MAX];
static int sgx_getaddrinfo(const char *node, const char *service,
        const struct addrinfo *hints, unsigned long *res)
{
    // TODO: copy res to enclave mem region
    int retv, i;
    sgx_status_t sgx_retv;
    if((sgx_retv = ocall_sgx_getaddrinfo(&retv, node, service, hints,
                    res)) != SGX_SUCCESS) {
        dlog("%s FAILED!, Error code = %d\n", __FUNCTION__, sgx_retv);
        abort();
    }
    i = 0;
    for (addrinfo *rp = (addrinfo *)*res; i < INFO_MAX && rp != NULL;
            rp = rp->ai_next, ++i)
        cpy((char *)&addrinfoarr[i], (char *)rp, sizeof(addrinfo));
    if (i == INFO_MAX) {
        ocall_sgx_puts(&retv, "FAILED!, increase INFO_MAX!");
        abort();
    }
    return retv;
}

static char envret[0x100];
static char *sgx_getenv(const char *env)
{
    // TODO: copy res to enclave mem region
    char *retv;
    sgx_status_t sgx_retv;
    if((sgx_retv = ocall_sgx_getenv(&retv, env)) != SGX_SUCCESS) {
        dlog("%s FAILED!, Error code = %d\n", __FUNCTION__, sgx_retv);
        abort();
    }
    size_t i;
    for (i = 0;retv[i];++i) envret[i] = retv[i];
    envret[i] = '\0';
    return envret;
}
static int sgx_getsockname(int s, struct sockaddr *name, socklen_t *addrlen)
{
    int retv;
    sgx_status_t sgx_retv;
    if((sgx_retv = ocall_sgx_getsockname(&retv, s, name, (size_t)*addrlen,
                    addrlen)) != SGX_SUCCESS) {
        dlog("%s FAILED!, Error code = %d\n", __FUNCTION__, sgx_retv);
        abort();
    }
    return retv;
}
static int sgx_getsockopt(int s, int level, int optname, void *optval,
        socklen_t* optlen)
{
    int retv;
    sgx_status_t sgx_retv;
    if((sgx_retv = ocall_sgx_getsockopt(&retv, s, level, optname, optval,
                    *optlen, optlen)) != SGX_SUCCESS) {
        dlog("%s FAILED!, Error code = %d\n", __FUNCTION__, sgx_retv);
        abort();
    }
    return retv;
}
static struct servent serventret;
static struct servent *sgx_getservbyname(const char *name, const char *proto)
{
    // TODO: copy res to enclave mem region
    struct servent *retv;
    sgx_status_t sgx_retv;
    if((sgx_retv = ocall_sgx_getservbyname(&retv, name, proto)) != SGX_SUCCESS) {
        dlog("%s FAILED!, Error code = %d\n", __FUNCTION__, sgx_retv);
        abort();
    }
    cpy((char *)&serventret, (char *)retv, sizeof(struct servent));
    return &serventret;
}
static struct protoent protoentret;
static struct protoent *sgx_getprotobynumber(int proto)
{
    // TODO: copy res to enclave mem region
    struct protoent *retv;
    sgx_status_t sgx_retv;
    if((sgx_retv = ocall_sgx_getprotobynumber(&retv, proto)) != SGX_SUCCESS) {
        dlog("%s FAILED!, Error code = %d\n", __FUNCTION__, sgx_retv);
        abort();
    }
    cpy((char *)&protoentret, (char *)retv, sizeof(struct protoent));
    return &protoentret;
}
static int sgx_setsockopt(int s, int level, int optname, const void *optval, size_t optlen)
{
    int retv;
    sgx_status_t sgx_retv;
    if((sgx_retv = ocall_sgx_setsockopt(&retv, s, level, optname, optval, optlen)) != SGX_SUCCESS) {
        dlog("%s FAILED!, Error code = %d\n", __FUNCTION__, sgx_retv);
        abort();
    }
    return retv;
}
static unsigned short sgx_htons(unsigned short hostshort)
{
    unsigned short retv;
    sgx_status_t sgx_retv;
    if((sgx_retv = ocall_sgx_htons(&retv, hostshort)) != SGX_SUCCESS) {
        dlog("%s FAILED!, Error code = %d\n", __FUNCTION__, sgx_retv);
        abort();
    }
    return retv;
}
static unsigned long sgx_htonl(unsigned long hostlong)
{
    unsigned long retv;
    sgx_status_t sgx_retv;
    if((sgx_retv = ocall_sgx_htonl(&retv, hostlong)) != SGX_SUCCESS) {
        dlog("%s FAILED!, Error code = %d\n", __FUNCTION__, sgx_retv);
        abort();
    }
    return retv;
}
static unsigned short sgx_ntohs(unsigned short netshort)
{
    unsigned short retv;
    sgx_status_t sgx_retv;
    if((sgx_retv = ocall_sgx_ntohs(&retv, netshort)) != SGX_SUCCESS) {
        dlog("%s FAILED!, Error code = %d\n", __FUNCTION__, sgx_retv);
        abort();
    }
    return retv;
}
static unsigned long sgx_ntohl(unsigned long netlong)
{
    unsigned long retv;
    sgx_status_t sgx_retv;
    if((sgx_retv = ocall_sgx_ntohl(&retv, netlong)) != SGX_SUCCESS) {
        dlog("%s FAILED!, Error code = %d\n", __FUNCTION__, sgx_retv);
        abort();
    }
    return retv;
}
static sighandler_t sgx_signal(int signum, sighandler_t a)
{
    sighandler_t retv;
    sgx_status_t sgx_retv;
    if((sgx_retv = ocall_sgx_signal(&retv, signum, a)) != SGX_SUCCESS) {
        dlog("%s FAILED!, Error code = %d\n", __FUNCTION__, sgx_retv);
        abort();
    }
    return retv;
}
static int sgx_shutdown(int a, int b)
{
    int retv;
    sgx_status_t sgx_retv;
    if((sgx_retv = ocall_sgx_shutdown(&retv, a, b)) != SGX_SUCCESS) {
        dlog("%s FAILED!, Error code = %d\n", __FUNCTION__, sgx_retv);
        abort();
    }
    return retv;
}

static void sgx_push_gadget(unsigned long gadget)
{
    sgx_status_t sgx_retv;
    if((sgx_retv = ocall_sgx_push_gadget(gadget)) != SGX_SUCCESS) {
        dlog("%s FAILED!, Error code = %d\n", __FUNCTION__, sgx_retv);
        abort();
    }
}
