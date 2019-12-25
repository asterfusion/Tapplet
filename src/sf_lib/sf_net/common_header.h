/*
 *------------------------------------------------------------------
 * Copyright (c) 2019 Asterfusion.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *------------------------------------------------------------------
 */
#ifndef __common_header_h__
#define __common_header_h__

#include <vppinfra/types.h>

#include <stdint.h>


enum
{
    SIG_PKT_UL_DIRECTION = 1,
    SIG_PKT_DL_DIRECTION = 2,
    SIG_PKT_UNKNOWN_DIRECTION = 3,
};

enum packet_type_e
{
    PACKET_TYPE_ERROR = 0,
    PACKET_TYPE_VLAN ,
    PACKET_TYPE_MPLS,
    PACKET_TYPE_GRE,
    PACKET_TYPE_VXLAN,
    PACKET_TYPE_NETFLOW_TEMPLATE,
    PACKET_TYPE_NETFLOW_DATA,
    PACKET_TYPE_SFLOW,
    PACKET_TYPE_SSL_TLS,
    PACKET_TYPE_CNT,

    PACKET_TYPE_MAX = 64, //keep consistent with  packet_common.h wdata_t definition
};

#define IP4_STR_MAX_LEN 16
#define IP6_STR_MAX_LEN 46
#define IP_STR_MAX_LEN IP6_STR_MAX_LEN

typedef union {
    struct
    {
        uint32_t addr_v4;
        uint32_t reserved;
    };
    struct
    {
        uint64_t addr_v6_upper;
        uint64_t addr_v6_lower;
    };
    uint8_t ipv4_byte[4];
    uint8_t ipv6_byte[16];
    uint32_t value_32[4];
    uint64_t value[2];
} ip_addr_t;

typedef enum
{
  IP_INDEX_IPV6 = 0,
  IP_INDEX_IPV4, /* for "is_ipv4" */
  IP_INDEX_CNT,
} sf_ip_index_t;
/*************************************/
/************** ethernet *************/
/*************************************/
#define ETHER_VN_TAG 0x8926
#define ETHER_MPLS_UC 0x8847
#define ETHER_MPLS_MC 0x8848
#define ETHER_VLAN 0x8100 //802.1Q
#define ETHER_QINQ 0x88a8 //802.1ad
#define ETHER_PPPoE_DISCOVERY 0x8863
#define ETHER_PPPoE_SESSION 0x8864
#define ETHER_DCE 0x8903
#define ETHER_TRILL 0x22f3
#define ETHER_MACINMAC 0x88e7 //802.1ah
#define ETHER_IPV4 0x0800
#define ETHER_IPV6 0x86DD
#define ETHER_ARP 0x0806

#undef FUNC
#ifdef CPU_BIG_ENDIAN
#define FUNC(name, byte0, byte1) name##_NET_ORDER = 0x##byte0##byte1
#else
#define FUNC(name, byte0, byte1) name##_NET_ORDER = 0x##byte1##byte0
#endif

enum
{
    ETHER_INVALID_TYPE = 0,
    FUNC(ETHER_VN_TAG, 89, 26),
    FUNC(ETHER_MPLS_UC, 88, 47),
    FUNC(ETHER_MPLS_MC, 88, 48),
    FUNC(ETHER_VLAN, 81, 00),
    FUNC(ETHER_QINQ, 88, a8),
    FUNC(ETHER_PPPoE_DISCOVERY, 88, 63),
    FUNC(ETHER_PPPoE_SESSION, 88, 64),
    FUNC(ETHER_DCE, 89, 03),
    FUNC(ETHER_TRILL, 22, f3),
    FUNC(ETHER_MACINMAC, 88, e7),
    FUNC(ETHER_IPV4, 08, 00),
    FUNC(ETHER_IPV6, 86, DD),
    FUNC(ETHER_ARP, 08, 06),
} sf_eth_type_t;

#undef FUNC

typedef struct
{
    uint8_t dst_mac[6];
    uint8_t src_mac[6];
    uint16_t eth_type;
} __attribute__((packed)) eth_header_t;
#define ETH_HEADER_LEN (sizeof(eth_header_t))

/*************************************/
/************** vlan     *************/
/*************************************/
typedef struct
{
#ifdef CPU_BIG_ENDIAN
    uint16_t priority : 3;
    uint16_t cfi : 1;
    uint16_t vlan_id : 12;

    uint16_t eth_type;
#else
    uint8_t vlan_id_up4 : 4;
    uint8_t cfi : 1;
    uint8_t priority : 3;
	uint8_t vlan_id_low8;

    uint16_t eth_type;
#endif // CPU_BIG_ENDIAN

} __attribute__((packed)) vlan_head_t;

