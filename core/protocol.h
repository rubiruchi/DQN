#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <stdlib.h>
#include <math.h>
#include "bloom.h"


// define DQN timing
#define DQN_GUARD 15
#define DQN_SHORT_GUARD 5
#define DQN_TR_LENGTH 150
#define DQN_PREAMBLE 6

//#define DQN_PREAMBLE 12

// define DQN encodings
// // TODO: fix all the rates
// #define DQN_RATE_FEEDBACK RH_RF95::Bw500Cr48Sf4096
// #define DQN_SLOW_CRC RH_RF95::Bw500Cr48Sf4096
// #define DQN_FAST_CRC RH_RF95::Bw500Cr45Sf4096
// #define DQN_SLOW_NOCRC RH_RF95::Bw500Cr48Sf4096NoHeadNoCrc

// device only
#define DQN_IDLE 0
#define DQN_SYNC 1
#define DQN_TRAN 2
#define DQN_CRQ  3
#define DQN_DTQ  4
#define DQN_REQ  5
#define DQN_ADJT 6
#define DQN_SENT 7

// define DQN meta
#define DQN_VERSION                 0x27
#define DQN_MESSAGE_TR              0x80 // need to use mask
#define DQN_MESSAGE_FEEDBACK        0x01
#define DQN_MESSAGE_TR_JOIN         0x90 // need to use mask
#define DQN_MESSAGE_JOIN_REQ        0xa0
#define DQN_MESSAGE_JOIN_RESP       0xa1
#define DQN_MESSAGE_MASK            0x0f
#define DQN_MESSAGE_DOWNSTREAM      0x04
// define sync
#define DQN_SYNC_INTERVAL           36000000    // 10 hours
// define retry times
// value for testing only.
#define DQN_SYNC_RETRY              2 // if it fails twice receiving feedback, we need to re-sync.
// define hardware information
#define HW_ADDR_LENGTH      6

// radio configuration
// arduino
#ifdef ARDUINO
// remove pin numbers from protocol
// #define RFM95_CS 8
// #define RFM95_RST 4
// #define RFM95_INT 3
// #define VBATPIN A7

#include <SPI.h>
#include <RH_RF95.h>

#undef max      // Arduino toolchain will report error if standard max macro is around
#undef min
#include <queue.h>
#include <map.h>

using namespace etl;

#else
// raspberry pi
// remove pin numbers from protocol
// #define RF95_RESET_PIN 0  // this is BCM pin 17, physical pin 11.
// #define RF95_INT_PIN 7    // this is BCM pin 4, physical pin 7.
// #define RF95_CS_PIN 10    // this is BCM pin 8, physical pin 24
// // wiringPi pin numbers
// #define TX_PIN 4
// #define RX_PIN 5

#include <wiringPi.h>
#include <RH_RF95.h>
#include <queue>
using namespace std;

#include <map>
#endif

#define RF95_FREQ 915.0

// frame config
#define DQN_BF_ERROR 0.01
#define DQN_FRAME_SF 12

//#define DQN_FRAME_SF 10
#define DQN_FRAME_BW 500
#define DQN_FRAME_CRC true
#define DQN_FRAME_NOCRC false

// FIXEDLEN means IMPLICIT header (TRUE)
// VARIABELLEN means EXPLICIT header (false)
#define DQN_FRAME_FIXED_LEN true
#define DQN_FRAME_VARIABLE_LEN false

#define DQN_FRAME_CR 4
#define DQN_FRAME_LOW_DR false

// limitation of arduino-based server
#define DQN_DEVICE_QUEUE_SIZE 255
#define DQN_SERVER_MAX_TR 256
#define DQN_SERVER_MAX_BLOOM 256
#define DQN_SERVER_MAX_DATA_SLOT 256
#define DQN_NODE_CAPACITY 256 // may increase this size later
#define DQN_MESSAGE_QUEUE_SIZE 10


struct dqn_tr{
    uint8_t         version;
    uint8_t         messageid;
    uint16_t        nodeid;  // upstream only. otherwise ignored
    // 1 byte
    uint8_t         crc;
} __attribute__((packed));  // total is 5 bytes


struct  dqn_feedback{
    uint8_t         version;        // 1
    uint8_t         messageid;      // 1
    uint32_t        networkid;      // 4
    uint32_t        timestamp;      // 4
    uint16_t        crq_length;     // 2
    uint16_t        dtq_length;     // 2
    uint16_t        frame_param;    // 2
    uint8_t         data[255 - 16]; // this is a placeholder. actual size needs to be computed
} __attribute__((packed));  // total is 24 bytes for DQN_M = 32

struct dqn_ack{
    uint8_t         version;
    uint8_t         messageid;
    uint8_t         data_acks[32]; // this is a buf
} __attribute__((packed));  // total is 18 bytes

