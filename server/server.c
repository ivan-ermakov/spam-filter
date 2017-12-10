#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>
#include "lib/sf.h"
#include "lib/buf.h"
#include "lib/spam_filter.h"
#include "lib/protocol.h"
#include "client.h"
#include "server.h"

struct server_s
{
    uv_tcp_t sock;
    spam_filter_t* sf;
    uv_signal_t sigterm;
    uv_signal_t sigint;
};

static void on_new_connection(uv_stream_t* serv_sock, int status);

void server_signal_close(uv_signal_t* signal_handle, int signum)
{
    server_free((server_t*) signal_handle->data);
}

server_t* server_init(int port)
{
    server_t* serv = malloc(sizeof(server_t));
    if (!serv)
        goto fail;

    serv->sf = spam_filter_init("rules.txt");
    if (!serv->sf)
    {
        printf("No rules\n");
        goto free_serv;
    }

    uv_tcp_init(uv_default_loop(), &serv->sock);
    serv->sock.data = serv;

    struct sockaddr_in addr;
    uv_ip4_addr("0.0.0.0", port, &addr);
    uv_tcp_bind(&serv->sock, (const struct sockaddr*) &addr, 0);
    int ret = uv_listen((uv_stream_t*) &serv->sock, DEFAULT_BACKLOG, on_new_connection);
    if (ret)
    {
        fprintf(stderr, "Listen error %s\n", uv_strerror(ret));
        goto free_sf;
    }

    /* signals */

    if (uv_signal_init(uv_default_loop(), &serv->sigterm))
        goto free_sigterm;

    serv->sigterm.data = serv;
    if (uv_signal_start(&serv->sigterm, server_signal_close, SIGTERM))
        goto free_sigterm;

    if (uv_signal_init(uv_default_loop(), &serv->sigint))
        goto free_sigint;

    serv->sigint.data = serv;
    if (uv_signal_start(&serv->sigint, server_signal_close, SIGINT))
        goto free_sigint;

    uv_unref((uv_handle_t *) &serv->sigterm);
    uv_unref((uv_handle_t *) &serv->sigint);

    printf("Listening on port %d\n", port);
    goto done;

free_sigterm:
    uv_close((uv_handle_t*) &serv->sigterm, NULL);
free_sigint:
    uv_close((uv_handle_t*) &serv->sigint, NULL);
free_sf:
    spam_filter_free(serv->sf);
free_serv:
    free(serv);
fail:
    return NULL;

done:
    return serv;
}

void server_free(server_t* serv)
{
    spam_filter_free(serv->sf);
    uv_close((uv_handle_t*) &serv->sock, NULL);
    uv_close((uv_handle_t*) &serv->sigint, NULL);
    uv_close((uv_handle_t*) &serv->sigterm, NULL);
}

static void on_write(uv_write_t* req, int status)
{
    client_t* client = (client_t*) req->handle->data;

    if (status < 0)
        fprintf(stderr, "Write error: %s\n", uv_strerror(status));

    free(req);
    client_close(client);
}

static void on_read(uv_stream_t* sock, ssize_t nread, const uv_buf_t* buf)
{
    client_t* client = (client_t*) sock->data;

    if (nread < 0)
    {
        fprintf(stderr, "Read error: %s\n", uv_strerror(nread));
        goto free_buf;
    }
    else if (nread == 0)
        return;

    int ret;
    if ((ret = buf_append(client_get_buf(client), buf->base, nread)) < 0)
    {
        fprintf(stderr, "Buffer error: %s\n", uv_strerror(ret));
        goto free_buf;
    }

    free(buf->base);

    char* msg = NULL;
    ret = sf_protocol_read_request(client_get_buf(client), &msg);
    if (ret == SF_PROTOCOL_INCOMPLETE_MSG)
        return;
    else if (ret < 0)
        goto fail;

    uv_read_stop(sock);

    if (!msg)
    {
        fprintf(stderr, "Msg error\n");
        goto fail;
    }

    printf("Client: '%s'\n", msg);

    free(client_get_buf(client)->base);

    /* Response */

    msg_type_t msg_type;
    ret = spam_filter_check_msg(client_get_serv(client)->sf, msg, &msg_type);
    free(msg);

    client_set_buf(client, uv_buf_init(NULL, 0));
    sf_protocol_write_response(client_get_buf(client), ret, msg_type);

    uv_write_t* write_req = (uv_write_t*) malloc(sizeof(uv_write_t));
    write_req->data = client;
    uv_write(write_req, (uv_stream_t*) sock, client_get_buf(client), 1, on_write);
    goto done;

free_buf:
    free(buf->base);
fail:
    client_close(client);
done:
    return;
}

static void on_new_connection(uv_stream_t* serv_sock, int status)
{
    if (status < 0)
    {
        fprintf(stderr, "New connection error %s\n", uv_strerror(status));
        return;
    }

    client_t* client = client_init((server_t*) serv_sock->data);
    if (!client)
        return;

    if ((status = uv_accept(serv_sock, (uv_stream_t*) client_get_sock(client))) == 0)
        uv_read_start((uv_stream_t*) client_get_sock(client), alloc_buffer, on_read);
    else
    {
        fprintf(stderr, "Failed to accept connection %s\n", uv_strerror(status));
        client_free(client);
    }
}
