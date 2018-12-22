/*--------------------------------------------------------------------*/
/* Distance vector routing Program */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <string.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <sys/types.h>
#include <strings.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "routingUtils.hpp"

#define MAXSIZE 1024

using namespace std;

/*----------------------------------------------------------------------*/

int InitiateRouter(int control_port, char * name){
	int sd;
	struct sockaddr_in router_addr;
	ushort port = (ushort)control_port;

	sd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sd == -1){
		perror("Error in socket");
		exit(1);
	}

	struct hostent* host_entry;
	host_entry = gethostbyname(name);

	if(!host_entry){
		perror(" Error in gethostbyname");
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


/*-------------------------- CONTROL THREAD --------------------------*/

void deletePath(int destination_node, Node * node) {
	int i = 0;
	while( i < node->routing_table.size()){
		if(node->routing_table[i].destination == destination_node){
			(node->routing_table).erase(node->routing_table.begin()+i);
			break;
		}
		i = i + 1;
	}
}

char * createRemoveBuffer(int destination_node, Node * node, Neighbor n) {
	int i = 0;
	char * c =  (char*) malloc(sizeof(HeaderControl) + sizeof(PayloadControl));

	HeaderControl header;
	PayloadControl payload;
	memset(payload.reachable_nodes, -5, sizeof(payload.reachable_nodes));
	memset(payload.costs, -5, sizeof(payload.costs));

	header.source_node_id = destination_node;
	header.dest_node_id = n.node_id;

	memcpy(c, &header, sizeof(HeaderControl));
	memcpy(c+(sizeof(HeaderControl)), &payload, sizeof(PayloadControl));

	return c;
}


void notifyDelete(int destination_node, Node * node, int node_sock) {
	struct hostent * host_entry;
	struct sockaddr_in router_addr;
	socklen_t router_size = sizeof(router_addr);
        int j = 0;
	for(Neighbor n : node->neighbors){
		char * routing_buffer = createRemoveBuffer(destination_node, node, n);
		memset((char*) &router_addr, 0, router_size);
		router_addr.sin_family = AF_INET;
		router_addr.sin_port = htons(n.control_port);
		host_entry = gethostbyname(n.host_name);
		if(!host_entry){
			perror("Error in (gethostbyname: Send) ");
			exit(1);
		}
		memcpy(&router_addr.sin_addr, host_entry->h_addr, host_entry->h_length);

		if(sendto(node_sock, routing_buffer, (sizeof(HeaderControl)+sizeof(PayloadControl)), 0, (struct sockaddr *) &router_addr, router_size) == -1){
			perror("Error in sendto()");
			exit(1);
		}
	}
	cout<< "\n------ UPDATED ROUTING TABLE AFTER DELETION -----\n" << endl;
	cout<< "(DESTINATION, NEXT HOP, DISTANCE)\n" << endl;
	j=0;
	while(j < node->routing_table.size()){
		cout<< "(" << node->routing_table[j].destination << "," << node->routing_table[j].next_hop << "," << node->routing_table[j].distance << ")" << endl;
		j = j+1 ;
	}
	cout<< "\n";
}

void deleteLink(int source_node, int destination_node, Node * node, int node_sock) {
	if(node->id != source_node && node->id != destination_node){
		int destination_index = -1;
		int source_index = -1;
		int i = 0, j = 0;
		while(i < node->neighbors.size()){
			if(node->neighbors[i].node_id == source_node){
				source_index = i;
			}
			if(node->neighbors[i].node_id == destination_node){
				destination_index = i;
			}
		 	i = i+1;
		}
		if(source_index == -1){
			j = 0;
			while(j < node->routing_table.size()) {
				if (node->routing_table[j].destination == source_node) {
					(node->routing_table).erase(node->routing_table.begin()+j);
					j=0;
				}
				if (node->routing_table[j].next_hop == source_node || node->routing_table[j].next_hop == destination_node) {
					(node->routing_table).erase(node->routing_table.begin()+j);
					j=0;
				}
				j = j+1;
			}
		}
		if(destination_index == -1){
			j = 0;
			while(j < node->routing_table.size()) {
				if (node->routing_table[j].destination == destination_node) {
					(node->routing_table).erase(node->routing_table.begin()+j);
					j=0;
				}
				if (node->routing_table[j].next_hop == destination_node || node->routing_table[j].next_hop == source_node) {
					(node->routing_table).erase(node->routing_table.begin()+j);
					j=0;
				}
				j = j+1;
			}
		}


	}

	else if (node->id == source_node) {
         	int i = 0;
		while(i < node->neighbors.size()) {
			if (node->neighbors[i].node_id == destination_node) {
				(node->neighbors).erase(node->neighbors.begin()+i);
				i=0;
			}
			i = i+1;
		}

		int j = 0;
		while(j < node->routing_table.size()) {
			if (node->routing_table[j].destination == destination_node) {
				(node->routing_table).erase(node->routing_table.begin()+j);
				j=0;
			}
			if (node->routing_table[j].next_hop == destination_node) {
				(node->routing_table).erase(node->routing_table.begin()+j);
				j=0;
			}
			j = j+1;
		}

	}
	cout<< "\n-------- UPDATED ROUTING TABLE AFTER DELETION ------\n";
	cout<< "(DESTINATION, NEXT HOP, DISTANCE)\n";
       int j = 0;
	while(j < node->routing_table.size()){
		cout<< "(" << node->routing_table[j].destination << "," << node->routing_table[j].next_hop << "," << node->routing_table[j].distance << ")" << endl;
		j = j + 1;
	}
	cout<< "\n";

}


void linkRoute(vector<Route> &routing_table, Route new_route, char source_id) {

	int i = 0;

	while(i<routing_table.size()) {
		if(routing_table[i].destination == new_route.destination){
			if(new_route.distance + 1 < routing_table.at(i).distance){
				break;
			}
			else if(source_id == routing_table.at(i).next_hop) {
				break;
    		}
    			else {
      				return;
    			}
		}
		i = i+1;
	}
	if (i == routing_table.size()){
		Route r;
		routing_table.push_back(r);
	}

	routing_table.at(i) = new_route;
	routing_table.at(i).next_hop = source_id;
	++routing_table.at(i).distance;

}

char * createControlBuffer(Node * node, Neighbor n) {
	int i = 0;
	char * c =  (char*) malloc(sizeof(HeaderControl) + sizeof(PayloadControl));

	HeaderControl header;
	PayloadControl payload;
	memset(payload.reachable_nodes, -1, sizeof(payload.reachable_nodes));
	memset(payload.costs, -1, sizeof(payload.costs));

	header.source_node_id = node->id;
	header.dest_node_id = n.node_id;

	while(i < node->routing_table.size()) {
		payload.reachable_nodes[i] = node->routing_table[i].destination;
		payload.costs[i] = node->routing_table[i].distance;
		i = i+1;
	}

	memcpy(c, &header, sizeof(HeaderControl));
	memcpy(c+(sizeof(HeaderControl)), &payload, sizeof(PayloadControl));

	return c;
}

void updateNeighbours(Node * node, int node_sock){
	struct sockaddr_in router_addr;
	struct hostent * host_entry;
	socklen_t router_size = sizeof(router_addr);

	for(Neighbor n : node->neighbors){
		char * routing_buffer = createControlBuffer(node, n);
		memset((char*) &router_addr, 0, router_size);
		router_addr.sin_family = AF_INET;
		router_addr.sin_port = htons(n.control_port);
		host_entry = gethostbyname(n.host_name);
		if(!host_entry){
			perror("Error in (gethostbyname: send) ");
			exit(1);
		}
		memcpy(&router_addr.sin_addr, host_entry->h_addr, host_entry->h_length);

		if(sendto(node_sock, routing_buffer, (sizeof(HeaderControl)+sizeof(PayloadControl)), 0, (struct sockaddr *) &router_addr, router_size) == -1){
			perror("Error in sendto");
			exit(1);
		}
	}
}

void createLink(int source_node, int destination_node, Node * node, int node_sock) {

	if (source_node != destination_node){

		Route r;
		r.destination = destination_node;
		r.next_hop = destination_node;
		r.distance = 1;

		cout<< "\n--------- PROPOSED ROUTE FROM CREATE-LINK --------\n";
		cout<< "[ DESTINATION : NEXT HOP : DISTANCE ]\n" << endl;
   		printf("(%d,%d,%d)\n", r.destination, r.next_hop, r.distance);
    		linkRoute(node->routing_table, r, destination_node);

		ifstream input_file(filename);
		if (!input_file.good()) {
			perror("Error in opening the file");
			exit(1);
		}

		Neighbor neighbor;
		int control_port, data_port, node_ID;
		string hostname, line;

		while(getline(input_file, line)) {
			istringstream ss(line);
			ss >> node_ID;
			if (node_ID == destination_node) {
				ss >> hostname >> control_port >> data_port;
				neighbor.node_id = node_ID;
				char * temp_char = new char[hostname.length()+1];
				strcpy(temp_char, hostname.c_str());
				neighbor.host_name = temp_char;
				neighbor.control_port = control_port;
				neighbor.data_port = data_port;
				node->neighbors.push_back(neighbor);
				updateNeighbours(node, node_sock);
				break;
			}
		}
		input_file.close();

	}
}

void initialDistanceVector(Node * node){
	Route r;
	r.destination = node->id;
	r.next_hop = -1;
	r.distance = 0;
	node->routing_table.push_back(r);
	for(Neighbor n : node->neighbors){
		r.destination = n.node_id;
		r.next_hop = n.node_id;
		r.distance = 1;
		node->routing_table.push_back(r);
	}
}

void parseControlBuffer(char * routing_buffer, Node* node) {
	int next_hop, control_port;
	int k = 0;
	HeaderControl header;
	PayloadControl payload;

	memcpy(&header, routing_buffer, sizeof(HeaderControl));
	memcpy(&payload, (routing_buffer+sizeof(HeaderControl)), sizeof(PayloadControl));

	memset(CLIENT_ARRAY, -1, sizeof(CLIENT_ARRAY));
	CLIENT_ARRAY[0] = header.source_node_id;
	CLIENT_ARRAY[1] = header.dest_node_id;

	char comparison = payload.reachable_nodes[0];
	while(k<sizeof(payload.reachable_nodes)) {
		if (payload.reachable_nodes[k] != comparison) {
			comparison = 0;
			break;
		}
		k = k+1;
	}

	if (comparison == -2) {
		MESSAGE_RECEIVED = 2;
	}
	else if (comparison == -3) {
		MESSAGE_RECEIVED = 3;
	}
	else if (comparison == -4) {
		MESSAGE_RECEIVED = 4;
	}
	else if (comparison == -5) {
		MESSAGE_RECEIVED = 5;
	}
	else {
		Route r;
		r.next_hop = -2;
		MESSAGE_RECEIVED = 0;
		int i = 0;
		while(i<(sizeof(payload.reachable_nodes))) {
			if (payload.reachable_nodes[i] != -1) {
				r.destination = payload.reachable_nodes[i];
				r.distance = payload.costs[i];
				pthread_mutex_lock(&mutex_lock);
				linkRoute(node->routing_table, r, header.source_node_id);
				pthread_mutex_unlock(&mutex_lock);
			}
			else {
				cout<< "\n------------ UPDATED ROUTING TABLE ------------\n";
				cout<<"[ DESTINATION : NEXT HOP : DISTANCE ]\n";
				for(int j=0; j < node->routing_table.size(); j++){
					cout<< "[" << node->routing_table[j].destination << ":" << node->routing_table[j].next_hop << ":" << node->routing_table[j].distance << "]" << endl;
				}
				cout<<"\n";
				break;
			}
		}
	}
}

static void* control(void * n) {
	Node * node = (Node *) n;

	fd_set rfds;
	FD_ZERO(&rfds);
	int node_sock = InitiateRouter(node->control_port, node->hostname);
	int livesdmax = node_sock+1;
	FD_SET(node_sock, &rfds);

	initialDistanceVector(node);
	struct sockaddr_in router_addr;
	socklen_t router_size = sizeof(router_addr);
	struct timeval tv;
	time_t start, end;
	tv.tv_sec = 0;
	tv.tv_usec = 10000;

	char message[MAXSIZE];

	start = time(0);

	while(1) {
		memset((char*) &router_addr, -1, router_size);
		memset(message, -1, MAXSIZE);

		if(select(livesdmax, &rfds, NULL, NULL, &tv) == -1){
			perror("Error in select");
			exit(1);
		}
		if(FD_ISSET(node_sock, &rfds)){
			if(recvfrom(node_sock, message, MAXSIZE, 0, (struct sockaddr *) &router_addr, &router_size) == -1){
				perror("Error in recvfrom");
				exit(1);
			}
			parseControlBuffer(message, node);
		}
		end = time(0);
		if(difftime(end, start)*1000 > 5){
			start = end;
			updateNeighbours(node, node_sock);
		}

		if (MESSAGE_RECEIVED == 3) {
			pthread_mutex_lock(&mutex_lock);
			MESSAGE_RECEIVED = 0;
			createLink(CLIENT_ARRAY[0], CLIENT_ARRAY[1], node, node_sock);
			memset(CLIENT_ARRAY, -1, sizeof(CLIENT_ARRAY));
			pthread_mutex_unlock(&mutex_lock);
		}
		else if (MESSAGE_RECEIVED == 4) {
			pthread_mutex_lock(&mutex_lock);
			MESSAGE_RECEIVED = 0;
			deleteLink(CLIENT_ARRAY[0], CLIENT_ARRAY[1], node, node_sock);
			memset(CLIENT_ARRAY, -1, sizeof(CLIENT_ARRAY));
			pthread_mutex_unlock(&mutex_lock);
		}
		else if (MESSAGE_RECEIVED == 5) {
			MESSAGE_RECEIVED = 0;
			deletePath(CLIENT_ARRAY[0], node);
			memset(CLIENT_ARRAY, -1, sizeof(CLIENT_ARRAY));
		}

		FD_ZERO(&rfds);
		FD_SET(node_sock, &rfds);
	}
}

/*---------------------------- DATA THREAD ---------------------------*/

void dataPacketForwarding(char * data_packet, Node * node, int node_sock) {
	int j, k, found = 0;
	struct sockaddr_in router_addr;
	struct hostent* host_entry;
	memset((char*) &router_addr, 0, sizeof(router_addr));

	HeaderData header;
	PayloadData payload;
	memcpy(&header, data_packet, sizeof(HeaderData));
	memcpy(&payload, (data_packet+sizeof(HeaderData)), sizeof(PayloadData));

	for(j = 0; j < node->neighbors.size(); j++){
		if(node->neighbors[j].node_id == header.dest_node_id){
			found = 1;
			break;
		}
	}

	if (found == 1) {
		router_addr.sin_family = AF_INET;
		router_addr.sin_port = htons(node->neighbors[j].data_port);
		host_entry = gethostbyname(node->neighbors[j].host_name);
		if(!host_entry){
			perror("Error in (gethostbyname: send)");
			exit(1);
		}
		memcpy(&router_addr.sin_addr, host_entry->h_addr, host_entry->h_length);
		cout<< "---------------- FORWARDED PACKET --------------\n";
		cout<<"HEADER" << endl;
		cout<<"source_nodeID:" << header.source_node_id << "dest_nodeID:" << header.dest_node_id << "packet_id:" << header.packet_id << "ttl:" << header.ttl << endl;
		if(sendto(node_sock, data_packet, (sizeof(HeaderData)+sizeof(PayloadData)), 0, (struct sockaddr *) &router_addr, sizeof(router_addr)) == -1){
			perror("Error in sendto");
			exit(1);
		}
	}
	else { // LOOKING FOR NEXT HOP TO GET TO DESTINATION NODE
		int next_hop_found = 0;
		for (int i = 0; i < node->routing_table.size(); i++) {
			if (header.dest_node_id == node->routing_table[i].destination) {
				for (k = 0; k < node->neighbors.size(); k++) {
					if (node->neighbors[k].node_id == node->routing_table[i].next_hop) {
						next_hop_found = 1;
						break;
					}
				}
				if (next_hop_found == 1) break;
			}
		}
		if (next_hop_found == 1) {
			router_addr.sin_family = AF_INET;
			router_addr.sin_port = htons(node->neighbors[k].data_port);
			host_entry = gethostbyname(node->neighbors[k].host_name);
			if(!host_entry){
				perror("Error in (gethostbyname: send)");
				exit(1);
			}
			memcpy(&router_addr.sin_addr, host_entry->h_addr, host_entry->h_length);
			cout<< "------------------ FORWARDED PACKET ----------------\n";
			cout<<"HEADER" << endl;
			cout<<"source_nodeID:" << header.source_node_id << "dest_nodeID:" << header.dest_node_id << "packet_id:" << header.packet_id << "ttl:" << header.ttl << endl;
			if(sendto(node_sock, data_packet, (sizeof(HeaderData)+sizeof(PayloadData)), 0, (struct sockaddr *) &router_addr, sizeof(router_addr)) == -1){
				perror("Error in sendto");
				exit(1);
			}
		}
	}
}

void routeTrace(int source_node, int destination_node, Node * node, int node_sock) {

	HeaderData header;
	PayloadData payload;

	char * data_packet =  (char*) malloc(sizeof(HeaderData) + sizeof(PayloadData));
	header.source_node_id = source_node;
	header.dest_node_id = destination_node;

	if (packet < 255) {
		header.packet_id = ++packet;
	}
	else {
		packet = 0;
		header.packet_id = packet;
	}

	header.ttl = 15;

	memset(payload.node_path, -1, sizeof(payload.node_path));
	payload.node_path[0] = source_node;

	memcpy(data_packet, &header, sizeof(HeaderData));
	memcpy(data_packet+(sizeof(HeaderData)), &payload, sizeof(PayloadData));

	cout<< "-------------- ROUTE TRACE -----------------\n";
	cout<<"HEADER" << endl;
	cout<<"source_nodeID:" << header.source_node_id << "dest_nodeID:" << header.dest_node_id << "packet_id:" << header.packet_id << "ttl:" << header.ttl << endl;
	dataPacketForwarding(data_packet, node, node_sock);
}

void dataBufferParsing(char * data_packet, Node* node, int node_sock) {

	int next_hop, control_port;
	int appended = 0;
	HeaderData header;
	PayloadData payload;
    int i = 0;
	memcpy(&header, data_packet, sizeof(HeaderData));
	memcpy(&payload, (data_packet+sizeof(HeaderData)), sizeof(PayloadData));

	if (header.ttl > 0) {
		cout<<"------------- RECEIVED PACKET -------------\n";
		cout<<"HEADER" << endl;
	    cout<<"source_nodeID:" << header.source_node_id << "dest_nodeID:" << header.dest_node_id << "packet_id:" << header.packet_id << "ttl:" << header.ttl << endl;
		if (header.dest_node_id == node->id) {
			cout<< "NODE PATH:" << endl;
		     while(i<(sizeof(payload.node_path))) {
				if (payload.node_path[i] != -1) {
					printf(" %d", payload.node_path[i]);
				}
				i = i+1;
			}
			cout<< "\n";
		}

		for (int i=0; i<(sizeof(payload.node_path)); i++) {
			if (payload.node_path[i] == -1) {
				payload.node_path[i] = node->id;
				header.ttl = header.ttl - 1;
				appended = 1;
				break;
			}
		}
		if (appended == 1){  //successfully appended to node path
			if (header.dest_node_id == node->id) {
				return;
			}
			else {
				char * updated_packet = (char*) malloc(sizeof(HeaderData) + sizeof(PayloadData));
				memcpy(updated_packet, &header, sizeof(HeaderData));
				memcpy(updated_packet+(sizeof(HeaderData)), &payload, sizeof(PayloadData));

				pthread_mutex_lock(&mutex_lock);
				dataPacketForwarding(updated_packet, node, node_sock);
				pthread_mutex_unlock(&mutex_lock);
			}
		}
	}
	else {
		cout<< "------------------ DROPPED PACKET ------------------\n";
		cout<<"HEADER" << endl;
		cout<<"source_nodeID:" << header.source_node_id << "dest_nodeID:" << header.dest_node_id << "packet_id:" << header.packet_id << "ttl:" << header.ttl << endl;
		return;
	}
}

static void* data(void * n) {
	Node * node = (Node *) n;

	fd_set rfds;
	FD_ZERO(&rfds);
	int node_sock = InitiateRouter(node->data_port, node->hostname);
	int livesdmax = node_sock+1;
	FD_SET(node_sock, &rfds);

	struct sockaddr_in router_addr;
	socklen_t router_size = sizeof(router_addr);
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 10000;

	char message[MAXSIZE];

	while(1) {

		memset((char*) &router_addr, -1, router_size);
		memset(message, -1, MAXSIZE);

		if(select(livesdmax, &rfds, NULL, NULL, &tv) == -1){
			perror("Error in select()");
			exit(1);
		}
		if(FD_ISSET(node_sock, &rfds)){
			if(recvfrom(node_sock, message, MAXSIZE, 0, (struct sockaddr *) &router_addr, &router_size) == -1){
				perror("Error in recvfrom()");
				exit(1);
			}
			dataBufferParsing(message, node, node_sock);
		}

		pthread_mutex_lock(&mutex_lock);
		if (MESSAGE_RECEIVED == 2) {
			MESSAGE_RECEIVED = 0;
			routeTrace(CLIENT_ARRAY[0], CLIENT_ARRAY[1], node, node_sock);
			memset(CLIENT_ARRAY, -1, sizeof(CLIENT_ARRAY));
		}
		pthread_mutex_unlock(&mutex_lock);

		FD_ZERO(&rfds);
		FD_SET(node_sock, &rfds);
	}
}

/*--------------------------------------------------------------------*/

Node* parse_file(char * filename, int current_ID) {

	int control_port, data_port, node_ID, neighbor;
	string hostname, line;
	vector<int> neighbor_ids;

	Node * node = NULL;

	ifstream input_file(filename);
	if (!input_file.good()) {
		perror("Error in opening the file");
		exit(1);
	}

	while(getline(input_file, line)) {
		istringstream ss(line);
		ss >> node_ID >> hostname >> control_port >> data_port;
		if(node_ID == current_ID){
			while (ss >> neighbor) {
				neighbor_ids.push_back(neighbor);
			}
			node = new Node();
			node->id = node_ID;
			char * temp_char = new char[hostname.length()+1];
			strcpy(temp_char, hostname.c_str());
			node->hostname = temp_char;
			node->control_port = control_port;
			node->data_port = data_port;
		}
	}
	input_file.close();

	if(neighbor_ids.size() > 0){
		ifstream file_second(filename);
		if (!file_second.good()) {
			perror("Error in opening the file for second time");
			exit(1);
		}
		while(getline(file_second, line)) {
			istringstream ss(line);
			ss >> node_ID;
			for(int i : neighbor_ids){
				if(node_ID == i){
					ss >> hostname >> control_port >> data_port;
					Neighbor n1;
					n1.node_id = i;
					n1.control_port = control_port;
					n1.data_port = data_port;
					char * temp_char = new char[hostname.length()+1];
					strcpy(temp_char, hostname.c_str());
					n1.host_name = temp_char;
					node->neighbors.push_back(n1);
				}
			}
		}
		file_second.close();
	}
 	return node;
}

int main(int argc, char * argv[]){
	MESSAGE_RECEIVED = 0;
	packet = 0;
	pthread_mutex_init(&mutex_lock, NULL);

	if (argc == 3) {

		int node_ID = atoi(argv[2]);
		Node * node = parse_file(argv[1], node_ID);
		filename = argv[1];

		printf("%d ", node->id);
		cout << node->hostname << " " << node->control_port << " " << node->data_port << " " << endl;
		for(Neighbor n : node->neighbors){
			printf("(%d ", n.node_id); cout << " " << n.host_name << " " << n.control_port << " " << n.data_port << ")"  << endl;
		}


		pthread_t control_thread; // Control Thread
		if (pthread_create(&control_thread, NULL, control, (void*) node)) {
			perror("Error in pthread_create");
			exit(1);
		}


		pthread_t data_thread; // Data Thread
		if (pthread_create(&data_thread, NULL, data, (void*) node)) {
			perror("Error in pthread_create");
			exit(1);
		}

		pthread_join(control_thread, NULL);
		pthread_join(data_thread, NULL);

	}
	else {
		cout << "Enter the correct format : ./route <input_file> <node_ID>" << endl;
		exit(1);
	}
	return 0;
}
