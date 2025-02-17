# Routing and Packet Transmission System (Router)  

This project implements a **routing and packet transmission system**, focusing on efficient packet forwarding, **IP address matching**, and **ARP table management**. It provides hands-on experience with packet structures, transmission protocols, and key networking concepts, resulting in a functional and optimized router implementation.  

---

## **Key Features**  
### **Packet Forwarding and Header Processing**  
- **Packet Reception**: Parses and processes IPv4 packet headers.  
- **Checksum Verification**: Ensures data integrity by verifying packet checksums.  
- **Next-Hop Calculation**: Determines the next destination for packets and forwards them efficiently.  

### **Efficient Longest Prefix Match (LPM)**  
- **Trie-Based LPM**: Replaces traditional route lookup algorithms with a **Trie data structure** for faster and more efficient IP address matching.  
- **Trie Construction**: Each node represents a bit of an IP address, storing links to the routing table for matches found at specific prefixes.  
- **Optimized Lookup**: Traverses the Trie based on the target IP’s bits, records potential candidates, and selects the best match.  

### **ICMP Protocol Support**  
- **TTL Expiry**: Sends an ICMP TTL expired message back to the source if the packet’s TTL is less than or equal to 1.  
- **No Route Found**: Generates an ICMP destination unreachable message if no matching route exists.  
- **Echo Request and Reply**: Processes ICMP echo requests by converting them into replies, updating checksums, and swapping source/destination addresses.  

### **Dynamic ARP Table Management**  
- **Dynamic ARP Table**: Converts a static ARP table into a dynamic one, populated as MAC-to-IP mappings are discovered.  
- **ARP Request Handling**: Sends ARP requests for unknown MAC addresses and temporarily queues packets.  
- **ARP Reply Handling**: Adds MAC-to-IP mappings to the ARP table upon receiving replies and forwards queued packets.  
- **Queue Management**: Maintains three queues for buffered packets, lengths, and interfaces to ensure smooth forwarding after MAC resolution.  

---

## **Technologies Used**  
- **C Programming**: Core language for implementing packet handling and routing logic.  
- **Data Structures**: Trie for efficient Longest Prefix Match (LPM).  
- **Networking Protocols**: IPv4, ICMP, and ARP.  

---

## **Challenges and Solutions**  
- **Trie-Based LPM**: Designed and implemented a Trie for efficient route lookups, significantly improving performance.  
- **ICMP Packet Construction**: Mastered ICMP header structures and checksum calculations for accurate packet handling.  
- **Dynamic ARP Table**: Integrated dynamic ARP table management with packet queuing for seamless MAC resolution.  

---

## **Lessons Learned**  
- **Packet Structures**: Gained a detailed understanding of Ethernet, IP, and ICMP headers.  
- **Optimization**: Learned to optimize route lookups using data structures like Tries for real-world performance gains.  
- **Networking Protocols**: Developed a strong grasp of ARP and ICMP protocols and their role in network communication.  
- **Debugging Skills**: Improved debugging skills for low-level packet handling and transmission.  

---

## **How to Run**  
1. Clone the repository to your local machine.  
2. Compile the project using a C compiler (e.g., `gcc`).  
3. Run the executable to start the router simulation.  

---

## **Key Takeaways**  
- A fully functional and optimized routing system capable of handling packet forwarding, IP matching, and ARP table management.  
- Hands-on experience with **low-level networking**, **data structures**, and **protocol implementation**.  
- Improved problem-solving and debugging skills for complex networking scenarios.  

This project represents a comprehensive exploration of packet routing and management, combining theoretical knowledge with practical implementation. The resulting system is efficient, flexible, and serves as a strong foundation for further development in networking applications.  
