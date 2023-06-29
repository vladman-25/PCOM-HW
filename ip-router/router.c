#include "queue.h"
#include "skel.h"
#include <netinet/if_ether.h>
#include <netinet/ip_icmp.h>

struct icmphdr* get_icmp(struct iphdr *ipv4_header) {
	if(ipv4_header->protocol == 1) {
		return (struct icmphdr*)(ipv4_header + sizeof(struct iphdr));
	} else {
		return NULL;
	}

} 
void icmp_send(struct ether_header* ether_header, struct iphdr* ipv4_header, int type) {
	packet newP;

	struct ether_header eth_to_send;
	memcpy(eth_to_send.ether_dhost, ether_header->ether_shost, ETH_ALEN);
	memcpy(eth_to_send.ether_shost, ether_header->ether_dhost, ETH_ALEN);
	eth_to_send.ether_type = htons(ETHERTYPE_IP);

	struct iphdr ipv4_to_send;
	ipv4_to_send.version = 4;
	ipv4_to_send.ihl = 5;
	ipv4_to_send.tos = 0;
	ipv4_to_send.protocol = IPPROTO_ICMP;
	ipv4_to_send.tot_len = htons(sizeof(struct iphdr) + sizeof(struct icmphdr));
	ipv4_to_send.id = htons(1);
	ipv4_to_send.frag_off = 0;
	ipv4_to_send.ttl = 64;
	ipv4_to_send.check = 0;
	ipv4_to_send.daddr = ipv4_header->saddr;
	ipv4_to_send.saddr = ipv4_header->daddr;
	ipv4_to_send.check = ip_checksum(&ipv4_to_send, sizeof(struct iphdr));

	struct icmphdr icmp_to_send;
	icmp_to_send.type = type;
	icmp_to_send.code = 0;
	icmp_to_send.checksum = 0;
	icmp_to_send.checksum = icmp_checksum((uint16_t *)&icmp_to_send,sizeof(struct icmphdr));

	void* payload = newP.payload;
	memcpy(payload, &eth_to_send, sizeof(struct ether_header));
	payload += sizeof(struct ether_header);
	memcpy(payload, &ipv4_to_send, sizeof(struct iphdr));
	payload += sizeof(struct iphdr);
	memcpy(payload, &icmp_to_send, sizeof(struct icmphdr));
	payload += sizeof(struct icmphdr);
	
	newP.len = sizeof(struct ether_header) + sizeof(struct iphdr) + sizeof(struct icmphdr);

	send_packet(&newP);
}

////////////////////////////////////////////////////
struct route_table_entry *rtable;
struct arp_entry *arp_table;
int rtable_len;
int arp_table_len;

struct arp_entry *get_arp_entry(uint32_t dest_ip) {
    for (size_t i = 0; i < arp_table_len; i++) {
        if (memcmp(&dest_ip, &arp_table[i].ip, sizeof(struct in_addr)) == 0)
	    	return &arp_table[i];
    }
    return NULL;
}

int cmpfunc(const void *a, const void *b) {
	uint32_t pa = ntohl(((struct route_table_entry *)a)->prefix);
	uint32_t pb = ntohl(((struct route_table_entry *)b)->prefix);
	uint32_t ma = ntohl(((struct route_table_entry *)a)->mask);
	uint32_t mb = ntohl(((struct route_table_entry *)b)->mask);
	if(ma == mb) {
		return pa - pb;
	} else {
		return ma - mb;
	}
}

struct route_table_entry *get_best_route_BS(int l, int r, uint32_t dest_ip) {
    
