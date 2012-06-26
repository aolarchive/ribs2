#ifndef _RIBIFY__H_
#define _RIBIFY__H_

int ribs_getaddrinfo(const char *node, const char *service,
                     const struct addrinfo *hints,
                     struct addrinfo **results);

#endif // _RIBIFY__H_