struct dqn_join_req{
    uint8_t         version;
    uint8_t         messageid;
    uint8_t         hw_addr[HW_ADDR_LENGTH];
} __attribute__((packed));  // total is 8 bytes


struct dqn_join_resp{
    uint8_t         version;
    uint8_t         messageid;
    uint8_t         hw_addr[HW_ADDR_LENGTH];
    uint16_t        nodeid;
} __attribute__((packed)); // total is 10 bytes


struct dqn_data_request{
    // this is only used by server
    // assume that it is pretty good memory management
    //
    uint8_t messageid;
    uint16_t nodeid;
} __attribute__((packed));


struct dqn_node_message{
    uint8_t* data;
    uint8_t size;
} __attribute__((packed));

class SendFunction;
class RadioDevice;
class Node;
class Server;

uint16_t dqn_make_feedback(
        struct dqn_feedback* feedback,
        uint32_t        networkid,
        uint16_t        crq_length,
        uint16_t        dtq_length,
        uint16_t         frame_param,
        uint8_t         *slots,
        uint16_t        num_of_slots,
        struct bloom    *bloom);

struct dqn_tr* dqn_make_tr(
        struct          dqn_tr* tr,
        uint8_t         num_of_slots,
        bool            high_rate,
        uint16_t        nodeid);

struct dqn_tr* dqn_make_tr_down(
        struct          dqn_tr* tr,
        uint8_t         num_of_slots,
        bool            high_rate,
        uint16_t        nodeid);

struct dqn_tr* dqn_make_tr_join(
        struct          dqn_tr *tr,
        bool            high_rate);

struct dqn_join_req* dqn_make_join_req(
        struct dqn_join_req* req,
        uint8_t         *hw_addr);

struct dqn_join_resp* dqn_make_join_resp(
        struct dqn_join_resp* resp,
        uint8_t  *hw_addr,
        uint16_t nodeid);

// these are wrapper functions to send data through RadioHead library
void dqn_send(
        RH_RF95 *rf95,
        const void* data,
        size_t size);
//
// void dqn_send(
//         RH_RF95 *rf95,
//         const void* data,
//         size_t size,
//         RH_RF95::ModemConfigChoice choice);

// these are wrapper functions for receive functions
// if 0 is passed to wait_time, it will block the execution till a
// packet is received.
// uint8_t dqn_recv(
//         RH_RF95 *rf95,
//         uint8_t* buf,
//         uint32_t wait_time,
//         RH_RF95::ModemConfigChoice choice,
//         uint32_t *received_time);

uint8_t dqn_recv(
         RH_RF95 *rf95,
         uint8_t* buf,
         uint32_t wait_time,
         uint32_t *received_time);

uint8_t dqn_recv(
        RH_RF95 *rf95,
        uint8_t* buf,
        uint32_t wait_time);


RH_RF95* setup_radio(RH_RF95 *rf95);

// compute crc8
uint8_t get_crc8(char *data, int len);

// message printing
void mprint(const char *format, ...);

// for debugging and printing info only
void print_feedback(struct dqn_feedback* feedback, int8_t rssi);

// defining the base class for both server and device
// a base class wrapper for all DQN methods
class RadioDevice{
    private:
        uint8_t _rf95_buf[sizeof(RH_RF95)];

    protected:
        float   freq;
        // the following four attributes specifies how long (ms) wach sub-frame is
        // how long each data slot is
        uint16_t data_length;
        uint16_t feedback_length;
        uint16_t ack_length;
        uint32_t frame_length;

        // hardware address
        uint8_t hw_addr[HW_ADDR_LENGTH];

        // reconfigurable network information
        uint16_t num_tr;
        uint16_t num_data_slot;
        double bf_error;
        uint16_t max_payload;

        // used to receive and send message
        uint8_t _msg_buf[255];

        // it will set all the network information based on the feedback
        void parse_frame_param(
                struct dqn_feedback *feedback);
        // returns feedabck length
        uint32_t get_feedback_length();

        // helper function to tell whether the device is busy
        bool is_receiving();

    public:

        typedef enum
        {
          TR = 0,
          Feedback,
          Data,
          Ack
        } DqnModemMode;

        RH_RF95 *rf95;
        RadioDevice(struct RH_RF95::pin_config pc, float freq);
        bool configureModem(DqnModemMode mode);
        // set up the radio
        //void setup(struct RH_RF95::pin_config pc);
        void set_hw_addr(const uint8_t *hw_addr);
        uint16_t get_frame_param();
        // get lora air time
        // TODO: consider to remove this since RadioHead already has this one
        uint16_t get_lora_air_time(uint32_t bw, uint32_t sf, uint32_t pre,
                uint32_t packet_len, bool crc, bool fixed_len, uint32_t cr, bool low_dr);
        // return the length (ms) of the entire frame
        // only usefull once the device knows the network configuration
        uint32_t get_frame_length();
        // just print
        void print_frame_info();
};

