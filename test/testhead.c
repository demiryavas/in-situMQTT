
#include "mqtt_types.h"
#include "mqtt_message.h"

#ifdef WIN32
  #include <stdint.h>
#else
  #include <inttypes.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFSIZE 1024

void parsing_test(void)
{
  unsigned char buffer[BUFFSIZE];
  cstr_t buff;// = {&buffer[0], (uint32_t)BUFFSIZE};
  buff.str = &buffer[0];
  buff.length = BUFFSIZE;

  uint8_t connect1[] = {0x10, 0x17, 0x00, 0x04, 0x4d, 0x51, 0x54, 0x54,
                        0x04, 0x02, 0x00, 0x3c, 0x00, 0x0b, 0x74, 0x65,
                        0x73, 0x74, 0x5f, 0x63, 0x6c, 0x69, 0x65, 0x6e,
                        0x74};

  uint8_t connect2[] = {0x10, 0x62, 0x00, 0x04, 0x4d, 0x51, 0x54, 0x54,
                        0x04, 0xc6, 0x00, 0x11, 0x00, 0x0b, 0x54, 0x65,
                        0x73, 0x74, 0x43, 0x6c, 0x69, 0x65, 0x6e, 0x74,
                        0x32, 0x00, 0x0f, 0x77, 0x69, 0x6c, 0x6c, 0x2f,
                        0x74, 0x6f, 0x70, 0x69, 0x63, 0x2f, 0x74, 0x65,
                        0x73, 0x74, 0x00, 0x1b, 0x77, 0x69, 0x6c, 0x6c,
                        0x20, 0x74, 0x6f, 0x70, 0x69, 0x63, 0x20, 0x74,
                        0x65, 0x73, 0x74, 0x20, 0x6d, 0x65, 0x73, 0x73,
                        0x61, 0x67, 0x65, 0x2e, 0x2e, 0x2e, 0x21, 0x00,
                        0x09, 0x74, 0x65, 0x73, 0x74, 0x2e, 0x75, 0x73,
                        0x65, 0x72, 0x00, 0x10, 0x31, 0x45, 0x67, 0x66,
                        0x2a, 0x21, 0x23, 0x24, 0x30, 0x30, 0x38, 0x73,
                        0x73, 0x6a, 0x6a, 0x4c};

  uint8_t disconnect1[] = {0xe0, 0x00};

  uint8_t publish1[] = {0x30, 0x1a, 0x00, 0x07, 0x73, 0x65, 0x6e, 0x73,
                        0x6f, 0x72, 0x73, 0x50, 0x75, 0x62, 0x6c, 0x69,
                        0x73, 0x68, 0x65, 0x64, 0x20, 0x64, 0x61, 0x74,
                        0x61, 0x2e, 0x2e, 0x2e};

  uint8_t publish2[] = {0x33, 0x1c, 0x00, 0x07, 0x73, 0x65, 0x6e, 0x73,
                        0x6f, 0x72, 0x73, 0x00, 0x01, 0x50, 0x75, 0x62,
                        0x6c, 0x69, 0x73, 0x68, 0x65, 0x64, 0x20, 0x64,
                        0x61, 0x74, 0x61, 0x2e, 0x2e, 0x2e};

  uint8_t publish_corrupted[] = {0x30, 0x07, 0x00, 0x07, 0x73, 0x65, 0x6e, 0x73,
                                 0x6f, 0x72, 0x73, 0x00, 0x01, 0x4f, 0x4b};

  uint8_t subscribe1[] = {0x82, 0x29, 0x00, 0x01, 0x00, 0x24, 0x2f, 0x6f,
                          0x6e, 0x65, 0x4d, 0x32, 0x4d, 0x2f, 0x72, 0x65,
                          0x73, 0x70, 0x2f, 0x43, 0x53, 0x45, 0x33, 0x34,
                          0x30, 0x39, 0x31, 0x36, 0x35, 0x2f, 0x43, 0x53,
                          0x45, 0x31, 0x35, 0x33, 0x34, 0x31, 0x32, 0x33,
                          0x2f, 0x23, 0x01};

  uint8_t suback1[] = {0x90, 0x03, 0x00, 0x01, 0x00};

  uint8_t suback2[] = {0x90, 0x07, 0x00, 0x01, 0x02, 0x00, 0x01, 0x01, 0x80};

  uint8_t puback1[] = {0x40, 0x02, 0x00, 0x01};
  uint8_t pubrec1[] = {0x50, 0x02, 0x00, 0x01};
  uint8_t pubrel1[] = {0x62, 0x02, 0x00, 0x01};
  uint8_t pubcomp1[] = {0x70, 0x02, 0x00, 0x01};

  uint8_t unsubscribe1[] = {0xa2, 0x0c, 0x00, 0x01, 0x00, 0x03, 0x61, 0x2f,
                            0x62, 0x00, 0x03, 0x63, 0x2f, 0x64};

  uint8_t unsuback1[] = {0xb0, 0x02, 0x00, 0x01};

  uint8_t pingreq1[] = {0xc0, 0x00};
  uint8_t pingresp1[] = {0xd0, 0x00};

  uint32_t parse_error = 0;
  mqtt_message* msg = NULL;
  /* Use attached_raw>0 to provide deallocation of raw-packet on message destroy
   * for dynamically allocated packets. Note that; this is not the case for this
   * test code since raw-packet is statically allocated, so we pass 0 for attached_raw
   */
  msg = mqtt_message_parse_raw_packet(connect1, sizeof(connect1), &parse_error, 0);
  if (parse_error == MQTT_SUCCESS) {
    memset(buff.str, 0, buff.length);
    mqtt_message_dump(msg, &buff, 1);
    printf("%s", buff.str);
  }
  else {
    printf("\n*** Parse error: %d for connect1\n", parse_error);
  }
  if (msg) {
    mqtt_message_destroy(msg);
  }

  msg = mqtt_message_parse_raw_packet(connect2, sizeof(connect2), &parse_error, 0);
  if (parse_error == MQTT_SUCCESS) {
    memset(buff.str, 0, buff.length);
    mqtt_message_dump(msg, &buff, 1);
    printf("%s", buff.str);
  }
  else {
    printf("\n*** Parse error: %d for connect2\n", parse_error);
  }
  if (msg) {
    mqtt_message_destroy(msg);
  }

  msg = mqtt_message_parse_raw_packet(disconnect1, sizeof(disconnect1), &parse_error, 0);
  if (parse_error == MQTT_SUCCESS) {
    memset(buff.str, 0, buff.length);
    mqtt_message_dump(msg, &buff, 1);
    printf("%s", buff.str);
  }
  else {
    printf("\n*** Parse error: %d for disconnect1\n", parse_error);
  }
  if (msg) {
    mqtt_message_destroy(msg);
  }

  msg = mqtt_message_parse_raw_packet(publish1, sizeof(publish1), &parse_error, 0);
  if (parse_error == MQTT_SUCCESS) {
    memset(buff.str, 0, buff.length);
    mqtt_message_dump(msg, &buff, 1);
    printf("%s", buff.str);
  }
  else {
    printf("\n*** Parse error: %d for publish1\n", parse_error);
  }
  if (msg) {
    mqtt_message_destroy(msg);
  }

  msg = mqtt_message_parse_raw_packet(publish2, sizeof(publish2), &parse_error, 0);
  if (parse_error == MQTT_SUCCESS) {
    memset(buff.str, 0, buff.length);
    mqtt_message_dump(msg, &buff, 1);
    printf("%s", buff.str);
  }
  else {
    printf("Parse error: %d for publish2\n", parse_error);
  }
  if (msg) {
    mqtt_message_destroy(msg);
  }

  msg = mqtt_message_parse_raw_packet(publish_corrupted, sizeof(publish_corrupted), &parse_error, 0);
  if (parse_error == MQTT_SUCCESS) {
    memset(buff.str, 0, buff.length);
    mqtt_message_dump(msg, &buff, 1);
    printf("%s", buff.str);
  }
  else {
    printf("Parse error: %d for publish_corrupted\n", parse_error);
  }
  if (msg) {
    mqtt_message_destroy(msg);
  }

  msg = mqtt_message_parse_raw_packet(subscribe1, sizeof(subscribe1), &parse_error, 0);
  if (parse_error == MQTT_SUCCESS) {
    memset(buff.str, 0, buff.length);
    mqtt_message_dump(msg, &buff, 1);
    printf("%s", buff.str);
  }
  else {
    printf("Parse error: %d for subscribe1\n", parse_error);
  }
  if (msg) {
    mqtt_message_destroy(msg);
  }

  msg = mqtt_message_parse_raw_packet(suback1, sizeof(suback1), &parse_error, 0);
  if (parse_error == MQTT_SUCCESS) {
    memset(buff.str, 0, buff.length);
    mqtt_message_dump(msg, &buff, 1);
    printf("%s", buff.str);
  }
  else {
    printf("Parse error: %d for suback1\n", parse_error);
  }
  if (msg) {
    mqtt_message_destroy(msg);
  }

  msg = mqtt_message_parse_raw_packet(suback2, sizeof(suback2), &parse_error, 0);
  if (parse_error == MQTT_SUCCESS) {
    memset(buff.str, 0, buff.length);
    mqtt_message_dump(msg, &buff, 1);
    printf("%s", buff.str);
  }
  else {
    printf("Parse error: %d for suback2\n", parse_error);
  }
  if (msg) {
    mqtt_message_destroy(msg);
  }

  msg = mqtt_message_parse_raw_packet(puback1, sizeof(puback1), &parse_error, 0);
  if (parse_error == MQTT_SUCCESS) {
    memset(buff.str, 0, buff.length);
    mqtt_message_dump(msg, &buff, 1);
    printf("%s", buff.str);
  }
  else {
    printf("Parse error: %d for puback1\n", parse_error);
  }
  if (msg) {
    mqtt_message_destroy(msg);
  }

  msg = mqtt_message_parse_raw_packet(pubrec1, sizeof(pubrec1), &parse_error, 0);
  if (parse_error == MQTT_SUCCESS) {
    memset(buff.str, 0, buff.length);
    mqtt_message_dump(msg, &buff, 1);
    printf("%s", buff.str);
  }
  else {
    printf("Parse error: %d for pubrec1\n", parse_error);
  }
  if (msg) {
    mqtt_message_destroy(msg);
  }

  msg = mqtt_message_parse_raw_packet(pubrel1, sizeof(pubrel1), &parse_error, 0);
  if (parse_error == MQTT_SUCCESS) {
    memset(buff.str, 0, buff.length);
    mqtt_message_dump(msg, &buff, 1);
    printf("%s", buff.str);
  }
  else {
    printf("Parse error: %d for pubrel1\n", parse_error);
  }
  if (msg) {
    mqtt_message_destroy(msg);
  }

  msg = mqtt_message_parse_raw_packet(pubcomp1, sizeof(pubcomp1), &parse_error, 0);
  if (parse_error == MQTT_SUCCESS) {
    memset(buff.str, 0, buff.length);
    mqtt_message_dump(msg, &buff, 1);
    printf("%s", buff.str);
  }
  else {
    printf("Parse error: %d for pubcomp1\n", parse_error);
  }
  if (msg) {
    mqtt_message_destroy(msg);
  }

  msg = mqtt_message_parse_raw_packet(unsubscribe1, sizeof(unsubscribe1), &parse_error, 0);
  if (parse_error == MQTT_SUCCESS) {
    memset(buff.str, 0, buff.length);
    mqtt_message_dump(msg, &buff, 1);
    printf("%s", buff.str);
  }
  else {
    printf("Parse error: %d for unsubscribe1\n", parse_error);
  }
  if (msg) {
    mqtt_message_destroy(msg);
  }

  msg = mqtt_message_parse_raw_packet(unsuback1, sizeof(unsuback1), &parse_error, 0);
  if (parse_error == MQTT_SUCCESS) {
    memset(buff.str, 0, buff.length);
    mqtt_message_dump(msg, &buff, 1);
    printf("%s", buff.str);
  }
  else {
    printf("Parse error: %d for unsuback1\n", parse_error);
  }
  if (msg) {
    mqtt_message_destroy(msg);
  }

  msg = mqtt_message_parse_raw_packet(pingreq1, sizeof(pingreq1), &parse_error, 0);
  if (parse_error == MQTT_SUCCESS) {
    memset(buff.str, 0, buff.length);
    mqtt_message_dump(msg, &buff, 1);
    printf("%s", buff.str);
  }
  else {
    printf("Parse error: %d for pingreq1\n", parse_error);
  }
  if (msg) {
    mqtt_message_destroy(msg);
  }

  msg = mqtt_message_parse_raw_packet(pingresp1, sizeof(pingresp1), &parse_error, 0);
  if (parse_error == MQTT_SUCCESS) {
    memset(buff.str, 0, buff.length);
    mqtt_message_dump(msg, &buff, 1);
    printf("%s", buff.str);
  }
  else {
    printf("Parse error: %d for pingresp1\n", parse_error);
  }
  if (msg) {
    mqtt_message_destroy(msg);
  }
}