#define VLAN_HEADER_LEN (sizeof(vlan_head_t))


#ifdef CPU_BIG_ENDIAN
    #define get_vlan_id(vlan_hdr) (vlan_hdr->vlan_id)
    #define get_vlan_id_netorder(vlan_hdr) (vlan_hdr->vlan_id)

#else
   #define get_vlan_id(vlan_hdr) \
   ( ( (uint16_t)((vlan_hdr)->vlan_id_up4) << 8) | (vlan_hdr)->vlan_id_low8 )

   #define get_vlan_id_netorder(vlan_hdr) \
   (  ((uint16_t) (  (vlan_hdr)->vlan_id_low8     )  << 8)  |   (vlan_hdr)->vlan_id_up4  )
#endif // CPU_BIG_ENDIAN


/*************************************/
/************** mpls     *************/
/*************************************/

typedef struct
{
#ifdef CPU_BIG_ENDIAN
    uint32_t label : 20;
    uint32_t exp : 3;
    uint32_t s : 1;
    uint32_t ttl : 8;
#else
    uint8_t label_1;
	uint8_t label_2;
    uint8_t  s : 1;
    uint8_t exp : 3;
	uint8_t label_3 : 4;
    uint8_t ttl : 8;

#endif // CPU_BIG_ENDIAN

} __attribute__((packed)) mpls_head_t;

#define MPLS_HEADER_LEN (sizeof(mpls_head_t))

/*************************************/
/************** ipv4     *************/
/*************************************/

#define IPV4_FLAGS_DONT_FRAG_MASK (0x1 << 1)
#define IPV4_FLAGS_MORE_FRAG_MASK (0x1)

#define IP_PRO_TCP 0x06
#define IP_PRO_UDP 0x11
#define IP_PRO_SCTP 132
#define IP_PRO_HOPOPT 0
#define IP_PRO_ICMP 1                    /* control message protocol */
#define IP_PRO_IGMP 2                    /* group mgmt protocol */
#define IP_PRO_GGP 3                     /* gateway^2 (deprecated) */
#define IP_PRO_IP 4                      /* IP inside IP */
#define IP_PRO_IPIP 4                    /* IP inside IP */
#define IP_PRO_EGP 8                     /* exterior gateway protocol */
#define IP_PRO_PUP 12                    /* pup */
#define IP_PRO_IDP 22                    /* xns idp */
#define IP_PRO_TP 29                     /* tp-4 w/ class negotiation */
#define IP_PRO_IPV6 41                   /* IPv6 in IPv6 */
#define IP_PRO_ROUTING 43                /* Routing header */
#define IP_PRO_FRAGMENT 44               /* Fragmentation/reassembly header */
#define IP_PRO_IPV6_FRAG IP_PRO_FRAGMENT /* Fragmentation/reassembly header */
#define IP_PRO_GRE 47                    /* GRE encap, RFCs 1701/1702 */
#define IP_PRO_ESP 50                    /* Encap. Security Payload */
#define IP_PRO_AH 51                     /* Authentication header */
#define IP_PRO_MOBILE 55                 /* IP Mobility, RFC 2004 */
#define IP_PRO_ICMPV6 58                 /* ICMP for IPv6 */
#define IP_PRO_NONE 59                   /* No next header */
#define IP_PRO_DSTOPTS 60                /* Destination options header */
#define IP_PRO_EON 80                    /* ISO cnlp */
#define IP_PRO_ETHERIP 97                /* Ethernet in IPv4 */
#define IP_PRO_ENCAP 98                  /* encapsulation header */
#define IP_PRO_PIM 103                   /* Protocol indep. multicast */

#define IP_PRO_IPCOMP 108 /* IP Payload Comp. Protocol */
#define IP_PRO_CARP 112   /* CARP */
#define IP_PRO_PFSYNC 240 /* PFSYNC */
#define IP_PRO_RAW 255    /* raw IP packet */
#define IP_PRO_OSPF 0x59
#define IP_PRO_RSVP 0x2e

