#ifndef FILTER_H
#define FILTER_H

#define HAVE_PTHREAD_H  1

#ifdef HAVE_PTHREAD_H
#include <getopt.h>
#endif

enum http_parser_user_fields {
    UF_SCHEMA   = 0
  , UF_HOST     = 1
  , UF_PORT     = 2
  , UF_PATH     = 3
  , UF_QUERY    = 4
  , UF_FRAGMENT = 5
  , UF_USERINFO = 6
  , UF_MAX      = 7
};

struct http_parser_url {
    uint16_t field_set;
    uint16_t port;

    struct {
        uint16_t off;
        uint16_t len;
    } field_data[UF_MAX];
};

enum state {
    /* important that this is > 0 */
    s_dead = 1 

  , s_start_req_or_res
  , s_res_or_resp_H
  , s_start_res
  , s_res_H
  , s_res_HT
  , s_res_HTT
  , s_res_HTTP
  , s_res_first_http_major
  , s_res_http_major
  , s_res_first_http_minor
  , s_res_http_minor
  , s_res_first_status_code
  , s_res_status_code
  , s_res_status_start
  , s_res_status
  , s_res_line_almost_done

  , s_start_req

  , s_req_method
  , s_req_spaces_before_url
  , s_req_schema
  , s_req_schema_slash
  , s_req_schema_slash_slash
  , s_req_server_start
  , s_req_server
  , s_req_server_with_at
  , s_req_path
  , s_req_query_string_start
  , s_req_query_string
  , s_req_fragment_start
  , s_req_fragment
  , s_req_http_start
  , s_req_http_H
  , s_req_http_HT
  , s_req_http_HTT
  , s_req_http_HTTP
  , s_req_first_http_major
  , s_req_http_major
  , s_req_first_http_minor
  , s_req_http_minor
  , s_req_line_almost_done

  , s_header_field_start
  , s_header_field
  , s_header_value_discard_ws
  , s_header_value_discard_ws_almost_done
  , s_header_value_discard_lws
  , s_header_value_start
  , s_header_value
  , s_header_value_lws

  , s_header_almost_done

  , s_chunk_size_start
  , s_chunk_size
  , s_chunk_parameters
  , s_chunk_size_almost_done

  , s_headers_almost_done
  , s_headers_done

  /* Important: 's_headers_done' must be the last 'header' state. All
   * states beyond this must be 'body' states. It is used for overflow
   * checking. See the PARSING_HEADER() macro.
   */

  , s_chunk_data
  , s_chunk_data_almost_done
  , s_chunk_data_done

  , s_body_identity
  , s_body_identity_eof

  , s_message_done

};

enum http_host_state
  {
    s_http_host_dead = 1
  , s_http_userinfo_start
  , s_http_userinfo
  , s_http_host_start
  , s_http_host_v6_start
  , s_http_host
  , s_http_host_v6
  , s_http_host_v6_end
  , s_http_host_v6_zone_start
  , s_http_host_v6_zone
  , s_http_host_port_start
  , s_http_host_port
};


#define T(v)    0
static const uint8_t normal_url_char[32] = {
/*   0 nul    1 soh    2 stx    3 etx    4 eot    5 enq    6 ack    7 bel  */
        0    |   0    |   0    |   0    |   0    |   0    |   0    |   0,
/*   8 bs     9 ht    10 nl    11 vt    12 np    13 cr    14 so    15 si   */
        0    | T(2)   |   0    |   0    | T(16)  |   0    |   0    |   0,
/*  16 dle   17 dc1   18 dc2   19 dc3   20 dc4   21 nak   22 syn   23 etb */
        0    |   0    |   0    |   0    |   0    |   0    |   0    |   0,
/*  24 can   25 em    26 sub   27 esc   28 fs    29 gs    30 rs    31 us  */
        0    |   0    |   0    |   0    |   0    |   0    |   0    |   0,
/*  32 sp    33  !    34  "    35  #    36  $    37  %    38  &    39  '  */
        0    |   2    |   4    |   0    |   16   |   32   |   64   |  128,
/*  40  (    41  )    42  *    43  +    44  ,    45  -    46  .    47  /  */
        1    |   2    |   4    |   8    |   16   |   32   |   64   |  128,
/*  48  0    49  1    50  2    51  3    52  4    53  5    54  6    55  7  */
        1    |   2    |   4    |   8    |   16   |   32   |   64   |  128,
/*  56  8    57  9    58  :    59  ;    60  <    61  =    62  >    63  ?  */
        1    |   2    |   4    |   8    |   16   |   32   |   64   |   0,
/*  64  @    65  A    66  B    67  C    68  D    69  E    70  F    71  G  */
        1    |   2    |   4    |   8    |   16   |   32   |   64   |  128,
/*  72  H    73  I    74  J    75  K    76  L    77  M    78  N    79  O  */
        1    |   2    |   4    |   8    |   16   |   32   |   64   |  128,
/*  80  P    81  Q    82  R    83  S    84  T    85  U    86  V    87  W  */
        1    |   2    |   4    |   8    |   16   |   32   |   64   |  128,
/*  88  X    89  Y    90  Z    91  [    92  \    93  ]    94  ^    95  _  */
        1    |   2    |   4    |   8    |   16   |   32   |   64   |  128,
/*  96  `    97  a    98  b    99  c   100  d   101  e   102  f   103  g  */
        1    |   2    |   4    |   8    |   16   |   32   |   64   |  128,
/* 104  h   105  i   106  j   107  k   108  l   109  m   110  n   111  o  */
        1    |   2    |   4    |   8    |   16   |   32   |   64   |  128,
/* 112  p   113  q   114  r   115  s   116  t   117  u   118  v   119  w  */
        1    |   2    |   4    |   8    |   16   |   32   |   64   |  128,
/* 120  x   121  y   122  z   123  {   124  |   125  }   126  ~   127 del */
        1    |   2    |   4    |   8    |   16   |   32   |   64   |   0, };

