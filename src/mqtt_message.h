
#ifndef MQTT_MESSAGE_H_
#define MQTT_MESSAGE_H_
/*---------------------------------------------------------------------------*/

#ifdef __cplusplus
  extern "C" {
#endif

#ifdef WIN32
  #include <stdint.h>
#else
  #include <inttypes.h>
#endif

#define  USE_STATIC_ARRAY

#ifdef USE_STATIC_ARRAY
  #define MAX_TOPICS 10
#endif

/* Function return codes */
#define MQTT_SUCCESS             0
#define MQTT_ERR_NOMEM           1
#define MQTT_ERR_PROTOCOL        2
#define MQTT_ERR_INVAL           3
#define MQTT_ERR_PAYLOAD_SIZE    4
#define MQTT_ERR_NOT_SUPPORTED   5
#define MQTT_ERR_NOT_FOUND       6
#define MQTT_ERR_MALFORMED       7

/* Compact string type */
typedef struct
{
  uint32_t      length;
  unsigned char *str;
} cstr_t;

/*****************************************************************************
 * Variable header parts
 ****************************************************************************/
typedef struct mqtt_connect_vhdr_t
{
	cstr_t   protocol_name;
  uint8_t  protocol_level;
  uint8_t  conn_flags;
  uint16_t keep_alive;
} mqtt_connect_vhdr;

typedef struct mqtt_connack_vhdr_t
{
  uint8_t  connack_flags;
  uint8_t  conn_return_code;
} mqtt_connack_vhdr;

typedef struct mqtt_publish_vhdr_t
{
  cstr_t   topic_name;
  uint16_t packet_id;
} mqtt_publish_vhdr;

typedef struct mqtt_puback_vhdr_t
{
  uint16_t packet_id;
} mqtt_puback_vhdr;

typedef struct mqtt_pubrec_vhdr_t
{
  uint16_t packet_id;
} mqtt_pubrec_vhdr;

typedef struct mqtt_pubrel_vhdr_t
{
  uint16_t packet_id;
} mqtt_pubrel_vhdr;

typedef struct mqtt_pubcomp_vhdr_t
{
  uint16_t packet_id;
} mqtt_pubcomp_vhdr;

typedef struct mqtt_subscribe_vhdr_t
{
  uint16_t packet_id;
} mqtt_subscribe_vhdr;

typedef struct mqtt_suback_vhdr_t
{
  uint16_t packet_id;
} mqtt_suback_vhdr;

typedef struct mqtt_unsubscribe_vhdr_t
{
  uint16_t packet_id;
} mqtt_unsubscribe_vhdr;

typedef struct mqtt_unsuback_vhdr_t
{
  uint16_t packet_id;
} mqtt_unsuback_vhdr;

/*****************************************************************************
 * Union to cover all Variable Header types
 ****************************************************************************/
union mqtt_variable_header
{
  mqtt_connect_vhdr     connect_vh;
  mqtt_connack_vhdr     connack_vh;
  mqtt_publish_vhdr     publish_vh;
  mqtt_puback_vhdr      puback_vh;
  mqtt_pubrec_vhdr      pubrec_vh;
  mqtt_pubrel_vhdr      pubrel_vh;
  mqtt_pubcomp_vhdr     pubcomp_vh;
  mqtt_subscribe_vhdr   subscribe_vh;
  mqtt_suback_vhdr      suback_vh;
  mqtt_unsubscribe_vhdr unsubscribe_vh;
  mqtt_unsuback_vhdr    unsuback_vh;
};

typedef struct mqtt_topic_t
{
  cstr_t   topic_filter;
  uint8_t  qos;
} mqtt_topic;

/*****************************************************************************
 * Payloads
 ****************************************************************************/
typedef struct mqtt_connect_pld_t
{
  cstr_t client_id;
  cstr_t will_topic;
  cstr_t will_msg;
  cstr_t user_name;
  cstr_t password;
} mqtt_connect_pld;

typedef struct mqtt_publish_pld_t
{
  cstr_t payload;
} mqtt_publish_pld;

typedef struct mqtt_subscribe_pld_t
{
#ifdef USE_STATIC_ARRAY
  mqtt_topic  topics[MAX_TOPICS];
#else
  mqtt_topic* topics;      /* array of mqtt_topic instances continuous in memory */
#endif
  uint32_t    topic_count; /* not included in the message itself */
} mqtt_subscribe_pld;

typedef struct mqtt_suback_pld_t
{
#ifdef USE_STATIC_ARRAY
  uint8_t  return_codes[MAX_TOPICS];
#else
  uint8_t* return_codes;  /* array of return codes continuous in memory */
#endif
  uint32_t retcode_count; /* not included in the message itself */
} mqtt_suback_pld;

typedef struct mqtt_unsubscribe_pld_t
{
#ifdef USE_STATIC_ARRAY
  cstr_t    topics[MAX_TOPICS];
#else
  cstr_t*   topics;      /* array of topics continuous in memory */
#endif
  uint32_t  topic_count; /* not included in the message itself */
} mqtt_unsubscribe_pld;

/*****************************************************************************
 * Union to cover all Payload types
 ****************************************************************************/
union mqtt_payload
{
  mqtt_connect_pld     connect_pld;
  mqtt_publish_pld     publish_pld;
  mqtt_subscribe_pld   subscribe_pld;
  mqtt_suback_pld      suback_pld;
  mqtt_unsubscribe_pld unsub_pld;
};

typedef struct mqtt_fixed_hdr_flags_in_bits_t
{
  uint8_t bit_0:1;
  uint8_t bit_1:1;
  uint8_t bit_2:1;
  uint8_t bit_3:1;
} mqtt_fixed_hdr_flags_in_bits;

typedef struct mqtt_publish_fixed_hdr_flags_t
{
  uint8_t retain:1;
  uint8_t qos:2;
  uint8_t dup:1;
} mqtt_publish_fixed_hdr_flags;

typedef struct mqtt_fixed_hdr_flags_t
{
  union
  {
    mqtt_fixed_hdr_flags_in_bits in_bits;
    mqtt_publish_fixed_hdr_flags pub_flags;
  } all;
} mqtt_fixed_hdr_flags;

/* CONNECT flags */
typedef struct conn_flags_t
{
  uint8_t clean_session:1;
  uint8_t will_flag:1;
  uint8_t will_qos:2;
  uint8_t will_retain:1;
  uint8_t password_flag:1;
  uint8_t username_flag:1;
} conn_flags;

typedef struct mqtt_message_t
{
  /* Fixed header part */
  uint8_t  packet_type;
  uint8_t  flags;
  uint32_t remaining_length; /* up to 268,435,455 (256 MB) */
  uint8_t  rl_byteCount;     /* byte count for used remainingLength representation
                                This information (combined with packetType and packetFlags)
                                may be used to jump the point where the actual data starts */
  union mqtt_variable_header vh;
  union mqtt_payload         pld;

  int    parser;           /* message is obtained from parsing or built */
  cstr_t whl_raw_msg;      /* raw representation of whole packet */
  int    attached_raw;     /* indicates if whl_raw_msg is to be owned */
} mqtt_message;

mqtt_message* mqtt_message_create_empty(void);

int mqtt_message_destroy(mqtt_message* self);

/* It builds a MQTT packet by using the information in 'msg'. Built raw message can be
 * obtained from 'whl_raw_msg' to be sent through the network. Providing 'fixed_flags'
 * is mandatory for building of PUBLISH message, otherwise defined values in MQTT 3.1.1
 * documentation, for each packet type, will be considered.
 */
int mqtt_message_build(mqtt_message* msg, mqtt_fixed_hdr_flags* fixed_flags);

mqtt_message* mqtt_message_parse_raw_packet_det(unsigned char* packet, uint32_t length,
                                                uint8_t packtype, uint8_t packflags,
                                                uint32_t remlength, uint8_t prebytes,
                                                uint32_t *parse_error, int attached_raw);

mqtt_message* mqtt_message_parse_raw_packet(unsigned char* packet, uint32_t length,
                                            uint32_t *parse_error, int attached_raw);

/* returns encoded connect flag byte according to provided flags_set */
uint8_t encode_connect_flags(conn_flags *flags_set);

/* build connect flag information from 'flags' into 'flags_set' */
int decode_connect_flags(uint8_t flags, conn_flags *flags_set);
int is_clean_session(uint8_t connflags);
int is_will_retain(uint8_t connflags);
uint8_t will_qos(uint8_t connflags);

/* returns '1' if packtype represents a MQTT connection control message,
 * otherwise returns '0' */
int is_connection_control_message(uint8_t packtype);
/* returns '1' if packtype represents a MQTT application request message,
 * otherwise return '0' */
int is_request_type_app_message(uint8_t packtype);

/* provides query if message is an application message and a request,
 * Returns '1'  for application layer messages and sets 'isRequest' to 1
 * if it is a request message. */
int is_application_message(uint8_t packtype, int* isrequest);

/* provides query if the packet types is for message that packet-identifier included.
 * 'packFlags' is to check for possible QoS value if packet is a PUBLISH packet.
 * Returns '1' if packet-identifier included, otherwise '0'.
 */
int is_packet_identifier_included(uint8_t packet_type, uint8_t packflags);

/* utility to extract packet identifier from raw-packet if available */
int get_packet_identifier(unsigned char* rawdata, uint32_t length, uint8_t prebytes, uint16_t* packid);

/* returns string representations of packet-type.Note that 'packtype' shall
 * be in range of 0-15 */
const char* get_packet_type_str(uint8_t packtype);

/* Valid for PUBLISH message */
int decode_publish_flags(uint8_t flags, mqtt_publish_fixed_hdr_flags* flags_set);
/* utility function to set DUP plag on PUBLISH raw-packet */
int set_dup_flag(unsigned char* rawdata, uint32_t length);
uint8_t get_publish_qos(uint8_t packflags);
/* utility to reset PUBLISH QoS value on raw-packet */
int reset_publish_qos(unsigned char* rawdata, uint32_t length, uint8_t new_qos);

/* A public utility function providing integer values encoded as variable-int form, such as
 * remaining-length value in the header of MQTT message. '*value' returns the value of
 * variable-int, while '*pos' returns byte number used to encode that integer.
 * This function could be useful in the case of receiving messages from network.
 */
int mqtt_message_read_variable_int(unsigned char* ptr, uint32_t length, uint32_t *value, uint8_t *pos);

/* Dumps string representation of message on the provided 'sb'.
 * Buffer shall be big enough to be able to cover all dumped info. */
int mqtt_message_dump(mqtt_message* msg, cstr_t* sb, int print_raw);

#ifdef __cplusplus
  }
#endif
/*---------------------------------------------------------------------------*/
#endif /* MQTT_MESSAGE_H_ */
