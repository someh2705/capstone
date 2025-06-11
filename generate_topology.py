import networkx as nx
import random
import json
import matplotlib.pyplot as plt

def generate_random_topology(num_switches: int, m_links: int = 2) -> nx.Graph:
    if num_switches <= m_links:
        m_links = num_switches - 1
    
    G = None
    while G is None or not nx.is_connected(G):
        G = nx.barabasi_albert_graph(n=num_switches, m=m_links)

    mapping = {i: f's{i+1}' for i in range(num_switches)}
    G = nx.relabel_nodes(G, mapping)

    print(f" A connected switch topology with {G.number_of_nodes()} nodes and {G.number_of_edges()} links is generated.")
    return G

def attach_hosts_and_clients(G: nx.Graph, num_clients: int = 3):
    switches = list(G.nodes())
    num_attachments = 1 + num_clients
    if len(switches) < num_attachments:
        raise ValueError("Not enough switches to attach all host/clients")
    
    selected_switches = random.sample(switches, num_attachments)
    
    attachments = {'stream_host': {}, 'clients': []}

    host_switch = selected_switches[0]
    host_name = 'h1'
    G.add_node(host_name, type='host')
    G.add_edge(host_name, host_switch)
    attachments['stream_host'] = {'name': host_name, 'attached_to': host_switch}

    for i in range(num_clients):
        client_switch = selected_switches[i + 1]
        client_name = f'c{i+1}'
        G.add_node(client_name, type='host')
        G.add_edge(client_name, client_switch)
        attachments['clients'].append({'name': client_name, 'attached_to': client_switch})

    return G, attachments

def export_to_json(G: nx.Graph, p4_program_name: str = "treedn"):
    topo_data = {
        "hosts": {},
        "switches": {},
        "links": []
    }

    p4rt_port = 50051

    for node, data in G.nodes(data=True):
        if node.startswith('s'):
            topo_data["switches"][node] = {
                "bmv2_json": f"build/{p4_program_name}.json",
                "p4rt_port": p4rt_port
            }
            p4rt_port += 1
        else:
            topo_data["hosts"][node] = {}

    for u, v in G.edges():
        topo_data["links"].append([u, v])

    with open("topology.json", "w") as f:
        json.dump(topo_data, f, indent=4)
    print("'topology.json' has been created successfully.")

    runtime_data = {}
    with open("runtime.json", "w") as f:
        json.dump(runtime_data, f, indent=4)
    print("An empty 'runtime.json' has been created (for dynamic controller testing).")

def draw_topology(G: nx.Graph, host_info: dict):
    pos = nx.spring_layout(G, seed=42)

    switches = [node for node in G.nodes() if node.startswith('s')]
    host = host_info['stream_host']['name']
    clients = [c['name'] for c in host_info['clients']]

    plt.figure(figsize=(12, 12))

    nx.draw_networkx_nodes(G, pos, nodelist=switches, node_color='skyblue', node_size=1500)
    nx.draw_networkx_labels(G, pos, labels={n: n for n in switches}, font_size=10, font_weight='bold')

    nx.draw_networkx_nodes(G, pos, nodelist=[host], node_color='lightgreen', node_size=2000)
    nx.draw_networkx_labels(G, pos, labels={host: f'{host}\n(Streamer)'}, font_size=10, font_weight='bold')

    nx.draw_networkx_nodes(G, pos, nodelist=clients, node_color='salmon', node_size=2000)
    nx.draw_networkx_labels(G, pos, labels={c: f'{c}\n(Client)' for c in clients}, font_size=10, font_weight='bold')

    nx.draw_networkx_edges(G, pos, width=1.5, alpha=0.8, edge_color='gray')

    plt.title("Generated TreeDN Test Topology", size=20)
    plt.show()

if __name__ == '__main__':
    NUM_SWITCHES = 20
    P4_PROGRAM_NAME = "treedn"

    switch_graph = generate_random_topology(NUM_SWITCHES)

    full_graph, attachments = attach_hosts_and_clients(switch_graph, num_clients=3)

    print("\n--- Host/Client Attachment Info ---")
    streamer = attachments['stream_host']
    print(f"Streaming Host: {streamer['name']} -> {streamer['attached_to']}")

    for client in attachments['clients']:
        print(f"Client: {client['name']} -> {client['attached_to']}")

    export_to_json(full_graph, P4_PROGRAM_NAME)

    draw_topology(full_graph, attachments)