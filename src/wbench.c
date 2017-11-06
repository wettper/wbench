#include "wbench.h"
#include "config.h"
#include "filter.h"
#include "stats.h"
#include "thread.h"
#include "socket.h"

int main(int argc, char **argv)
{
    char *url, **headers = calloc(1, argc * sizeof(char *));
    struct http_parser_url parts = {};

    /*参数过滤*/
    if (parse_args(&cfg, &url, &parts, headers, argc, argv))  {
        usage();
        exit(1);
    }

    char *schema    = copy_url_part(url, &parts, UF_SCHEMA);
    char *host      = copy_url_part(url, &parts, UF_HOST);
    char *port      = copy_url_part(url, &parts, UF_PORT);
    char *service   = port ? port : schema;

    cfg.host = host;
    cfg.port = port;

    signal(SIGPIPE, SIG_IGN);

    thread *threads = calloc(1, cfg.threads * sizeof(thread));
    
    struct sockaddr_in server_addr_in;
    bzero(&server_addr_in, sizeof(server_addr_in));
    server_addr_in.sin_family = AI_FAMILY;
    server_addr_in.sin_port = htons(atoi(service));
    inet_pton(AF_INET, host, &server_addr_in.sin_addr);
    
    uint64_t i = 0;
    for (i; i < cfg.threads; i ++) {
        thread *t = &threads[i];
        t->connections = cfg.connections / cfg.threads;
        t->addr = server_addr_in;
        if (pthread_create(&t->thread, NULL, &thread_main, t) != 0) {
            fprintf(stderr, "unabled to create thread: %"PRIu64" %s \n", i, strerror(errno));
            exit(2);
        } 
        /*printf("i: %"PRIu64"\n", i);*/
    }

    uint64_t start = time_us();
    uint64_t complete = 0;
    uint64_t bytes = 0;
    errors errors = { 0 };

    sleep(cfg.duration);
    
    uint64_t j = 0;
    for (j; j < cfg.threads; j++) {
        thread *t = &threads[j];
        pthread_join(t->thread, NULL);

        complete += t->complete;
        bytes += t->bytes;

        errors.connect += t->errors.connect;
        errors.write += t->errors.write;
        errors.read += t->errors.read;
        errors.timeout += t->errors.timeout;
        errors.status += t->errors.status;
    }
    
    return 0;
}

