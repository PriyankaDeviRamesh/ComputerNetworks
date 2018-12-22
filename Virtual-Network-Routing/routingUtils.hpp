/*--------------------------------------------------------------------*/
/* Utilities */

#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <vector>

using namespace std;

int packet;
char MESSAGE_RECEIVED;
int CLIENT_ARRAY[2];
char * filename;
pthread_mutex_t mutex_lock;

typedef struct {
        char id;
        char * hostname;
        int control_port;
} Neighbor_client;

typedef struct {
        char * hostname;
        int control_port;
        vector<Neighbor_client> neighbors;
} Client;

typedef struct {
        char destination;
        char next_hop;
        char distance;
} Route; 

typedef struct{
        char node_id;
        char * host_name;
        int data_port;
        int control_port;
} Neighbor;

typedef struct {
        char id;
        int control_port;
        int data_port;
        char * hostname;
        vector<Route> routing_table;
        vector<Neighbor> neighbors;
} Node;


typedef struct {
	char source_node_id;
	char dest_node_id;
	char packet_id;
	char ttl;
} HeaderData;

typedef struct {
	char node_path[40];
} PayloadData;

typedef struct {
	char source_node_id;
	char dest_node_id;
} HeaderControl;

typedef struct{
	char reachable_nodes[40];
	char costs[40];
} PayloadControl;

int InitiateRouter(int control_port, char * name);
Client* parse_file(char * filename);
void notifyLinks(int source_node, int destination_node, int sock, Client* client, int identifier);
void deletePath(int destination_node, Node * node);
char * createRemoveBuffer(int destination_node, Node * node, Neighbor n);
void notifyDelete(int destination_node, Node * node, int node_sock) ;
void deleteLink(int source_node, int destination_node, Node * node, int node_sock);
void linkRoute(vector<Route> &routing_table, Route new_route, char source_id);
char * createControlBuffer(Node * node, Neighbor n);
void updateNeighbours(Node * node, int node_sock);
void createLink(int source_node, int destination_node, Node * node, int node_sock);
void initialDistanceVector(Node * node);
void parseControlBuffer(char * routing_buffer, Node* node);
void dataPacketForwarding(char * data_packet, Node * node, int node_sock);
void routeTrace(int source_node, int destination_node, Node * node, int node_sock);
void dataBufferParsing(char * data_packet, Node* node, int node_sock);