#define IP_PRO_INVALID 0xff
/** IP (IPv4) Header Definition */
typedef union {
#ifdef CPU_BIG_ENDIAN
    struct
    {
        /* uint32_t word1; */
        uint32_t version : 4;
        uint32_t ipHeaderLen : 4;
        uint32_t ip_tos : 8;
        uint32_t totalLen : 16;
        /* uint32_t word2; */
        uint32_t id : 16;
        uint32_t flags : 3;
        uint32_t offset : 13;
        /* uint32_t word3; */
        uint32_t TTL : 8;
        uint32_t protocol : 8;
        uint32_t checksum : 16;
        /* uint32_t word4; */
        uint32_t src_ip;
        /* uint32_t word5; */
        uint32_t dst_ip;
    };
#else
    struct
    {
        /* uint32_t word1; */

        uint8_t ipHeaderLen : 4;
        uint8_t version : 4;
        uint8_t ip_tos;
        uint16_t totalLen;
        /* uint32_t word2; */
        uint16_t id : 16;
        union {
                struct {
                        uint8_t offset_5bit : 5;
                        uint8_t flags : 3;
                        uint8_t offset_8bit;
                };
                uint16_t flag_offset;
        };

        /* uint32_t word3; */
        uint8_t TTL;
        uint8_t protocol;
        uint16_t checksum;
        /* uint32_t word4; */
        uint32_t src_ip;
        /* uint32_t word5; */
        uint32_t dst_ip;
    };
#endif // CPU_BIG_ENDIAN
    uint8_t byte[20];
    uint32_t value_32[5];
} __attribute__((packed)) ipv4_header_t;
#define IPV4_HEADER_LEN (sizeof(ipv4_header_t))


#ifdef CPU_BIG_ENDIAN

#define get_ip4_hdr_flag(ip4_hdr) \
(  (ip4_hdr)->flags )

#define get_ip4_hdr_offset(ip4_hdr) \
(  (ip4_hdr)->offset )


#define ip4_is_frag(ip4_hdr) \
(  ((ip4_hdr)->flags   & IPV4_FLAGS_MORE_FRAG_MASK)  ||  (ip4_hdr)->offset  )

#define ip4_is_frag_first(ip4_hdr) \
(  ((ip4_hdr)->flags   & IPV4_FLAGS_MORE_FRAG_MASK)  &&  !((ip4_hdr)->offset)  )

#else

#define get_ip4_hdr_flag(ip4_hdr) \
(  (ip4_hdr)->flags )

#define get_ip4_hdr_offset(ip4_hdr) \
(  (((uint16_t)((ip4_hdr)->offset_5bit )) << 8)  | ((ip4_hdr)->offset_8bit ))

#define ip4_is_frag(ip4_hdr) \
(  ((ip4_hdr)->flags   & IPV4_FLAGS_MORE_FRAG_MASK)  ||  (ip4_hdr)->offset_8bit || (ip4_hdr)->offset_5bit )

#define ip4_is_frag_first(ip4_hdr) \
(  ((ip4_hdr)->flags   & IPV4_FLAGS_MORE_FRAG_MASK)  &&  !((ip4_hdr)->offset_8bit) && !((ip4_hdr)->offset_5bit) )


#endif


/*************************************/
/************** ipv6     *************/
/*************************************/

/** IP (IPv6) Header Definition */
typedef union { // total 40 bytes without options
    struct
    {
#ifdef CPU_BIG_ENDIAN
        /* uint32_t word1; */
        uint32_t version : 4;
        uint32_t traffic_class : 8;
        uint32_t flow_label : 20;

        /* uint32_t word2; */
        uint32_t payload_len : 16;
        uint32_t next_header : 8;
        uint32_t hop_limit : 8;

        ip_addr_t src_ip;
        ip_addr_t dst_ip;
#else  // CPU_BIG_ENDIAN
        uint8_t traffic_class_first4 : 4;
        uint8_t version : 4;

        uint8_t flow_label_first4 : 4;
        uint8_t traffic_class_seconde4: 4;

        uint8_t flow_label_second8 ;
        uint8_t flow_label_third8 ;

        /* uint32_t word2; */
        uint16_t payload_len;
        uint8_t next_header;
        uint8_t hop_limit;

        ip_addr_t src_ip;
        ip_addr_t dst_ip;
#endif // CPU_BIG_ENDIAN
    };
    uint8_t value[40];
    uint32_t value_32[10];
}__attribute__((packed)) ipv6_header_t;
#define IPV6_HEADER_LEN sizeof(ipv6_header_t)


#ifdef CPU_BIG_ENDIAN 
#define ip6_hdr_set_traffic_class(ipv6_hdr , u8_value) (ipv6_hdr)->traffic_class = u8_value;
#define ip6_hdr_get_traffic_class(ipv6_hdr) ((ipv6_hdr)->traffic_class)
#else
#define ip6_hdr_set_traffic_class(ipv6_hdr , u8_value) \
(ipv6_hdr)->traffic_class_first4 = (u8_value) >> 4 ;\
(ipv6_hdr)->traffic_class_seconde4 = (u8_value) & 0xF ;

