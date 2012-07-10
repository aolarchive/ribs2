#ifndef _RIBIFY__H_
#define _RIBIFY__H_

#ifdef UGLY_GETADDRINFO_WORKAROUND
/* In this ugly mode ribs_getaddrinfo is not blocking but getaddrinfo_a creates threads internally */
int ribs_getaddrinfo(const char *node, const char *service,
                     const struct addrinfo *hints,
                     struct addrinfo **results);
#else
/* In this mode ribs_getaddrinfo is blocking */
#define ribs_getaddrinfo getaddrinfo
#endif

#endif // _RIBIFY__H_