void building_test(void)
{
  int ret = 0;
  unsigned char buffer[BUFFSIZE];
  cstr_t buff;// = {&buffer[0], (uint32_t)BUFFSIZE};
  buff.str = &buffer[0];
  buff.length = BUFFSIZE;

  /* CONNECT */
  mqtt_message* connmsg = mqtt_message_create_empty();
  connmsg->packet_type = MQTT_CONNECT;
  connmsg->vh.connect_vh.protocol_name.str = (unsigned char*)"MQTT";
  connmsg->vh.connect_vh.protocol_name.length = 4;
  connmsg->vh.connect_vh.protocol_level = 4; //MQTT_VERSION_3_1_1;
  connmsg->vh.connect_vh.keep_alive = 350;

  conn_flags connflags;
  connflags.clean_session = 1;
  connflags.will_retain = 0;
  connflags.will_qos = 0;
  connflags.will_flag = 1;
  connflags.username_flag = 1;
  connflags.password_flag = 1;

  connmsg->pld.connect_pld.will_topic.str = (unsigned char*)"Test will topic...";
  connmsg->pld.connect_pld.will_topic.length = strlen("Test will topic...");
  connmsg->pld.connect_pld.will_msg.str = (unsigned char*)"Test will message...";
  connmsg->pld.connect_pld.will_msg.length = strlen("Test will message...");

  connmsg->pld.connect_pld.user_name.str = (unsigned char*)"Test-User";
  connmsg->pld.connect_pld.user_name.length = strlen("Test-User");

  connmsg->pld.connect_pld.password.str = (unsigned char*)"Abcd1234+!";
  connmsg->pld.connect_pld.password.length = strlen("Abcd1234+!");

  connmsg->vh.connect_vh.conn_flags = encode_connect_flags(&connflags);

  connmsg->pld.connect_pld.client_id.str = (unsigned char*)"Test-Client1";
  connmsg->pld.connect_pld.client_id.length = strlen("Test-Client1");

  ret = mqtt_message_build(connmsg, NULL);
  if (ret == 0) {
    memset(buff.str, 0, buff.length);
    mqtt_message_dump(connmsg, &buff, 1);
    printf("%s", buff.str);
  }
  else {
    printf("Problem on building connect example : %d\n", ret);
  }
  mqtt_message_destroy(connmsg);

  /* CONNACK */
  mqtt_message* connack = mqtt_message_create_empty();
  connack->packet_type = MQTT_CONNACK;
  connack->vh.connack_vh.conn_return_code = 4;
  connack->vh.connack_vh.connack_flags |= 1;
  ret = mqtt_message_build(connack, 0);
  if (ret == 0) {
    memset(buff.str, 0, buff.length);
    mqtt_message_dump(connack, &buff, 1);
    printf("%s", buff.str);
  }
  else {
    printf("Problem on building connack example : %d\n", ret);
  }
  mqtt_message_destroy(connack);

  /* PUBREL */
  mqtt_message* pubrel = mqtt_message_create_empty();
  pubrel->packet_type = MQTT_PUBREL;
  pubrel->vh.pubrel_vh.packet_id = 1;
  ret = mqtt_message_build(pubrel, NULL);
  if (ret == 0) {
    memset(buff.str, 0, buff.length);
    mqtt_message_dump(pubrel, &buff, 1);
    printf("%s", buff.str);
  }
  else {
    printf("Problem on building pubrel example : %d\n", ret);
  }
  mqtt_message_destroy(pubrel);

  /* PUBACK */
  mqtt_message* puback = mqtt_message_create_empty();
  puback->packet_type = MQTT_PUBACK;
  puback->vh.puback_vh.packet_id = 2;
  ret = mqtt_message_build(puback, NULL);
  if (ret == 0) {
    memset(buff.str, 0, buff.length);
    mqtt_message_dump(puback, &buff, 1);
    printf("%s", buff.str);
  }
  else {
    printf("Problem on building puback example : %d\n", ret);
  }
  mqtt_message_destroy(puback);

  mqtt_message* pubrec = mqtt_message_create_empty();
  pubrec->packet_type = MQTT_PUBREC;
  pubrec->vh.pubrec_vh.packet_id = 3;
  ret = mqtt_message_build(pubrec, NULL);
  if (ret == 0) {
    memset(buff.str, 0, buff.length);
    mqtt_message_dump(pubrec, &buff, 1);
    printf("%s", buff.str);
  }
  else {
    printf("Problem on building pubrec example : %d\n", ret);
  }
  mqtt_message_destroy(pubrec);

  /* PUBCOMP */
  mqtt_message* pubcomp = mqtt_message_create_empty();
  pubcomp->packet_type = MQTT_PUBCOMP;
  pubcomp->vh.pubcomp_vh.packet_id = 3;
  ret = mqtt_message_build(pubcomp, NULL);
  if (ret == 0) {
    memset(buff.str, 0, buff.length);
    mqtt_message_dump(pubcomp, &buff, 1);
    printf("%s", buff.str);
  }
  else {
    printf("Problem on building pubcomp example : %d\n", ret);
  }
  mqtt_message_destroy(pubcomp);

  /* SUBSCRIBE */
  mqtt_message* submsg = mqtt_message_create_empty();
  submsg->packet_type = MQTT_SUBSCRIBE;
#ifdef USE_STATIC_ARRAY
  submsg->pld.subscribe_pld.topics[0].topic_filter.str = (unsigned char*)"sub/topic/one";
  submsg->pld.subscribe_pld.topics[0].topic_filter.length = strlen("sub/topic/one");
  submsg->pld.subscribe_pld.topics[0].qos = 2;
  submsg->pld.subscribe_pld.topics[1].topic_filter.str = (unsigned char*)"sub/topic/two";
  submsg->pld.subscribe_pld.topics[1].topic_filter.length = strlen("sub/topic/two");
  submsg->pld.subscribe_pld.topics[1].qos = 0;
  submsg->pld.subscribe_pld.topics[2].topic_filter.str = (unsigned char*)"sub/topic/three";
  submsg->pld.subscribe_pld.topics[2].topic_filter.length = strlen("sub/topic/three");
  submsg->pld.subscribe_pld.topics[2].qos = 1;
#else
  mqtt_topic topics[3];
  topics[0].topic_filter.str = (unsigned char*)"sub/topic/one";
  topics[0].topic_filter.length = strlen((const char*)topics[0].topic_filter.str);
  topics[0].qos = 2;
  topics[1].topic_filter.str = (unsigned char*)"sub/topic/two";
  topics[1].topic_filter.length = strlen((const char*)topics[1].topic_filter.str);
  topics[1].qos = 0;
  topics[2].topic_filter.str = (unsigned char*)"sub/topic/three";
  topics[2].topic_filter.length = strlen((const char*)topics[2].topic_filter.str);
  topics[2].qos = 1;
  submsg->pld.subscribe_pld.topics = &topics[0];
#endif
  submsg->pld.subscribe_pld.topic_count = 3;
  submsg->vh.subscribe_vh.packet_id = 45;

  ret = mqtt_message_build(submsg, NULL);
  if (ret == 0) {
    memset(buff.str, 0, buff.length);
    mqtt_message_dump(submsg, &buff, 1);
    printf("%s", buff.str);
  }
  else {
    printf("Problem on building subscribe example : %d\n", ret);
  }
  mqtt_message_destroy(submsg);

  /* SUBACK */
  mqtt_message* suback = mqtt_message_create_empty();
  suback->packet_type = MQTT_SUBACK;
  suback->pld.suback_pld.retcode_count = 3;
#ifdef USE_STATIC_ARRAY

#else
  suback->pld.suback_pld.return_codes = malloc(suback->pld.suback_pld.retcode_count*sizeof(uint8_t));
#endif
  suback->pld.suback_pld.return_codes[0] = 0;
  suback->pld.suback_pld.return_codes[1] = 2;
  suback->pld.suback_pld.return_codes[2] = 0x80; /* failure */
  suback->vh.suback_vh.packet_id = 45;
  ret = mqtt_message_build(suback, NULL);
  if (ret == 0) {
    memset(buff.str, 0, buff.length);
    mqtt_message_dump(suback, &buff, 1);
    printf("%s", buff.str);
  }
  else {
    printf("Problem on building suback example : %d\n", ret);
  }
  /* deallocate return-codes array */
#ifndef USE_STATIC_ARRAY
  free(suback->pld.suback_pld.return_codes);
#endif
  mqtt_message_destroy(suback);

  /* PUBLISH */
  mqtt_message* pubmsg = mqtt_message_create_empty();
  pubmsg->packet_type = MQTT_PUBLISH;
  pubmsg->vh.publish_vh.packet_id = 876;
  pubmsg->vh.publish_vh.topic_name.str = (unsigned char*)"/oneM2M/req/CSE3409165/CSE1534123/JSON";
  pubmsg->vh.publish_vh.topic_name.length = strlen("/oneM2M/req/CSE3409165/CSE1534123/JSON");
  pubmsg->pld.publish_pld.payload.str = (unsigned char*)"{\"fr\" : \"/CSE3409165\",\"op\" : 1}";
  pubmsg->pld.publish_pld.payload.length = strlen("{\"fr\" : \"/CSE3409165\",\"op\" : 1}");
  mqtt_fixed_hdr_flags flgs;
  flgs.all.pub_flags.dup = 0;
  flgs.all.pub_flags.qos = 2;
  flgs.all.pub_flags.retain = 1;
  ret = mqtt_message_build(pubmsg, &flgs);
  if (ret == 0) {
    memset(buff.str, 0, buff.length);
    mqtt_message_dump(pubmsg, &buff, 1);
    printf("%s", buff.str);
  }
  else {
    printf("Problem on building pubmsg example : %d\n", ret);
  }
  mqtt_message_destroy(pubmsg);

  /* UNSUBSCRIBE */
  mqtt_message* unsub = mqtt_message_create_empty();
  unsub->packet_type = MQTT_UNSUBSCRIBE;
  unsub->vh.unsubscribe_vh.packet_id = 46;
#ifdef USE_STATIC_ARRAY
  unsub->pld.unsub_pld.topics[0].str = (unsigned char*)"sub/topic/one";
  unsub->pld.unsub_pld.topics[0].length = strlen("sub/topic/one");
  unsub->pld.unsub_pld.topics[1].str = (unsigned char*)"sub/topic/three";
  unsub->pld.unsub_pld.topics[1].length = strlen("sub/topic/three");
#else
  cstr_t untopics[2];
  untopics[0].str = (unsigned char*)"sub/topic/one";
  untopics[0].length = strlen("sub/topic/one");
  untopics[1].str = (unsigned char*)"sub/topic/three";
  untopics[1].length = strlen("sub/topic/three");
  unsub->pld.unsub_pld.topics = &untopics[0];
#endif
  unsub->pld.unsub_pld.topic_count = 2;
  ret = mqtt_message_build(unsub, NULL);
  if (ret == 0) {
    memset(buff.str, 0, buff.length);
    mqtt_message_dump(unsub, &buff, 1);
    printf("%s", buff.str);
  }
  else {
    printf("Problem on building unsubscribe example : %d\n", ret);
  }
  mqtt_message_destroy(unsub);

  /* UNSUBACK */
  mqtt_message* unsuback = mqtt_message_create_empty();
  unsuback->packet_type = MQTT_UNSUBACK;
  unsuback->vh.unsuback_vh.packet_id = 46;
  ret = mqtt_message_build(pubrec, NULL);
  if (ret == 0) {
    memset(buff.str, 0, buff.length);
    mqtt_message_dump(unsuback, &buff, 1);
    printf("%s", buff.str);
  }
  else {
    printf("Problem on building unsuback example : %d\n", ret);
  }
  mqtt_message_destroy(unsuback);

  /* PINGREQ */
  mqtt_message* pingreq = mqtt_message_create_empty();
  pingreq->packet_type = MQTT_PINGREQ;
  ret = mqtt_message_build(pingreq, NULL);
  if (ret == 0) {
    memset(buff.str, 0, buff.length);
    mqtt_message_dump(pingreq, &buff, 1);
    printf("%s", buff.str);
  }
  else {
    printf("Problem on building pingreq example : %d\n", ret);
  }
  mqtt_message_destroy(pingreq);

  /* PINGRESP */
  mqtt_message* pingresp = mqtt_message_create_empty();
  pingresp->packet_type = MQTT_PINGRESP;
  ret = mqtt_message_build(pingresp, NULL);
  if (ret == 0) {
    memset(buff.str, 0, buff.length);
    mqtt_message_dump(pingresp, &buff, 1);
    printf("%s", buff.str);
  }
  else {
    printf("Problem on building pingresp example : %d\n", ret);
  }
  mqtt_message_destroy(pingresp);

  /* DISCONNECT */
  mqtt_message* disconn = mqtt_message_create_empty();
  disconn->packet_type = MQTT_DISCONNECT;
  ret = mqtt_message_build(disconn, NULL);
  if (ret == 0) {
    memset(buff.str, 0, buff.length);
    mqtt_message_dump(disconn, &buff, 1);
    printf("%s", buff.str);
  }
  else {
    printf("Problem on building disconnect example : %d\n", ret);
  }
  mqtt_message_destroy(disconn);
}

