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

#define MAX_TOPICS 1000
#define MS 1000
#define STARTUP_WAIT 100*MS
#define DROP_IDX(x) ((x * DROP_RATIO)/100)
#define LOG(...) do { if(LOG_ENABLE) printf(__VA_ARGS__); } while(0)
#define ERR(...) do { fprintf(stderr, __VA_ARGS__); } while(0)

const char END_TOKEN = '$';
  
char* BROKER = "localhost:1883";
char* CLIENTID = "test-client";
int NUM_PUBS = 0;
int NUM_SUBS = 0;
char* PUBS[MAX_TOPICS] = { NULL };
char* SUBS[MAX_TOPICS] = { NULL };

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

#define ENCODE_PL(payload, val, sz) {  \
  memcpy (payload + msg_pt, val, sz); \
  msg_pt += (sz);  \
}
#define DECODE_PL(val, payload, sz) { \
  memcpy (val, payload + msg_pt, sz); \
  msg_pt += (sz); \
}

static uint64_t inline get_us_time() {
  struct timespec tv;
  clock_gettime(CLOCK_REALTIME, &tv);
  uint64_t micros = ((uint64_t)(tv.tv_sec) * 1000000) + ((uint64_t)(tv.tv_nsec) / 1000);
  return micros;
}

void delivered(void *context, MQTTClient_deliveryToken dt) {
  LOG("Message with token value %d delivery confirmed\n", dt);
  deliveredtoken = dt;
}
 
int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
  /* Record current time */
  TS_TYPE receive_time = get_us_time();

  /* Payload format: [CLIENTID][packetctr][timestamp] */
  char end_tok = *((char*)(message->payload));
  char* client_id = message->payload + 1;
  uint32_t it;
  TS_TYPE deliver_time;

  if (!strncmp(CLIENTID, client_id, strlen(CLIENTID))) {
    /* Check if last message */
    if (end_tok == END_TOKEN) {
      LOG("End token received!\n");
      done_sending = 1;
    }
    else {
      uint32_t msg_pt = strlen(client_id) + 2;
      /* Get receive time */
      DECODE_PL(&it, message->payload, sizeof(it));
      receive_ts[it] = receive_time;

      /* Compute RTT */
      DECODE_PL(&deliver_time, message->payload, sizeof(deliver_time));
      rtt_ts[it] = receive_ts[it] - deliver_time;
      LOG("Message arrived (%d) | Recv time: %lu (RTT = %u us)\n", 
              it, receive_ts[it], rtt_ts[it]);
    } 
  } 
  else {
      LOG("Message arrived [EXTERNAL]\n");
  }
  MQTTClient_freeMessage(&message);
  MQTTClient_free(topicName);
  return 1;
}
 
void connlost(void *context, char *cause) {
  printf("\nConnection lost\n");
  printf("     cause: %s\n", cause);
}
 
/* Main Subscription Thread */
void *subscribe_thread(void *arg) {
  int rc;
  /* Init receive timestamps and RTT */
  receive_ts = (TS_TYPE*) calloc(MAX_ITER, sizeof(TS_TYPE));
  rtt_ts = (uint32_t*) calloc(MAX_ITER, sizeof(uint32_t));

  printf("Subscribing: \"%s\" ;  QoS: %d\n", SUBS[0], QOS);
  if ((rc = MQTTClient_subscribe(client, SUBS[0], QOS)) != MQTTCLIENT_SUCCESS) {
    printf("Failed to subscribe, return code %d\n", rc);
    rc = EXIT_FAILURE;
    exit(rc);
  }

  while (!done_sending) { usleep(1000); }
  LOG("Unsubscribing!\n");
  if ((rc = MQTTClient_unsubscribe(client, SUBS[0])) != MQTTCLIENT_SUCCESS) {
    printf("Failed to unsubscribe, return code %d\n", rc); 
    rc = EXIT_FAILURE; 
  }
}


