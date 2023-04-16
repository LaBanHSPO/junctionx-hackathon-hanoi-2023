#include <SoftwareSerial.h>

#define DEBUG true
#define MODE_1A

#define DTR_PIN 9
#define RI_PIN 8
#define interval 120

#define LTE_PWRKEY_PIN 5
#define LTE_RESET_PIN 6
#define LTE_FLIGHT_PIN 7
#define LTE_POWER_PIN 27

char* SEND_IOT_SIGNAL_DATA_TOPIC = "/iot/signal-data"; // length = 16
char* IOT_LISTEN_COMMAND = "/iot/command"; // lenght = 12


int flag = 1;
int flag1 = 0;
int state = 0;

int setup_done = 0;
long int previousMillis  = 0;


SoftwareSerial simSerial(2, 3); // RX - TX

void reset_connection() {
  sim_at_cmd("AT+CMQTTREL=0");// Release a client
  delay(1000);
  sim_at_cmd("AT+CMQTTSTOP");// Close connection
  delay(1000);
}
void sim_at_wait()
{
 // if (setup_done == 1) {return;};
 // Serial.println("Setup...");
  delay(100);
   while (simSerial.available()) {
        Serial.write(simSerial.read());
    }
}

bool sim_at_cmd(String cmd) {
  simSerial.println(cmd);
  sim_at_wait();
}

bool sim_at_send(char c) {
  simSerial.println(c);
}

void reset_modem() {
  sim_at_cmd("AT+CRESET ");
}


void setup() {
  Serial.begin(115200);
  simSerial.begin(115200);
  // simSerial.write(16, HIGH);
  pinMode(LTE_RESET_PIN, OUTPUT);
  digitalWrite(LTE_RESET_PIN, LOW);

  pinMode(LTE_PWRKEY_PIN, OUTPUT);
  digitalWrite(LTE_RESET_PIN, LOW);
  delay(100);
  digitalWrite(LTE_PWRKEY_PIN, HIGH);
  delay(2000);
  digitalWrite(LTE_PWRKEY_PIN, LOW);

  pinMode(LTE_FLIGHT_PIN, OUTPUT);
  digitalWrite(LTE_FLIGHT_PIN, LOW); //Normal Mode
  // digitalWrite(LTE_FLIGHT_PIN, HIGH);//Flight Mode

  delay(1000);
  sim_at_cmd("AT");

  //=====================
//    reset_modem();
//    return;
  //=====================


  // setup 4G internet connection
  sim_at_cmd("AT+CGSOCKCONT=1,\"IP\",\"v-internet\""); // APN | AT+CGSOCKCONT=1,"IP","v-internet"
  delay(1000);
  Serial.println("auth start");
  sim_at_cmd("AT+CGAUTH=1,1,\"v-internet\",\" \""); // Username     | AT+CGAUTH=1,1,"v-internet","no_pass"
  Serial.println("auth result");
  delay(1000);
  sim_at_cmd("AT+CGATT=1");  // Enabling GPRS
  delay(1000);
  sim_at_cmd("AT+CGPADDR"); // Show IPv4 from 4G network
  delay(3000);
    sim_at_cmd("AT+COPS?");
  delay(1000);
  reset_connection();
  
  //check MQTT related operations
  sim_at_cmd("AT+CSQ");
  delay(1000);
  sim_at_cmd("AT+CREG?");
  delay(1000);
  sim_at_cmd("AT+CGREG?");
  delay(1000);
  // setup MQTT connection
  sim_at_cmd("AT+CMQTTSTART");
   sim_at_cmd("ATE0");
  delay(2000);
  sim_at_cmd("AT+CMQTTACCQ=0,\"helsiki\"");// Client ID | AT+CMQTTACCQ=0,"helsi"
  delay(2000);

  sim_at_cmd("AT+CMQTTCFG=\"checkUTF8\",0,0");
  delay(1000);
  
//  sim_at_cmd("AT+CMQTTCONNECT=0,\"tcp://test.mosquitto.org:1883\",60,1"); //MQTT Server Name for connecting this client
   sim_at_cmd("AT+CMQTTCONNECT=0,\"tcp://103.110.87.102:6011\",90,1,\"helsinki\",\"Ai4TlnL18e8e9aQB95qxrqW\""); // "AT+CMQTTCONNECT=0,"tcp://103.110.87.102:6011",90,1,"helsinki","Ai4TlnL18e8e9aQB95qxrqW" | <server_addr>,<keepalive_time>,<clean_session>[,<user_name>[,<pass_word> |
  delay(1000);


  //SUBSCRIBE MESSAGE
  //Need to be executed once
  simSerial.println("AT+CMQTTSUBTOPIC=0,12,0"); //AT Command for Setting up the Subscribe Topic Name
  delay(500);
  simSerial.println(IOT_LISTEN_COMMAND); //Topic Name
  delay(1000);
  
 // Subscribe a message
  sim_at_cmd("AT+CMQTTSUB=0");
  delay(1000);
  Serial.println("Setup Done");
  setup_done = 1;
}

