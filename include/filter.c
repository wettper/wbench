#include "filter.h"

units metric_units = {
    .scale = 1000,
    .base = "",
    .units = { "k", "M", "G", "T", "P", NULL }
};

units time_units_s = {
    .scale = 60,
    .base = "s",
    .units = { "m", "h", NULL }
};

static int scan_units(char *s, uint64_t *n, units *m)
{
    uint64_t base, scale = 1;
    char unit[3] = { 0, 0, 0 };
    int i, c;

    if ((c = sscanf(s, "%d %2s", &base, unit)) < 1) {
        return -1;
    }

    if (c == 2 && strncasecmp(unit, m->base, 3)) {
        for (i = 0; m->units[i] != NULL; i++) {
            scale *= m->scale;
            if (!strncasecmp(unit, m->units[i], 3)) break;
        }
        if (m->units[i] == NULL) return -1;
    }
    *n = base * scale;
    return 0;
}

static int scan_metric(char *s, uint64_t *n)
{
    return scan_units(s, n, &metric_units);
}

static int scan_time(char *s, uint64_t *n)
{
    return scan_units(s, n, &time_units_s);
}

static enum state parse_url_char(enum state s, const char ch)
{
    if (ch == ' ' || ch == '\r' || ch == '\n' || ch == '\t' || ch == '\f') {
        return s_dead;
    }
    switch (s) {
        case s_req_spaces_before_url:
            if (ch == '/' || ch == '*') {
                return s_req_path;
            }

            if (IS_ALPHA(ch)) {
                return s_req_schema;
            }
            break;
        case s_req_schema:
            if (IS_ALPHA(ch)) {
                return s;
            }
            if (ch == ':') {
                return s_req_schema_slash;
            }
            break;
        case s_req_schema_slash:
            if (ch == '/') {
                return s_req_schema_slash_slash;
            }
            break;
        case s_req_schema_slash_slash:
            if (ch == '/') {
                return s_req_server_start;
            }
            break;
        case s_req_server_with_at:
            if (ch == '@') {
                return s_dead;
            }
        /* FALLTHROUGH */
        case s_req_server_start:
        case s_req_server:
            if (ch == '/') {
                return s_req_path;
            }
            if (ch == '?') {
                return s_req_query_string_start;
            }
            if (ch == '@') {
                return s_req_server_with_at;
            }
            if (IS_USERINFO_CHAR(ch) || ch == '[' || ch == ']') {
                return s_req_server;
            }
            break;
        case s_req_path:
            if (IS_URL_CHAR(ch)) {
                return s;
            }
            switch (ch) {
                case '?':
                    return s_req_query_string_start;
                case '#':
                    return s_req_fragment_start;
            }
            break;
        case s_req_query_string_start:
        case s_req_query_string:
            if (IS_URL_CHAR(ch)) {
                return s_req_query_string;
            }
            switch (ch) {
                case '?':
                    /* allow extra '?' in query string */
                    return s_req_query_string;
                case '#':
                    return s_req_fragment_start;
            }
            break;
        case s_req_fragment_start:
            if (IS_URL_CHAR(ch)) {
                return s_req_fragment;
            }
            switch (ch) {
                case '?':
                    return s_req_fragment;
                case '#':
                    return s;
            }
            break;
        case s_req_fragment:
            if (IS_URL_CHAR(ch)) {
                return s;
            }
            switch (ch) {
                case '?':
                case '#':
                    return s;
            }
            break;
        default:
            break;
    } 
}

