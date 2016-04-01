#ifndef _HTTP_CONN_H_
#define _HTTP_CONN_H_

typedef enum http_state
{
    REQUEST_LINE_RECV;
    REQUEST_HEADERS_RECV;
} http_state_t;

typedef struct http_conn
{
    int fd;
    int state;
} http_conn_t;
#endif /* _HTTP_CONN_H_ */
