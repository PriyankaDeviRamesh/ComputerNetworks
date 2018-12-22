/*--------------------------------------------------------------------*/
/* Client program */

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <cstring>
#include <fstream>
#include <sstream>
#include "routingUtils.hpp"
#include <vector>
#define MAX_SIZE 1024

using namespace std;

Client* parse_file(char * filename) {

	int control_port, data_port, node_ID, input_neighbor;
	vector<int> neighbor_ids;
	string hostname, line;

	Client * client = new Client();

	ifstream input_file(filename);
	if (!input_file.good()) {
		perror("Error in opening the file");
		exit(1);
	} else {

	while(getline(input_file, line)) {
		istringstream ss(line);	
		ss >> node_ID >> hostname >> control_port;
		Neighbor_client neighbor;
		neighbor.id = node_ID;
		char * temp_char = new char[hostname.length()+1];
		strcpy(temp_char, hostname.c_str());
		neighbor.hostname = temp_char;
		neighbor.control_port = control_port;
		client->neighbors.push_back(neighbor);	
	}
	input_file.close();
      		}
 	return client;
}

/*--------------------------------------------------------------------*/
int InitiateRouter(int control_port, char * name){
	int sd;
	struct sockaddr_in router_addr;
	ushort port = (ushort)control_port;

	sd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sd == -1){
		perror("socket");
		exit(1);
	}

	struct hostent* host_entry;
	host_entry = gethostbyname(name);

	if(!host_entry){
		perror("Error in gethostbyname");
		exit(1);
	}

	char ip[1024];
	struct in_addr ** addr_list;
	addr_list = (struct in_addr **) host_entry->h_addr_list;
	strcpy(ip, inet_ntoa(*addr_list[0]));

	memset(&router_addr, 0, sizeof(struct sockaddr_in));
	router_addr.sin_family = AF_INET;
	router_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	if(control_port == -2){
		router_addr.sin_port = htons(0);
	}	
	else router_addr.sin_port = htons(port);

	if( bind(sd, (struct sockaddr*) &router_addr, sizeof(router_addr)) == -1){
		perror("Error in bind");
		exit(1);
	}
	return sd;
}

void notifyLinks(int source_node, int destination_node, int sock, Client* client, int identifier) {
	char link_message[MAX_SIZE];
	int found = 0, destination_found = 0, remove_link_set = 0;
	int j =0;
	if (identifier == 2) { 
		memset(link_message, -2, MAX_SIZE);
	}
	else if (identifier == 3) { 
		memset(link_message, -3, MAX_SIZE);
	}
	else { 
		memset(link_message, -4, MAX_SIZE);
		remove_link_set = 1;
	}

	if(remove_link_set == 0){
		HeaderControl header;
		header.source_node_id = source_node;
		header.dest_node_id = destination_node;
		memcpy(link_message, &header, sizeof(HeaderControl));
	
		struct sockaddr_in node_addr;
		memset((char*) &node_addr, 0, sizeof(node_addr));
		struct hostent * host_entry;

		node_addr.sin_family = AF_INET;
		j=0;
		while(j < client->neighbors.size()) {
			if(client->neighbors[j].id == source_node) {
				node_addr.sin_port = htons(client->neighbors[j].control_port);	
				host_entry = gethostbyname(client->neighbors[j].hostname);
				if(!host_entry){
					perror("Error in (gethostbyname: new link)");
					exit(1);
				}
				found = 1;
				break;
			}
			j = j+1;
		}
	
		if(found == 1){
			found = 0;
			 j=0;
			while(j < client->neighbors.size()){
				if(destination_node == client->neighbors[j].id){
					found = 1;
					destination_found = 1;
					break;
				}
				j=j+1;
			}
			if(found == 1){
				memcpy(&node_addr.sin_addr, host_entry->h_addr, host_entry->h_length);
				if(sendto(sock, link_message, (sizeof(HeaderControl)+sizeof(PayloadControl)), 0, (struct sockaddr *) &node_addr, sizeof(node_addr)) == -1){
					perror("Error in sendto");
					exit(1);
				}
			}
		}
		if(found == 0 || destination_found == 0) cout << "Node id is not recognized" << endl;
	}
	else{
		HeaderControl header;
		int removed_found = 0;
		header.source_node_id = source_node;
		header.dest_node_id = destination_node;
		j=0;
		while(j < client->neighbors.size()){
			if(client->neighbors[j].id == destination_node || client->neighbors[j].id == source_node){
				removed_found = 1;
			}
		    j=j+1;
		}
		if(removed_found == 0){
			cout << "Node id is not recognized" << endl;
			return;
		}

		memcpy(link_message, &header, sizeof(HeaderControl));

		struct sockaddr_in node_addr;
		struct hostent * host_entry;
		j=0;
		while(j < client->neighbors.size()) {
			memset((char*) &node_addr, 0, sizeof(node_addr));
			node_addr.sin_family = AF_INET;
			node_addr.sin_port = htons(client->neighbors[j].control_port);
			host_entry = gethostbyname(client->neighbors[j].hostname);
			if(!host_entry){
				perror("Error in (gethostbyname: remove link)");
				exit(1);
			}
			memcpy(&node_addr.sin_addr, host_entry->h_addr, host_entry->h_length);
			if(sendto(sock, link_message, (sizeof(HeaderControl)+sizeof(PayloadControl)), 0, (struct sockaddr *) &node_addr, sizeof(node_addr)) == -1){
				perror("Error in sendto");
				exit(1);
			}
		}
	}

}

