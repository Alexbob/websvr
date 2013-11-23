/*
 Copyright (c) 2013 by XiongBo.  All Rights Reserved.
*/

#include "head/unp.h"
#include "head/svr.h"
#include "head/log.h"
#include "head/conf.h"

#include "evlib/include/event.h"
#include "evlib/include/evhttp.h"
#include "evlib/include/http-internal.h"

#define MAX_DIR_LENGTH 50

/**
 * Callback function
 * @param req
 * @param arg
 */
static void RequestHandler(struct evhttp_request *req, void *arg)
{

    const char *rootdir = arg;
    const char *uri = req->uri;

    /* This holds the content we're sending. */
    struct evbuffer *evb = evbuffer_new();

    struct evhttp_connection *evcon = req->evcon;
    struct evbuffer *conn_inpuf_buf = evcon->input_buffer;

    /* Record User request and agent to log */
    // libevent bug EVBUFFER_LENGTH(x)	(x)->len
    conn_inpuf_buf->off = conn_inpuf_buf->totallen;
    char *first_line = evbuffer_readline(conn_inpuf_buf);
    char *user_agent = evhttp_find_header(req->input_headers, "User-Agent");
    AccessLog(req->remote_host, first_line, user_agent);

    evbuffer_add_printf(evb, "<h1>\n hello </h1>\n");

    evhttp_add_header(req->output_headers, "Content-Type", "text/html");

    evhttp_send_reply(req, 200, "OK", evb);

    evbuffer_free(evb);
}

/**
 * Run http server
 */
void RunServer()
{
    /* Parse configure file */
    char *http_opt_dir = (char *) malloc(sizeof(char) * (MAX_DIR_LENGTH));
    ParseCfgSeverDir(http_opt_dir);
    int http_opt_port = ParseCfgServerPort();
    char *http_opt_addr = "0.0.0.0";

    char info[100];
    sprintf(info, "Server started at port no. %d with root directory as %s", http_opt_port, http_opt_dir);
    ErrorLog("NORMAL", info);

    /* Initialize http object */
    struct event_base *base;
    struct evhttp *http;

    base = event_init();
    if (!base)
    {
        ErrorLog("FATAL", "Couldn't create an event_base: exiting");
        exit(EXIT_FAILURE);
    }

    /* Create a new evhttp object to handle requests. */
    http = evhttp_new(base);
    if (!http)
    {
        ErrorLog("FATAL", "Couldn't create evhttp: exiting");
        exit(EXIT_FAILURE);
    }
    /* Binds an HTTP server on the specified address and port.*/
    if (evhttp_bind_socket(http, http_opt_addr, http_opt_port) != 0)
    {
        ErrorLog("FATAL", "Couldn't bind http server: exiting");
        exit(EXIT_FAILURE);
    }


    /* Set callback function */
    evhttp_set_gencb(http, RequestHandler, http_opt_dir);

    /* loop to handle request */
    event_base_dispatch(base);
}
