
#include "mqtt_types.h"
#include "mqtt_message.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

struct rawposbuf
{
  unsigned char* curpos;
  unsigned char* endpos;
};

mqtt_message* mqtt_message_create_empty(void)
{
  mqtt_message* msg = (mqtt_message*)malloc(sizeof(mqtt_message));
  memset((char*)msg, 0, sizeof(mqtt_message));

  return msg;
}

int mqtt_message_destroy(mqtt_message* self)
{
  if (self->whl_raw_msg.str) {
    /* If we are builder or a parser that have an attached raw data,
     * we should destroy raw data too. */
    if ((!self->parser) || (self->parser && self->attached_raw)) {
      free(self->whl_raw_msg.str);
    }
    self->whl_raw_msg.str = NULL;
    self->whl_raw_msg.length = 0;
  }
  free(self);

  return 0;
}

int byte_number_for_variable_length(uint32_t variable)
{
  if (variable < 128) {
    return 1;
  }
  else if (variable < 16384) {
    return 2;
  }
  else if (variable < 2097152) {
    return 3;
  }
  else if (variable < 268435456) {
    return 4;
  }
  return 5;
}

int write_variable_length_value(uint32_t value, struct rawposbuf *buf)
{
  uint8_t byte;
  int count = 0;

  do {
    byte = value % 128;
    value = value / 128;
    /* If there are more digits to encode, set the top bit of this digit */
    if (value > 0) {
      byte = byte | 0x80;
    }
    *(buf->curpos++) = byte;
    count++;
  } while (value > 0 && count < 5);

  if (count == 5) {
    return -1;
  }
  return count;
}

int write_byte(uint8_t val, struct rawposbuf *buf)
{
  if ((buf->endpos - buf->curpos) < 1) {
    return MQTT_ERR_NOMEM;
  }

  *(buf->curpos++) = val;

  return 0;
}

int write_uint16(uint16_t value, struct rawposbuf *buf)
{
  if ((buf->endpos - buf->curpos) < 2) {
    return MQTT_ERR_NOMEM;
  }

  *(buf->curpos++) = (value >> 8) & 0xFF;
  *(buf->curpos++) = value & 0xFF;

  return 0;
}

int write_byte_string(cstr_t *str, struct rawposbuf *buf)
{
  if ((buf->endpos - buf->curpos) < (str->length + 2)) {
    return MQTT_ERR_NOMEM;
  }
  write_uint16(str->length, buf);

  memcpy(buf->curpos, str->str, str->length);
  str->str = buf->curpos; /* reset data position to indicate data in raw data block */
  buf->curpos += str->length;

  return 0;
}

uint8_t encode_connect_flags(conn_flags *flags_set)
{
  uint8_t connflags = 0;
  if (flags_set->clean_session) {
    connflags |= CONN_FLAG_CLEAN_SESSION;
  }
  if (flags_set->will_flag) {
    connflags |= CONN_FLAG_WILL_FLAG;
  }
  if (flags_set->will_qos) {
    connflags |= ((flags_set->will_qos & 0x3) << 3);
  }
  if (flags_set->will_retain) {
    connflags |= CONN_FLAG_WILL_RETAIN;
  }
  if (flags_set->password_flag) {
    connflags |= CONN_FLAG_PASSWORD;
  }
  if (flags_set->username_flag) {
    connflags |= CONN_FLAG_USER_NAME;
  }
  return connflags;
}

int decode_connect_flags(uint8_t connflags, conn_flags *flags_set)
{
  flags_set->clean_session = ((connflags & CONN_FLAG_CLEAN_SESSION) > 0) ? 1 : 0;
  flags_set->will_flag = ((connflags & CONN_FLAG_WILL_FLAG) == CONN_FLAG_WILL_FLAG) ? 1 : 0;
  flags_set->will_qos = ((connflags & CONN_FLAG_WILL_QOS) >> 3);
  flags_set->will_retain = ((connflags & CONN_FLAG_WILL_RETAIN) == CONN_FLAG_WILL_RETAIN) ? 1 : 0;
  flags_set->password_flag = ((connflags & CONN_FLAG_PASSWORD) == CONN_FLAG_PASSWORD) ? 1 : 0;
  flags_set->username_flag = ((connflags & CONN_FLAG_USER_NAME) == CONN_FLAG_USER_NAME) ? 1 : 0;

  return 0;
}

int is_clean_session(uint8_t connflags)
{
  return ((connflags & CONN_FLAG_CLEAN_SESSION) > 0) ? 1 : 0;
}

int is_will_retain(uint8_t connflags)
{
  return ((connflags & CONN_FLAG_WILL_RETAIN) == CONN_FLAG_WILL_RETAIN) ? 1 : 0;
}

uint8_t will_qos(uint8_t connflags)
{
  return ((connflags & CONN_FLAG_WILL_QOS) >> 3);
}

int is_connection_control_message(uint8_t packtype)
{
  int result = 0;
  switch (packtype) {
    case MQTT_CONNECT:
    case MQTT_CONNACK:
    case MQTT_PINGREQ:
    case MQTT_PINGRESP:
    case MQTT_DISCONNECT:
      result = 1;
      break;
  }
  return result;
}

int is_request_type_app_message(uint8_t packtype)
{
  int result = 0;
  switch (packtype) {
    case MQTT_PUBLISH:
    case MQTT_SUBSCRIBE:
    case MQTT_UNSUBSCRIBE:
      result = 1;
      break;
  }
  return result;
}

int is_application_message(uint8_t packtype, int* isrequest)
{
  int result = 0;
  *isrequest = 0;
  switch (packtype)
  {
    case MQTT_PUBLISH:
    case MQTT_SUBSCRIBE:
    case MQTT_UNSUBSCRIBE:
    case MQTT_PUBREL:
      result = 1;
      *isrequest = 1;
      break;

    case MQTT_PUBACK:
    case MQTT_PUBREC:
    //case MQTT_PUBREL:
    case MQTT_PUBCOMP:
    case MQTT_SUBACK:
      result = 1;
      break;
  }
  return result;
}

int is_packet_identifier_included(uint8_t packet_type, uint8_t packflags)
{
  /* Mqtt 3.1.1: SUBSCRIBE, UNSUBSCRIBE, and PUBLISH (in cases where QoS > 0)
   * Control Packets MUST contain a non-zero 16-bit Packet Identifier
   */
  switch (packet_type) {
    case MQTT_SUBSCRIBE:
    case MQTT_SUBACK:
    case MQTT_UNSUBSCRIBE:
    case MQTT_UNSUBACK:
    case MQTT_PUBACK:
    case MQTT_PUBREC:
    case MQTT_PUBREL:
    case MQTT_PUBCOMP:
      return 1;

    case MQTT_PUBLISH:
    {
      uint8_t qos = ((packflags & PK_PUBLISH_QOS) >> 1);
      if (qos > 0) {
        return 1;
      }
      return 0;
    }

    default:
      return 0;
  }
  return 0;
}

/*============================================================================
 * Message building
 *===========================================================================*/

int build_connect_message(mqtt_message* msg, mqtt_fixed_hdr_flags* fixed_flags)
{
  /* we try to calculate the length of the possible raw data by using the provided data */
  uint8_t connflags = 0;
  int poslength = 6; /* 'length' part of Protocol Name(2) +
                        Protocol Level/Version(1) +
                        Connect Flags(1) +
                        Keep Alive(2) */

  mqtt_connect_vhdr* vhdr = &msg->vh.connect_vh;

  /* length of protocol-name (consider "MQTT" by default */
  poslength += (vhdr->protocol_name.length == 0) ? 4 : vhdr->protocol_name.length;

  /* add the length of payload part */
  mqtt_connect_pld* pld = &msg->pld.connect_pld;
  /* client identifier. 0 length of client identifier may be allowed (in this case, server
   * may produce a client identifier to identify the client internally)
   */
  poslength += 2 + pld->client_id.length; /* '2' is for length field */

  /* Will Topic */
  if (pld->will_topic.length > 0) {
    poslength += 2 + pld->will_topic.length;
    connflags |= CONN_FLAG_WILL_FLAG;
  }
  /* Will Message */
  if (pld->will_msg.length > 0) {
    poslength += 2 + pld->will_msg.length;
    connflags |= CONN_FLAG_WILL_FLAG;
  }
  /* User Name */
  if (pld->user_name.length > 0) {
    poslength += 2 + pld->user_name.length;
    connflags |= CONN_FLAG_USER_NAME;
  }
  /* Password */
  if (pld->password.length > 0) {
    poslength += 2 + pld->password.length;
    connflags |= CONN_FLAG_PASSWORD;
  }
  msg->remaining_length = poslength;
  if (msg->remaining_length > MQTT_MAX_MSG_LEN) {
    return MQTT_ERR_PAYLOAD_SIZE;
  }
  uint32_t hdrlen = byte_number_for_variable_length(msg->remaining_length) + 1;

  uint32_t totallength = poslength + hdrlen;

  msg->whl_raw_msg.length = totallength;
  msg->whl_raw_msg.str = (unsigned char*)malloc(totallength);
  memset(msg->whl_raw_msg.str, 0, msg->whl_raw_msg.length);

  struct rawposbuf buf;
  buf.curpos = &msg->whl_raw_msg.str[0];
  buf.endpos = &msg->whl_raw_msg.str[msg->whl_raw_msg.length];

  write_byte(MQTT_CONNECT, &buf);

  /* Remaining Length */
  msg->rl_byteCount = write_variable_length_value(poslength, &buf);

  /* Protocol Name */
  if (vhdr->protocol_name.length == 0) {
    vhdr->protocol_name.str = (unsigned char*)"MQTT";
    vhdr->protocol_name.length = 4;
  }
  write_byte_string(&vhdr->protocol_name, &buf);

  /* Protocol Level/Version */
  write_byte(vhdr->protocol_level, &buf);

  /* Connect Flags */
  write_byte(vhdr->conn_flags, &buf);

  /* Keep Alive */
  write_uint16(vhdr->keep_alive, &buf);

  /* Now we are in payload part */

  /* Client Identifier */
  /* Client Identifier is mandatory */
  write_byte_string(&pld->client_id, &buf);

  /* Will Topic */
  if (pld->will_topic.length) {
    if (!(vhdr->conn_flags & CONN_FLAG_WILL_FLAG)) {
      return MQTT_ERR_PROTOCOL;
    }
    write_byte_string(&pld->will_topic, &buf);
  }
  else {
    if (vhdr->conn_flags & CONN_FLAG_WILL_FLAG) {
      return MQTT_ERR_PROTOCOL;
    }
  }

  /* Will Message */
  if (pld->will_msg.length) {
    if (!(vhdr->conn_flags & CONN_FLAG_WILL_FLAG)) {
      return MQTT_ERR_PROTOCOL;
    }
    write_byte_string(&pld->will_msg, &buf);
  }
  else {
    if (vhdr->conn_flags & CONN_FLAG_WILL_FLAG) {
      return MQTT_ERR_PROTOCOL;
    }
  }

  /* User-Name */
  if (pld->user_name.length) {
    if (!(vhdr->conn_flags & CONN_FLAG_USER_NAME)) {
      return MQTT_ERR_PROTOCOL;
    }
    write_byte_string(&pld->user_name, &buf);
  }
  else {
    if (vhdr->conn_flags & CONN_FLAG_USER_NAME) {
      return MQTT_ERR_PROTOCOL;
    }
  }

  /* Password */
  if (pld->password.length) {
    if (!(vhdr->conn_flags & CONN_FLAG_PASSWORD)) {
      return MQTT_ERR_PROTOCOL;
    }
    write_byte_string(&pld->password, &buf);
  }
  else {
    if (vhdr->conn_flags & CONN_FLAG_PASSWORD) {
      return MQTT_ERR_PROTOCOL;
    }
  }

  return MQTT_SUCCESS;
}

