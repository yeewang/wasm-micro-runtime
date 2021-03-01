
#ifndef CONNECTION_UART
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#else
#include <termios.h>
#endif

#include <arpa/inet.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/types.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <strings.h>

#include "runtime_lib.h"
#include "runtime_timer.h"
#include "native_interface.h"
#include "app_manager_export.h"
#include "bh_platform.h"
#include "runtime_sensor.h"
#include "bi-inc/attr_container.h"
#include "module_wasm_app.h"
#include "wasm_export.h"

#define MAX 2048

#define SA struct sockaddr
static char *host_address = "127.0.0.1";
static int port = 8888;

extern void init_sensor_framework();
extern void exit_sensor_framework();
extern void exit_connection_framework();
extern int aee_host_msg_callback(void *msg, uint16_t msg_len);
extern bool init_connection_framework();

int listenfd = -1;
int sockfd = -1;
static pthread_mutex_t sock_lock = PTHREAD_MUTEX_INITIALIZER;

static bool server_mode = false;

// Function designed for chat between client and server.
void *func(void *arg)
{
    char buff[MAX];
    int n;
    struct sockaddr_in servaddr;

    while (1) {
        if (sockfd != -1)
            close(sockfd);
        // socket create and verification
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd == -1) {
            printf("socket creation failed...\n");
            return NULL;
        }
        else
            printf("Socket successfully created..\n");
        bzero(&servaddr, sizeof(servaddr));
        // assign IP, PORT
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = inet_addr(host_address);
        servaddr.sin_port = htons(port);

        // connect the client socket to server socket
        if (connect(sockfd, (SA *)&servaddr, sizeof(servaddr)) != 0) {
            printf("connection with the server failed...\n");
            sleep(10);
            continue;
        }
        else {
            printf("connected to the server..\n");
        }

        // infinite loop for chat
        for (;;) {
            bzero(buff, MAX);

            // read the message from client and copy it in buffer
            n = read(sockfd, buff, sizeof(buff));
            // print buffer which contains the client contents
            //fprintf(stderr, "recieved %d bytes from host: %s", n, buff);

            // socket disconnected
            if (n <= 0)
                break;

            aee_host_msg_callback(buff, n);
        }
    }

    // After chatting close the socket
    close(sockfd);
}

static bool host_init()
{
    return true;
}

int host_send(void *ctx, const char *buf, int size)
{
    int ret;

    if (pthread_mutex_trylock(&sock_lock) == 0) {
        if (sockfd == -1) {
            pthread_mutex_unlock(&sock_lock);
            return 0;
        }

        ret = write(sockfd, buf, size);

        pthread_mutex_unlock(&sock_lock);
        return ret;
    }

    return -1;
}

void host_destroy()
{
    if (server_mode)
        close(listenfd);

    pthread_mutex_lock(&sock_lock);
    close(sockfd);
    pthread_mutex_unlock(&sock_lock);
}

host_interface interface = { .init = host_init,
                             .send = host_send,
                             .destroy = host_destroy };

/* Change it to 1 when fuzzing test */
#define WASM_ENABLE_FUZZ_TEST 0

void *func_server_mode(void *arg)
{
    int clilent;
    struct sockaddr_in serv_addr, cli_addr;
    int n;
    char buff[MAX];

    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &sa, 0);

    /* First call to socket() function */
    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    if (listenfd < 0) {
        perror("ERROR opening socket");
        exit(1);
    }

    /* Initialize socket structure */
    bzero((char *)&serv_addr, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);

    /* Now bind the host address using bind() call.*/
    if (bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR on binding");
        exit(1);
    }

    listen(listenfd, 5);
    clilent = sizeof(cli_addr);

    while (1) {
        pthread_mutex_lock(&sock_lock);

        sockfd = accept(listenfd, (struct sockaddr *)&cli_addr, &clilent);

        pthread_mutex_unlock(&sock_lock);

        if (sockfd < 0) {
            perror("ERROR on accept");
            exit(1);
        }

        printf("connection established!\n");

        for (;;) {
            bzero(buff, MAX);

            // read the message from client and copy it in buffer
            n = read(sockfd, buff, sizeof(buff));

            // socket disconnected
            if (n <= 0) {
                pthread_mutex_lock(&sock_lock);
                close(sockfd);
                sockfd = -1;
                pthread_mutex_unlock(&sock_lock);

                sleep(1);
                break;
            }

            aee_host_msg_callback(buff, n);
        }
#if WASM_ENABLE_FUZZ_TEST != 0
        /* Exit the process when host disconnect.
         * This is helpful for reproducing failure case. */
        close(sockfd);
        exit(1);
#endif
    }
}

