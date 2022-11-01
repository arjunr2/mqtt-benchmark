#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <inttypes.h>
#include <getopt.h>
#include <math.h>

#include "MQTTClient.h"

#include <unistd.h>
 
#define MS 1000

#define STARTUP_WAIT 100*MS

#define DROP_IDX(x) ((x * DROP_RATIO)/100)

#define TIMEOUT     10000L

#define LOG(...) do { if(LOG_ENABLE) printf(__VA_ARGS__); } while(0)
  
char* ADDRESS = "localhost:1883";
char* CLIENTID = "test-client";
char* TOPIC = "realm/test-topic";

uint32_t MSG_INTERVAL = 10*MS;
uint32_t MAX_ITER = 20;
uint32_t PAYLOAD_SIZE = 64;
uint32_t QOS = 1;
uint32_t DROP_RATIO = 0;
int LOG_ENABLE = 0;

typedef uint64_t TS_TYPE;

MQTTClient_deliveryToken deliveredtoken = -1;
MQTTClient client;
 
TS_TYPE *deliver_ts;
TS_TYPE *receive_ts;
uint32_t *rtt_ts;

int done_sending = 0;


static uint64_t inline get_us_time() {
  struct timespec tv;
  clock_gettime(CLOCK_REALTIME, &tv);
  uint64_t micros = ((uint64_t)(tv.tv_sec) * 1000000) + ((uint64_t)(tv.tv_nsec) / 1000);
  return micros;
}

void delivered(void *context, MQTTClient_deliveryToken dt)
{
    LOG("Message with token value %d delivery confirmed\n", dt);
    deliveredtoken = dt;
}
 
int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    static int itr_ct = 0;
    if (!strncmp(CLIENTID, message->payload, strlen(CLIENTID))) {
      receive_ts[itr_ct] = get_us_time();
      rtt_ts[itr_ct] = receive_ts[itr_ct] - deliver_ts[itr_ct];
      LOG("Message arrived (%d) | Recv time: %lu (RTT = %u us)\n", 
              itr_ct, receive_ts[itr_ct], rtt_ts[itr_ct]);
      itr_ct++;
    } else {
      LOG("Message arrived [EXTERNAL]\n");
    }
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
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
    exit(rc);
  }

  while (!done_sending) {
    usleep(1000);
  }
  LOG("Unsubscribing!\n");
  if ((rc = MQTTClient_unsubscribe(client, TOPIC)) != MQTTCLIENT_SUCCESS) {
    printf("Failed to unsubscribe, return code %d\n", rc);
    rc = EXIT_FAILURE;
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
  {"drop-ratio", optional_argument, NULL, 'd'},
  {"topic", optional_argument, NULL, 't'},
  {"log", no_argument, NULL, 'v'},
  {"help", no_argument, NULL, 'h'}
};

void parse_args(int argc, char* argv[]) {
  int opt;
  while ((opt = getopt_long(argc, argv, "a:n:m:i:q:s:d:t:vh", long_options, NULL)) != -1) {
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
      case 'd':
        DROP_RATIO = atoi(optarg);
        break;
      case 't':
        TOPIC = optarg;
        break;
      case 'v':
        LOG_ENABLE = 1;
        break;
      case 'h':
      default:
        fprintf(stderr, "Usage: %s [--address=ADDRESS (str)] [--name=NAME (str)] "
                                "[--interval=INTERVAL (us)] [--iterations=ITERATIONS (int)] "
                                "[--qos=QOS (int)] [--drop-ratio (int)] [--size=SIZE (int)] "
                                "[--topic=TOPIC (str)] [--log]\n", argv[0]);
        exit(0);
    }
  }

  printf("----- Configuration -----\n");
  printf("  Address       : %s\n", ADDRESS);
  printf("  Client        : %s\n", CLIENTID);
  printf("  Topic         : %s\n", TOPIC);
  printf("  Msg Interval  : %d\n", MSG_INTERVAL);
  printf("  Iterations    : %d\n", MAX_ITER);
  printf("  Msg Size      : %d\n", PAYLOAD_SIZE);
  printf("  QOS           : %d\n", QOS);
  printf("  Drop          : %d\n", DROP_RATIO);
  printf("  Log           : %d\n", LOG_ENABLE);
  printf("-------------------------\n");
  return;
}


int cmpfunc (const void* a, const void* b) {
  return ( *(int*)a - *(int*)b );
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
    memcpy(payload, CLIENTID, strlen(CLIENTID)); 

    deliver_ts = (TS_TYPE*) calloc(MAX_ITER, sizeof(TS_TYPE));
    receive_ts = (TS_TYPE*) calloc(MAX_ITER, sizeof(TS_TYPE));
    rtt_ts = (uint32_t*) calloc(MAX_ITER, sizeof(uint32_t));

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
      deliver_ts[it] = get_us_time();
      LOG("Message publish (%d) | Send time: %lu\n", it, deliver_ts[it]);
      if ((rc = MQTTClient_publishMessage(client, TOPIC, &pubmsg, &token)) != MQTTCLIENT_SUCCESS) {
          printf("Failed to publish message, return code %d\n", rc);
          rc = EXIT_FAILURE;
      }
      else {
          if (QOS != 0) {
            while (deliveredtoken != token) { };
          }
      }
      usleep(MSG_INTERVAL);
      it++;
    } while(it < MAX_ITER);

    /* End subscribe thread */
    done_sending = 1;
    pthread_join(recv_tid, NULL);

    if ((rc = MQTTClient_disconnect(client, 10000)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to disconnect, return code %d\n", rc);
        rc = EXIT_FAILURE;
    }

    /* Stats : Sort and drop outliers*/
    qsort(rtt_ts, MAX_ITER, sizeof(uint32_t), cmpfunc);

    uint32_t idx = DROP_IDX(MAX_ITER);
    uint64_t acc = 0;
    uint32_t ct = 0;
    for (int i = idx; i < MAX_ITER - idx; i++) {
      acc += rtt_ts[i];
      ct++;
    }
    uint32_t mean = acc / ct;

    acc = 0;
    for (int i = idx; i < MAX_ITER - idx; i++) {
      acc += ((rtt_ts[i] - mean) * (rtt_ts[i] - mean));
    }
    uint32_t std_dev = (uint32_t)(sqrt(acc/ct));

    printf("\n-- SUMMARY STATS --\n");
    printf("Mean-RTT: %u\n", mean);
    printf("Std-Dev: %u\n", std_dev);
    printf("Data Pts: ");
    for (uint32_t i = idx; i < MAX_ITER - idx; i++) {
      printf("%d,", rtt_ts[i]);
    }
    printf("\n");
    printf("-------------------\n\n");

 
destroy_exit:
    MQTTClient_destroy(&client);
 
exit:
    return rc;
}

/*
int compute_least(uint32_t* means, uint32_t* arr, uint32_t sz, int k) {
  for (int i = 0; i < sz; i++) {
    uint32_t min_idx = 0;
    uint32_t min = arr[min_idx];
    for (int j = 0; j < k; j++) {
      uint32_t val = abs(arr[i] - means[j]);
      if (val < min) {
        min = val;
        min_idx = sz;
      }
    }
  }
}

uint32_t* kmeans(uint32_t* arr, uint32_t sz, int k) {
  uint32_t* means = (uint32_t*) malloc(k * sizeof(int));
  uint32_t* cluster_map = (uint32_t*) malloc(sz * sizeof(uint32_t));
  for (int i = 0; i < k; i++) {
    means = arr[(sz/k)*i];
  }
  for (int n = 0; n < 10; n++) {
  }
  
}*/