int build_connack_message(mqtt_message* msg, mqtt_fixed_hdr_flags* fixed_flags)
{
  /* we try to calculate the length of the possible raw data by using the provided data */
  int poslength = 2; /* ConnAck Flags(1) + Connect Return Code(1) */

  mqtt_connack_vhdr* vhdr = &msg->vh.connack_vh;

  msg->remaining_length = poslength;
  uint32_t hdrlen = byte_number_for_variable_length(msg->remaining_length) + 1;
  uint32_t totallength = poslength + hdrlen;

  msg->whl_raw_msg.length = totallength;
  msg->whl_raw_msg.str = (unsigned char*)malloc(totallength);
  memset(msg->whl_raw_msg.str, 0, msg->whl_raw_msg.length);

  struct rawposbuf buf;
  buf.curpos = &msg->whl_raw_msg.str[0];
  buf.endpos = &msg->whl_raw_msg.str[msg->whl_raw_msg.length];

  write_byte(MQTT_CONNACK, &buf);

  /* Remaining Length */
  msg->rl_byteCount = write_variable_length_value(poslength, &buf);

  /* Connect Acknowledge Flags */
  write_byte(vhdr->connack_flags, &buf);

  /* Connect Return Code */
  write_byte(vhdr->conn_return_code, &buf);

  return MQTT_SUCCESS;
}

/* [MQTT-3.8.1-1]: Bits 3,2,1 and 0 of the fixed header of
   the SUBSCRIBE Control Packet are reserved and MUST be set
   to 0,0,1 and 0 respectively. The Server MUST treat any other
   value as malformed and close the Network Connection */
#define SUBSCRIBE_PACKFLAGS 0x02

int build_subscribe_message(mqtt_message* msg, mqtt_fixed_hdr_flags* fixed_flags)
{
  /* we try to calculate the length of the possible raw data by using the provided data */
  int poslength = 0;

  poslength += 2; /* for Packet Identifier */

  mqtt_subscribe_pld* spld = &msg->pld.subscribe_pld;

  /* Go through topic filters to calculate length information */
  for (size_t i = 0; i < spld->topic_count; i++) {
    mqtt_topic* topic = &spld->topics[i];
    poslength += topic->topic_filter.length;
    poslength += 1; // for 'options' byte
    poslength += 2; // for 'length' field of Topic Filter, which is encoded as UTF-8 encoded strings */
  }

  msg->remaining_length = poslength;
  uint32_t hdrlen = byte_number_for_variable_length(msg->remaining_length) + 1;
  uint32_t totallength = poslength + hdrlen;

  msg->whl_raw_msg.length = totallength;
  msg->whl_raw_msg.str = (unsigned char*)malloc(totallength);
  memset(msg->whl_raw_msg.str, 0, msg->whl_raw_msg.length);

  struct rawposbuf buf;
  buf.curpos = &msg->whl_raw_msg.str[0];
  buf.endpos = &msg->whl_raw_msg.str[msg->whl_raw_msg.length];

  write_byte((MQTT_SUBSCRIBE | SUBSCRIBE_PACKFLAGS), &buf);
  /* set 'flags' of message for debug purposes */
  msg->flags = SUBSCRIBE_PACKFLAGS;

  /* Remaining Length */
  msg->rl_byteCount = write_variable_length_value(poslength, &buf);

  mqtt_subscribe_vhdr* vhdr = &msg->vh.subscribe_vh;
  /* Packet Id */
  write_uint16(vhdr->packet_id, &buf);

  /* Subscribe topics */
  for (size_t i = 0; i < spld->topic_count; i++) {
    mqtt_topic* topic = &spld->topics[i];
    write_byte_string(&topic->topic_filter, &buf);
    write_byte(topic->qos, &buf);
  }

  return MQTT_SUCCESS;
}

int build_suback_message(mqtt_message* msg, mqtt_fixed_hdr_flags* fixed_flags)
{
  /* we try to calculate the length of the possible raw data by using the provided data */
  int poslength = 2; /* for Packet Identifier */

  mqtt_suback_vhdr* vhdr = &msg->vh.suback_vh;
  mqtt_suback_pld* spld = &msg->pld.suback_pld;

  poslength += spld->retcode_count;

  msg->remaining_length = poslength;
  uint32_t hdrlen = byte_number_for_variable_length(msg->remaining_length) + 1;
  uint32_t totallength = poslength + hdrlen;

  msg->whl_raw_msg.length = totallength;
  msg->whl_raw_msg.str = (unsigned char*)malloc(totallength);
  memset(msg->whl_raw_msg.str, 0, msg->whl_raw_msg.length);

  struct rawposbuf buf;
  buf.curpos = &msg->whl_raw_msg.str[0];
  buf.endpos = &msg->whl_raw_msg.str[msg->whl_raw_msg.length];

  write_byte(MQTT_SUBACK, &buf);

  /* Remaining Length */
  msg->rl_byteCount = write_variable_length_value(poslength, &buf);

  /* Packet Identifier */
  write_uint16(vhdr->packet_id, &buf);

  for (uint32_t i = 0; i < spld->retcode_count; i++) {
    write_byte(spld->return_codes[i], &buf);
  }

  return MQTT_SUCCESS;
}

int build_publish_message(mqtt_message* msg, mqtt_fixed_hdr_flags* fixed_flags)
{
  /* we try to calculate the length of the possible raw data by using the provided data */
  int poslength = 0;

  poslength += 2; /* for Topic Name length field */
  poslength += msg->vh.publish_vh.topic_name.length;
  /* Packet Identifier is requested if QoS>0 */
  if (fixed_flags->all.pub_flags.qos > 0)
  {
    poslength += 2; /* for Packet Identifier */
  }
  poslength += msg->pld.publish_pld.payload.length;

  msg->remaining_length = poslength;
  uint32_t hdrlen = byte_number_for_variable_length(msg->remaining_length) + 1;
  uint32_t totallength = poslength + hdrlen;

  msg->whl_raw_msg.length = totallength;
  msg->whl_raw_msg.str = (unsigned char*)malloc(totallength);
  memset(msg->whl_raw_msg.str, 0, msg->whl_raw_msg.length);

  struct rawposbuf buf;
  buf.curpos = &msg->whl_raw_msg.str[0];
  buf.endpos = &msg->whl_raw_msg.str[msg->whl_raw_msg.length];

  uint8_t pubflags = 0;
  pubflags |= ((fixed_flags->all.pub_flags.qos & 0x3) << 1);
  if (fixed_flags->all.pub_flags.dup) {
    pubflags |= PK_PUBLISH_FLAG_DUP;
  }
  if (fixed_flags->all.pub_flags.retain) {
    pubflags |= PK_PUBLISH_FLAG_RETAIN;
  }
  write_byte((MQTT_PUBLISH | pubflags), &buf);

  /* set 'flags' of message for debug purposes */
  msg->flags = pubflags;

  /* Remaining Length */
  msg->rl_byteCount = write_variable_length_value(poslength, &buf);

  mqtt_publish_vhdr* vhdr = &msg->vh.publish_vh;

  /* Topic Name */
  write_byte_string(&vhdr->topic_name, &buf);

  if (fixed_flags->all.pub_flags.qos > 0) {
    /* Packet Id */
    write_uint16(vhdr->packet_id, &buf);
  }

  /* Payload */
  if (msg->pld.publish_pld.payload.length > 0) {
    memcpy(buf.curpos, msg->pld.publish_pld.payload.str, msg->pld.publish_pld.payload.length);
    msg->pld.publish_pld.payload.str = buf.curpos; /* reset data position to indicate data in raw data block */
  }

  return MQTT_SUCCESS;
}

