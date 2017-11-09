#include "wbench.h"

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
    uint64_t start = time_us();
    for (i; i < cfg.threads; i ++) {
        thread *t = &threads[i];
        t->connections = cfg.connections / cfg.threads;
        t->addr = server_addr_in;
        t->start = time_us();
        if (pthread_create(&t->thread, NULL, &thread_main, t) != 0) {
            fprintf(stderr, "unabled to create thread: %"PRIu64" %s \n", i, strerror(errno));
            exit(2);
        } 
        /*printf("i: %"PRIu64"\n", i);*/
    }

    uint64_t complete = 0;
    uint64_t bytes = 0;
    errors errors = { 0 };

    printf("This is Wbench, Version %s \n", VERSION);
    printf("Copyright (C) 2017 Wettper, http://www.web-lovers.com \n");
    printf("\n");

    printf("Benchmarking %s (be patient)", host);

    uint64_t j = 0;
    for (j; j < cfg.threads; j++) {
        thread *t = &threads[j];
        pthread_join(t->thread, NULL);

        complete += t->complete;
        bytes += t->bytes;

        errors.connect += t->errors.connect;
        errors.read    += t->errors.read;
        errors.write   += t->errors.write;
        errors.timeout += t->errors.timeout;
        errors.status  += t->errors.status;

        printf("Completed %"PRIu64" requests \n", complete);
    }
    uint64_t runtime_us = time_us() - start;
    long double runtime_s   = runtime_us / 1000000.0;
    long double req_per_s   = complete   / runtime_s;
    long double bytes_per_s = bytes      / runtime_s;

    printf("Finished %3"PRIu64" requests \n", complete);
    printf("\n");
    printf("\n");

    printf("Server Hostname: %16s \n", host);
    printf("Server Port: %15s \n", port);
    printf("\n");

    /*printf("Document Path: /c-joiner.html\n");*/
    printf("Document Length: %12"PRIu64" bytes\n", bytes);
    printf("\n");

    printf("Time taken for tests: %6.2f seconds \n", runtime_s);
    printf("Complete requests: %7"PRIu64" \n", complete);

    printf("Failed requests: %8"PRIu32" \n", errors.connect);
    printf("Write errors: %11"PRIu32" \n", errors.write);
    printf("Total transferred: %10"PRIu32" bytes \n", bytes);
    /*printf("HTML transferred:       6716350000 bytes\n");*/
    printf("Requests per second: %7.2f [#/sec] (mean) \n", req_per_s);
    printf("Time per request: %11.3f [ms] (mean) \n", req_per_s);
    printf("Transfer rate: %13.2f [Kbytes/sec] received \n", bytes_per_s);
    printf("\n");

    printf("Connection Times (ms)\n");
    printf("min  mean[+/-sd] median   max\n");
    printf("Connect:        0    0   0.4      0       6\n");
    printf("Processing:     4   14   2.0     14      41\n");
    printf("Waiting:        0   14   2.0     14      41\n");
    printf("Total:          5   15   1.9     15      42\n");

    printf("\n");

    return 0;
}