/* Main Publisher Thread */
void *publish_thread(void *arg) {
  int rc;
  MQTTClient_deliveryToken token;
  
  /* Payload init and timestamp buffer */
  char* payload = malloc(PAYLOAD_SIZE);
  for (int i = 0; i < PAYLOAD_SIZE - 1; i++) {
    payload[i] = (rand() % 26) + 'A';
  }
  payload[PAYLOAD_SIZE-1] = 0;
  
  /* Payload format: [END/SEND][CLIENTID][packetctr][timestamp] */
  uint32_t msg_pt = 0;
  char send_tok = '*';
  ENCODE_PL (payload, &send_tok, sizeof(send_tok)); 
  ENCODE_PL (payload, CLIENTID, strlen(CLIENTID) + 1);

  /* Init send timestamps */
  deliver_ts = (TS_TYPE*) calloc(MAX_ITER, sizeof(TS_TYPE));

  /* Publish Message Characteristics */
  MQTTClient_message pubmsg = MQTTClient_message_initializer;
  pubmsg.payload = payload;
  pubmsg.payloadlen = PAYLOAD_SIZE;
  pubmsg.qos = QOS;
  pubmsg.retained = 0;

  /* Publish routine */
  #define PUBLISH_MESSAGE() \
    if ((rc = MQTTClient_publishMessage(client, PUBS[0], &pubmsg, &token)) != MQTTCLIENT_SUCCESS) { \
        printf("Failed to publish message, return code %d\n", rc);  \
        rc = EXIT_FAILURE;  \
        goto finish_publish; \
    } else {  \
        LOG("Message publish (%d) | Send time: %lu\n", it, deliver_ts[it]); \
        if (QOS != 0) { while (deliveredtoken != token) { };  } \
    }

  #define PUBLISH_MESSAGE_END() \
    if ((rc = MQTTClient_publishMessage(client, PUBS[0], &pubmsg, &token)) != MQTTCLIENT_SUCCESS) { \
        printf("Failed to publish message, return code %d\n", rc);  \
        rc = EXIT_FAILURE;  \
        goto finish_publish; \
    } else {  \
        LOG("Message publish END\n"); \
        if (QOS != 0) { while (deliveredtoken != token) { };  } \
    }

  LOG("Publishing: %s\n", PUBS[0]);
  uint32_t it;
  for (it = 0; it < MAX_ITER; it++) {
    deliveredtoken = 0;
    /* Append timestamp and counter to payload */
    deliver_ts[it] = get_us_time();
    msg_pt = strlen(CLIENTID) + 2;
    ENCODE_PL (payload, &it, sizeof(it));
    ENCODE_PL (payload, &deliver_ts[it], sizeof(deliver_ts[it]));

    PUBLISH_MESSAGE();
    /* Wait interval */
    usleep(MSG_INTERVAL);
  }

  /* Send END_TOKEN to subscribe thread with QOS 2 */
  pubmsg.qos = 2;
  payload[0] = END_TOKEN;
  PUBLISH_MESSAGE_END();

finish_publish:
  return NULL;  
}

/* Arg Parsing */
static struct option long_options[] = {
  {"broker", optional_argument, NULL, 'b'},  // pubsub
  {"name", optional_argument, NULL, 'n'},
  {"interval", optional_argument, NULL, 'm'},
  {"iterations", optional_argument, NULL, 'i'},
  {"qos", optional_argument, NULL, 'q'},
  {"size", optional_argument, NULL, 's'},
  {"pub", optional_argument, NULL, 'u'},
  {"sub", optional_argument, NULL, 't'},
  {"drop-ratio", optional_argument, NULL, 'd'},
  {"topic", optional_argument, NULL, 't'},
  {"log", no_argument, NULL, 'v'},
  {"help", no_argument, NULL, 'h'}
};

char* parse_topic_list(char** buf, int ct, char* arg) {
  char* full_str = strdup(arg);
  const char d[2] = ",";
  char* token = strtok(arg, d);
  while (token != NULL) {
    buf[ct++] = strdup(token);
    token = strtok(NULL, d);
  }
  return full_str;
}


