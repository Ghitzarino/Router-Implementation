# Routing and Packet Transmission System (Router)

## Overview:
This project implements a routing and packet transmission system, focusing on efficient packet forwarding, IP address matching, and ARP table management.
The project required approximately 18–20 hours of work over a week and presented a moderate to high level of difficulty. It was an excellent learning opportunity, providing hands-on experience with packet structures, transmission protocols, and key networking concepts.

## Features:
1) Packet Forwarding and Header Processing:
    * Implemented packet reception and header parsing to handle IPv4 packets.
    * Verified checksum correctness to ensure data integrity.
    * Calculated the next destination for packets and forwarded them accordingly.
    * Used a skeleton structure based on prior knowledge and refined it for enhanced functionality.

2) Efficient Longest Prefix Match (LPM):
    * Replaced the traditional route lookup algorithm with a Trie-based LPM search for higher efficiency.
    * Constructed a Trie structure, where each node represents a bit of an IP address. Nodes store links to the routing table for matches found at specific prefixes.
    * During a lookup, the Trie is traversed according to the bits of the target IP. Potential candidates are recorded, and the best match is selected.
      
3) ICMP Protocol Support - Handled three key ICMP scenarios:
    * TTL Expiry: If the packet’s TTL is less than or equal to 1, an ICMP TTL expired message is sent back to the source.
    * No Route Found: If no matching route exists, an ICMP destination unreachable message is generated.
    * Echo Request and Reply: Processed ICMP echo requests by modifying the header to convert it into a reply, updating the checksum, swapping source and destination addresses, and sending the packet back.

4) Dynamic ARP Table Management:
    * Converted the static ARP table into a dynamic table that is populated as MAC-to-IP mappings are discovered.
    * ARP Request Handling:
        When the destination MAC is unknown, an ARP request is sent, and the packet is temporarily stored in a queue.
    * ARP Reply Handling:
        Upon receiving a reply, the MAC-to-IP mapping is added to the ARP table. The queued packet is retrieved and forwarded with the newly learned MAC address.
    * Maintained three queues for buffered packets, lengths, and interfaces, ensuring smooth forwarding after MAC resolution.
   
## Challenges and Solutions:
* Trie-Based LPM Implementation:
    Designing a Trie for LPM was challenging but resulted in a significant performance improvement for route lookups.
* ICMP Packet Construction:
    Creating and modifying ICMP packets required a deep understanding of header structures and checksum calculations.
* Dynamic ARP Table Integration:
    Managing the ARP table dynamically and ensuring synchronization between ARP requests, replies, and the packet queue required careful planning and testing.
  
## Lessons Learned:
* Gained a detailed understanding of packet structures, including Ethernet, IP, and ICMP headers.
* Learned to optimize route lookups using data structures like Tries for real-world performance gains.
* Developed a strong grasp of networking protocols, including ARP and ICMP, and how they interact to enable seamless communication in a network.
* Improved debugging skills for low-level packet handling and transmission.
  
This project represents a comprehensive exploration of packet routing and management, combining theoretical knowledge with practical implementation.
The resulting system is efficient, flexible, and serves as a strong foundation for further development in networking applications.