	if (l > r) {
		struct route_table_entry *best = NULL;
		for (int i = -1; i < rtable_len; i++){
			if(ntohl(dest_ip & rtable[i].mask) == ntohl(rtable[i].prefix)) {
				if((best == NULL) || (ntohl(best->mask) < ntohl(rtable[i].mask))) {
					// printf("found_i:%d\n",i);
					best = &rtable[i];
				}
			}
		}
		return best;
	} else {
		int mid = l + (r - l) / 2;
		if ((dest_ip & rtable[mid].mask) == rtable[mid].prefix) {
			// printf("mid%d\n",mid);
			struct route_table_entry *best = NULL;
			for (int  i = mid; i < rtable_len; i++) { 
				if(ntohl(dest_ip & rtable[i].mask) == ntohl(rtable[i].prefix)) {
					if((best == NULL) || ((ntohl(best->mask) < ntohl(rtable[i].mask)))) {
						// printf("found_i:%d\n",i);
						best = &rtable[i];
					}
				}
			}
			return best;
		} else if (ntohl(dest_ip& rtable[mid].mask) < ntohl(rtable[mid].prefix)) {
			get_best_route_BS(l,mid-1,dest_ip);
		} else {
			get_best_route_BS(mid+1,r,dest_ip);
		}
	}
}
/////////////////////
void print_rtable() {
	for (int i = 0; i < rtable_len; i++) {
		printf("%u %u\n",rtable[i].prefix,rtable[i].mask);
	}
}
/////////////////////////
struct arp_proto {
    __u32 ip;
    uint8_t mac[6];
};

struct arp_proto *arp_cache;
int arp_cache_len;

struct arp_entry *get_arp_cache_entry(uint32_t dest_ip) {
    for (size_t i = 0; i < arp_cache_len; i++) {
        if (memcmp(&dest_ip, &arp_cache[i].ip, sizeof(struct in_addr)) == 0)
	    	return &arp_cache[i];
    }
    return NULL;
}



