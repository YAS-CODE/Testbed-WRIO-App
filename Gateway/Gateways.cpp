#define MQTTCLIENT_QOS2 1

#include <memory.h>

#include "MQTTClient.h"

#define DEFAULT_STACK_SIZE -1

#include "linux.cpp"

/*---------------------------------------------------------------------------*/
#include "tools-utils.h"


#include <pthread.h> 


int arrivedcount = 0;
IPStack ipstack = IPStack();
MQTT::Client<IPStack, Countdown> client = MQTT::Client<IPStack, Countdown>(ipstack);

int messagePublish(const char* topic,const char* messagedata){
	int rc;
	MQTT::Message message;
	char buf[100];
	sprintf(buf, "%s", messagedata);
	message.qos = MQTT::QOS0;
	message.retained = false;
	message.dup = false;
	message.payload = (void*)buf;
	message.payloadlen = strlen(buf)+1;

	rc = client.publish(topic, message);
	if (rc != 0)
		printf("Error %d from sending QoS 0 message\n", rc);
}

void zolertiaMessageArrived(MQTT::MessageData& md)
{
	MQTT::Message &message = md.message;

	printf("Message %d arrived: qos %d, retained %d, dup %d, packetid %d\n", 
			++arrivedcount, message.qos, message.retained, message.dup, message.id);
	printf("Payload %.*s\n", (int)message.payloadlen, (char*)message.payload);

	if(strncmp("on",(char *)message.payload,(int)message.payloadlen)==0){
		system("sudo /share/yepkit-USB-hub/ykushcmd -u 1");
		sleep(1);
		messagePublish("/iot/zolertia/reply","on");
	}
	if(strncmp("off",(char *)message.payload,(int)message.payloadlen)==0){
		system("sudo /share/yepkit-USB-hub/ykushcmd -d 1");
		sleep(1);
		messagePublish("/iot/zolertia/reply","off");
	}    
}

void sensorMessageArrived(MQTT::MessageData& md)
{
	MQTT::Message &message = md.message;

	printf("Message %d arrived: qos %d, retained %d, dup %d, packetid %d\n", 
			++arrivedcount, message.qos, message.retained, message.dup, message.id);
	printf("Payload %.*s\n", (int)message.payloadlen, (char*)message.payload);    	
}



void *SensorPolling(void *vargp) 
{ 
	printf("Starting polling from sensors\n");
	char reply_command[500];
	char json_str[500];
	
	while (1){
		strcpy(json_str,"{");
		memset(reply_command,0,sizeof(char)*500);
		SerialCommand("READTEMP\n",reply_command);
		strcat(json_str,"\"Temperature\":\"");
		if(strlen(reply_command)>0){
			strcat(json_str,reply_command);			
		}
		else{
			strcat(json_str,"serialerror");			
		}
		strcat(json_str,"\"");			
        
		//printf("temperature %s\n",reply_command);
		strcat(json_str,"}");
		messagePublish("/iot/zolertia/data",json_str);
		client.yield(100);
		printf("publishing message %s\n",json_str);
    	sleep(5); 
	}     
    return NULL; 
} 


int main(int argc, char* argv[])
{

    pthread_t thread_id;
	float version = 0.3;

	printf("Version is %f\n", version);

	const char* hostname = "52.14.169.245";
	int port = 1883;
	printf("Connecting to %s:%d\n", hostname, port);
	int rc = ipstack.connect(hostname, port);
	if (rc != 0)
		printf("rc from TCP connect is %d\n", rc);

	printf("MQTT connecting\n");
	MQTTPacket_connectData data = MQTTPacket_connectData_initializer;       
	data.MQTTVersion = 3;
	data.clientID.cstring = (char*)"mbed-icraggs";
	rc = client.connect(data);
	if (rc != 0)
		printf("rc from MQTT connect is %d\n", rc);
	printf("MQTT connected\n");

	rc = client.subscribe("/iot/sensor/cmd", MQTT::QOS1, sensorMessageArrived);   
	if (rc != 0)
		printf("rc from MQTT subscribe is %d\n", rc);
	rc = client.subscribe("/iot/zolertia/cmd", MQTT::QOS1, zolertiaMessageArrived);   
	if (rc != 0)
		printf("rc from MQTT subscribe is %d\n", rc);

    pthread_create(&thread_id, NULL, SensorPolling, NULL); 
	while(1){
		client.yield(100);
	}

	rc = client.unsubscribe("/iot/zolertia/cmd");
	if (rc != 0)
		printf("rc from unsubscribe was %d\n", rc);
	rc = client.unsubscribe("/iot/sensor/cmd");
	if (rc != 0)
		printf("rc from unsubscribe was %d\n", rc);

	rc = client.disconnect();
	if (rc != 0)
		printf("rc from disconnect was %d\n", rc);

	ipstack.disconnect();

	printf("Finishing with %d messages received\n", arrivedcount);

	return 0;
}

