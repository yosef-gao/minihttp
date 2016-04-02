#include "common.h"
#include "sysutil.h"
#include "threadpool.h"

#define ROOT_PATH "./public"

extern char **environ;

void* doit(void* arg);
void client_error(int fd, char *cause, char *err_num, char *short_msg, char *long_msg);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, size_t filesize);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void get_filetype(char *filename, char *filetype);

int main(int argc, char *argv[])
{
    threadpool_t *pool = threadpool_init(20, 40); // 20个线程，40工作

    int listenfd, clientfd;

    char ch;
    int port_flag = 0;
    char *port = NULL;
    while ((ch = getopt(argc, argv, "p:")) != -1)
    {
        switch (ch)
        {
            case 'p':
                port = optarg;
                printf("port is %s\n", port);
                port_flag = 1;
                break;
            case '?':
                fprintf(stderr, "Unknown command opt: %c\n", (char)optopt);
                break;
        }
    }

    if (!port_flag)
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* 设置信号 */
    signal(SIGPIPE, SIG_IGN);

    listenfd = tcp_server(NULL, atoi(port));
    while (1)
    {
        if ((clientfd = accept(listenfd, NULL, NULL)) < 0)
        {
            err_sys("accept");
        }
        
        /* 用线程池来处理连接 */
        int *tmp_fd = malloc(sizeof(int));
        *tmp_fd = clientfd;
        threadpool_add_job(pool, doit, (void *)tmp_fd);
    }

    // threadpool_destory(pool);
    // 这里应该接收ctrl c 信号，然后调用threadpool_destory函数
    exit(EXIT_SUCCESS);
}

// 处理HTTP事务
void* doit(void *arg)
{
    int fd = *((int *)arg);
    int is_static;
    struct stat sbuf;
    char buf[LINE_MAX] = {0}, method[LINE_MAX] = {0}, uri[LINE_MAX] = {0}, version[LINE_MAX] = {0};
    char filename[LINE_MAX] = {0}, cgiargs[LINE_MAX] = {0};

    do
    {
        // 读入请求行
        if (readline(fd, buf, sizeof(buf)) > 0)
        {
            if (sscanf(buf, "%s %s %s", method, uri, version) != 3)
            {
                client_error(fd, "GET", "501", "Not Impelemented", "error request line");
                break;
            }
        }

        // 读入请求报头,简单打印到标准输出
        while (readline(fd, buf, sizeof(buf)) > 0)
        {
            if (strcmp(buf, "\r\n") == 0)
                break;
        }

        /* 过滤所有非GET请求*/
        if (strcmp(method, "GET"))
        {
            client_error(fd, method, "501", "Not Impelementd", "minihttp does not impelement this method");
            break;
        }

        is_static = parse_uri(uri, filename, cgiargs);
        if (stat(filename, &sbuf) < 0)
        {
            client_error(fd, filename, "404", "Not found", "miniftp couldn't find this file");
            break;
        }

        if (is_static) /* Serve static content */
        {
            if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode))
            {
                client_error(fd, filename, "403", "Forbidden", "minihttp couldn't read the file");
                break;
            }
            serve_static(fd, filename, sbuf.st_size);
        }
        else /* Serve dynamic content */
        {
            if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode))
            {
                client_error(fd, filename, "403", "Forbidden", "minihttp couldn't run the CGI program");
                break;
            } 
            serve_dynamic(fd, filename, cgiargs);
        }
    } while (0);

    close(fd);
    free((int *)arg);
    return NULL;
}

void client_error(int fd, char *cause, char *err_num, char *short_msg, char *long_msg)
{
    char buf[MAX_BUF], body[MAX_BUF];

    // Build the HTTP response
    sprintf(body, "<html><title>minihttp error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, err_num, short_msg);
    sprintf(body, "%s<p>%s: %s\r\n", body, long_msg, cause);
    sprintf(body, "%s<hr><em>The minihttp web server</em>\r\n", body);

    // Print the HTTP response
    sprintf(buf, "HTTP/1.0 %s %s\r\n", err_num, short_msg);
    sprintf(buf, "%sContent-type: text/html\r\n", buf);
    sprintf(buf, "%sContent-length: %d\r\n\r\n", buf, (int)strlen(body));

    if (writen(fd, buf, strlen(buf)) < 0)
    {
        return;
    }
    if (writen(fd, body, strlen(body)) < 0)
    {
        return;
    }
}

int parse_uri(char *uri, char *filename, char *cgiargs)
{
    // char *ptr;

    if (!strstr(uri, "cgi-bin")) // static content
    {
        strcpy(cgiargs, "");
        strcpy(filename, ROOT_PATH);
        strcat(filename, uri);
        if (uri[strlen(uri) - 1] == '/')
            strcat(filename, "index.html");
        return 1;
    }
    else // dynamic content
        return 0;
    /*
    {
        ptr = index(uri, '?');
        if (ptr)
        {
            strcpy(cgiargs, ptr + 1);
            *ptr = '\0';
        }
        else
        {
            strcpy(cgiargs, "");
            strcpy(filename, ".");
            strcat(filename, uri);
            return 0;
        }
    }*/

    return 0;
}

void serve_static(int fd, char *filename, size_t filesize)
{
    int srcfd;
    char filetype[LINE_MAX], buf[LINE_MAX];

    /* Send response headers to client */
    get_filetype(filename, filetype);
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    sprintf(buf, "%sServer: minihttp web server\r\n", buf);
    sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
    sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
    writen(fd, buf, strlen(buf));

    /* Send response body to client */
    srcfd = open(filename, O_RDONLY, 0);
    if (srcfd < 0)
        err_sys("open");

    Sendfile(srcfd, fd, filesize);

    close(srcfd);
}

void get_filetype(char *filename, char *filetype)
{
    if (strstr(filename, ".html"))
        strcpy(filetype, "text/html");
    else if (strstr(filename, ".gif"))
        strcpy(filetype, "image/gif");
    else if (strstr(filename, ".jpg"))
        strcpy(filetype, "image/jpge");
    else
        strcpy(filetype, "text/plain");
}

void serve_dynamic(int fd, char *filename, char *cgiargs)
{
    char buf[LINE_MAX], *emptylist[] = {NULL};

    /* Return first part of HTTP response */
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    writen(fd, buf, strlen(buf));
    sprintf(buf, "Server: minihttp web server\r\n");
    writen(fd, buf, strlen(buf));

    if (fork() == 0)
    {
        setenv("QUERY_STRING", cgiargs, 1);
        dup2(fd, STDOUT_FILENO);
        execve(filename, emptylist, environ);
    }
    while (waitpid(-1, NULL, 0) <= 0); /* parent waits for and reaps child */ 
}
