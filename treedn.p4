#include <core.p4>
#include <v1model.p4>

const bit<16> TYPE_TREEDN = 0x1234;

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

header treedn_h {
    bit<4> version;
    bit<4> packet_type; // interest, receive data
    bit<8> hop_limit; // ttl
    bit<16> name_length; // stream name
}

header treedn_name_h {
    varbit<1024> name;
}

struct headers {
    ethernet_t ethernet;
    treedn_h treedn;
    treedn_name_h treedn_name;
}

parser ParseImpl(packet_in packet,
                 out headers hdr,
                 inout metadata meta,
                 inout standard_metadata_t standard_metadata) {
    
    state start {
        transition select(packet.lookahead<ethernet_t>().etherType) {
            TYPE_TREEDN : parse_treedn;
            default: accept;
        }
    }

    state parse_treedn {
        packet.extract(hdr.ethernet);

        packet.extract(hdr.treedn);
        transition parse_treedn_name;
    }

    state parse_treedn_name {
        packet.extract(hdr.treedn_name, hdr.treedn.name_length * 8);
        transition accept;
    }
}

control VerifyChecksum(inout headers hdr,
                       inout metadata meta) {
    apply {}
}

control IngressImpl(inout headers hdr,
                    inout metadata meta,
                    inout standard_metadata_t standard_metadata) {

    action treedn_forward(port_t egress_port) {
        standard_metadata.egress_spec = egress_port;
    }

    action set_multicast_group(mcast_grp_t mcast_group_id) {
        standard_metadata.mcast_grp = mcast_group_id;
    }

    action send_to_cpu() {
        standard_metadata.egress_spec = 511;
    }

    action treedn_drop() {
        mark_to_drop(standard_metadata);
    }

    table forwarding_information {
        key = {
            hdr.treedn_name.name: lpm;
        }

        actions = {
            treedn_forward;
            send_to_cpu;
            treedn_drop;
        }

        size = 1024;

        default_action = send_to_cpu();
    }

    table pending_interest {
        key = {
            hdr.treedn_name.name: exact;
        }

        actions = {
            set_multicast_group;
            treedn_drop;
        }

        size = 4096;

        default_action = treedn_drop();
    }
            
    apply {
        if (hdr.treedn.isValid()) {
            if (hdr.treedn.hop_limit <= 1) {
                treedn_drop();
                return;
            }

            hdr.treedn.hop_limit = hdr.treedn.hop_limit - 1;

            if (hdr.treedn.packet_type == 0x1) {
                apply(forwarding_information);
            }
            else if (hdr.treedn.packet_type == 0x2) {
                apply(pending_interest);
            }
        }
    }
}

control EgressImpl(inout headers hdr,
                   inout metadata meta,
                   inout standard_metadata_t standard_metadata) {
    apply {}
}

control ComputeChecksum(inout headers hdr,
                        inout metadata meta) {
    apply {}
}

control DeparserImpl(packet_out packetj,
                     in headers hdr) {
    apply {
        packet.emit(hdr.ethernet);
        if (hdr.treedn.isValid()) {
            packet.emit(hdr.treedn);
            packet.emit(hdr.treedn_name);
        }
    }
}

V1Switch(
    ParserImpl(),
    VerifyChecksum(),
    IngressImpl(),
    EgressImpl(),
    ComputeChecksum(),
    DeparserImpl()
) main;