int main(int argc, char * argv[]){

	if (argc == 2) {
		
		int argument_flag = 0;
	
		Client * client = parse_file(argv[1]);	

		char client_host[MAX_SIZE];
		if (gethostname(client_host, sizeof(client_host)) == -1) {
			perror("Error in gethostname");
			exit(1);
		}
		client->hostname = client_host;
		int client_sock = InitiateRouter(-2, client->hostname);

		cout << "Correct Format: <command> <node_1> <node_2>" << endl;
		cout << "<command>: 'route-trace', 'create-link', 'remove-link'" << endl;

		while (1) {

			cout << ">> ";

			char * command = NULL;
			char * line = NULL;
			char * user_input[3];
			int node_1, node_2, i = 0;
			size_t len = 0;

			getline(&line, &len, stdin);
			command = strtok(line, " ");

			while (line != NULL) {
				if (i < 3) {
					user_input[i] = line;
					line = strtok(NULL, " ");
					i++;
				}
				else {
					argument_flag = 1;
					break;
				}
			}

			if (i < 3 || argument_flag == 1) {
				argument_flag = 0;
				cout << "Correct Format: <command> <node_1> <node_2>" << endl;
				cout << "<command>: 'route-trace', 'create-link', 'remove-link'" << endl;
				continue;
			}

			command = user_input[0];
			node_1 = atoi(user_input[1]);
			node_2 = atoi(user_input[2]);

			if (strcmp(command, "route-trace") == 0) {
				notifyLinks(node_1, node_2, client_sock, client, 2);
			}
			else if (strcmp(command, "create-link") == 0) {
				notifyLinks(node_1, node_2, client_sock, client, 3);
				notifyLinks(node_2, node_1, client_sock, client, 3);
			}
			else if (strcmp(command, "remove-link") == 0) {
				notifyLinks(node_1, node_2, client_sock, client, 4);
				notifyLinks(node_2, node_1, client_sock, client, 4);
			}
			else {
				cout << "Correct Format: <command> <node_1> <node_2>" << endl;
				cout << "<command>: 'route-trace', 'create-link', 'remove-link'" << endl;
				continue;
			}
		}
	}
	else {
		cout << "Enter the correct format: ./client <input_file>" << endl;
		exit(1);
	}

	return 0;
}