int build_puback_message(mqtt_message* msg, mqtt_fixed_hdr_flags* fixed_flags)
{
  /* we try to calculate the length of the possible raw data by using the provided data */
  int poslength = 2; /* for Packet Identifier */

  mqtt_puback_vhdr* vhdr = &msg->vh.puback_vh;

  msg->remaining_length = poslength;
  uint32_t hdrlen = byte_number_for_variable_length(msg->remaining_length) + 1;
  uint32_t totallength = poslength + hdrlen;

  msg->whl_raw_msg.length = totallength;
  msg->whl_raw_msg.str = (unsigned char*)malloc(totallength);
  memset(msg->whl_raw_msg.str, 0, msg->whl_raw_msg.length);

  struct rawposbuf buf;
  buf.curpos = &msg->whl_raw_msg.str[0];
  buf.endpos = &msg->whl_raw_msg.str[msg->whl_raw_msg.length];

  write_byte(MQTT_PUBACK, &buf);

  /* Remaining Length */
  msg->rl_byteCount = write_variable_length_value(poslength, &buf);

  /* Packet Identifier */
  write_uint16(vhdr->packet_id, &buf);

  return MQTT_SUCCESS;
}

int build_pubrec_message(mqtt_message* msg, mqtt_fixed_hdr_flags* fixed_flags)
{
  /* we try to calculate the length of the possible raw data by using the provided data */
  int poslength = 2; /* for Packet Identifier */

  mqtt_pubrec_vhdr* vhdr = &msg->vh.pubrec_vh;

  msg->remaining_length = poslength;
  uint32_t hdrlen = byte_number_for_variable_length(msg->remaining_length) + 1;
  uint32_t totallength = poslength + hdrlen;

  msg->whl_raw_msg.length = totallength;
  msg->whl_raw_msg.str = (unsigned char*)malloc(totallength);
  memset(msg->whl_raw_msg.str, 0, msg->whl_raw_msg.length);

  struct rawposbuf buf;
  buf.curpos = &msg->whl_raw_msg.str[0];
  buf.endpos = &msg->whl_raw_msg.str[msg->whl_raw_msg.length];

  write_byte(MQTT_PUBREC, &buf);

  /* Remaining Length */
  msg->rl_byteCount = write_variable_length_value(poslength, &buf);

  /* Packet Identifier */
  write_uint16(vhdr->packet_id, &buf);

  return MQTT_SUCCESS;
}

int build_pubrel_message(mqtt_message* msg, mqtt_fixed_hdr_flags* fixed_flags)
{
  /* we try to calculate the length of the possible raw data by using the provided data */
  int poslength = 2; /* for Packet Identifier */

  mqtt_pubrel_vhdr* vhdr = &msg->vh.pubrel_vh;

  msg->remaining_length = poslength;
  uint32_t hdrlen = byte_number_for_variable_length(msg->remaining_length) + 1;
  uint32_t totallength = poslength + hdrlen;

  msg->whl_raw_msg.length = totallength;
  msg->whl_raw_msg.str = (unsigned char*)malloc(totallength);
  memset(msg->whl_raw_msg.str, 0, msg->whl_raw_msg.length);

  struct rawposbuf buf;
  buf.curpos = &msg->whl_raw_msg.str[0];
  buf.endpos = &msg->whl_raw_msg.str[msg->whl_raw_msg.length];

  write_byte((MQTT_PUBREL | 0x02), &buf);

  /* Remaining Length */
  msg->rl_byteCount = write_variable_length_value(poslength, &buf);

  /* Packet Identifier */
  write_uint16(vhdr->packet_id, &buf);

  return MQTT_SUCCESS;
}

int build_pubcomp_message(mqtt_message* msg, mqtt_fixed_hdr_flags* fixed_flags)
{
  /* we try to calculate the length of the possible raw data by using the provided data */
  int poslength = 2; /* for Packet Identifier */

  mqtt_pubcomp_vhdr* vhdr = &msg->vh.pubcomp_vh;

  msg->remaining_length = poslength;
  uint32_t hdrlen = byte_number_for_variable_length(msg->remaining_length) + 1;
  uint32_t totallength = poslength + hdrlen;

  msg->whl_raw_msg.length = totallength;
  msg->whl_raw_msg.str = (unsigned char*)malloc(totallength);
  memset(msg->whl_raw_msg.str, 0, msg->whl_raw_msg.length);

  struct rawposbuf buf;
  buf.curpos = &msg->whl_raw_msg.str[0];
  buf.endpos = &msg->whl_raw_msg.str[msg->whl_raw_msg.length];

  write_byte(MQTT_PUBCOMP, &buf);

  /* Remaining Length */
  msg->rl_byteCount = write_variable_length_value(poslength, &buf);

  /* Packet Identifier */
  write_uint16(vhdr->packet_id, &buf);

  return MQTT_SUCCESS;
}

/* [MQTT-3.8.1-1]: Bits 3,2,1 and 0 of the fixed header of
   the UNSUBSCRIBE Control Packet are reserved and MUST be set
   to 0,0,1 and 0 respectively. The Server MUST treat any other
   value as malformed and close the Network Connection */
#define UNSUBSCRIBE_PACKFLAGS 0x02

int build_unsubscribe_message(mqtt_message* msg, mqtt_fixed_hdr_flags* fixed_flags)
{
  /* we try to calculate the length of the possible raw data by using the provided data */
  int poslength = 0;

  poslength += 2; /* for Packet Identifier */

  mqtt_unsubscribe_pld* uspld = &msg->pld.unsub_pld;

  /* Go through topic filters to calculate length information */
  for (size_t i = 0; i < uspld->topic_count; i++) {
    cstr_t* topic = &uspld->topics[i];
    poslength += topic->length;
    poslength += 2; // for 'length' field of Topic Filter, which is encoded as UTF-8 encoded strings */
  }

  msg->remaining_length = poslength;
  uint32_t hdrlen = byte_number_for_variable_length(msg->remaining_length) + 1;
  uint32_t totallength = poslength + hdrlen;

  msg->whl_raw_msg.length = totallength;
  msg->whl_raw_msg.str = (unsigned char*)malloc(totallength);
  memset(msg->whl_raw_msg.str, 0, msg->whl_raw_msg.length);

  struct rawposbuf buf;
  buf.curpos = &msg->whl_raw_msg.str[0];
  buf.endpos = &msg->whl_raw_msg.str[msg->whl_raw_msg.length];

  write_byte((MQTT_UNSUBSCRIBE | UNSUBSCRIBE_PACKFLAGS), &buf);
  /* set 'flags' of message for debug purposes */
  msg->flags = UNSUBSCRIBE_PACKFLAGS;

  /* Remaining Length */
  msg->rl_byteCount = write_variable_length_value(poslength, &buf);

  mqtt_subscribe_vhdr* vhdr = &msg->vh.subscribe_vh;
  /* Packet Id */
  write_uint16(vhdr->packet_id, &buf);

  /* Subscribe topics */
  for (size_t i = 0; i < uspld->topic_count; i++) {
    cstr_t* topic = &uspld->topics[i];
    write_byte_string(topic, &buf);
  }

  return MQTT_SUCCESS;
}

int build_unsuback_message(mqtt_message* msg, mqtt_fixed_hdr_flags* fixed_flags)
{
  /* we try to calculate the length of the possible raw data by using the provided data */
  int poslength = 2; /* for Packet Identifier */

  mqtt_unsuback_vhdr* vhdr = &msg->vh.unsuback_vh;

  msg->remaining_length = poslength;
  uint32_t hdrlen = byte_number_for_variable_length(msg->remaining_length) + 1;
  uint32_t totallength = poslength + hdrlen;

  msg->whl_raw_msg.length = totallength;
  msg->whl_raw_msg.str = (unsigned char*)malloc(totallength);
  memset(msg->whl_raw_msg.str, 0, msg->whl_raw_msg.length);

  struct rawposbuf buf;
  buf.curpos = &msg->whl_raw_msg.str[0];
  buf.endpos = &msg->whl_raw_msg.str[msg->whl_raw_msg.length];

  write_byte(MQTT_UNSUBACK, &buf);

  /* Remaining Length */
  msg->rl_byteCount = write_variable_length_value(poslength, &buf);

  /* Packet Identifier */
  write_uint16(vhdr->packet_id, &buf);

  return MQTT_SUCCESS;
}

int build_pingreq_message(mqtt_message* msg, mqtt_fixed_hdr_flags* fixed_flags)
{
  /* we try to calculate the length of the possible raw data by using the provided data */
  int poslength = 0; /* No additional information included in PING message */

  msg->remaining_length = poslength;
  uint32_t hdrlen = byte_number_for_variable_length(msg->remaining_length) + 1;
  uint32_t totallength = poslength + hdrlen;

  msg->whl_raw_msg.length = totallength;
  msg->whl_raw_msg.str = (unsigned char*)malloc(totallength);
  memset(msg->whl_raw_msg.str, 0, msg->whl_raw_msg.length);

  struct rawposbuf buf;
  buf.curpos = &msg->whl_raw_msg.str[0];
  buf.endpos = &msg->whl_raw_msg.str[msg->whl_raw_msg.length];

  write_byte(MQTT_PINGREQ, &buf);

  /* Remaining Length */
  msg->rl_byteCount = write_variable_length_value(poslength, &buf);

  return MQTT_SUCCESS;
}