class Node: public RadioDevice{
    private:
        // timing control varialbes
        uint32_t time_offset;
        uint16_t nodeid;
        bool has_sync;
        uint32_t last_sync_time;
        bool has_joined;

        // if the device doesn't receive feedback for certain times
        // it will try to re-sync. retry_count keeps track of the failures
        uint16_t retry_count;

        // return which rate to use
        // current always false (slow)
        bool determine_rate();

        void (*on_receive)(uint8_t*, uint8_t);


        // message queue
        uint8_t _queue_buf[DQN_MESSAGE_QUEUE_SIZE * 30];
#ifdef ARDUINO
        queue<struct dqn_node_message, DQN_MESSAGE_QUEUE_SIZE> message_queue;
#else
        queue<struct dqn_node_message> message_queue;
#endif

        // old C++ doesn't have delegating constructors
        // I miss C# (c++11 has it)
        void ctor(struct RH_RF95::pin_config pc,
          uint8_t *hw_addr);

        // this is for all the communication requests to the base station
        uint16_t send_request(struct dqn_tr *tr, uint8_t num_of_slots,
                void (*on_feedback_received)(struct dqn_feedback *), uint8_t send_command);

        // these functions are called in the DTQ
        // the index tells you which data slot is being used
        void send_data(int index);
        void join_data(int index); // for join process only
        void receive_data(int index);
    public:
        // this will generate a fixed hardware addresss
        Node(struct RH_RF95::pin_config pc, float freq = RF95_FREQ);
        Node(struct RH_RF95::pin_config pc, float freq, uint8_t *hw_addr);
        
        void set_hw_addr(uint8_t *hw_addr);
        
        void sync();
        // send returns how many bytes been sent
        uint32_t send();
        // this is send with ACK
        uint32_t send(bool *ack);
        // calls when the nodes wish to receive some data
        void recv();
        void sleep(int32_t time);
        void join();
        void check_sync();
        // has to be called if the data wish to send any data
        bool add_data_to_send(uint8_t * data, uint8_t size);
        // helper function to tell device the maximum payload size
        uint16_t mpl();
        // set on_receive function callback
        void set_on_receive(void (*on_receive)(uint8_t*, uint8_t));
};

class Server: public RadioDevice{
    private:
        uint32_t networkid;
        uint32_t crq;
        uint32_t dtq;
        struct bloom bloom;
        uint8_t tr_status[DQN_SERVER_MAX_TR];
        // this holds the bloom filter
        uint8_t _bloom_buf[DQN_SERVER_MAX_BLOOM];

        // the following is used for static allocation
        // this holds all the TR requests in the queue
        uint8_t _tr_data_buf[sizeof(struct dqn_data_request) * DQN_SERVER_MAX_TR];
        // this holds all the hardware address registed in the server
        uint8_t _hw_addr_buf[HW_ADDR_LENGTH * DQN_NODE_CAPACITY];
        uint8_t ack_buf[DQN_SERVER_MAX_DATA_SLOT / 8];

#ifdef ARDUINO
        // use ETL for arduino
        queue<struct dqn_data_request *, DQN_DEVICE_QUEUE_SIZE> dtqueue;
        etl::map<uint16_t, uint8_t *, DQN_NODE_CAPACITY> node_table;
        etl::map<uint8_t *, uint16_t, DQN_NODE_CAPACITY> node_table_invert;
#else
        queue<struct dqn_data_request *> dtqueue;
        // use double table to reduce the implementation difficulty
        // at the cost of memory space
        // since the buffer is already allocated, it is actually not that bad.
        map<uint16_t, uint8_t*> node_table;
        map<uint8_t *, uint16_t> node_table_invert;
#endif

        // function call backs
        void (*on_receive)(uint8_t *data, size_t size, uint8_t *hw_addr);
        uint16_t (*on_download)(uint8_t *hw_addr, uint8_t *data, uint8_t);

        void send_feedback();
        void receive_tr();
        void send_ack();
        void reset_frame();
        void recv_data();
        uint16_t register_device(uint8_t *hw_addr);
        void end_cycle();
        void recv_node();
    public:
        Server(uint32_t networkid,
                float freq,
                struct RH_RF95::pin_config pc,
                void (*on_receive)(uint8_t*, size_t, uint8_t *),
                uint16_t (*on_download)(uint8_t*, uint8_t*, uint8_t));
        // this is a blocking method
        void run();

        void change_network_config(uint8_t trf, double fpp, int dtr, uint8_t mpl);
};

#endif