void parse_args(int argc, char* argv[]) {
  int opt;
  char *pubarg, *subarg;
  while ((opt = getopt_long(argc, argv, "a:n:m:i:q:s:d:t:vh", long_options, NULL)) != -1) {
    switch(opt) {
      case 'a': BROKER = strdup(optarg);                break;
      case 'n': CLIENTID = strdup(optarg);              break;
      case 'm': MSG_INTERVAL = atoi(optarg);            break;
      case 'i': MAX_ITER = atoi(optarg);                break;
      case 'q': QOS = atoi(optarg);                     break;
      case 's': PAYLOAD_SIZE = atoi(optarg);            break;
      case 'd': DROP_RATIO = atoi(optarg);              break;
      case 'u': pubarg = parse_topic_list(PUBS, NUM_PUBS, optarg);    break;
      case 't': subarg = parse_topic_list(SUBS, NUM_SUBS, optarg);    break;
      case 'v': LOG_ENABLE = 1;                         break;
      case 'h':
      default:
        ERR("Usage: %s [--broker=BROKER (str)] [--name=NAME (str)] "
                      "[--interval=INTERVAL (us)] [--iterations=ITERATIONS (int)] "
                      "[--qos=QOS (int)] [--drop-ratio (int)] [--size=SIZE (int)] "
                      "[--pub=TOPIC1,TOPIC2,.. (str)] "
                      "[--sub=TOPIC1,TOPIC2,.. (str)] [--log]\n", 
                      argv[0]);
        exit(0);
    }
  }

  if (!PUBS[0] && !SUBS[0]) {
    ERR("Error: Must specify either --pub or --sub list. "
        "Run with -h for help menu\n");
    exit(0);
  }

  printf("----- Configuration -----\n");
  printf("  Broker        : %s\n", BROKER);
  printf("  Client        : %s\n", CLIENTID);
  printf("  Pubtopics     : %s\n", pubarg);
  printf("  Subtopics     : %s\n", subarg);
  printf("  Msg Interval  : %d\n", MSG_INTERVAL);
  printf("  Iterations    : %d\n", MAX_ITER);
  printf("  Msg Size      : %d\n", PAYLOAD_SIZE);
  printf("  QOS           : %d\n", QOS);
  printf("  Drop          : %d\n", DROP_RATIO);
  printf("  Log           : %d\n", LOG_ENABLE);
  printf("-------------------------\n");
  return;
}


/* Print summary statistics for benchmark */
void summary_stats(uint32_t* rtt_ts);

int cmpfunc (const void* a, const void* b) {
  return ( *(int*)a - *(int*)b );
}

int main(int argc, char* argv[])
{
    srand(time(NULL));

    parse_args(argc, argv);

    /* MQTT Init */
    int rc;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;

    if ((rc = MQTTClient_create(&client, BROKER, CLIENTID,
        MQTTCLIENT_PERSISTENCE_NONE, NULL)) != MQTTCLIENT_SUCCESS)
    {
        ERR("Failed to create client, return code %d\n", rc);
        rc = EXIT_FAILURE;
        goto exit;
    }
 
    if ((rc = MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered)) != MQTTCLIENT_SUCCESS)
    {
        ERR("Failed to set callbacks, return code %d\n", rc);
        rc = EXIT_FAILURE;
        goto destroy_exit;
    }
 
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
    {
        ERR("Failed to connect, return code %d\n", rc);
        rc = EXIT_FAILURE;
        goto destroy_exit;
    }


    /* Publish/Subscribe thread create */
    pthread_t sub_tid, pub_tid;
    pthread_create(&sub_tid, NULL, subscribe_thread, NULL);
    usleep(STARTUP_WAIT);
    pthread_create(&pub_tid, NULL, publish_thread, NULL);

    /* Merge all threads */
    pthread_join(pub_tid, NULL);
    pthread_join(sub_tid, NULL);

    if ((rc = MQTTClient_disconnect(client, 10000)) != MQTTCLIENT_SUCCESS)  {
        ERR("Failed to disconnect, return code %d\n", rc);
        rc = EXIT_FAILURE;
    }

    summary_stats(rtt_ts);

destroy_exit:
    MQTTClient_destroy(&client);
 
exit:
    return rc;
}


/* Print summary statistics for benchmark */
void summary_stats(uint32_t* rtt_ts) {
    /* Sort and drop outliers*/
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

}