int build_pingresp_message(mqtt_message* msg, mqtt_fixed_hdr_flags* fixed_flags)
{
  /* we try to calculate the length of the possible raw data by using the provided data */
  int poslength = 0; /* No additional information included in PING message */

  msg->remaining_length = poslength;
  uint32_t hdrlen = byte_number_for_variable_length(msg->remaining_length) + 1;
  uint32_t totallength = poslength + hdrlen;

  msg->whl_raw_msg.length = totallength;
  msg->whl_raw_msg.str = (unsigned char*)malloc(totallength);
  memset(msg->whl_raw_msg.str, 0, msg->whl_raw_msg.length);

  struct rawposbuf buf;
  buf.curpos = &msg->whl_raw_msg.str[0];
  buf.endpos = &msg->whl_raw_msg.str[msg->whl_raw_msg.length];

  write_byte(MQTT_PINGRESP, &buf);

  /* Remaining Length */
  msg->rl_byteCount = write_variable_length_value(poslength, &buf);

  return MQTT_SUCCESS;
}

int build_disconnect_message(mqtt_message* msg, mqtt_fixed_hdr_flags* fixed_flags)
{
  /* we try to calculate the length of the possible raw data by using the provided data */
  int poslength = 0; /* No additional information included in DISCONNECT message */

  msg->remaining_length = poslength;
  uint32_t hdrlen = byte_number_for_variable_length(msg->remaining_length) + 1;
  uint32_t totallength = poslength + hdrlen;

  msg->whl_raw_msg.length = totallength;
  msg->whl_raw_msg.str = (unsigned char*)malloc(totallength);
  memset(msg->whl_raw_msg.str, 0, msg->whl_raw_msg.length);

  struct rawposbuf buf;
  buf.curpos = &msg->whl_raw_msg.str[0];
  buf.endpos = &msg->whl_raw_msg.str[msg->whl_raw_msg.length];

  write_byte(MQTT_DISCONNECT, &buf);

  /* Remaining Length */
  msg->rl_byteCount = write_variable_length_value(poslength, &buf);

  return MQTT_SUCCESS;
}

int mqtt_message_build(mqtt_message* msg, mqtt_fixed_hdr_flags* fixed_flags)
{
  if (!msg) {
    return MQTT_ERR_INVAL;
  }

  switch (msg->packet_type) {
    case MQTT_CONNECT:
      return build_connect_message(msg, fixed_flags);
      break;

    case MQTT_CONNACK:
      return build_connack_message(msg, fixed_flags);
      break;

    case MQTT_SUBSCRIBE:
      return build_subscribe_message(msg, fixed_flags);
      break;

    case MQTT_SUBACK:
      return build_suback_message(msg, fixed_flags);
      break;

    case MQTT_PUBLISH:
      return build_publish_message(msg, fixed_flags);
      break;

    case MQTT_PUBACK:
      return build_puback_message(msg, fixed_flags);
      break;

    case MQTT_PUBREC:
      return build_pubrec_message(msg, fixed_flags);
      break;

    case MQTT_PUBREL:
      return build_pubrel_message(msg, fixed_flags);
      break;

    case MQTT_PUBCOMP:
      return build_pubcomp_message(msg, fixed_flags);
      break;

    case MQTT_UNSUBSCRIBE:
      return build_unsubscribe_message(msg, fixed_flags);
      break;

    case MQTT_UNSUBACK:
      return build_unsuback_message(msg, fixed_flags);
      break;

    case MQTT_PINGREQ:
      return build_pingreq_message(msg, fixed_flags);
      break;

    case MQTT_PINGRESP:
      return build_pingresp_message(msg, fixed_flags);
      break;

    case MQTT_DISCONNECT:
      return build_disconnect_message(msg, fixed_flags);
      break;

    default:
      return MQTT_ERR_PROTOCOL;
  }

  return MQTT_SUCCESS;
}

/*****************************************************************************
 *    Parser Part
 *****************************************************************************/
int read_byte(struct rawposbuf* buf, uint8_t *val)
{
  if ((buf->endpos - buf->curpos) < 1) {
    return MQTT_ERR_NOMEM;
  }

  *val = *(buf->curpos++);

  return 0;
}

int read_uint16(struct rawposbuf* buf, uint16_t *val)
{
  if ((buf->endpos - buf->curpos) < sizeof(uint16_t)) {
    return MQTT_ERR_INVAL;
  }

  *val = *(buf->curpos++) << 8; /* MSB */
  *val |= *(buf->curpos++); /* LSB */

  return 0;
}

int read_utf8_str(struct rawposbuf* buf, cstr_t* val)
{
  uint16_t length  = 0;
  int ret = read_uint16(buf, &length);
  if (ret != 0) {
    return ret;
  }
  if ((buf->endpos - buf->curpos) < length) {
    return MQTT_ERR_INVAL;
  }

  val->length = length;
  /* Zero length UTF8 strings are permitted. */
  if (length > 0) {
    val->str = buf->curpos;
    buf->curpos += length;
  }
  else {
    val->str = NULL;
  }
  return 0;
}

int read_str_data(struct rawposbuf* buf, cstr_t* val)
{
  uint16_t length  = 0;
  int ret = read_uint16(buf, &length);
  if (ret != 0) {
    return ret;
  }
  if ((buf->endpos - buf->curpos) < length) {
    return MQTT_ERR_INVAL;
  }

  val->length = length;
  if (length > 0) {
    val->str = buf->curpos;
    buf->curpos += length;
  }
  else {
    val->str = NULL;
  }
  return 0;
}

int read_packet_length(struct rawposbuf* buf, uint32_t *length)
{
  uint8_t shift = 0;
  uint8_t bytes = 0;

  *length = 0;
  do {
    if (bytes >= MQTT_MAX_MSG_LEN) {
      return MQTT_ERR_INVAL;
    }

    if (buf->curpos >= buf->endpos) {
      return MQTT_ERR_MALFORMED;
    }

    *length += ((uint32_t)*(buf->curpos) & MQTT_LENGTH_VALUE_MASK)
                << shift;
    shift += MQTT_LENGTH_SHIFT;
    bytes++;
  } while ((*(buf->curpos++) & MQTT_LENGTH_CONTINUATION_BIT) != 0U);

  if (*length > MQTT_MAX_MSG_LEN) {
    return MQTT_ERR_INVAL;
  }

  return 0;
}

/* A public utility function providing integer values encoded as variable-int form, such as
 * remaining-length value in the header of MQTT message. '*value' returns the value of
 * variable-int, while '*pos' returns byte number used to encode that integer.
 */
int mqtt_message_read_variable_int(unsigned char* ptr, uint32_t length, uint32_t *value, uint8_t *pos)
{
  int i;
  uint8_t byte;
  int multiplier = 1;
  int32_t lword = 0;
  uint8_t lbytes = 0;
  unsigned char* start = ptr;

  if (!ptr) {
    return MQTT_ERR_PAYLOAD_SIZE;
  }
  for (i = 0; i<4; i++) {
    if ((ptr - start + 1) > length) {
      return MQTT_ERR_PAYLOAD_SIZE;
    }
    lbytes++;
    byte = ptr[0];
    lword += (byte & 127) * multiplier;
    multiplier *= 128;
    ptr++;
    if ((byte & 128) == 0) {
      if (lbytes > 1 && byte == 0) {
        /* Catch overlong encodings */
        return MQTT_ERR_INVAL;
      }
      else {
        *value = lword;
        if (pos) {
          (*pos) = lbytes;
        }
        return MQTT_SUCCESS;
      }
    }
    else {
      //return MQTT_ERR_INVAL;
    }
  }
  return MQTT_ERR_INVAL;
}

int get_packet_identifier(unsigned char* rawdata, uint32_t length, uint8_t prebytes, uint16_t* packid)
{
  *packid = 0;
  uint8_t packtype = (rawdata[0] & 0xf0);
  uint8_t packflags = (rawdata[0] & 0x0f);

  if (is_packet_identifier_included(packtype, packflags) == 0) {
    return MQTT_ERR_NOT_FOUND;
  }
  if (packtype == MQTT_PUBLISH) {
    if ((packflags & PK_PUBLISH_QOS) >> 1) {
      /* We need to skip topic */
      struct rawposbuf buf;
      buf.curpos = &rawdata[prebytes];
      buf.endpos = &rawdata[length];
      uint16_t topiclen;
      if (read_uint16(&buf, &topiclen) == 0) {
        buf.curpos = &rawdata[prebytes + topiclen + 2];
        return read_uint16(&buf, packid);
      }
      else {
        return MQTT_ERR_INVAL;
      }
    }
  }
  else {
    /* For all other message/packet types, including packid, the position of it is the same */
    struct rawposbuf buf;
    buf.curpos = &rawdata[prebytes];
    buf.endpos = &rawdata[length];
    return read_uint16(&buf, packid);
  }
  return MQTT_ERR_NOT_FOUND;
}