static enum http_host_state http_parse_host_char(enum http_host_state s, const char ch)
{
    switch(s) {
        case s_http_userinfo:
        case s_http_userinfo_start:
            if (ch == '@') {
                return s_http_host_start;
            }
            
            if (IS_USERINFO_CHAR(ch)) {
                return s_http_userinfo;
            }
            break;
        
        case s_http_host_start:
            if (ch == '[') {
                return s_http_host_v6_start;
            }
            
            if (IS_HOST_CHAR(ch)) {
                return s_http_host;
            }
            
            break;
        
        case s_http_host:
            if (IS_HOST_CHAR(ch)) {
                return s_http_host;
            }
            
            /* FALLTHROUGH */
        case s_http_host_v6_end:
            if (ch == ':') {
                return s_http_host_port_start;
            }
            
            break;
        
        case s_http_host_v6:
            if (ch == ']') {
                return s_http_host_v6_end;
            }
            
            /* FALLTHROUGH */
        case s_http_host_v6_start:
            if (IS_HEX(ch) || ch == ':' || ch == '.') {
                return s_http_host_v6;
            }
            
            if (s == s_http_host_v6 && ch == '%') {
                return s_http_host_v6_zone_start;
            }
            break;
        
        case s_http_host_v6_zone:
            if (ch == ']') {
                return s_http_host_v6_end;
            }
            
        /* FALLTHROUGH */
        case s_http_host_v6_zone_start:
            /* RFC 6874 Zone ID consists of 1*( unreserved / pct-encoded) */
            if (IS_ALPHANUM(ch) || ch == '%' || ch == '.' || ch == '-' || ch == '_' || ch == '~') {
                return s_http_host_v6_zone;
            }
            break;
        
        case s_http_host_port:
        case s_http_host_port_start:
            if (IS_NUM(ch)) {
                return s_http_host_port;
            }
            
            break;
        
        default:
            break;
    }
    return s_http_host_dead;
}


static int http_parse_host(const char * buf, struct http_parser_url *u, int found_at)
{
    enum http_host_state s;

    const char *p;
    
    size_t buflen = u->field_data[UF_HOST].off + u->field_data[UF_HOST].len;
    
    assert(u->field_set & (1 << UF_HOST));
    
    u->field_data[UF_HOST].len = 0;

    s = found_at ? s_http_userinfo_start : s_http_host_start;

    for (p = buf + u->field_data[UF_HOST].off; p < buf + buflen; p++) {
        enum http_host_state new_s = http_parse_host_char(s, *p);
        
        if (new_s == s_http_host_dead) {
            return 1;
        }

        switch(new_s) {
            case s_http_host:
                if (s != s_http_host) {
                    u->field_data[UF_HOST].off = p - buf;
                }
                u->field_data[UF_HOST].len++;
                break;
            
            case s_http_host_v6:
                if (s != s_http_host_v6) {
                    u->field_data[UF_HOST].off = p - buf;
                }
                u->field_data[UF_HOST].len++;
                break;

            case s_http_host_v6_zone_start:
            case s_http_host_v6_zone:
                u->field_data[UF_HOST].len++;
                break;

            case s_http_host_port:
                if (s != s_http_host_port) {
                    u->field_data[UF_PORT].off = p - buf;
                    u->field_data[UF_PORT].len = 0;
                    u->field_set |= (1 << UF_PORT);
                }
                u->field_data[UF_PORT].len++;
                break;

            case s_http_userinfo:
                if (s != s_http_userinfo) {
                    u->field_data[UF_USERINFO].off = p - buf ;
                    u->field_data[UF_USERINFO].len = 0;
                    u->field_set |= (1 << UF_USERINFO);
                }
                u->field_data[UF_USERINFO].len++;
                break;

            default:
                break;
        }
        s = new_s;
    }

    /* Make sure we don't end somewhere unexpected */
    switch (s) {
        case s_http_host_start:
        case s_http_host_v6_start:
        case s_http_host_v6:
        case s_http_host_v6_zone_start:
        case s_http_host_v6_zone:
        case s_http_host_port_start:
        case s_http_userinfo:
        case s_http_userinfo_start:
            return 1;
        default:
            break;
    }
    
    return 0;
}

static int http_parser_parse_url(const char *buf, size_t buflen, int is_connect, 
        struct http_parser_url *u)
{
    enum state s;
    const char *p;
    enum http_parser_user_fields uf, old_uf;
    int found_at = 0;

    u->port = u->field_set = 0;
    s = is_connect ? s_req_server_start : s_req_spaces_before_url;
    old_uf = UF_MAX;

    for (p = buf; p < buf + buflen; p ++) {
        s = parse_url_char(s, *p);

        switch (s) {
            case s_dead:
                return 1;

            case s_req_schema_slash:
            case s_req_schema_slash_slash:
            case s_req_server_start:
            case s_req_query_string_start:
            case s_req_fragment_start:
                continue;

            case s_req_schema:
                uf = UF_SCHEMA;
                break;

            case s_req_server_with_at:
                found_at = 1;

            case s_req_server:
                uf = UF_HOST;
                break;

            case s_req_path:
                uf = UF_PATH;
                break;

            case s_req_query_string:
                uf = UF_QUERY;
                break;

            case s_req_fragment:
                uf = UF_FRAGMENT;
                break;

            default:
                assert(!"Unexpected state .");
        }

        if (uf == old_uf) {
            u->field_data[uf].len++;
            continue;
        }

        u->field_data[uf].off = p - buf;
        u->field_data[uf].len = 1;

        u->field_set |= (1 << uf);
        old_uf = uf;
    }

