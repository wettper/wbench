#include "wbench.h"

int main(int argc, char **argv)
{
    uint64_t start = time_us();
    char *url = calloc(1, argc * sizeof(char *));
    struct http_parser_url parts = {};

    /*参数过滤*/
    if (parse_args(&cfg, &url, &parts, argc, argv))  {
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
        t->host = host;
        t->port = service;
        t->start = time_us();
        t->timeout = cfg.timeout;
        strcpy(t->params, cfg.data);
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

    printf("Benchmarking %s (be patient)\n", host);

    uint64_t k, j = 0;
    long double socket_req_times = 0;
    long double thread_req_times = 0;
    uint64_t connect_times[cfg.connections];
    for (j; j < cfg.threads; j++) {
        thread *t = &threads[j];
        pthread_join(t->thread, NULL);

        k = 0;
        for (k; k < t->connections; k ++) {
            socket_info s = t->socket_info[k];
            uint64_t timediff = (uint64_t)(s.end - s.start);
            socket_req_times += timediff;
            connect_times[(complete + k)] = (uint64_t)(timediff / 1000.00);
        }
        thread_req_times += (uint64_t)(t->end - t->start);

        errors.connect += t->errors.connect;
        errors.read    += t->errors.read;
        errors.write   += t->errors.write;
        errors.timeout += t->errors.timeout;
        errors.status  += t->errors.status;

        complete += t->complete;
        bytes += t->bytes;
        printf("Completed %"PRIu64" requests \n", complete);
    }
    uint64_t runtime_us = time_us() - start;
    double runtime_s    = runtime_us / 1000000.000;
    double stime_pre_s  = (socket_req_times / 1000.000) / complete;
    double ttime_pre_s  = (thread_req_times / 1000.000) / complete;
    double req_per_s    = complete   / runtime_s;
    double bytes_per_s  = bytes      / runtime_s;

    printf("Finished %3"PRIu64" requests \n", complete);
    printf("\n");
    printf("\n");

    return 0;

    printf("Server Hostname: %16s \n", host);
    printf("Server Port: %15s \n", port);
    printf("\n");

    printf("Document Length: \t%"PRIu64" bytes\n", bytes);
    printf("\n");

    printf("Concurrency Level:\t%"PRIu64" \n", cfg.threads);
    printf("Time taken for tests: \t%.3f seconds \n", runtime_s);
    printf("Complete requests: \t%"PRIu64" \n", complete);

    printf("Failed requests: \t%"PRIu32" \n", errors.connect);
    printf("Write errors: \t\t%"PRIu32" \n", errors.write);
    printf("Total transferred: \t%"PRIu32" bytes \n", bytes);
    printf("Requests per second: \t%.3f [#/sec] (mean) \n", req_per_s);
    printf("Time per request: \t%.3f [ms] (mean) \n", stime_pre_s);
    printf("Time per request: \t%.3f [ms] (mean, across all concurrent requests)\n", ttime_pre_s);
    printf("Transfer rate: \t\t%.3f [Kbytes/sec] received \n", bytes_per_s);
    printf("\n");

    /*printf("Connection Times (ms)\n");*/
    /*printf("\t\tmin\tmean[+/-sd]\tmedian\tmax\n");*/
    /*printf("Connect:\t0\t0\t0.4\t0\t6\n");*/
    /*printf("Processing:\t4\t14\t2.0\t14\t41\n");*/
    /*printf("Waiting:\t0\t14\t2.0\t14\t41\n");*/
    /*printf("Total:\t\t5\t15\t1.9\t15\t42\n");*/

    printf("\n");

    qsort(connect_times, cfg.connections, sizeof(uint64_t), compare_fun);
    /*uint64_t n = 0;*/
    /*for (n; n < cfg.connections; n ++) {*/
        /*printf("connect_times: %"PRIu64" \n", connect_times[n]);*/
    /*}*/
    printf("Percentage of the requests served within a certain time (ms) \n");
    printf("  50%\t%"PRIu64" \n", connect_times[(uint64_t)(cfg.connections * 0.5 - 1)]);
    printf("  66%\t%"PRIu64" \n", connect_times[(uint64_t)(cfg.connections * 0.66 - 1)]);
    printf("  75%\t%"PRIu64" \n", connect_times[(uint64_t)(cfg.connections * 0.75 - 1)]);
    printf("  80%\t%"PRIu64" \n", connect_times[(uint64_t)(cfg.connections * 0.80 - 1)]);
    printf("  90%\t%"PRIu64" \n", connect_times[(uint64_t)(cfg.connections * 0.90 - 1)]);
    printf("  95%\t%"PRIu64" \n", connect_times[(uint64_t)(cfg.connections * 0.95 - 1)]);
    printf("  98%\t%"PRIu64" \n", connect_times[(uint64_t)(cfg.connections * 0.98 - 1)]);
    printf("  99%\t%"PRIu64" \n", connect_times[(uint64_t)(cfg.connections * 0.99 - 1)]);
    printf("  100%\t%"PRIu64" (longest request) \n", connect_times[(uint64_t)(cfg.connections - 1)]);

    return 0;
}