int decode_publish_flags(uint8_t flags, mqtt_publish_fixed_hdr_flags* flags_set)
{
  flags_set->dup = flags & PK_PUBLISH_FLAG_DUP;
  flags_set->retain = flags & PK_PUBLISH_FLAG_RETAIN;
  flags_set->qos = ((flags & PK_PUBLISH_QOS) >> 1);

  return 0;
}

int set_dup_flag(unsigned char* rawdata, uint32_t length)
{
  uint8_t packtype = (rawdata[0] & 0xf0);
  if (packtype == MQTT_PUBLISH) {
    rawdata[0] |= PK_PUBLISH_FLAG_DUP;
  }
  return 0;
}

uint8_t get_publish_qos(uint8_t packflags)
{
  return ((packflags & PK_PUBLISH_QOS) >> 1);
}

int reset_publish_qos(unsigned char* rawdata, uint32_t length, uint8_t new_qos)
{
  if (rawdata == NULL)
  {
    return 1;
  }
  uint8_t QoS = 3; /* to clear qos bits */
  rawdata[0] &= (~((QoS & 0x3) << 1));
  rawdata[0] |= ((new_qos & 0x3) << 1);

  return 0;
}

mqtt_message* parse_raw_packet_connect_message(unsigned char* packet, uint32_t length,
                                               uint8_t packtype, uint8_t packflags,
                                               uint32_t remlength, uint8_t prebytes,
                                               uint32_t *parse_error, int attached_raw)
{
  *parse_error = MQTT_SUCCESS;
  int ret;
  conn_flags connflags = {0};

  mqtt_message* msg = mqtt_message_create_empty();
  msg->packet_type = packtype;
  msg->flags = packflags;
  msg->remaining_length = remlength;
  msg->rl_byteCount = prebytes-1;
  msg->parser = 1;
  msg->attached_raw = attached_raw;
  msg->whl_raw_msg.str = packet;
  msg->whl_raw_msg.length = length;

  /* Set the pointer where the actual data block has started. */
  struct rawposbuf buf;
  buf.curpos = &msg->whl_raw_msg.str[prebytes];
  buf.endpos = &msg->whl_raw_msg.str[msg->whl_raw_msg.length];

  /* Protocol Name */
  ret = read_str_data(&buf, &msg->vh.connect_vh.protocol_name);
  if (ret != 0) {
    *parse_error = MQTT_ERR_PROTOCOL;
    goto ERRORCASE;
  }
  /* Protocol Level */
  ret = read_byte(&buf, &msg->vh.connect_vh.protocol_level);
  if (ret != 0) {
    *parse_error = MQTT_ERR_PROTOCOL;
    goto ERRORCASE;
  }
  /* Protocol Level */
  ret = read_byte(&buf, &msg->vh.connect_vh.conn_flags);
  if (ret != 0) {
    *parse_error = MQTT_ERR_PROTOCOL;
    goto ERRORCASE;
  }
  decode_connect_flags(msg->vh.connect_vh.conn_flags, &connflags);

  /* Keep Alive */
  ret = read_uint16(&buf, &msg->vh.connect_vh.keep_alive);
  if (ret != 0) {
    *parse_error = MQTT_ERR_PROTOCOL;
    goto ERRORCASE;
  }
  /* Client Identifier */
  ret = read_utf8_str(&buf, &msg->pld.connect_pld.client_id);
  if (ret != 0) {
    *parse_error = MQTT_ERR_PROTOCOL;
    goto ERRORCASE;
  }
  if (connflags.will_flag) {
    /* Will Topic */
    ret = read_utf8_str(&buf, &msg->pld.connect_pld.will_topic);
    if (ret != 0) {
      *parse_error = MQTT_ERR_PROTOCOL;
      goto ERRORCASE;
    }
    /* Will Message */
    ret = read_str_data(&buf, &msg->pld.connect_pld.will_msg);
    if (ret != 0) {
      *parse_error = MQTT_ERR_PROTOCOL;
      goto ERRORCASE;
    }
  }
  if (connflags.username_flag) {
    /* Will Topic */
    ret = read_utf8_str(&buf, &msg->pld.connect_pld.user_name);
    if (ret != 0) {
      *parse_error = MQTT_ERR_PROTOCOL;
      goto ERRORCASE;
    }
  }
  if (connflags.password_flag) {
    /* Will Topic */
    ret = read_str_data(&buf, &msg->pld.connect_pld.password);
    if (ret != 0) {
      *parse_error = MQTT_ERR_PROTOCOL;
      goto ERRORCASE;
    }
  }

  return msg;

ERRORCASE:
  if (msg->attached_raw) {
    free(msg->whl_raw_msg.str);
  }
  free(msg);
  return NULL;
}

mqtt_message* parse_raw_packet_connack_message(unsigned char* packet, uint32_t length,
                                               uint8_t packtype, uint8_t packflags,
                                               uint32_t remlength, uint8_t prebytes,
                                               uint32_t *parse_error, int attached_raw)
{
  *parse_error = MQTT_SUCCESS;

  mqtt_message* msg = mqtt_message_create_empty();
  msg->packet_type = packtype;
  msg->flags = packflags;
  msg->remaining_length = remlength;
  msg->rl_byteCount = prebytes-1;
  msg->parser = 1;
  msg->attached_raw = attached_raw;
  msg->whl_raw_msg.str = packet;
  msg->whl_raw_msg.length = length;

  /* Set the pointer where the actual data block has started. */
  struct rawposbuf buf;
  buf.curpos = &msg->whl_raw_msg.str[prebytes];
  buf.endpos = &msg->whl_raw_msg.str[msg->whl_raw_msg.length];

  /* Variable Header part */
  /* The variable header for the CONNACK Packet consists of two fields in the following order:
     - ConnAck Flags, and
     - Return Code.
   */

  /* Connack Flags */
  int result = read_byte(&buf, &msg->vh.connack_vh.connack_flags);
  if (result != 0) {
    *parse_error = MQTT_ERR_PROTOCOL;
    goto ERRORCASE;
  }

  /* Connect Return Code */
  result = read_byte(&buf, &msg->vh.connack_vh.connack_flags);
  if (result != 0) {
    *parse_error = MQTT_ERR_PROTOCOL;
    goto ERRORCASE;
  }

  return msg;

ERRORCASE:
  if (msg->attached_raw) {
    free(msg->whl_raw_msg.str);
  }
  free(msg);
  return NULL;
}

mqtt_message* parse_raw_packet_subscribe_message(unsigned char* packet, uint32_t length,
                                                 uint8_t packtype, uint8_t packflags,
                                                 uint32_t remlength, uint8_t prebytes,
                                                 uint32_t *parse_error, int attached_raw)
{
  *parse_error = MQTT_SUCCESS;
  int ret = 0;
  unsigned char* saved_current_pos = NULL;
  uint16_t temp_length = 0;
  uint32_t topic_count = 0;

  mqtt_message* msg = mqtt_message_create_empty();
  msg->packet_type = packtype;
  msg->flags = packflags;
  msg->remaining_length = remlength;
  msg->rl_byteCount = prebytes-1;
  msg->parser = 1;
  msg->attached_raw = attached_raw;
  msg->whl_raw_msg.str = packet;
  msg->whl_raw_msg.length = length;

  mqtt_subscribe_pld* spld = &msg->pld.subscribe_pld;

  /* Set the pointer where the actual data block has started. */
  struct rawposbuf buf;
  buf.curpos = &msg->whl_raw_msg.str[prebytes];
  buf.endpos = &msg->whl_raw_msg.str[msg->whl_raw_msg.length];

  /* Packet Identifier */
  ret = read_uint16(&buf, &msg->vh.subscribe_vh.packet_id);
  if (ret != 0) {
    *parse_error = MQTT_ERR_PROTOCOL;
    goto ERRORCASE;
  }

  /* The loop to determine the number of topics.
   * TODO: Some other way may be used such as std::vector to collect topics but there
   *       is a question that which is faster
   */
  /* Save the current position to back */
  saved_current_pos = buf.curpos;
  while (buf.curpos < buf.endpos) {
    ret = read_uint16(&buf, &temp_length);
    /* jump to the end of topic-name */
    buf.curpos += temp_length;
    /* skip QoS field */
    buf.curpos++;
    topic_count++;
  }

#ifdef  USE_STATIC_ARRAY
  if (topic_count > MAX_TOPICS) {
    *parse_error = MQTT_ERR_NOMEM;
    goto ERRORCASE;
  }
#else
  /* Allocate topic array */
  spld->topics = (mqtt_topic*)malloc(topic_count * sizeof(mqtt_topic));
#endif
  /* Set back current position */
  buf.curpos = saved_current_pos;
  while (buf.curpos < buf.endpos) {
    /* Topic Name */
    ret = read_utf8_str(&buf, &spld->topics[spld->topic_count].topic_filter);
    if (ret != 0) {
      *parse_error = MQTT_ERR_PROTOCOL;
      goto ERRORCASE;
    }
    /* QoS */
    ret = read_byte(&buf, &spld->topics[spld->topic_count].qos);
    if (ret != 0) {
      *parse_error = MQTT_ERR_PROTOCOL;
      goto ERRORCASE;
    }
    spld->topic_count++;
  }
  return msg;

ERRORCASE:
  if (msg->attached_raw) {
    free(msg->whl_raw_msg.str);
  }
  free(msg);
  return NULL;
}

