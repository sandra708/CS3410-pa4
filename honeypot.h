#ifndef HONEYPOT_H_
#define HONEYPOT_H_

/*
 * Honeypot command packets have the following format.
 *
 *  +--------------------+---------+---------+--------------+---------------+
 *  | IP and UDP headers | secret  |   cmd   |     data     |  garbage      |
 *  |    28 bytes        | 2 bytes | 2 bytes |   4 bytes    |  >= 0 bytes   |
 *  +--------------------+---------+---------+--------------+---------------+
 *
 *  All of the fields are BIG ENDIAN, so you have to reverse the bytes to little
 *  endian before comparing them to an integer, or equivalently, reverse your
 *  integers into big endian before comparing them with the bytes. This is true
 *  of the IP and UDP headers as well, and generally true for network packets in
 *  the real world.
 *
 *  The 'secret' will be the number 0x3410. It is in BIG ENDIAN format, so it
 *  will appear as a 0x34 byte followed by a 0x10 byte. On our MIPS little
 *  endian machine, if this were cast directly to a 2-byte short, it would
 *  appear as 0x1034.  If any other value is found in this field, then it isn't
 *  a command packet, and should be processed as a regular non-command packet.
 *  (Yes, this is pathetically weak security. We considered making you do
 *  secure digital signatures, so be glad.)
 *
 *  The 'cmd' will be one of the values below in BIG ENDIAN format.
 *  The 'data' is the argument to the command, in BIG ENDIAN format.
 */

// Layout of network packet IP and UDP headers
struct packet_header {
  // IP layer headers: 20 bytes
  // You only care about 'source address'.
  unsigned char ip_version;
  unsigned char ip_qos;
  unsigned short ip_len;
  unsigned short ip_id;
  unsigned short ip_flags;
  unsigned char ip_ttl;
  unsigned char ip_protocol;
  unsigned short ip_checksum;
  unsigned int ip_source_address_big_endian; // 'source address' in big-endian order
  unsigned int ip_dest_address_big_endian;

  // UDP layer headers: 8 bytes
  // You only care about 'destination port'.
  unsigned short udp_source_port_big_endian;
  unsigned short udp_dest_port_big_endian; // 'destination port' in big-endian order
  unsigned short udp_len;
  unsigned short udp_checksum;
};

// Layout of a honeypot command packet
struct honeypot_command_packet {
  struct packet_header headers;

  unsigned short secret_big_endian;
  unsigned short cmd_big_endian;
  unsigned int data_big_endian;
  
  // note: zero or more bytes of garbage follow after the data field
};

#define HONEYPOT_CMD_PKT_MIN_LEN (sizeof(struct honeypot_command_packet))

#define HONEYPOT_SECRET 0x3410




/* The Honeypot keeps 3 lists: 
 * - a list of 'source addresses' for known spammers
 * - a list of hashes of known evil packets
 * - a list of 'destination ports' for known vulnerabilities
 *
 * The addresses and hashes for the first two lists are 4 byte "int" values.
 * The ports for the third list are 2 byte "short" values.
 *
 * For the 2 byte port values, the 'data' field will always contain a number
 * small enough to fit in 2 bytes.
 */

// Add a new address, port, or hash to a list.
// The 'data' field contains the value to add.
#define HONEYPOT_ADD_SPAMMER    0x101
#define HONEYPOT_ADD_EVIL       0x102
#define HONEYPOT_ADD_VULNERABLE 0x103

// Remove an existing address, port, or hash from a list.
// The 'data' field contains the value to remove.
#define HONEYPOT_DEL_SPAMMER    0x201
#define HONEYPOT_DEL_EVIL       0x202
#define HONEYPOT_DEL_VULNERABLE 0x203

// Print out all of the current statistics.
// The 'data' field can be ignored.
#define HONEYPOT_PRINT		0x301

#endif
