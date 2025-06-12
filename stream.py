import argparse
import sys
import time
import logging

from scapy.all import (
    Packet,
    ShortField,
    BitField,
    ByteField,
    StrLenField,
    Ether,
    bind_layers,
    sendp,
    sniff,
    get_if_list,
    get_if_hwaddr
)

TYPE_TREEDN = 0x1234
TYPE_INTEREST = 0x1
TYPE_DATA = 0x2

class TreeDN(Packet):
    name = "TreeDN"
    fields_desc = [
        BitField("version", 1, 4),
        BitField("packet_type", 0, 4),
        ByteField("hop_limit", 64),
        ShortField("name_length", 0)
    ]

    def post_build(self, pkt, pay):
        if pay:
            self.name_length = len(pay)
        return pkt + pay
    
class TreeDN_Name(Packet):
    name = "TreeDN_Name"
    fields_desc = [
        StrLenField("name", "", length_from=lambda pkt: pkt.underlayer.name_length)
    ]

bind_layers(Ether, TreeDN, type=TYPE_TREEDN)
bind_layers(TreeDN, TreeDN_Name)

def get_if():
    iface = None
    for i in get_if_list():
        if "eth0" in i:
            iface = i
            break
    if not iface:
        print("Cannot find eth0 interface")
        exit(1)
    return iface

def handle_packet(packet, role):
    if TreeDN in packet:
        packet_type = packet[TreeDN].packet_type
        content_name = packet[TreeDN_Name].name.decode('utf-8')

        if role == 'client' and packet_type == TYPE_DATA:
            payload = packet[TreeDN_Name].payload.decode('utf-8')
            print(f"CLIENT: Receive DATA for '{content_name}' -> '{payload}'")
        
        elif role =='host' and packet_type == TYPE_INTEREST:
            print(f"Host: Receive INTEREST for '{content_name}'. Starting stream...")
            global is_streaming
            is_streaming = True
    
def run_client(iface, content_name):
    print(f"CLIENT: Sending INTEREST for content: '{content_name}")
    pkt = Ether(dst='ff:ff:ff:ff:ff:ff', src=get_if_hwaddr(iface), type=TYPE_TREEDN)
    pkt = pkt / TreeDN(packet_type=TYPE_INTEREST) / TreeDN_Name(name=content_name)

    sendp(pkt, iface=iface, verbose=False)
    sniff(iface=iface,
          prn=lambda x: handle_packet(x, 'client'),
          filter=f"ether proto {TYPE_TREEDN}",
          timeout=10
    )
    print("CLIENT: Sniffing finished.")

is_streaming = False

def run_host(iface, content_name):
    print(f"HOST: Listening for INTERESTs for content: '{content_name}'")

    sniff(iface=iface,
          prn=lambda x: handle_packet(x, 'host'),
          filter=f"ether proto {TYPE_TREEDN}",
          stop_filter=lambda x: is_streaming
    )

    if not is_streaming:
        print("HOST: No Interest received. Existing.")

    for i in range(10):
        payload = f"Streaming packet #{i+1}"
        print(f"HOST: Sending DATA packet #{i+1}")

        pkt = Ether(dst='ff:ff:ff:ff:ff:ff', src=get_if_hwaddr(iface), type=TYPE_TREEDN)
        pkt = pkt / TreeDN(packet_type=TYPE_DATA) / TreeDN_Name(name=content_name) / payload

        sendp(pkt, iface=iface, verbose=False)
        time.sleep(1)
    
    print("HOST: Streaming finished")

def main():
    parser = argparse.ArgumentParser(description='TreeDN Streaming Simulation')
    parser.add_argument('--role', choices=['host', 'client'], required=True, help='Role to play: host or client')
    parser.add_argument('--name', type=str, required=True, help='Content name for the stream (e.g., /live/stream1)')
    args = parser.parse_args()

    iface = get_if()

    if args.role == 'client':
        run_client(iface, args.name)
    elif args.role == 'host':
        run_host(iface, args.name)
    else:
        parser.print_help()
        sys.exit(1)
    
if __name__ == '__main__':
    main()