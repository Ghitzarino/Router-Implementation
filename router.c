#include <arpa/inet.h>
#include <linux/if_ether.h>
#include <string.h>

#include "lib.h"
#include "protocols.h"
#include "queue.h"

// Routing table
struct route_table_entry *rtable;
int rtable_len;

// ARP table
struct arp_table_entry *arp_table;
int arp_table_len;

// Function to get the mask length from uint32_t form
int get_mask_length(uint32_t mask) {
  int length = 0;
  for (int i = 0; i < 32; i++) {
    if ((mask >> i) & 1) {
      length++;
    } else {
      break;
    }
  }
  return length;
}

// Structure for a node in the trie
struct trie_node {
  struct trie_node *children[2];    // 0/1 for the ip bits
  struct route_table_entry *route;  // pointer to routing table entry
};

// Initialize the root of the trie
struct trie_node *root = NULL;

// Function to create a new trie node
struct trie_node *create_node() {
  struct trie_node *node = (struct trie_node *)malloc(sizeof(struct trie_node));
  if (node != NULL) {
    node->children[0] = NULL;
    node->children[1] = NULL;
    node->route = NULL;
  }
  return node;
}

// Function to insert a route into the trie
void insert_route(struct route_table_entry *route) {
  if (root == NULL) {
    root = create_node();
  }

  struct trie_node *current = root;
  int mask_length = get_mask_length(route->mask);

  for (int i = 0; i <= mask_length; i++) {
    // Get the i-th bit of the prefix
    int bit = (route->prefix >> i) & 1;
    // Create the child node if it doesn't already exist
    if (current->children[bit] == NULL) {
      current->children[bit] = create_node();
    }
    current = current->children[bit];
  }

  // Set the route in the final node
  current->route = route;
}

// Function to build the trie from the routing table
void build_trie() {
  for (int i = 0; i < rtable_len; i++) {
    insert_route(&rtable[i]);
  }
}

// Function to get the best route using trie (or NULL if it doesn't exist)
struct route_table_entry *get_best_route(uint32_t ip_dest) {
  struct trie_node *current = root;
  struct route_table_entry *result = NULL;

  for (int i = 0; i < 32; i++) {
    int bit = (ip_dest >> i) & 1;

    // Update the result to get the longest prefix match for the ip
    if (current->route != NULL) {
      if (result == NULL ||
          (ntohl(current->route->mask) > ntohl(result->mask))) {
        result = current->route;
      }
    }

    if (current->children[bit] != NULL) {
      current = current->children[bit];
    } else {
      break;
    }
  }

  return result;
}

// Function to get from the ARP cache the MAC address from its IPv4
struct arp_table_entry *get_arp_entry(uint32_t given_ip) {
  for (int i = 0; i < arp_table_len; i++) {
    if (given_ip == arp_table[i].ip) return &arp_table[i];
  }

  return NULL;
}

// Function to create from scratch and send the ICMP packet
void send_icmp_packet(struct icmphdr *icmp_hdr,
                      struct ether_header *original_eth_hdr,
                      struct iphdr *original_ip_hdr, int interface) {
  // Allocate memory for the frame
  size_t frame_len = sizeof(struct ether_header) + sizeof(struct iphdr) +
                     sizeof(struct icmphdr);
  char *frame = (char *)malloc(frame_len);

  // Fill the ether header of the frame
  memcpy(frame, original_eth_hdr, sizeof(struct ether_header));

  // Update the source and destination MAC addresses
  get_interface_mac(interface, original_eth_hdr->ether_shost);

  struct iphdr *ip_hdr = (struct iphdr *)(frame + sizeof(struct ether_header));
  struct icmphdr *icmp_hdr_frame =
      (struct icmphdr *)(frame + sizeof(struct ether_header) +
                         sizeof(struct iphdr));

  // Fill the IP header of the frame
  ip_hdr->version = 4;
  ip_hdr->ihl = 5;
  ip_hdr->tos = 0;
  ip_hdr->tot_len = htons(sizeof(struct iphdr) + sizeof(struct icmphdr));
  ip_hdr->id = 1;
  ip_hdr->frag_off = 0;
  ip_hdr->ttl = 64;
  ip_hdr->protocol = 1;  // Protocol number for ICMP
  ip_hdr->check = 0;
  // Interchange the source and destination address
  ip_hdr->saddr = original_ip_hdr->daddr;
  ip_hdr->daddr = original_ip_hdr->saddr;

  // Fill the ICMP header of the frame
  memcpy(icmp_hdr_frame, icmp_hdr, sizeof(struct icmphdr));

  // Calculate the IP checksum
  ip_hdr->check = checksum((uint16_t *)ip_hdr, sizeof(struct iphdr));

  // Send the ICMP packet
  send_to_link(interface, frame, frame_len);

  free(frame);
}