/* Tokens as defined by rfc 2616. Also lowercases them.
 *        token       = 1*<any CHAR except CTLs or separators>
 *     separators     = "(" | ")" | "<" | ">" | "@"
 *                    | "," | ";" | ":" | "\" | <">
 *                    | "/" | "[" | "]" | "?" | "="
 *                    | "{" | "}" | SP | HT
 */
static const char tokens[256] = {
/*   0 nul    1 soh    2 stx    3 etx    4 eot    5 enq    6 ack    7 bel  */
        0,       0,       0,       0,       0,       0,       0,       0,
/*   8 bs     9 ht    10 nl    11 vt    12 np    13 cr    14 so    15 si   */
        0,       0,       0,       0,       0,       0,       0,       0,
/*  16 dle   17 dc1   18 dc2   19 dc3   20 dc4   21 nak   22 syn   23 etb */
        0,       0,       0,       0,       0,       0,       0,       0,
/*  24 can   25 em    26 sub   27 esc   28 fs    29 gs    30 rs    31 us  */
        0,       0,       0,       0,       0,       0,       0,       0,
/*  32 sp    33  !    34  "    35  #    36  $    37  %    38  &    39  '  */
        0,      '!',      0,      '#',     '$',     '%',     '&',    '\'',
/*  40  (    41  )    42  *    43  +    44  ,    45  -    46  .    47  /  */
        0,       0,      '*',     '+',      0,      '-',     '.',      0,
/*  48  0    49  1    50  2    51  3    52  4    53  5    54  6    55  7  */
       '0',     '1',     '2',     '3',     '4',     '5',     '6',     '7',
/*  56  8    57  9    58  :    59  ;    60  <    61  =    62  >    63  ?  */
       '8',     '9',      0,       0,       0,       0,       0,       0,
/*  64  @    65  A    66  B    67  C    68  D    69  E    70  F    71  G  */
        0,      'a',     'b',     'c',     'd',     'e',     'f',     'g',
/*  72  H    73  I    74  J    75  K    76  L    77  M    78  N    79  O  */
       'h',     'i',     'j',     'k',     'l',     'm',     'n',     'o',
/*  80  P    81  Q    82  R    83  S    84  T    85  U    86  V    87  W  */
       'p',     'q',     'r',     's',     't',     'u',     'v',     'w',
/*  88  X    89  Y    90  Z    91  [    92  \    93  ]    94  ^    95  _  */
       'x',     'y',     'z',      0,       0,       0,      '^',     '_',
/*  96  `    97  a    98  b    99  c   100  d   101  e   102  f   103  g  */
       '`',     'a',     'b',     'c',     'd',     'e',     'f',     'g',
/* 104  h   105  i   106  j   107  k   108  l   109  m   110  n   111  o  */
       'h',     'i',     'j',     'k',     'l',     'm',     'n',     'o',
/* 112  p   113  q   114  r   115  s   116  t   117  u   118  v   119  w  */
       'p',     'q',     'r',     's',     't',     'u',     'v',     'w',
/* 120  x   121  y   122  z   123  {   124  |   125  }   126  ~   127 del */
       'x',     'y',     'z',      0,      '|',      0,      '~',       0 };

#ifndef ULLONG_MAX
# define ULLONG_MAX ((uint64_t) -1) /* 2^64-1 */
#endif

#ifndef MIN
# define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

#ifndef ARRAY_SIZE
# define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif

#ifndef BIT_AT
# define BIT_AT(a, i)                                                \
  (!!((unsigned int) (a)[(unsigned int) (i) >> 3] &                  \
   (1 << ((unsigned int) (i) & 7))))
#endif

