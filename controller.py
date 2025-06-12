#!/usr/bin/env python3
import argparse
import json
import threading
import time

import networkx as nx
from scapy.all import Packet, Ether, StrLenField, bind_layers, BitField, ByteField, ShortField

import p4runtime_lib.bmv2
from p4runtime_lib.helper import P4InfoHelper
from p4runtime_lib.switch import ShutdownAllSwitchConnections

TYPE_TREEDN = 0x1234
TYPE_INTEREST = 0x1

class TreeDN(Packet):
    name = "TreeDN"
    fields_desc = [
        BitField("version", 1, 4), BitField("packet_type", 0, 4),
        ByteField("hop_limit", 64), ShortField("name_length", 0)
    ]
class TreeDN_Name(Packet):
    name = "TreeDN_Name"
    fields_desc = [ StrLenField("name", "", length_from=lambda pkt: pkt.underlayer.name_length) ]

bind_layers(Ether, TreeDN, type=TYPE_TREEDN)
bind_layers(TreeDN, TreeDN_Name)


class TreeDNController:
    def __init__(self, p4info_file_path, bmv2_json_file_path):
        self.p4info_helper = P4InfoHelper(p4info_file_path)
        self.bmv2_json_file_path = bmv2_json_file_path
        self.switches = {}
        self.topology = nx.Graph()
        self.host_locations = {}
        self.mcast_groups = {}
        self.mcast_id_counter = 1

    def load_topology(self, topo_file='topology.json'):
        print("INFO: Loading topology from 'topology.json'...")
        with open(topo_file, 'r') as f:
            topo = json.load(f)

        for switch_name, switch_info in topo['switches'].items():
            self.switches[switch_name] = p4runtime_lib.bmv2.Bmv2SwitchConnection(
                name=switch_name,
                address=f"127.0.0.1:{switch_info['p4rt_port']}",
                device_id=int(switch_name.lstrip('s')) - 1
            )
            self.topology.add_node(switch_name)

        for link in topo['links']:
            node1, node2 = link[0], link[1]
            if node1.startswith('h') and node2.startswith('s'):
                self.host_locations[node1] = node2
            elif node2.startswith('h') and node1.startswith('s'):
                self.host_locations[node2] = node1
            elif node1.startswith('s') and node2.startswith('s'):
                self.topology.add_edge(node1, node2)
        print(f"INFO: Topology loaded. Switches: {list(self.switches.keys())}, Host locations: {self.host_locations}")

    def connect_to_switches(self):
        for switch_conn in self.switches.values():
            print(f"INFO: Connecting to {switch_conn.name}...")
            switch_conn.MasterArbitrationUpdate()
            switch_conn.SetForwardingPipelineConfig(
                p4info=self.p4info_helper.p4info,
                bmv2_json_file_path=self.bmv2_json_file_path
            )
            threading.Thread(target=self.receive_packet_in, args=(switch_conn)).start()

    def receive_packet_in(self, switch_conn):
        print(f"INFO: Listening for Packet-In on {switch_conn.name}")
        while True:
            packet_in = switch_conn.PacketIn()
            if packet_in:
                payload = packet_in.packet.payload
                pkt = Ether(payload)
                if TreeDN in pkt and pkt[TreeDN].packet_type == TYPE_INTEREST:
                    content_name = pkt[TreeDN_Name].name.decode('utf-8')
                    ingress_port = int.from_bytes(packet_in.packet.metadata[0].value, 'big')
                    print(f"INFO: [{switch_conn.name}] Received Packet-In (Interest) for '{content_name}' from port {ingress_port}")
                    self.handle_interest(switch_conn, content_name, ingress_port)

    def handle_interest(self, switch_conn, content_name, ingress_port):
        content_source_switch = self.host_locations.get('h1')
        if not content_source_switch:
            print(f"ERROR: Stream host 'h1' not found in topology.")
            return

        path_to_source = nx.shortest_path(self.topology, source=switch_conn.name, target=content_source_switch)
        print(f"INFO: Path for '{content_name}' from {switch_conn.name} to {content_source_switch}: {path_to_source}")

        for i in range(len(path_to_source) - 1):
            current_s = path_to_source[i]
            next_s = path_to_source[i+1]
            output_port = self.topology.get_edge_data(current_s, next_s).get('port', 1)

            output_port = int(next_s.lstrip('s'))
            
            print(f"INFO: Installing FIB rule on {current_s}: '{content_name}' -> port {output_port}")
            self.write_fib_rule(self.switches[current_s], content_name, output_port)

        self.setup_multicast_path(content_name, switch_conn, ingress_port)

    def setup_multicast_path(self, content_name, switch_conn, ingress_port):
        if content_name not in self.mcast_groups:
            mcast_id = self.mcast_id_counter
            self.mcast_groups[content_name] = {
                'mcast_id': mcast_id,
                'ports': {switch_conn.name: [ingress_port]}
            }
            self.mcast_id_counter += 1
        else:
            mcast_info = self.mcast_groups[content_name]
            if switch_conn.name not in mcast_info['ports']:
                mcast_info['ports'][switch_conn.name] = []
            if ingress_port not in mcast_info['ports'][switch_conn.name]:
                mcast_info['ports'][switch_conn.name].append(ingress_port)

        mcast_id = self.mcast_groups[content_name]['mcast_id']
        ports_for_switch = self.mcast_groups[content_name]['ports'][switch_conn.name]
        
        print(f"INFO: [{switch_conn.name}] Setting up Multicast Group {mcast_id} for '{content_name}' with ports {ports_for_switch}")
        self.write_multicast_group(switch_conn, mcast_id, ports_for_switch)
        
        print(f"INFO: [{switch_conn.name}] Installing PIT rule for '{content_name}' -> mcast_group {mcast_id}")
        self.write_pit_rule(switch_conn, content_name, mcast_id)

    def write_fib_rule(self, switch_conn, content_name, egress_port):
        table_entry = self.p4info_helper.buildTableEntry(
            table_name="MyIngress.fib",
            match_fields={"hdr.treedn_name.name": (content_name.encode('utf-8'), 32)}, # LPM, 32비트 접두사로 가정
            action_name="MyIngress.treedn_forward",
            action_params={"egress_port": egress_port}
        )
        switch_conn.WriteTableEntry(table_entry)

    def write_pit_rule(self, switch_conn, content_name, mcast_group_id):
        table_entry = self.p4info_helper.buildTableEntry(
            table_name="MyIngress.pit",
            match_fields={"hdr.treedn_name.name": content_name.encode('utf-8')}, # Exact Match
            action_name="MyIngress.set_multicast_group",
            action_params={"mcast_group_id": mcast_group_id}
        )
        switch_conn.WriteTableEntry(table_entry)
        
    def write_multicast_group(self, switch_conn, mcast_id, ports):
        mc_group_entry = self.p4info_helper.buildMulticastGroupEntry(
            multicast_group_id=mcast_id,
            replicas=[{"egress_port": port, "instance": 1} for port in ports]
        )
        switch_conn.WritePREEntry(mc_group_entry)

    def run(self):
        self.load_topology()
        self.connect_to_switches()
        print("\nINFO: Controller is running. Press Ctrl-C to exit.")
        try:
            while True:
                time.sleep(1)
        except KeyboardInterrupt:
            print("INFO: Shutting down.")
        finally:
            ShutdownAllSwitchConnections()


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='P4 TreeDN Controller')
    parser.add_argument('--p4info', help='Path to P4Info file', type=str, action="store", required=True)
    parser.add_argument('--bmv2-json', help='Path to BMv2 JSON file', type=str, action="store", required=True)
    args = parser.parse_args()

    controller = TreeDNController(args.p4info, args.bmv2_json)
    controller.run()