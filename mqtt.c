#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <inttypes.h>
#include <getopt.h>

#include "MQTTClient.h"

#include <sys/time.h>
#include <unistd.h>
 
#define MS 1000

//#define ADDRESS     "tcp://localhost:1883"
#define TOPIC       "realm/test-topic"
#define STARTUP_WAIT 100*MS

#define TIMEOUT     10000L

#define LOG(...) do { if(LOG_ENABLE) printf(__VA_ARGS__); } while(0)
  
char* ADDRESS = "tcp://localhost:1883";
char* CLIENTID = "test-client";
uint32_t MSG_INTERVAL = 10*MS;
uint32_t MAX_ITER = 20;
uint32_t PAYLOAD_SIZE = 64;
uint32_t QOS = 1;
int LOG_ENABLE = 0;

typedef uint64_t TS_TYPE;

MQTTClient_deliveryToken deliveredtoken = -1;
MQTTClient client;
 
TS_TYPE *deliver_ts;
TS_TYPE *receive_ts;

static uint64_t inline get_ms_time() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  uint64_t millis = (uint64_t)(tv.tv_sec) * 1000000 + (uint64_t)(tv.tv_usec);
  return millis;
}

void delivered(void *context, MQTTClient_deliveryToken dt)
{
    LOG("Message with token value %d delivery confirmed\n", dt);
    deliveredtoken = dt;
}
 
int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    static int itr_ct = 0;
    receive_ts[itr_ct] = get_ms_time();
    LOG("Message arrived (%d) | Recv time: %lu (RTT = %lu us)\n", 
            itr_ct, receive_ts[itr_ct], receive_ts[itr_ct] - deliver_ts[itr_ct]);
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    itr_ct++;
    return 1;
}
 
void connlost(void *context, char *cause)
{
    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);
}
 


void *subscribe_thread(void *arg) {
  int rc;
  printf("Subscribing: \"%s\" ;  QoS: %d\n"
        "Press q to quit\n", TOPIC, QOS);
  if ((rc = MQTTClient_subscribe(client, TOPIC, QOS)) != MQTTCLIENT_SUCCESS) {
    printf("Failed to subscribe, return code %d\n", rc);
    rc = EXIT_FAILURE;
  }
  else {
    int ch;
    do {
      ch = getchar();
    } while(ch != 'q');
    if ((rc = MQTTClient_unsubscribe(client, TOPIC)) != MQTTCLIENT_SUCCESS) {
      printf("Failed to unsubscribe, return code %d\n", rc);
      rc = EXIT_FAILURE;
    }
  }
}


/* Arg Parsing */
static struct option long_options[] = {
  {"address", optional_argument, NULL, 'a'},
  {"name", optional_argument, NULL, 'n'},
  {"interval", optional_argument, NULL, 'm'},
  {"iterations", optional_argument, NULL, 'i'},
  {"qos", optional_argument, NULL, 'q'},
  {"size", optional_argument, NULL, 's'},
  {"log", no_argument, NULL, 'v'},
};

void parse_args(int argc, char* argv[]) {
  int opt;
  while ((opt = getopt_long(argc, argv, "a:n:m:i:q:s:v", long_options, NULL)) != -1) {
    switch(opt) {
      case 'a':
        ADDRESS = strdup(optarg);
        break;
      case 'n':
        CLIENTID = strdup(optarg);
        break;
      case 'm':
        MSG_INTERVAL = atoi(optarg);
        break;
      case 'i':
        MAX_ITER = atoi(optarg);
        break;
      case 'q':
        QOS = atoi(optarg);
        break;
      case 's':
        PAYLOAD_SIZE = atoi(optarg);
        break;
      case 'v':
        LOG_ENABLE = 1;
        break;
      default:
        fprintf(stderr, "Usage: %s [--interval=INTERVAL]\n", argv[0]);
    }
  }

  printf("-- Configuration --\n");
  printf("  Address       : %s\n", ADDRESS);
  printf("  Client        : %s\n", CLIENTID);
  printf("  Msg Interval  : %d\n", MSG_INTERVAL);
  printf("  Iterations    : %d\n", MAX_ITER);
  printf("  Msg Size      : %d\n", PAYLOAD_SIZE);
  printf("  QOS           : %d\n", QOS);
  printf("  LOG           : %d\n", LOG_ENABLE);
  printf("-----\n");
  return;
}

int main(int argc, char* argv[])
{
    srand(time(NULL));

    parse_args(argc, argv);

    /* MQTT Init */
    int rc;
    MQTTClient_deliveryToken token;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_message pubmsg = MQTTClient_message_initializer;

    if ((rc = MQTTClient_create(&client, ADDRESS, CLIENTID,
        MQTTCLIENT_PERSISTENCE_NONE, NULL)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to create client, return code %d\n", rc);
        rc = EXIT_FAILURE;
        goto exit;
    }
 
    if ((rc = MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to set callbacks, return code %d\n", rc);
        rc = EXIT_FAILURE;
        goto destroy_exit;
    }
 
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to connect, return code %d\n", rc);
        rc = EXIT_FAILURE;
        goto destroy_exit;
    }

    
    /* Payload init and timestamp buffer */
    char* payload = malloc(PAYLOAD_SIZE);
    for (int i = 0; i < PAYLOAD_SIZE - 1; i++) {
      payload[i] = (rand() % 26) + 'A';
    }
    payload[PAYLOAD_SIZE-1] = 0;

    deliver_ts = (TS_TYPE*) calloc(MAX_ITER, sizeof(TS_TYPE));
    receive_ts = (TS_TYPE*) calloc(MAX_ITER, sizeof(TS_TYPE));

    /* Subscribe thread */
    pthread_t recv_tid;
    pthread_create(&recv_tid, NULL, subscribe_thread, NULL);
    usleep(STARTUP_WAIT);


    pubmsg.payload = payload;
    pubmsg.payloadlen = (int)strlen(payload);
    pubmsg.qos = QOS;
    pubmsg.retained = 0;
    deliveredtoken = 0;

    int it = 0;
    do {
      deliver_ts[it] = get_ms_time();
      LOG("Message publish (%d) | Send time: %lu\n", it, deliver_ts[it]);
      if ((rc = MQTTClient_publishMessage(client, TOPIC, &pubmsg, &token)) != MQTTCLIENT_SUCCESS) {
          printf("Failed to publish message, return code %d\n", rc);
          rc = EXIT_FAILURE;
      }
      else {
          while (deliveredtoken != token)
          {
            usleep(10000L);
          }
      }
      usleep(MSG_INTERVAL);
      it++;
    } while(it < MAX_ITER);

    
    /* Stats */
    uint64_t acc = 0;
    for (int i = 0; i < MAX_ITER; i++) {
      acc += (receive_ts[i] - deliver_ts[i]);
    }
    acc /= MAX_ITER;

    printf("\n== SUMMARY STATS ==\n");
    printf("Average RTT: %lu\n", acc);


    if ((rc = MQTTClient_disconnect(client, 10000)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to disconnect, return code %d\n", rc);
        rc = EXIT_FAILURE;
    }
 
destroy_exit:
    MQTTClient_destroy(&client);
 
exit:
    return rc;
}