mqtt_message* parse_raw_packet_suback_message(unsigned char* packet, uint32_t length,
                                              uint8_t packtype, uint8_t packflags,
                                              uint32_t remlength, uint8_t prebytes,
                                              uint32_t *parse_error, int attached_raw)
{
  *parse_error = MQTT_SUCCESS;
  uint8_t* ptr = NULL;

  mqtt_message* msg = mqtt_message_create_empty();
  msg->packet_type = packtype;
  msg->flags = packflags;
  msg->remaining_length = remlength;
  msg->rl_byteCount = prebytes-1;
  msg->parser = 1;
  msg->attached_raw = attached_raw;
  msg->whl_raw_msg.str = packet;
  msg->whl_raw_msg.length = length;

  /* Set the pointer where the actual data block has started. */
  struct rawposbuf buf;
  buf.curpos = &msg->whl_raw_msg.str[prebytes];
  buf.endpos = &msg->whl_raw_msg.str[msg->whl_raw_msg.length];

  /* Suback Packet-Id */
  int result = read_uint16(&buf, &msg->vh.suback_vh.packet_id);
  if (result != 0) {
    *parse_error = MQTT_ERR_PROTOCOL;
    goto ERRORCASE;
  }

  /* Suback Return Codes */
  msg->pld.suback_pld.retcode_count = buf.endpos - buf.curpos;
#ifdef  USE_STATIC_ARRAY
  if (msg->pld.suback_pld.retcode_count > MAX_TOPICS) {
    *parse_error = MQTT_ERR_NOMEM;
    goto ERRORCASE;
  }
#else
  msg->pld.suback_pld.return_codes = (uint8_t*)malloc(msg->pld.suback_pld.retcode_count * sizeof(uint8_t));
#endif
  ptr = msg->pld.suback_pld.return_codes;
  for (uint32_t i=0; i<msg->pld.suback_pld.retcode_count; i++) {
    result = read_byte(&buf, ptr);
    if (result != 0) {
      *parse_error = MQTT_ERR_PROTOCOL;
      goto ERRORCASE;
    }
    else {

    }
    ptr++;
  }
  return msg;

ERRORCASE:
  if (msg->attached_raw) {
    free(msg->whl_raw_msg.str);
  }
  free(msg);
  return NULL;
}

mqtt_message* parse_raw_packet_publish_message(unsigned char* packet, uint32_t length,
                                               uint8_t packtype, uint8_t packflags,
                                               uint32_t remlength, uint8_t prebytes,
                                               uint32_t *parse_error, int attached_raw)
{
  *parse_error = MQTT_SUCCESS;
  int ret = 0;
  int packid_length = 0;
  mqtt_publish_fixed_hdr_flags flags_set = {0, 0, 0};

  mqtt_message* msg = mqtt_message_create_empty();
  msg->packet_type = packtype;
  msg->flags = packflags;
  msg->remaining_length = remlength;
  msg->rl_byteCount = prebytes-1;
  msg->parser = 1;
  msg->attached_raw = attached_raw;
  msg->whl_raw_msg.str = packet;
  msg->whl_raw_msg.length = length;

  /* Set the pointer where the actual data block has started. */
  struct rawposbuf buf;
  buf.curpos = &msg->whl_raw_msg.str[prebytes];
  buf.endpos = &msg->whl_raw_msg.str[msg->whl_raw_msg.length];

  /* Topic Name */
  ret = read_utf8_str(&buf, &msg->vh.publish_vh.topic_name);
  if (ret != 0) {
    *parse_error = MQTT_ERR_PROTOCOL;
    goto ERRORCASE;
  }

  decode_publish_flags(msg->flags, &flags_set);

  if (flags_set.qos > MQTT_QOS_0_AT_MOST_ONCE) {
    /* Packet Identifier */
    ret = read_uint16(&buf, &msg->vh.publish_vh.packet_id);
    if (ret != 0) {
      *parse_error = MQTT_ERR_PROTOCOL;
      goto ERRORCASE;
    }
    packid_length = 2;
  }

  /* Payload */
  /* No length information for payload. The length of the payload can be
     calculated by subtracting the length of the variable header from the
     Remaining Length field that is in the Fixed Header. It is valid for a
     PUBLISH Packet to contain a zero length payload.*/
  msg->pld.publish_pld.payload.length = msg->remaining_length - (2 /* Length bytes of Topic Name */ + msg->vh.publish_vh.topic_name.length + packid_length);
  msg->pld.publish_pld.payload.str = (msg->pld.publish_pld.payload.length > 0) ? buf.curpos : NULL;

  return msg;

ERRORCASE:
  if (msg->attached_raw) {
    free(msg->whl_raw_msg.str);
  }
  free(msg);
  return NULL;
}

mqtt_message* parse_raw_packet_puback_message(unsigned char* packet, uint32_t length,
                                              uint8_t packtype, uint8_t packflags,
                                              uint32_t remlength, uint8_t prebytes,
                                              uint32_t *parse_error, int attached_raw)
{
  *parse_error = MQTT_SUCCESS;

  mqtt_message* msg = mqtt_message_create_empty();
  msg->packet_type = packtype;
  msg->flags = packflags;
  msg->remaining_length = remlength;
  msg->rl_byteCount = prebytes-1;
  msg->parser = 1;
  msg->attached_raw = attached_raw;
  msg->whl_raw_msg.str = packet;
  msg->whl_raw_msg.length = length;

  /* Set the pointer where the actual data block has started. */
  struct rawposbuf buf;
  buf.curpos = &msg->whl_raw_msg.str[prebytes];
  buf.endpos = &msg->whl_raw_msg.str[msg->whl_raw_msg.length];

  int result = read_uint16(&buf, &msg->vh.puback_vh.packet_id);
  if (result != 0) {
    *parse_error = MQTT_ERR_PROTOCOL;
    goto ERRORCASE;
  }
  return msg;

ERRORCASE:
  if (msg->attached_raw) {
    free(msg->whl_raw_msg.str);
  }
  free(msg);
  return NULL;
}

mqtt_message* parse_raw_packet_pubrec_message(unsigned char* packet, uint32_t length,
                                              uint8_t packtype, uint8_t packflags,
                                              uint32_t remlength, uint8_t prebytes,
                                              uint32_t *parse_error, int attached_raw)
{
  *parse_error = MQTT_SUCCESS;

  mqtt_message* msg = mqtt_message_create_empty();
  msg->packet_type = packtype;
  msg->flags = packflags;
  msg->remaining_length = remlength;
  msg->rl_byteCount = prebytes-1;
  msg->parser = 1;
  msg->attached_raw = attached_raw;
  msg->whl_raw_msg.str = packet;
  msg->whl_raw_msg.length = length;

  /* Set the pointer where the actual data block has started. */
  struct rawposbuf buf;
  buf.curpos = &msg->whl_raw_msg.str[prebytes];
  buf.endpos = &msg->whl_raw_msg.str[msg->whl_raw_msg.length];

  int result = read_uint16(&buf, &msg->vh.pubrec_vh.packet_id);
  if (result != 0) {
    *parse_error = MQTT_ERR_PROTOCOL;
    goto ERRORCASE;
  }
  return msg;

ERRORCASE:
  if (msg->attached_raw) {
    free(msg->whl_raw_msg.str);
  }
  free(msg);
  return NULL;
}

mqtt_message* parse_raw_packet_pubrel_message(unsigned char* packet, uint32_t length,
                                              uint8_t packtype, uint8_t packflags,
                                              uint32_t remlength, uint8_t prebytes,
                                              uint32_t *parse_error, int attached_raw)
{
  *parse_error = MQTT_SUCCESS;

  mqtt_message* msg = mqtt_message_create_empty();
  msg->packet_type = packtype;
  msg->flags = packflags;
  msg->remaining_length = remlength;
  msg->rl_byteCount = prebytes-1;
  msg->parser = 1;
  msg->attached_raw = attached_raw;
  msg->whl_raw_msg.str = packet;
  msg->whl_raw_msg.length = length;

  /* Set the pointer where the actual data block has started. */
  struct rawposbuf buf;
  buf.curpos = &msg->whl_raw_msg.str[prebytes];
  buf.endpos = &msg->whl_raw_msg.str[msg->whl_raw_msg.length];

  int result = read_uint16(&buf, &msg->vh.pubrel_vh.packet_id);
  if (result != 0) {
    *parse_error = MQTT_ERR_PROTOCOL;
    goto ERRORCASE;
  }
  return msg;

ERRORCASE:
  if (msg->attached_raw) {
    free(msg->whl_raw_msg.str);
  }
  free(msg);
  return NULL;
}

mqtt_message* parse_raw_packet_pubcomp_message(unsigned char* packet, uint32_t length,
                                               uint8_t packtype, uint8_t packflags,
                                               uint32_t remlength, uint8_t prebytes,
                                               uint32_t *parse_error, int attached_raw)
{
  *parse_error = MQTT_SUCCESS;

  mqtt_message* msg = mqtt_message_create_empty();
  msg->packet_type = packtype;
  msg->flags = packflags;
  msg->remaining_length = remlength;
  msg->rl_byteCount = prebytes-1;
  msg->parser = 1;
  msg->attached_raw = attached_raw;
  msg->whl_raw_msg.str = packet;
  msg->whl_raw_msg.length = length;

  /* Set the pointer where the actual data block has started. */
  struct rawposbuf buf;
  buf.curpos = &msg->whl_raw_msg.str[prebytes];
  buf.endpos = &msg->whl_raw_msg.str[msg->whl_raw_msg.length];

  int result = read_uint16(&buf, &msg->vh.pubcomp_vh.packet_id);
  if (result != 0) {
    *parse_error = MQTT_ERR_PROTOCOL;
    goto ERRORCASE;
  }
  return msg;

ERRORCASE:
  if (msg->attached_raw) {
    free(msg->whl_raw_msg.str);
  }
  free(msg);
  return NULL;
}