/* After building completion, data pointer is mqtt_message structure re=arranged
 * to point into the raw data has been built. So the external data passed during
 * the building can be deallocated from the memory. After that the mqtt_message
 * instance reflects a compact structure which is independent of external data.
 * */
void test_for_insitu_on_building(void)
{
  int ret = 0;
  unsigned char buffer[BUFFSIZE];
  cstr_t buff;
  buff.str = &buffer[0];
  buff.length = BUFFSIZE;
  char* tpn = "/oneM2M/req/CSE3409165/CSE1534123/JSON";
  int tpn_len = strlen(tpn);
  char* pyd = "{\"fr\" : \"/CSE3409165\",\"op\" : 1}";
  int pyd_len = strlen(pyd);
  char* topic_name = malloc(tpn_len);
  char* payload = malloc(pyd_len);
  memcpy(topic_name, tpn, tpn_len);
  memcpy(payload, pyd, pyd_len);

  /* PUBLISH */
  mqtt_message* pubmsg = mqtt_message_create_empty();
  pubmsg->packet_type = MQTT_PUBLISH;
  pubmsg->vh.publish_vh.packet_id = 876;
  pubmsg->vh.publish_vh.topic_name.str = (unsigned char*)topic_name;
  pubmsg->vh.publish_vh.topic_name.length = tpn_len;
  pubmsg->pld.publish_pld.payload.str = (unsigned char*)payload;
  pubmsg->pld.publish_pld.payload.length = pyd_len;
  mqtt_fixed_hdr_flags flgs;
  flgs.all.pub_flags.dup = 0;
  flgs.all.pub_flags.qos = 2;
  flgs.all.pub_flags.retain = 1;
  ret = mqtt_message_build(pubmsg, &flgs);
  if (ret == 0) {
    /* free allocated texts to verify that now built message uses inner raw data */
    free(topic_name);
    topic_name = NULL;
    free(payload);
    payload = NULL;
    memset(buff.str, 0, buff.length);
    mqtt_message_dump(pubmsg, &buff, 1);
    printf("%s", buff.str);
  }
  else {
    printf("Problem on building pubmsg example : %d\n", ret);
  }
  mqtt_message_destroy(pubmsg);
}

int main(int argc, char *argv[])
{
  parsing_test();

  building_test();

  test_for_insitu_on_building();

  return 0;
}