////////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[])
{
	setvbuf(stdout,NULL,_IONBF,0);
	packet m;
	int rc;

	// Do not modify this line
	init(argc - 2, argv + 2);
	rtable = malloc(sizeof(struct route_table_entry) * 100000);
	DIE(rtable == NULL, "memory");

//////////////
	// arp_table = malloc(sizeof(struct arp_entry) * 100);
	// DIE(rtable == NULL, "memory");
//////////////

	arp_cache = malloc(sizeof(struct arp_entry) * 100);
	arp_cache_len = 0;
	queue q = queue_create();


	rtable_len = read_rtable(argv[1],rtable);
	// print_rtable();

	qsort(rtable, rtable_len, sizeof(struct route_table_entry), cmpfunc);
	printf("START\n");
	// print_rtable();
	// arp_table_len = parse_arp_table("arp_table.txt",arp_table);

	while (1) {
		printf("//////////////////////////////////////////////////////////\n");
		rc = get_packet(&m);
		
		DIE(rc < 0, "get_packet");
		printf("//////////////////////////////////////////////////////////\n");
		printf("GETPACK\n");
		struct ether_header *ether_header = (struct ether_header*) m.payload;
		struct iphdr *ipv4_header = (struct iphdr*) (m.payload + sizeof(struct ether_header));
		struct arp_header *arp_header = (struct arp_header*) (m.payload + sizeof(struct ether_header));
		struct in_addr dest_ipv4; 
		if (ntohs(ether_header->ether_type) == ETHERTYPE_IP) {

			struct icmphdr* icmp_header = get_icmp(ipv4_header);
			if (ipv4_header->daddr == inet_addr) {
				// icmp_send(ether_header, ipv4_header, 0);
				printf("Router is destination\n");
				
				continue;		
			}

			if (ip_checksum((void *) ipv4_header, sizeof(struct iphdr)) != 0) {
				printf("CHECKSUM\n");
				continue;
			}

			if (ipv4_header->ttl <= 1) {
				// icmp_send(ether_header, ipv4_header, 11);
				printf("ttl\n");
				continue;
			}
			ipv4_header->ttl--;
			ipv4_header->check = 0;
			ipv4_header->check = ip_checksum((void *) ipv4_header, sizeof(struct iphdr));

			struct route_table_entry *route = get_best_route_BS(0,rtable_len,ipv4_header->daddr);
			if (route == NULL) {
				// icmp_send(ether_header, ipv4_header, 3);
				printf("noroute\n");
				continue;
			}
			// struct arp_entry *arp = get_arp_entry(route->next_hop);
			// if (arp == NULL) {
			// 	printf("noarp\n");
			// 	continue;
			// }
			
			//ARP PAS 1
			struct arp_proto *arp = get_arp_cache_entry(route->next_hop);
			if (arp == NULL) {
				printf("NO ARP\n");
				//ARP PAS 2
				packet* toQ = malloc(sizeof(packet));
				toQ->interface = route->interface;
				toQ->len = m.len;
				memcpy(toQ->payload,m.payload, sizeof(m.payload));

				ether_header->ether_type = htons(ETHERTYPE_ARP);
				memset(ether_header->ether_dhost,0xff,6);//
				queue_enq(q,toQ);


		

				// ARP PAS 3

				struct arp_header arp_header_reply;

				packet newP;
				arp_header_reply.htype = htons(1);
				arp_header_reply.ptype = htons(2048);
				arp_header_reply.hlen = 6;
				arp_header_reply.plen = 4;
				arp_header_reply.op = htons(ARPOP_REQUEST);
				memcpy(arp_header_reply.sha, ether_header->ether_shost, 6);
				memcpy(arp_header_reply.tha, ether_header->ether_dhost, 6);

				struct in_addr* send = malloc(sizeof(struct in_addr));
				inet_aton(get_interface_ip(m.interface),send);
				arp_header_reply.spa = inet_addr(get_interface_ip(route->interface));
				arp_header_reply.tpa = route->next_hop;

				newP.len = sizeof(struct arp_header) + sizeof(struct ether_arp);
				newP.interface = route->interface;

				memcpy(newP.payload, ether_header, sizeof(struct ether_header));
				memcpy(newP.payload + sizeof(struct ether_header), &arp_header_reply, sizeof(struct arp_header));
				send_packet(&newP);
				printf("ARP SEND\n");
				continue;
			} else {
				memcpy(ether_header->ether_dhost, arp->mac, 6);

				get_interface_mac(route->interface, ether_header->ether_shost);
				m.interface = route->interface;
				printf("sent\n");

				send_packet(&m);
			}
			
		}  
		if (ntohs(ether_header->ether_type) == ETHERTYPE_ARP) {
		
			if(arp_header->op == htons(ARPOP_REQUEST)) {
				printf("ARP recv REQ1\n");
				memcpy(ether_header->ether_dhost, ether_header->ether_shost, 6);
				get_interface_mac(m.interface, ether_header->ether_shost);

				struct arp_header arp_header_reply;
				packet newP;
				arp_header_reply.htype = htons(1);
				arp_header_reply.ptype = htons(2048);
				arp_header_reply.hlen = 6;
				arp_header_reply.plen = 4;
				arp_header_reply.op = htons(ARPOP_REPLY);
				memcpy(arp_header_reply.sha, ether_header->ether_shost, 6);
				memcpy(arp_header_reply.tha, ether_header->ether_dhost, 6);
				arp_header_reply.spa = arp_header->tpa;
				arp_header_reply.tpa = arp_header->spa;

				newP.len = sizeof(struct arp_header) + sizeof(struct ether_arp);
				newP.interface = m.interface;

				memcpy(newP.payload, ether_header, sizeof(struct ether_header));
				memcpy(newP.payload + sizeof(struct ether_header), &arp_header_reply, sizeof(struct arp_header));
				send_packet(&newP);
				printf("ARP SEND\n");

				continue;
			}

			if(arp_header->op == htons(ARPOP_REPLY)) {
				printf("ARP recv REQ2\n");
				arp_cache[arp_cache_len].ip = arp_header->spa;
				for(int i = 0; i < 6; i++) {
					arp_cache[arp_cache_len].mac[i] = arp_header->sha[i];
				}
				arp_cache_len++;

				queue aux = queue_create();

				while(!queue_empty(q)) {
					printf("ARP enter while Q\n");
					packet* newP = queue_deq(q);

					struct ether_header *ether_header_deq = (struct ether_header*) newP->payload;
					struct iphdr *ipv4_header_deq = (struct iphdr*) (newP->payload + sizeof(struct ether_header));

					struct route_table_entry *route = get_best_route_BS(0,rtable_len,ipv4_header_deq->daddr);
					struct arp_proto *arp = get_arp_cache_entry(route->next_hop);
					if (arp == NULL) {
						queue_enq(aux,newP);
						continue;
					}

					memcpy(ether_header_deq->ether_dhost, arp->mac, 6);
					get_interface_mac(route->interface, ether_header_deq->ether_shost);
					newP->interface = route->interface;

					send_packet(newP);
					printf("sent\n");

				}
				q = aux;
				printf("renew Q\n");

				continue;
			}
			//printf("NO ID FOR PCK\n");
		}	

		//printf("NO ID FOR PCK2\n");


	}
}