mqtt_message* parse_raw_packet_unsubscribe_message(unsigned char* packet, uint32_t length,
                                                   uint8_t packtype, uint8_t packflags,
                                                   uint32_t remlength, uint8_t prebytes,
                                                   uint32_t *parse_error, int attached_raw)
{
  *parse_error = MQTT_SUCCESS;
  int ret = 0;
  unsigned char* saved_current_pos = NULL;
  uint16_t temp_length = 0;
  uint32_t topic_count = 0;

  mqtt_message* msg = mqtt_message_create_empty();
  msg->packet_type = packtype;
  msg->flags = packflags;
  msg->remaining_length = remlength;
  msg->rl_byteCount = prebytes-1;
  msg->parser = 1;
  msg->attached_raw = attached_raw;
  msg->whl_raw_msg.str = packet;
  msg->whl_raw_msg.length = length;

  mqtt_unsubscribe_pld* uspld = &msg->pld.unsub_pld;

  /* Set the pointer where the actual data block has started. */
  struct rawposbuf buf;
  buf.curpos = &msg->whl_raw_msg.str[prebytes];
  buf.endpos = &msg->whl_raw_msg.str[msg->whl_raw_msg.length];

  /* Packet Identifier */
  ret = read_uint16(&buf, &msg->vh.unsubscribe_vh.packet_id);
  if (ret != 0) {
    *parse_error = MQTT_ERR_PROTOCOL;
    goto ERRORCASE;
  }

  /* The loop to determine the number of topics.
   * TODO: Some other way may be used such as std::vector to collect topics but there
   *       is a question that which is faster
   */
  /* Save the current position to back */
  saved_current_pos = buf.curpos;
  while (buf.curpos < buf.endpos) {
    ret = read_uint16(&buf, &temp_length);
    /* jump to the end of topic-name */
    buf.curpos += temp_length;
    /* skip QoS field */
    topic_count++;
  }

#ifdef USE_STATIC_ARRAY
  if (topic_count > MAX_TOPICS) {
    *parse_error = MQTT_ERR_NOMEM;
    goto ERRORCASE;
  }
#else
  /* Allocate topic array */
  uspld->topics = (cstr_t*)malloc(topic_count * sizeof(cstr_t));
#endif
  /* Set back current position */
  buf.curpos = saved_current_pos;
  while (buf.curpos < buf.endpos) {
    /* Topic Name */
    ret = read_utf8_str(&buf, &uspld->topics[uspld->topic_count]);
    if (ret != 0) {
      *parse_error = MQTT_ERR_PROTOCOL;
      goto ERRORCASE;
    }
    uspld->topic_count++;
  }
  return msg;

ERRORCASE:
  if (msg->attached_raw) {
    free(msg->whl_raw_msg.str);
  }
  free(msg);
  return NULL;
}

mqtt_message* parse_raw_packet_unsuback_message(unsigned char* packet, uint32_t length,
                                                uint8_t packtype, uint8_t packflags,
                                                uint32_t remlength, uint8_t prebytes,
                                                uint32_t *parse_error, int attached_raw)
{
  *parse_error = MQTT_SUCCESS;

  mqtt_message* msg = mqtt_message_create_empty();
  msg->packet_type = packtype;
  msg->flags = packflags;
  msg->remaining_length = remlength;
  msg->rl_byteCount = prebytes-1;
  msg->parser = 1;
  msg->attached_raw = attached_raw;
  msg->whl_raw_msg.str = packet;
  msg->whl_raw_msg.length = length;

  /* Set the pointer where the actual data block has started. */
  struct rawposbuf buf;
  buf.curpos = &msg->whl_raw_msg.str[prebytes];
  buf.endpos = &msg->whl_raw_msg.str[msg->whl_raw_msg.length];

  /* Unsuback Packet-Id */
  int result = read_uint16(&buf, &msg->vh.unsuback_vh.packet_id);
  if (result != 0) {
    *parse_error = MQTT_ERR_PROTOCOL;
    goto ERRORCASE;
  }

  return msg;

ERRORCASE:
  if (msg->attached_raw) {
    free(msg->whl_raw_msg.str);
  }
  free(msg);
  return NULL;
}

mqtt_message* mqtt_message_parse_raw_packet_det(unsigned char* packet, uint32_t length,
                                                uint8_t packtype, uint8_t packflags,
                                                uint32_t remlength, uint8_t prebytes,
                                                uint32_t *parse_error, int attached_raw)
{
  mqtt_message* msg = NULL;
  *parse_error = 0;
  switch (packtype) {
    case MQTT_CONNECT:
      msg = parse_raw_packet_connect_message(packet, length, packtype, packflags,
                                             remlength, prebytes, parse_error, attached_raw);
      break;

    case MQTT_CONNACK:
      msg = parse_raw_packet_connack_message(packet, length, packtype, packflags,
                                             remlength, prebytes, parse_error, attached_raw);
      break;

    case MQTT_PUBLISH:
      msg = parse_raw_packet_publish_message(packet, length, packtype, packflags,
                                             remlength, prebytes, parse_error, attached_raw);
      break;

    case MQTT_PUBACK:
      msg = parse_raw_packet_puback_message(packet, length, packtype, packflags,
                                            remlength, prebytes, parse_error, attached_raw);
      break;

    case MQTT_PUBREC:
      msg = parse_raw_packet_pubrec_message(packet, length, packtype, packflags,
                                            remlength, prebytes, parse_error, attached_raw);
      break;

    case MQTT_PUBREL:
      msg = parse_raw_packet_pubrel_message(packet, length, packtype, packflags,
                                            remlength, prebytes, parse_error, attached_raw);
      break;

    case MQTT_PUBCOMP:
      msg = parse_raw_packet_pubcomp_message(packet, length, packtype, packflags,
                                             remlength, prebytes, parse_error, attached_raw);
      break;

    case MQTT_SUBSCRIBE:
      msg = parse_raw_packet_subscribe_message(packet, length, packtype, packflags,
                                               remlength, prebytes, parse_error, attached_raw);
      break;

    case MQTT_SUBACK:
      msg = parse_raw_packet_suback_message(packet, length, packtype, packflags,
                                            remlength, prebytes, parse_error, attached_raw);
      break;

    case MQTT_UNSUBSCRIBE:
      msg = parse_raw_packet_unsubscribe_message(packet, length, packtype, packflags,
                                                 remlength, prebytes, parse_error, attached_raw);
      break;

    case MQTT_UNSUBACK:
      msg = parse_raw_packet_unsuback_message(packet, length, packtype, packflags,
                                              remlength, prebytes, parse_error, attached_raw);
      break;

    case MQTT_PINGREQ:
    case MQTT_PINGRESP:
    case MQTT_DISCONNECT:
    {
      msg = mqtt_message_create_empty();
      msg->packet_type = packtype;
      msg->flags = packflags;
      msg->remaining_length = remlength;
      msg->rl_byteCount = prebytes-1;
      msg->parser = 1;
      msg->attached_raw = attached_raw;
      msg->whl_raw_msg.str = packet;
      msg->whl_raw_msg.length = length;
    }
      break;

    default:
      *parse_error = MQTT_ERR_NOT_SUPPORTED;
      break;
  }
  return msg;
}

mqtt_message* mqtt_message_parse_raw_packet(unsigned char* packet, uint32_t length,
                                            uint32_t *parse_error, int attached_raw)
{
  uint8_t firstbyte = packet[0];
  uint8_t packtype = (firstbyte & 0xf0);
  uint8_t packflags = (firstbyte & 0x0f);
  uint32_t remlength = 0;
  uint8_t count;
  int result = mqtt_message_read_variable_int((packet + 1), length, &remlength, &count);
  if (result != MQTT_SUCCESS) {
    *parse_error = MQTT_ERR_INVAL;
    return NULL;
  }
  uint8_t prebytes = count+1;
  /* Check for length consistency */
  if (remlength != (length - prebytes)) {
    *parse_error = MQTT_ERR_MALFORMED;
    return NULL;
  }
  return mqtt_message_parse_raw_packet_det(packet,length, packtype, packflags,
                                           remlength, prebytes,
                                           parse_error, attached_raw);
}

const char* get_packet_type_str(uint8_t packtype)
{
  static const char* packTypeNames[16] = { "Forbidden-0",
                                           "CONNECT",
                                           "CONNACK",
                                           "PUBLISH",
                                           "PUBACK",
                                           "PUBREC",
                                           "PUBREL",
                                           "PUBCOMP",
                                           "SUBSCRIBE",
                                           "SUBACK",
                                           "UNSUBSCRIBE",
                                           "UNSUBACK",
                                           "PINGREQ",
                                           "PINGRESP",
                                           "DISCONNECT",
                                           "Forbidden-15"
                                         };
  if (packtype > 15) {
    packtype = 0;
  }
  return packTypeNames[packtype];
}

