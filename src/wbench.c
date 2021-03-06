#include "wbench.h"

int main(int argc, char **argv)
{
    uint64_t start = time_us();
    char *url = calloc(argc, sizeof(char *));
    struct http_parser_url parts = {};

    if (parse_args(&cfg, &url, &parts, argc, argv))  {
        usage();
        exit(1);
    }

    char *schema    = copy_url_part(url, &parts, UF_SCHEMA);
    char *host      = copy_url_part(url, &parts, UF_HOST);
    char *port      = copy_url_part(url, &parts, UF_PORT);
    char *path      = copy_url_part(url, &parts, UF_PATH);
    char *query     = copy_url_part(url, &parts, UF_QUERY);
    char *service   = port ? port : "80";
    char *protocol  = schema ? schema : "ws";
    if (strcmp(protocol, "ws") != 0) {
        printf("Please use the [ws] protocol for stress testing \n\n");
        usage();
        exit(1);
    }
    char *uri = malloc(strlen(url));
    if (path != 0x00) {
        strcat(uri, path); 
    }
    if (query != 0x00) {
        strcat(uri, "?"); 
        strcat(uri, query); 
    }
    get_host_by_name(&host);

    cfg.host        = host;
    cfg.port        = service;
    cfg.protocol    = protocol;

    signal(SIGPIPE, SIG_IGN);

    thread *threads = calloc(cfg.threads, sizeof(thread));
    
    struct sockaddr_in server_addr_in;
    bzero(&server_addr_in, sizeof(server_addr_in));
    server_addr_in.sin_family = AI_FAMILY;
    server_addr_in.sin_port = htons(atoi(service));
    inet_pton(AF_INET, host, &server_addr_in.sin_addr);

    data_queue *queue = calloc(1, sizeof(data_queue));
    if (!cfg.script) {
        if (!populate_data_queue(queue, &cfg)) {
            printf("Please pass valid test data by [-d|-f] \n\n");
            exit(1);
        }
    }
    
    uint64_t i = 0;
    for (i; i < cfg.threads; i ++) {
        thread *t       = &threads[i];
        t->connections  = cfg.connections / cfg.threads;
        t->script       = cfg.script;
        t->addr         = server_addr_in;
        t->protocol     = protocol;
        t->host         = host;
        t->port         = service;
        t->uri          = uri;
        t->start        = time_us();
        t->timeout      = cfg.timeout;
        t->params       = queue;
        if (pthread_create(&t->thread, NULL, &thread_main, t) != 0) {
            fprintf(stderr, "unabled to create thread: %"PRIu64" %s \n", i, strerror(errno));
            exit(2);
        }
    }

    uint64_t complete = 0;
    uint64_t bytes = 0;
    errors errors = { 0 };

    printf("This is Wbench, Version %s \n", VERSION);
    printf("Copyright (C) 2017 Wettper, http://www.web-lovers.com \n");
    printf("\n");

    printf("Benchmarking %s (be patient)\n", host);

    uint64_t m = 0, k, j = 0;
    long double socket_req_times = 0;
    long double thread_req_times = 0;
    uint64_t connect_times[cfg.connections];
    uint64_t print_prethread = (uint64_t)(cfg.threads / 10);
    /*printf("print_prethread: %"PRIu64" \n", print_prethread);*/
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
        /*printf("m: %"PRIu64" \n", m);*/
        if (m >= print_prethread - 1) {
            printf("Completed %"PRIu64" requests \n", complete);
            m = 0;
        } else {
            m ++;
        }
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

    printf("Server Hostname: \t%s \n", host);
    printf("Server Port: \t\t%s \n", service);
    printf("\n");

    printf("Document Length: \t%"PRIu64" bytes\n", bytes);
    printf("\n");

    printf("Concurrency Level:\t%"PRIu64" \n", cfg.threads);
    printf("Time taken for tests: \t%.3f seconds \n", runtime_s);
    printf("Complete requests: \t%"PRIu64" \n", complete);

    if (bytes == 0) {
        printf("Failed requests: \t%"PRIu32" \n", complete);
    } else {
        printf("Failed requests: \t%"PRIu32" \n", errors.connect);
    }
    printf("Write errors: \t\t%"PRIu32" \n", errors.write);
    printf("Total transferred: \t%"PRIu32" bytes \n", bytes);
    printf("Requests per second: \t%.3f [#/sec] (mean) \n", req_per_s);
    printf("Time per request: \t%.3f [ms] (mean) \n", stime_pre_s);
    printf("Time per request: \t%.3f [ms] (mean, across all concurrent requests)\n", ttime_pre_s);
    printf("Transfer rate: \t\t%.3f [Kbytes/sec] received \n", bytes_per_s);
    printf("\n");

    printf("\n");

    qsort(connect_times, cfg.connections, sizeof(uint64_t), compare_fun);
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