static attr_container_t *read_test_sensor(void *sensor)
{
    //luc: for test
    attr_container_t *attr_obj =
      attr_container_create("read test sensor data");
    if (attr_obj) {
        bool ret =
          attr_container_set_string(&attr_obj, "name", "read test sensor");
        if (!ret) {
            attr_container_destroy(attr_obj);
            return NULL;
        }
        return attr_obj;
    }
    return NULL;
}

static bool config_test_sensor(void *s, void *config)
{
    return false;
}

static char global_heap_buf[1024 * 1024] = { 0 };

static void showUsage()
{
    printf("Usage:\n");
    printf("\nWork as TCP server mode:\n");
    printf("\tsimple -s|--server_mode -p|--port <Port>\n");
    printf("where\n");
    printf("\t<Port> represents the port that would be listened on and the "
           "default is 8888\n");
    printf("\nWork as TCP client mode:\n");
    printf("\tsimple -a|--host_address <Host Address> -p|--port <Port>\n");
    printf("where\n");
    printf("\t<Host Address> represents the network address of host and the "
           "default is 127.0.0.1\n");
    printf(
      "\t<Port> represents the listen port of host and the default is 8888\n");
}

static bool parse_args(int argc, char *argv[])
{
    int c;

    while (1) {
        int optIndex = 0;
        static struct option longOpts[] = {
            { "server_mode", no_argument, NULL, 's' },
            { "host_address", required_argument, NULL, 'a' },
            { "port", required_argument, NULL, 'p' },
            { "help", required_argument, NULL, 'h' },
            { 0, 0, 0, 0 }
        };

        c = getopt_long(argc, argv, "sa:p:u:b:w:h", longOpts, &optIndex);
        if (c == -1)
            break;

        switch (c) {
            case 's':
                server_mode = true;
                break;
            case 'a':
                host_address = optarg;
                printf("host address: %s\n", host_address);
                break;
            case 'p':
                port = atoi(optarg);
                printf("port: %d\n", port);
                break;
            case 'h':
                showUsage();
                return false;
            default:
                showUsage();
                return false;
        }
    }

    return true;
}

// Driver function
int native_init(int argc, char *argv[])
{
    RuntimeInitArgs init_args;
    korp_tid tid;

    if (!parse_args(argc, argv))
        return -1;

    memset(&init_args, 0, sizeof(RuntimeInitArgs));

    init_args.mem_alloc_type = Alloc_With_Allocator;
    init_args.mem_alloc_option.allocator.malloc_func = malloc;
    init_args.mem_alloc_option.allocator.realloc_func = realloc;
    init_args.mem_alloc_option.allocator.free_func = free;

    /* initialize runtime environment */
    if (!wasm_runtime_full_init(&init_args)) {
        printf("Init runtime environment failed.\n");
        return -1;
    }

    /* timer manager */
    init_wasm_timer();

    /* connection framework */
    if (!init_connection_framework()) {
        goto fail1;
    }

    /* sensor framework */
    init_sensor_framework();
    // add the sys sensor objects
    add_sys_sensor("sensor_test", "This is a sensor for test", 0, 1000,
                   read_test_sensor, config_test_sensor);
    start_sensor_framework();

    if (server_mode)
        os_thread_create(&tid, func_server_mode, NULL,
                         BH_APPLET_PRESERVED_STACK_SIZE);
    else
        os_thread_create(&tid, func, NULL, BH_APPLET_PRESERVED_STACK_SIZE);

    app_manager_startup(&interface);

    exit_wasm_timer();
    exit_sensor_framework();
    exit_connection_framework();

fail1:
    wasm_runtime_destroy();

    return -1;
}
