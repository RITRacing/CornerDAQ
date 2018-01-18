#include <mbed.h>
#include <sstream>
#include <bitset>
#include <iostream>
#include <string>

/*
 * Continuously collects data from a specified number of analog sensors and
 * sends the data over CAN.
 * Depending on the amount of sensors, values may be distributed over
 * multiple packets.
 *
 * @author Chris Blust
 */

#define CAN_BPS 250000 //bits per second
#define DEBUG 1 //whether or not the program prints values over serial
#define DELAY 0.001 //how long to wait between messages

//references to the sensors, add new sensors here
AnalogIn sensors[] = {
  //A0-A5
  AnalogIn(PA_0),
  //AnalogIn(PA_1),
  //AnalogIn(PA_4),
  //AnalogIn(PB_0),
  //AnalogIn(PC_1),
  //AnalogIn(PC_0),

  //D12, D11
  //AnalogIn(PA_6),
  //AnalogIn(PA_7)
};

//must add new ID for each set of 4 sensors
const int CAN_IDS[] = {
  1234,
  1235
};

//the CAN bus
CAN can(PB_8, PB_9, CAN_BPS);

//for debugging purposes
CAN can2(PB_5, PB_6, CAN_BPS);

//an integer representing the number of sensors
const uint8_t NUM_SENS = sizeof(sensors)/sizeof(sensors[0]);

/*
 * takes in an array of 12 bit integers and constructs CAN packes with 4
 * values each
 */
CANMessage * construct_can(uint16_t * values, uint8_t msg_num){
  uint8_t values_left = NUM_SENS; //values left to transmit
  CANMessage * messages = new CANMessage[msg_num];

  for(int c = 0; c < msg_num; ++c){
      unsigned char ch[8] = {0,0,0,0,0,0,0,0}; //the CAN message
      uint8_t x = c * 4; //index to iterate over values
      uint8_t l = 8; //how many bytes of the message to fill
      if(values_left < 4){
          l = values_left * 2;
      }

      for(int i = 0; i < l; i += 2){
          ch[i+1] = (values[x] & 0x00FF); //least sig bits of value
          ch[i] = (values[x] & 0xFF00) >> 8; //most sig bits of value
          ++x;
      }

      messages[c] = CANMessage(CAN_IDS[c], (const char *)ch, 8);

      values_left -= 4;
  }

  return messages;
}

int main() {
    if(DEBUG) printf("Start\r\n");
    uint16_t values[NUM_SENS];
    while(1) {
        //get an array of current sensor readings
        for(uint8_t i = 0; i < NUM_SENS; ++i){
            values[i] = (uint16_t) 4095 * sensors[i].read();
        }

        if(DEBUG){
            printf("Sending: ");
            for(int i = 0; i < NUM_SENS; ++i){
                printf("%d ", values[i]);
            }
            printf("\r\n");
        }

        //determine the number of CAN packets required
        uint8_t msg_num = NUM_SENS / 4;
        if(NUM_SENS % 4 != 0){
            ++msg_num;
        }

        //construct an appropriate number of CAN packets
        CANMessage * messages = construct_can(values, msg_num);

        //send the CAN packets
        for(uint8_t i = 0; i < msg_num; ++i){
            if(!can.write(messages[i])){
                can.reset();
                if(DEBUG) printf("can resetting\r\n");
            }
        }
        free(messages);


/*
            CANMessage msg;
            for(int i = 0; i < msg_num; ++i){
                if(can2.read(msg)){
                    uint16_t vs[4] = {0,0,0,0};
                    uint8_t v = 0;
                    for(int x = 0; x < 4; ++x){
                        vs[x] = (vs[x] | msg.data[v]) << 8;
                        vs[x] = (vs[x] | msg.data[v + 1]);
                        v+=2;
                    }
                    printf("recieved: ");
                    for(int x = 0; x < 4; x++){
                        printf("%d ", vs[x]);
                    }
                    printf("\r\n");
                }
            }
            printf("\r\n");

            wait(1.3);
            printf("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");


        wait(DELAY);
        can.reset();
        */
    }
}