// Function to create from scratch and send the ARP packet request
void send_arp_request(uint32_t target_ip, int interface) {
  // Allocate memory for the frame
  size_t frame_len = sizeof(struct ether_header) + sizeof(struct arp_header);
  char *frame = (char *)malloc(frame_len);

  // Fill the ether header of the frame
  struct ether_header *eth_hdr = (struct ether_header *)frame;
  memset(eth_hdr->ether_dhost, 0xFF, 6);
  get_interface_mac(interface, eth_hdr->ether_shost);
  eth_hdr->ether_type = htons(0x0806);

  // Fill the ARP header of the frame
  struct arp_header *arp_hdr =
      (struct arp_header *)(frame + sizeof(struct ether_header));
  arp_hdr->htype = htons(1);
  arp_hdr->ptype = htons(0x0800);
  arp_hdr->hlen = 6;
  arp_hdr->plen = 4;
  arp_hdr->op = htons(1);  // ARP request
  get_interface_mac(interface, arp_hdr->sha);
  arp_hdr->spa = inet_addr(get_interface_ip(interface));
  memset(arp_hdr->tha, 0, 6);
  arp_hdr->tpa = target_ip;

  // Send the ARP request packet
  send_to_link(interface, frame, frame_len);

  free(frame);
}

// Function to create from scratch and send the ARP packet reply
void send_arp_response(uint8_t *dest_mac, uint32_t dest_ip, uint32_t src_ip,
                       int interface) {
  // Allocate memory for the frame
  size_t frame_len = sizeof(struct ether_header) + sizeof(struct arp_header);
  char *frame = (char *)malloc(frame_len);

  // Fill the ether header of the frame
  struct ether_header *eth_hdr = (struct ether_header *)frame;
  uint8_t src_mac[6];
  get_interface_mac(interface, src_mac);
  memcpy(eth_hdr->ether_dhost, dest_mac, 6);
  memcpy(eth_hdr->ether_shost, src_mac, 6);
  eth_hdr->ether_type = htons(0x0806);

  // Fill the ARP header of the frame
  struct arp_header *arp_hdr =
      (struct arp_header *)(frame + sizeof(struct ether_header));
  arp_hdr->htype = htons(1);
  arp_hdr->ptype = htons(0x0800);
  arp_hdr->hlen = 6;
  arp_hdr->plen = 4;
  arp_hdr->op = htons(2);  // ARP response
  memcpy(arp_hdr->sha, src_mac, 6);
  arp_hdr->spa = src_ip;
  memcpy(arp_hdr->tha, dest_mac, 6);
  arp_hdr->tpa = dest_ip;

  // Send the ARP reply packet
  send_to_link(interface, frame, frame_len);

  free(frame);
}