void loop() {
      String a;
      String combineResponse = "";
      String sinrResponse = "";
        //    send every 2 minutes, you need to edit the value above
      if (millis() - previousMillis > interval) {
        previousMillis = millis();
        //    get rsrq and rsrp value        
        simSerial.println("AT+CSQ");
        delay(100);
        while (simSerial.available()) {
           combineResponse = simSerial.readString();
        }
        
        Serial.print("combine response la");
        Serial.println(combineResponse);
        //get sinr value
          simSerial.println("AT+CESQ");
          while (simSerial.available()){
             sinrResponse =  simSerial.readString();
          }
        Serial.print("sinrResponse la:");
        Serial.println(sinrResponse);
        sim_at_send("AT+CMQTTTOPIC=0,16"); // AT Command for Setting up the Publish Topic Name
        delay(1000);
        sim_at_cmd(SEND_IOT_SIGNAL_DATA_TOPIC); //Topic Name
        delay(1000);
        int lengthPayload = combineResponse.length() + sinrResponse.length() + 1;
        Serial.print("so luong phan tu cua char"+ strlen(char(lengthPayload)));
        char payloadCommand = "AT+CMQTTPAYLOAD=0," + (char)(lengthPayload);
        simSerial.println(payloadCommand); //Payload length
        delay(1000);
        String payloadMessage = combineResponse + ";" + sinrResponse;

        char* payloadMessageArray = new char[payloadMessage.length() + 1];
        strcpy(payloadMessageArray, payloadMessage.c_str());

        simSerial.println(payloadMessageArray);
//        sim_at_send(payloadMessageArray); //Payload message
        delay(1000);
        sim_at_cmd("AT+CMQTTPUB=0,1,60"); //Acknowledgment
        delay(1000);
      }


// Receiving MODEM Response
  while(simSerial.available()>0)
  {
    delay(10);
    a = simSerial.readString();
    if(flag==0)
    {
      //Serial.println(a);
    flag = 1;
    }
    Serial.println(a);
    if(a.indexOf("command") != -1)
    {
       flag = 0;
       int new1 = a.indexOf("command");
       String neww = a.substring(new1);
       int new2 = neww.indexOf('\n');
       String new3 = neww.substring(new2+1);
       int new4 = new3.indexOf(';');
       String new5 = new3.substring(0,new4);
       // reassign interval_send_time
       Serial.println("Topic: ");
       Serial.print("Message is: "+ a);
     
    }      
  }
}


//    if(flag1 == 0)
//    {
//      //PUBLISH MESSAGE
//      flag1 = 1;
//      Serial.println("Publishing Message: LED ON");
//    }
//    else if(flag1 == 1)
//    {
//      flag1 = 0; 
//      Serial.println("Publishing Message: LED OFF");
//      sim_at_send("AT+CMQTTTOPIC=0,16"); //AT Command for Setting up the Publish Topic Name
//      delay(1000);
//      sim_at_cmd(SEND_IOT_SIGNAL_DATA_TOPIC); //Topic Name
//      delay(1000);
//      sim_at_send("AT+CMQTTPAYLOAD=0,1"); //Payload length
//      delay(1000);
//      sim_at_cmd("b"); //Payload message
//      delay(1000);
//      simSerial.println("AT+CMQTTPUB=0,1,60"); //Acknowledgment
//      delay(1000);
//    }
//  if(state==1)
//  {
//    if(flag1 == 0)
//    {
//      //PUBLISH MESSAGE
//      flag1 = 1;
//      Serial.println("Publishing Message: LED OFF");
//      simSerial.println("AT+CMQTTTOPIC=0,16"); //AT Command for Setting up the Publish Topic Name
//      delay(1000);
//      simSerial.println(SEND_IOT_SIGNAL_DATA_TOPIC); //Topic Name
//      delay(1000);
//      simSerial.println("AT+CMQTTPAYLOAD=0,1"); //Payload length
//      delay(1000);
//      simSerial.println("b"); //Payload message
//      delay(1000);
//      simSerial.println("AT+CMQTTPUB=0,1,60"); //Acknowledgment
//      delay(1000);
//    }
//    else if(flag1 == 1)
//    {
//      flag1 = 0;
//      Serial.println("Publishing Message: LED ON");
//      simSerial.println("AT+CMQTTTOPIC=0,16"); //AT Command for Setting up the Publish Topic Name
//      delay(1000);
//      simSerial.println(SEND_IOT_SIGNAL_DATA_TOPIC); //Topic Name
//      delay(1000);
//      simSerial.println("AT+CMQTTPAYLOAD=0,1"); //Payload length
//      delay(1000);
//      simSerial.println("a"); //Payload message
//      delay(1000);
//      simSerial.println("AT+CMQTTPUB=0,1,60"); //Acknowledgment
//      delay(1000);
//    }
//  }

//    if(flag1 == 0)
//    {
//      //PUBLISH MESSAGE
//      flag1 = 1;
//      Serial.println("Publishing Message: LED ON");