#define ip6_hdr_get_traffic_class(ipv6_hdr) ((ipv6_hdr)->traffic_class_first4 << 4 | (ipv6_hdr)->traffic_class_seconde4)
#endif 
/**
 * IPv6 extension header format.
 */
typedef struct
{
    uint8_t next_header;
    uint8_t hdr_len;
} ipv6_ext_t;
#define IPV6_EXT_HEADER_LEN sizeof(ipv6_ext_t)

/**
 * IPv6 fragment header format.
 */
typedef struct
{
    uint8_t next_header;   /*next header*/
    uint8_t ip6f_reserved; /*reserved field*/
#ifdef CPU_BIG_ENDIAN
    uint16_t ip6f_offset : 13;
    uint16_t ip6f_unused : 2; /*offset, reserved, and flag*/
    uint16_t ip6f_flags : 1;
#else                    // CPU_BIG_ENDIAN
    uint8_t ip6f_offset_8bit;
    uint8_t ip6f_flags : 1;
    uint8_t ip6f_unused : 2; /*offset, reserved, and flag*/
    uint8_t ip6f_offset_5bit : 5;
#endif                   // CPU_BIG_ENDIAN
    uint32_t ip6f_ident; /*identification*/
} ipv6_frag_t;
#define IPV6_FRAG_HEADER_LEN sizeof(ipv6_frag_t)


#define get_ip6_frag_flag(ipv6_frag_ptr)\
((ipv6_frag_ptr)->ip6f_flags)


#ifdef CPU_BIG_ENDIAN

#define get_ip6_frag_offset(ipv6_frag_ptr) \
(  (ipv6_frag_ptr)->ip6f_offset )

#else

#define get_ip6_frag_offset(ipv6_frag_ptr) \
(  (((uint16_t)((ipv6_frag_ptr)->ip6f_offset_8bit )) << 5)  | ((ipv6_frag_ptr)->ip6f_offset_5bit ))

#endif



/*************************************/
/************** udp     *************/
/*************************************/

typedef union {
    struct
    {
        uint16_t src_port;
        uint16_t dst_port;
        uint16_t length;
        uint16_t checksum;
    };
    uint8_t byte[8];
} __attribute__((packed)) sf_udp_header_t;
// #endif
#define UDP_HEADER_LEN (sizeof(sf_udp_header_t))

/*************************************/
/************** tcp     *************/
/*************************************/
#define TH_FIN 0x01
#define TH_SYN 0x02
#define TH_RST 0x04
#define TH_PUSH 0x08
#define TH_ACK 0x10
#define TH_URG 0x20

/** TCP Header Definition */
typedef union { // total 20 bytes
    struct
    {
#ifdef CPU_BIG_ENDIAN

        /* uint32_t word1; */
        uint16_t src_port;
        uint16_t dst_port;
        /* uint32_t word2; */
        uint32_t seq;
        /* uint32_t word3; */
        uint32_t ack;
        /* uint32_t word4; */
        uint32_t hdr_len : 4;
        uint32_t reserved : 6;
        uint32_t flags : 6;
        uint32_t window : 16;
        /* uint32_t word5; */
        uint32_t checksum : 16;
        uint32_t urgent : 16;
#else  // CPU_BIG_ENDIAN
        /* uint32_t word1; */
        uint16_t src_port;
        uint16_t dst_port;
        /* uint32_t word2; */
        uint32_t seq;
        /* uint32_t word3; */
        uint32_t ack;
        /* uint32_t word4; */
        uint8_t reserved : 4;
        uint8_t hdr_len : 4;
        uint8_t flags : 6;
        uint8_t reserved_1 : 2;
        uint16_t window;
        /* uint32_t word5; */
        uint16_t checksum;
        uint16_t urgent;
#endif // CPU_BIG_ENDIAN
    };
    uint8_t value[20];
} __attribute__((packed)) sf_tcp_header_t;
// #define idpi_sf_tcp_header_t sf_tcp_header_t
#define TCP_HEADER_LEN sizeof(sf_tcp_header_t)