#ifndef ELEM_AT
# define ELEM_AT(a, i, v) ((unsigned int) (i) < ARRAY_SIZE(a) ? (a)[(i)] : (v))
#endif
#define CR                  '\r'
#define LF                  '\n'
#define LOWER(c)            (unsigned char)(c | 0x20)
#define IS_ALPHA(c)         (LOWER(c) >= 'a' && LOWER(c) <= 'z') 
#define IS_NUM(c)           ((c) >= '0' && (c) <= '9')
#define IS_ALPHANUM(c)      (IS_ALPHA(c) || IS_NUM(c))
#define IS_HEX(c)           (IS_NUM(c) || (LOWER(c) >= 'a' && LOWER(c) <= 'f'))
#define IS_MARK(c)          ((c) == '-' || (c) == '_' || (c) == '.' || \
          (c) == '!' || (c) == '~' || (c) == '*' || (c) == '\'' || (c) == '(' || \
          (c) == ')')
#define IS_USERINFO_CHAR(c) (IS_ALPHANUM(c) || IS_MARK(c) || (c) == '%' || \
          (c) == ';' || (c) == ':' || (c) == '&' || (c) == '=' || (c) == '+' || \
          (c) == '$' || (c) == ',')

#define STRICT_TOKEN(c)     (tokens[(unsigned char)c])

#define TOKEN(c)            (tokens[(unsigned char)c])
#define IS_URL_CHAR(c)      (BIT_AT(normal_url_char, (unsigned char)c))
#define IS_HOST_CHAR(c)     (IS_ALPHANUM(c) || (c) == '.' || (c) == '-')



/*过滤规则*/
static struct option longopts[] = {
    { "connections", required_argument, NULL, 'c' }
  , { "duration",    required_argument, NULL, 'd' }
  , { "threads",     required_argument, NULL, 't' }
  , { "script",      required_argument, NULL, 's' }
  , { "header",      required_argument, NULL, 'H' }
  , { "latency",     no_argument,       NULL, 'L' }
  , { "timeout",     required_argument, NULL, 'T' }
  , { "help",        no_argument,       NULL, 'h' }
  , { "version",     no_argument,       NULL, 'v' }
  , { NULL,          0,                 NULL, '0' }
};

/*参数说明*/
static void usage()
{
    printf("Usage: wbench <options> <url>                               \n");
    printf("    Options:                                                \n");
    printf("        -c, --connection    <N> Connections to keep open    \n");
    printf("        -d, --duration      <T> Duration of test            \n");
    printf("        -t, --threads       <N> Number of threads to use    \n");
    printf("                                                            \n");
    printf("        -s, --script        <S> Load Lua script file        \n");
    printf("        -H, --header        <H> Add header to request       \n");
    printf("            --latency           Print latency statistics    \n");
    printf("            --timeout       <T> Socket/request timeout      \n");
    printf("        -v, --version       Print version details           \n");
    printf("                                                            \n");
    printf(" Numeric arguments may include a SI unit (1k, 1M, 1G)       \n");
    printf(" Time arguments may include a time unit (2s, 2m, 2h)        \n");
}

/*参数过滤*/
static int parse_args(struct config *cfg, char **url, struct http_parser_url *parts, char **headers, 
        int argc, char **argv)
{
    int c;
    char **header = headers;

    /*申请配置*/
    memset(cfg, 0, sizeof(struct config));
    cfg->threads        = THREADS_DEFAULT;
    cfg->connections    = CONNECTIONS_DEFAULT;
    cfg->duration       = DURATION_DEFAULT;
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
                if (scan_time(optarg, &cfg->duration)) return -1;
                break;
            case 's':
                cfg->script = optarg;
                break;
            case 'H':
                *header++ = optarg;
                break;
            case 'L':
                cfg->latency = true;
                break;
            case 'T':
                if (scan_time(optarg, &cfg->timeout)) return -1;
                cfg->timeout *= 1000;
                break;
            case 'v':
                printf("wbench %s ", VERSION);
                printf("Copyright (C) 2017 Wettper \n");
                break;
            case 'h':
            case '?':
            case ':':
            default:
                return -1;
        }
    }

    if (optind == argc || !cfg->threads || !cfg->duration)  return -1;

    if (!script_parse_url(argv[optind], parts)) {
        fprintf(stderr, "invalid URL: %s \n", argv[optind]);
        return -1;
    }

    if (!cfg->connections || cfg->connections < cfg->threads) {
        fprintf(stderr, "number of connections must be >= threads \n");
        return -1;
    }

    *url = argv[optind];
    *header = NULL;

    return 0;

}


typedef struct {
    int scale;
    char *base;
    char *units[];
} units;

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

int scan_metric(char *s, uint64_t *n)
{
    return scan_units(s, n, &metric_units);
}

int scan_time(char *s, uint64_t *n)
{
    return scan_units(s, n, &time_units_s);
}

static char *copy_url_part(char *url, struct http_parser_url *parts, 
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

int script_parse_url(char *url, struct http_parser_url *parts)
{
    if (!http_parser_parse_url(url, strlen(url), 0, parts)) {
        if (!(parts->field_set & (1 << UF_SCHEMA))) return 0;
        if (!(parts->field_set & (1 << UF_HOST))) return 0;
        return 1;
    }
    return 0;
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


int http_parser_parse_url(const char *buf, size_t buflen, int is_connect, struct http_parser_url *u)
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

#endif  /*FILTER_H*/