int main(int argc, char *argv[]) {
  char buf[MAX_PACKET_LEN];

  // Do not modify this line
  init(argc - 2, argv + 2);

  // Code to allocate the arp and route tables
  rtable = malloc(sizeof(struct route_table_entry) * 70000);
  DIE(rtable == NULL, "memory");

  arp_table = malloc(sizeof(struct arp_table_entry) * 20);
  DIE(arp_table == NULL, "memory");

  // Read the static routing table
  rtable_len = read_rtable(argv[1], rtable);

  arp_table_len = 0;

  // Bulid the trie from the routing table
  build_trie();

  // Create a queue for awaiting ARP packets (packet buf with its coresponding
  // len and interface)
  queue buf_q = queue_create();
  queue len_q = queue_create();
  queue interface_q = queue_create();

  while (1) {
    size_t len;

    // Receive the packet throught the interface
    int interface = recv_from_any_link(buf, &len);
    DIE(interface < 0, "interface error");

    // Extract the Ethernet header from the packet
    struct ether_header *eth_hdr = (struct ether_header *)buf;

    // Check if we got an ARP packet
    if (eth_hdr->ether_type == ntohs(0x0806)) {
      // Extract the ARP header from the packet
      struct arp_header *arp_hdr =
          (struct arp_header *)(buf + sizeof(struct ether_header));

      // Calculate the current IPv4 address
      uint32_t curr_ip_addr = inet_addr(get_interface_ip(interface));

      // If the ARP packet is a request send the reply
      if (arp_hdr->tpa == curr_ip_addr && arp_hdr->op == ntohs(1)) {
        send_arp_response(arp_hdr->sha, arp_hdr->spa, arp_hdr->tpa, interface);
        continue;
      }

      // If the ARP packet is a reply
      if (arp_hdr->tpa == curr_ip_addr && arp_hdr->op == ntohs(2)) {
        struct arp_table_entry *dest_mac = get_arp_entry(arp_hdr->spa);

        // If the MAC isn't stored in the cache
        if (dest_mac == NULL) {
          // Store the MAC and IPv4 addresses in the cache
          arp_table[arp_table_len].ip = arp_hdr->spa;
          memcpy(arp_table[arp_table_len].mac, arp_hdr->sha, 6);
          arp_table_len++;
          dest_mac = get_arp_entry(arp_hdr->spa);
        }

        // Check if the queue still has packets waiting
        if (queue_empty(buf_q)) {
          continue;
        }

        // Dequeue buf and len from the queues
        char *buf_ptr = (char *)queue_deq(buf_q);
        size_t *len_ptr = (size_t *)queue_deq(len_q);
        int *interface_ptr = (int *)queue_deq(interface_q);

        struct ether_header *aux_eth_hdr = (struct ether_header *)buf_ptr;

        // Complete the destination address
        memcpy(aux_eth_hdr->ether_dhost, dest_mac->mac, 6);

        send_to_link(*interface_ptr, buf_ptr, *len_ptr);
        continue;
      }
    }

    // Check if we got an IPv4 packet
    if (eth_hdr->ether_type != ntohs(0x0800)) {
      printf("Ignored non-IPv4 packet\n");
      continue;
    }

    // Extract the IPv4 header from the packet
    struct iphdr *ip_hdr = (struct iphdr *)(buf + sizeof(struct ether_header));

    // Verify the checksum
    if (checksum((uint16_t *)ip_hdr, sizeof(struct iphdr)) != 0) {
      printf("Packet is corrupted\n");
      continue;
    }

    // Implementation for ICMP echo request
    if (ip_hdr->protocol == 1) {
      // Calculate the current IPv4 address
      uint32_t curr_ip_addr = inet_addr(get_interface_ip(interface));

      // If the packet is echo request create the reply back
      if (curr_ip_addr == ip_hdr->daddr) {
        struct icmphdr *icmp_hdr =
            (struct icmphdr *)(buf + sizeof(struct ether_header) +
                               sizeof(struct iphdr));

        icmp_hdr->type = 0;
        icmp_hdr->code = 0;
        icmp_hdr->checksum = 0;

        // Calculate the ICMP checksum
        icmp_hdr->checksum =
            checksum((uint16_t *)icmp_hdr, sizeof(struct icmphdr));

        // Interchange the souce and destination
        uint32_t aux = ip_hdr->saddr;
        ip_hdr->saddr = ip_hdr->daddr;
        ip_hdr->daddr = aux;
      }
    }

    // Get the best route using the trie finding algorithm
    struct route_table_entry *best_route = get_best_route(ip_hdr->daddr);

    // Implementation for ICMP no route found
    if (best_route == NULL) {
      struct icmphdr *icmp_hdr = malloc(sizeof(struct icmphdr));
      icmp_hdr->type = 3;
      icmp_hdr->code = 0;
      icmp_hdr->checksum = 0;

      // Copy the first 64 bits of the payload of the original IP packet
      memcpy(&icmp_hdr->un.echo, ip_hdr, sizeof(uint64_t));

      // Calculate the ICMP checksum
      icmp_hdr->checksum =
          checksum((uint16_t *)icmp_hdr, sizeof(struct icmphdr));

      // Send the packet
      send_icmp_packet(icmp_hdr, eth_hdr, ip_hdr, interface);

      free(icmp_hdr);
      continue;
    }

    // Implementation for ICMP timeout
    if (ip_hdr->ttl <= 1) {
      struct icmphdr *icmp_hdr = malloc(sizeof(struct icmphdr));
      icmp_hdr->type = 11;
      icmp_hdr->code = 0;
      icmp_hdr->checksum = 0;

      // Copy the first 64 bits of the payload of the original IP packet
      memcpy(&icmp_hdr->un.echo, ip_hdr, sizeof(uint64_t));

      // Calculate the ICMP checksum
      icmp_hdr->checksum =
          checksum((uint16_t *)icmp_hdr, sizeof(struct icmphdr));

      // Send the packet
      send_icmp_packet(icmp_hdr, eth_hdr, ip_hdr, interface);

      free(icmp_hdr);
      continue;
    }

    // Update ttl and checksum
    ip_hdr->ttl -= 1;
    ip_hdr->check = 0;
    ip_hdr->check = htons(checksum((uint16_t *)ip_hdr, sizeof(struct iphdr)));

    // Get the destination MAC for the next hop
    struct arp_table_entry *dest_mac = get_arp_entry(best_route->next_hop);

    // Destination MAC is unknown
    if (dest_mac == NULL) {
      char aux_buf[MAX_PACKET_LEN];
      int aux_interface = best_route->interface;
      size_t aux_len = len;
      memcpy(aux_buf, buf, len);

      // Enqueue the packet with buf, len and interface to wait for the ARP
      // response
      queue_enq(buf_q, aux_buf);
      queue_enq(len_q, &aux_len);
      queue_enq(interface_q, &aux_interface);

      send_arp_request(best_route->next_hop, best_route->interface);
      continue;
    }

    // Update Ether source and destination address
    memcpy(eth_hdr->ether_dhost, dest_mac->mac, 6);
    get_interface_mac(best_route->interface, eth_hdr->ether_shost);

    send_to_link(best_route->interface, buf, len);
  }
}