/*************************************/
/************** gre     *************/
/*************************************/
typedef struct{
#ifdef CPU_BIG_ENDIAN
    union{
        struct{
            uint16_t flag_C:1; /* If C=1, the checksum field present.
                                  If C=1 or R=1, both checksum and offset fields present. */
            uint16_t flag_R:1; /* If R=1, offset and routing fields present. */
            uint16_t flag_K:1; /* If K=1, key field present. */
            uint16_t flag_S:1; /* If S=1, sequence number field present. */
            uint16_t flag_s:1;
            uint16_t Recur:3;
            uint16_t flag_A:1; /*if A=1, acknowlegment */
            uint16_t Flags:4;
            uint16_t Ver  :3;
        };
        uint16_t flag_version;
    };
#else
    union{
        struct{
            uint8_t Recur:3;
            uint8_t flag_s:1;
            uint8_t flag_S:1; /* If S=1, sequence number field present. */
            uint8_t flag_K:1; /* If K=1, key field present. */
            uint8_t flag_R:1; /* If R=1, offset and routing fields present. */
            uint8_t flag_C:1; /* If C=1, the checksum field present.
                                  If C=1 or R=1, both checksum and offset fields present. */
            uint8_t Ver  :3;
            uint8_t Flags:4;
            uint8_t flag_A:1; /*if A=1, acknowlegment */

        };
        uint16_t flag_version;
    };

#endif

    uint16_t protocol_type; /* payload protocol type */
}__attribute__((packed)) sf_gre_header_t;
#define GRE_HEADER_LEN (sizeof(sf_gre_header_t))


/*************************************/
/************** vxlan     *************/
/*************************************/
#ifdef CPU_BIG_ENDIAN
#define VXLAN_FLAGS 0x08000000
#define DEFAULT_VXLAN_UDP_PORT  0x12b5  //host is 4789  0x12b5
#else
#define VXLAN_FLAGS 0x08
#define DEFAULT_VXLAN_UDP_PORT  0xb512  //host is 4789  0x12b5
#endif

typedef struct _vxlan_header{
    uint32_t vx_flags_resv;
    union{
        uint32_t vx_vni_resv;    
        struct{
            uint8_t vx_vni_1;
            uint8_t vx_vni_2;
            uint8_t vx_vni_3;
            uint8_t resv_2;
        };
    };
}__attribute__((packed))sf_vxlan_header_t;
#define VXLAN_HEADER_LEN sizeof(sf_vxlan_header_t)


/*************************************/
/************* erspan_ii *************/
/*************************************/
typedef union{
    struct{
#ifdef CPU_BIG_ENDIAN
        uint16_t Ver:4;
        uint16_t Vlan:12;
        uint16_t COS:3;
        uint16_t En:2;
        uint16_t T:1;
        uint16_t Session_ID:10;
        uint32_t Reserved:12;
        uint32_t Index:20;
#else
        uint16_t Vlan:12;
        uint16_t Ver:4;
        uint16_t Session_ID:10;
        uint16_t T:1;
        uint16_t En:2;
        uint16_t COS:3;
        uint32_t Index:20;
        uint32_t Reserved:12;
#endif
    };
    uint8_t value[8];
} __attribute__((packed)) sf_erspan_ii_header_t;

/*************************************/
/************* erspan_iii ************/
/*************************************/
typedef union{
    struct{
#ifdef CPU_BIG_ENDIAN
        uint16_t Ver:4;
        uint16_t Vlan:12;
        uint16_t COS:3;
        uint16_t BSO:2;
        uint16_t T:1;
        uint16_t Session_ID:10;
        uint32_t Timestamp;
        uint16_t SGT;
        uint16_t P:1;
        uint16_t FT:5;
        uint16_t Hw_ID:6;
        uint16_t D:1;
        uint16_t Gra:2;
        uint16_t O:1;
#else
        uint16_t Vlan:12;
        uint16_t Ver:4;
        uint16_t Session_ID:10;
        uint16_t T:1;
        uint16_t BSO:2;
        uint16_t COS:3;
        uint32_t Timestamp;
        uint16_t SGT;
        uint16_t O:1;
        uint16_t Gra:2;
        uint16_t D:1;
        uint16_t Hw_ID:6;
        uint16_t FT:5;
        uint16_t P:1;
#endif
    };
    uint8_t value[12];
} __attribute__((packed)) sf_erspan_iii_header_t;

/*************************************/
/************* icmp      ************/
/*************************************/
typedef union{
    struct{
        uint8_t type;
        uint8_t code;
        uint16_t checksum;
        uint32_t reserved;
    };
    uint8_t value[8];
} __attribute__((packed)) icmp_header_t;

/*************tenant*****************/
typedef struct{
    uint8_t tenant_id[16];
    uint32_t vni;
} __attribute__((packed)) tenant_t;

/************* ssl tls      ************/
/*************************************/
#ifdef CPU_BIG_ENDIAN
#define DEFAULT_SSL_TLS_HTTPS_PORT  0x01BB  //host is 443  0x1BB
#else
#define DEFAULT_SSL_TLS_HTTPS_PORT  0xBB01  //host is 443  0x1BB
#endif

#endif //__common_header_h__