int mqtt_message_dump(mqtt_message* msg, cstr_t* sb, int print_raw)
{
  int pos = 0;
  int ret = 0;

  ret = sprintf((char*)&sb->str[pos], "\n----- MQTT Message Dump -----\nPacket Type: %d (%s)\nPacket Flags: %d\nRemaining Length: %d",
                (int)msg->packet_type, (char*)get_packet_type_str(msg->packet_type/16), (int)msg->flags, (int)msg->remaining_length);
  if ((ret < 0) || ((pos+ret) > sb->length)) {
    return 1;
  }
  pos += ret;

  /* Print variable header part */
  switch (msg->packet_type) {
    case MQTT_CONNECT:
    {
      ret = sprintf((char*)&sb->str[pos], "\nProtocol Name : %.*s\nProtocol Level: %d\nKeep Alive    : %d",
                    msg->vh.connect_vh.protocol_name.length, msg->vh.connect_vh.protocol_name.str,
                    (int)msg->vh.connect_vh.protocol_level, (int)msg->vh.connect_vh.keep_alive);
      if ((ret < 0) || ((pos+ret) > sb->length)) {
        return 1;
      }
      pos += ret;
      conn_flags flags_set;
      decode_connect_flags(msg->vh.connect_vh.conn_flags, &flags_set);
      ret =  sprintf((char*)&sb->str[pos], "\nConnect Flags:\n   Clean Session Flag : %s, \n   Will Flag          : %s"
                     "\n   Will Retain Flag   : %s\n   Will QoS Flag      : %d"
                     "\n   User Name Flag     : %s\n   Password Flag      : %s",
                     ((flags_set.clean_session) ? "true" : "false"), ((flags_set.will_flag) ? "true" : "false"),
                     ((flags_set.will_retain) ? "true" : "false"), (int)flags_set.will_qos,
                     ((flags_set.username_flag) ? "true" : "false"), ((flags_set.password_flag) ? "true" : "false"));
      if ((ret < 0) || ((pos+ret) > sb->length)) {
        return 1;
      }
      pos += ret;
      ret = sprintf((char*)&sb->str[pos], "\nClient Identifier: %.*s", msg->pld.connect_pld.client_id.length, msg->pld.connect_pld.client_id.str);
      if ((ret < 0) || ((pos+ret) > sb->length)) {
        return 1;
      }
      pos += ret;
      ret = sprintf((char*)&sb->str[pos], "\nWill Topic       : %.*s", msg->pld.connect_pld.will_topic.length, msg->pld.connect_pld.will_topic.str);
      if ((ret < 0) || ((pos+ret) > sb->length)) {
        return 1;
      }
      pos += ret;
      ret = sprintf((char*)&sb->str[pos], "\nWill Message     : %.*s", msg->pld.connect_pld.will_msg.length, msg->pld.connect_pld.will_msg.str);
      if ((ret < 0) || ((pos+ret) > sb->length)) {
        return 1;
      }
      pos += ret;
      ret = sprintf((char*)&sb->str[pos], "\nUser Name        : %.*s", msg->pld.connect_pld.user_name.length, msg->pld.connect_pld.user_name.str);
      if ((ret < 0) || ((pos+ret) > sb->length)) {
        return 1;
      }
      pos += ret;
      ret = sprintf((char*)&sb->str[pos], "\nPassword         : %.*s", msg->pld.connect_pld.password.length, msg->pld.connect_pld.password.str);
      if ((ret < 0) || ((pos+ret) > sb->length)) {
        return 1;
      }
      pos += ret;
    }
      break;

    case MQTT_CONNACK:
      ret = sprintf((char*)&sb->str[pos], "\nConnack Flags      : %d"
                                  "\nConnack Return-Code: %d",
                    (int)msg->vh.connack_vh.connack_flags, (int)msg->vh.connack_vh.conn_return_code);
      if ((ret < 0) || ((pos+ret) > sb->length)) {
        return 1;
      }
      pos += ret;
      break;

    case MQTT_PUBLISH:
    {
      mqtt_publish_fixed_hdr_flags flags_set;
      decode_publish_flags(msg->flags, &flags_set);
      ret = sprintf((char*)&sb->str[pos], "\nPublis Flags:\n   Retain: %s\n   QoS   : %d\n   DUP   : %s",
                    ((flags_set.retain) ? "true" : "false"), (int)flags_set.qos, ((flags_set.dup) ? "true" : "false"));
      if ((ret < 0) || ((pos+ret) > sb->length)) {
        return 1;
      }
      pos += ret;
      ret = sprintf((char*)&sb->str[pos], "\nTopic     : %.*s\nPacket Id : %d\nPayload   : %.*s",
                    msg->vh.publish_vh.topic_name.length, msg->vh.publish_vh.topic_name.str,
                    (int)msg->vh.publish_vh.packet_id,
                    msg->pld.publish_pld.payload.length, msg->pld.publish_pld.payload.str);
      if ((ret < 0) || ((pos+ret) > sb->length)) {
        return 1;
      }
      pos += ret;
    }
      break;

    case MQTT_PUBACK:
      ret = sprintf((char*)&sb->str[pos], "\nPacket-Id: %d", msg->vh.puback_vh.packet_id);
      if ((ret < 0) || ((pos+ret) > sb->length)) {
        return 1;
      }
      pos += ret;
      break;

    case MQTT_PUBREC:
      ret = sprintf((char*)&sb->str[pos], "\nPacket-Id: %d", msg->vh.pubrec_vh.packet_id);
      if ((ret < 0) || ((pos+ret) > sb->length)) {
        return 1;
      }
      pos += ret;
      break;

    case MQTT_PUBREL:
      ret = sprintf((char*)&sb->str[pos], "\nPacket-Id: %d", msg->vh.pubrel_vh.packet_id);
      if ((ret < 0) || ((pos+ret) > sb->length)) {
        return 1;
      }
      pos += ret;
      break;

    case MQTT_PUBCOMP:
      ret = sprintf((char*)&sb->str[pos], "\nPacket-Id: %d", msg->vh.pubcomp_vh.packet_id);
      if ((ret < 0) || ((pos+ret) > sb->length)) {
        return 1;
      }
      pos += ret;
      break;

    case MQTT_SUBSCRIBE:
    {
      ret = sprintf((char*)&sb->str[pos], "\nPacket-Id: %d", msg->vh.subscribe_vh.packet_id);
      if ((ret < 0) || ((pos+ret) > sb->length)) {
        return 1;
      }
      pos += ret;
      for (uint32_t i=0; i<msg->pld.subscribe_pld.topic_count; i++) {
        ret = sprintf((char*)&sb->str[pos], "\nTopic Filter[%u]: %.*s\nRequested QoS[%u]: %d",
                      i,
                      msg->pld.subscribe_pld.topics[i].topic_filter.length,
                      msg->pld.subscribe_pld.topics[i].topic_filter.str,
                      i,
                      (int)msg->pld.subscribe_pld.topics[i].qos);
        if ((ret < 0) || ((pos+ret) > sb->length)) {
          return 1;
        }
        pos += ret;
      }
    }
      break;

    case MQTT_SUBACK:
    {
      ret = sprintf((char*)&sb->str[pos], "\nPacket-Id: %d", msg->vh.suback_vh.packet_id);
      if ((ret < 0) || ((pos+ret) > sb->length)) {
        return 1;
      }
      pos += ret;
      for (uint32_t i=0; i<msg->pld.suback_pld.retcode_count; i++) {
        ret = sprintf((char*)&sb->str[pos], "\nReturn Code[%u]: %d", i, (int)msg->pld.suback_pld.return_codes[i]);
        if ((ret < 0) || ((pos+ret) > sb->length)) {
          return 1;
        }
        pos += ret;
      }
    }
      break;

    case MQTT_UNSUBSCRIBE:
    {
      ret = sprintf((char*)&sb->str[pos], "\nPacket-Id: %d", msg->vh.unsubscribe_vh.packet_id);
      if ((ret < 0) || ((pos+ret) > sb->length)) {
        return 1;
      }
      pos += ret;
      for (uint32_t i=0; i<msg->pld.unsub_pld.topic_count; i++) {
        ret = sprintf((char*)&sb->str[pos], "\nTopic Filter[%u]: %.*s",
                      i, msg->pld.unsub_pld.topics[i].length, (char*)msg->pld.unsub_pld.topics[i].str);
        if ((ret < 0) || ((pos+ret) > sb->length)) {
          return 1;
        }
        pos += ret;
      }
    }
      break;

    case MQTT_UNSUBACK:
      ret = sprintf((char*)&sb->str[pos], "\nPacket-Id: %d", msg->vh.unsuback_vh.packet_id);
      if ((ret < 0) || ((pos+ret) > sb->length)) {
        return 1;
      }
      pos += ret;
      break;

    case MQTT_PINGREQ:
    case MQTT_PINGRESP:
      break;

    case MQTT_DISCONNECT:
      break;

  }

  if (print_raw) {
    ret = sprintf((char*)&sb->str[pos], "\n*** Raw Message ***");
    if ((ret < 0) || ((pos+ret) > sb->length)) {
      return 1;
    }
    pos += ret;
    for (unsigned int i = 0; i<msg->whl_raw_msg.length; i++)
    {
      if ((i % 16) == 0) {
        sb->str[pos++] = '\n';
      }
      else {
        if ((i % 4) == 0) {
          sb->str[pos++] = ' ';
        }
      }
      ret = sprintf((char*)&sb->str[pos],"%02x", ((unsigned char)(msg->whl_raw_msg.str[i] & 0xff)));
      if ((ret < 0) || ((pos+ret) > sb->length)) {
        return 1;
      }
      pos += ret;

    }
    sb->str[pos++] = '\n';
    if (pos > msg->whl_raw_msg.length) {
      return 1;
    }
    sprintf((char*)&sb->str[pos], "------------------------\n");
  }
  return 0;
}