    if ((u->field_set & (1 << UF_SCHEMA)) && 
        (u->field_set & (1 << UF_HOST)) == 0) {
        return 1;
    }

    if (u->field_set & 1 << UF_HOST) {
        if (http_parse_host(buf, u, found_at) != 0) {
            return 1;
        }
    }

    if (is_connect && u->field_set != ((1 << UF_HOST) | (1 << UF_PORT))) {
        return 1;
    }

    if (u->field_set & (1 << UF_PORT)) {
        unsigned long v = strtoul(buf + u->field_data[UF_PORT].off, NULL, 10);
        if (v > 0xffff) {
            return 1;
        }

        u->port = (uint16_t) v;
    }

    return 0;

}


static int script_parse_url(char *url, struct http_parser_url *parts)
{
    if (!http_parser_parse_url(url, strlen(url), 0, parts)) {
        if (!(parts->field_set & (1 << UF_SCHEMA))) return 0;
        if (!(parts->field_set & (1 << UF_HOST))) return 0;
        return 1;
    }
    return 0;
}


/*参数说明*/
void usage()
{
    printf("Usage: wbench <options> <url>                               \n");
    printf("    Options:                                                \n");
    printf("        -c, --connection    <N> Connections to keep open    \n");
    printf("        -t, --threads       <N> Number of threads to use    \n");
    printf("                                                            \n");
    printf("        -d, --data          <H> Add data to request         \n");
    printf("            --timeout       <T> Socket/request timeout      \n");
    printf("        -v, --version       Print version details           \n");
    printf("                                                            \n");
    printf(" Numeric arguments may include a SI unit (1k, 1M, 1G)       \n");
    printf(" Time arguments may include a time unit (2s, 2m, 2h)        \n");
}

/*参数过滤*/
int parse_args(struct config *cfg, char **url, struct http_parser_url *parts, 
        int argc, char **argv)
{
    int c;

    /*申请配置*/
    memset(cfg, 0, sizeof(struct config));
    cfg->threads        = THREADS_DEFAULT;
    cfg->connections    = CONNECTIONS_DEFAULT;
    cfg->timeout        = SOCKET_TIMEOUT_DEFAULT;

    while ((c = getopt_long(argc, argv, "t:c:d:s:H:T:Lrv?", longopts, NULL)) != -1) {
        switch (c) {
            case 't':
                if (scan_metric(optarg, &cfg->threads)) return -1;
                break;
            case 'c':
                if (scan_metric(optarg, &cfg->connections)) return -1;
                break;
            case 'd':
                strcpy(cfg->data, optarg);
                break;
            case 'T':
                if (scan_time(optarg, &cfg->timeout)) return -1;
                cfg->timeout *= 1000;
                break;
            case 'v':
                printf("This is Wbench, Version %s \n", VERSION);
                printf("Copyright (C) 2017 Wettper, http://www.web-lovers.com \n");
                break;
            case 'h':
            case '?':
            case ':':
            default:
                return -1;
        }
    }

    if (optind == argc || !cfg->threads)  return -1;

    if (!script_parse_url(argv[optind], parts)) {
        fprintf(stderr, "invalid URL: %s \n", argv[optind]);
        return -1;
    }

    if (!cfg->connections || cfg->connections < cfg->threads) {
        fprintf(stderr, "number of connections must be >= threads \n");
        return -1;
    }

    *url = argv[optind];

    return 0;

}

char *copy_url_part(char *url, struct http_parser_url *parts, 
        enum http_parser_user_fields field)
{
    char *part = NULL;
    
    if (parts->field_set & (1 << field)) {
        uint16_t off = parts->field_data[field].off;
        uint16_t len = parts->field_data[field].len;
        part = calloc(1, len + 1 * sizeof(char));
        memcpy(part, &url[off], len);
    }

    return part;
}

