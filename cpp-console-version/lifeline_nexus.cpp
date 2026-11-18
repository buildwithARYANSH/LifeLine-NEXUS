// ============================================================
//  LifeLine-Nexus : District Emergency Triage, Routing & Isolation Engine
//  Language : C++ (console based)
//  Concepts : Classes/OOP, Graph (Adjacency List), Max-Heap (Priority Queue),
//             Dijkstra's Shortest Path, Disjoint Set Union (DSU), vectors
//  Visuals  : ANSI colors, ASCII district map, boxed menus, priority bars
// ============================================================
#include <iostream>
#include <vector>
#include <queue>
#include <string>
#include <climits>
#include <algorithm>
#ifdef _WIN32
#include <windows.h>
#endif
using namespace std;

const int INF = INT_MAX;

// ------------------------------------------------------------
// Colors (ANSI escape codes)
// ------------------------------------------------------------
const string RESET = "\033[0m";
const string RED   = "\033[31m";
const string GREEN = "\033[32m";
const string YEL   = "\033[1;33m";
const string CYAN  = "\033[1;36m";
const string BOLD  = "\033[1m";

void enableColors() {
#ifdef _WIN32   // old Windows consoles need this to understand colors
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(h, &mode);
    SetConsoleMode(h, mode | 0x0004);
    SetConsoleOutputCP(65001);
#endif
}

// ------------------------------------------------------------
// Names of the places in our district (node number = index)
// ------------------------------------------------------------
vector<string> placeName = {
    "Central HQ", "City Hospital", "Rural Hospital", "Market Junction", "River Bridge",
    "Highway Crossing", "Rampur Village", "Shivpur Village", "Hill Village", "Lake Village"
};
// shorter names used inside the ASCII map
vector<string> shortName = {
    "Central HQ", "City Hospital", "Rural Hospital", "Market Jn", "River Bridge",
    "Highway Cross", "Rampur Vil", "Shivpur Vil", "Hill Village", "Lake Village"
};
vector<bool> isHospital = {false, true, true, false, false, false, false, false, false, false};

// remembers the last dispatched route so the map can highlight it
bool routeEdge[10][10];
bool onRoute[10];
void clearRoute() {
    for (int i = 0; i < 10; i++) {
        onRoute[i] = false;
        for (int j = 0; j < 10; j++) routeEdge[i][j] = false;
    }
}

// ============================================================
// MODULE 1 : GRAPH (Adjacency List)
// ============================================================
struct Edge {
    int to;        // destination node
    int weight;    // travel time in minutes
    bool blocked;  // true = road blocked (weight becomes infinity)
};

class Graph {
public:
    int V;
    vector<vector<Edge>> adj;   // adj[u] = list of roads going out of u

    Graph(int n) {
        V = n;
        adj.resize(n);
    }

    // roads are two-way, so we add the edge in both lists
    void addRoad(int u, int v, int w) {
        adj[u].push_back({v, w, false});
        adj[v].push_back({u, w, false});
    }

    // returns true if road exists (and toggles it)
    bool toggleRoad(int u, int v) {
        bool found = false;
        for (auto &e : adj[u])
            if (e.to == v) { e.blocked = !e.blocked; found = true; }
        for (auto &e : adj[v])
            if (e.to == u) { e.blocked = !e.blocked; }
        return found;
    }

    const Edge* findEdge(int u, int v) {
        for (auto &e : adj[u])
            if (e.to == v) return &e;
        return nullptr;
    }

    void printRoads() {
        cout << "\n" << BOLD << "--- ROAD LIST ---" << RESET << "\n";
        for (int u = 0; u < V; u++)
            for (auto &e : adj[u])
                if (u < e.to)   // print each road once
                    cout << u << " (" << placeName[u] << ") <-> " << e.to << " (" << placeName[e.to]
                         << ") : " << e.weight << " min  "
                         << (e.blocked ? RED + "[BLOCKED]" : GREEN + "[open]") << RESET << "\n";
    }
};

// ============================================================
// VISUAL HELPERS (nothing here is algorithmic)
// ============================================================
string sp(int n) { return string(n, ' '); }

// one line inside a box, padded to the given width
string boxLine(const string &text, int width) {
    string t = " " + text;
    if ((int)t.size() < width) t += sp(width - t.size());
    return "║" + t + "║";
}
string boxTop(int width) {
    string s = "╔";
    for (int i = 0; i < width; i++) s += "═";
    return s + "╗";
}
string boxMid(int width) {
    string s = "╠";
    for (int i = 0; i < width; i++) s += "═";
    return s + "╣";
}
string boxBottom(int width) {
    string s = "╚";
    for (int i = 0; i < width; i++) s += "═";
    return s + "╝";
}

// ---- ASCII map helpers ----
string lbl(Graph &g, int u, int v) {      // weight text, or X if blocked
    const Edge *e = g.findEdge(u, v);
    return e->blocked ? "X" : to_string(e->weight);
}
string paint(Graph &g, int u, int v, const string &s) {   // color road by state
    const Edge *e = g.findEdge(u, v);
    if (e->blocked) return RED + s + RESET;
    if (routeEdge[u][v]) return YEL + s + RESET;
    return GREEN + s + RESET;
}
string node(int i) {                       // color place by type
    string s = "[" + to_string(i) + " " + shortName[i] + "]";
    if (onRoute[i]) return YEL + s + RESET;
    if (isHospital[i]) return CYAN + s + RESET;
    return BOLD + s + RESET;
}

void drawMap(Graph &g) {
    cout << "\n" << BOLD << "================ DISTRICT MAP ================" << RESET << "\n\n";
    cout << sp(2) << node(1) << paint(g, 1, 0, "------" + lbl(g, 1, 0) + "------") << node(0) << "\n";
    cout << sp(10) << paint(g, 1, 3, "|") << sp(28) << paint(g, 0, 3, "|") << "\n";
    cout << sp(10) << paint(g, 1, 3, lbl(g, 1, 3)) << sp(28) << paint(g, 0, 3, lbl(g, 0, 3)) << "\n";
    cout << sp(10) << paint(g, 1, 3, "|") << sp(28) << paint(g, 0, 3, "|") << "\n";
    cout << sp(10) << paint(g, 1, 3, "+-------") << node(3) << paint(g, 0, 3, "--------+") << "\n";
    cout << sp(19) << paint(g, 3, 4, "/") << sp(9) << paint(g, 3, 5, "\\") << "\n";
    cout << sp(18) << paint(g, 3, 4, lbl(g, 3, 4)) << sp(11) << paint(g, 3, 5, lbl(g, 3, 5)) << "\n";
    cout << sp(17) << paint(g, 3, 4, "/") << sp(13) << paint(g, 3, 5, "\\") << "\n";
    cout << sp(8) << node(4) << sp(4) << node(5) << "\n";
    cout << sp(10) << paint(g, 4, 6, "|") << sp(11) << paint(g, 4, 2, "\\") << sp(10)
         << paint(g, 5, 2, "/") << sp(8) << paint(g, 5, 7, "|") << "\n";
    cout << sp(10) << paint(g, 4, 6, lbl(g, 4, 6)) << sp(13) << paint(g, 4, 2, lbl(g, 4, 2)) << sp(6)
         << paint(g, 5, 2, lbl(g, 5, 2)) << sp(10) << paint(g, 5, 7, lbl(g, 5, 7)) << "\n";
    cout << sp(10) << paint(g, 4, 6, "|") << sp(15) << paint(g, 4, 2, "\\") << sp(2)
         << paint(g, 5, 2, "/") << sp(12) << paint(g, 5, 7, "|") << "\n";
    cout << sp(3) << node(6) << sp(2) << node(2) << sp(1) << node(7) << "\n";
    cout << sp(10) << paint(g, 6, 9, "|") << sp(16) << paint(g, 2, 8, "|") << sp(14) << paint(g, 7, 9, "|") << "\n";
    cout << sp(10) << paint(g, 6, 9, lbl(g, 6, 9)) << sp(16) << paint(g, 2, 8, lbl(g, 2, 8)) << sp(14)
         << paint(g, 7, 9, lbl(g, 7, 9)) << "\n";
    cout << sp(10) << paint(g, 6, 9, "|") << sp(16) << paint(g, 2, 8, "|") << sp(14) << paint(g, 7, 9, "|") << "\n";
    cout << sp(10) << paint(g, 6, 9, "|") << sp(8) << node(8) << sp(7) << paint(g, 7, 9, "|") << "\n";
    cout << sp(10) << paint(g, 6, 9, "+--------") << node(9) << paint(g, 7, 9, "-------+") << "\n\n";

    cout << "  Legend: " << GREEN << "green = open road" << RESET << " | " << RED << "red X = blocked" << RESET
         << " | " << YEL << "yellow = dispatch route" << RESET << " | " << CYAN << "cyan = hospital" << RESET << "\n";
    cout << "  (numbers on roads = travel time in minutes)\n";
}

// ============================================================
// MODULE 2 : MAX-HEAP  (Triage queue) - written by hand
// ============================================================
struct Emergency {
    int id;
    int village;     // node where the emergency is
    int type;        // 1 Flood, 2 Landslide, 3 Chemical spill, 4 Fire
    int severity;    // 1 to 10
    int population;  // people at risk
    int score;       // P = severity*10 + population
};

string hazardName(int t) {
    if (t == 1) return "Flood";
    if (t == 2) return "Landslide";
    if (t == 3) return "Chemical Spill";
    return "Fire";
}

string severityTag(int s) {
    if (s >= 8) return RED + "[CRITICAL]" + RESET;
    if (s >= 5) return YEL + "[SERIOUS] " + RESET;
    return GREEN + "[MINOR]   " + RESET;
}

string severityBar(int s) {
    string color = (s >= 8) ? RED : (s >= 5 ? YEL : GREEN);
    string bar = color;
    for (int i = 0; i < 10; i++) {
        if (i == s) bar += RESET + "\033[2m";   // dim for empty part
        bar += (i < s) ? "█" : "░";
    }
    return "[" + bar + RESET + "] " + to_string(s) + "/10";
}

class MaxHeap {
    vector<Emergency> h;

    void heapifyUp(int i) {
        while (i > 0) {
            int parent = (i - 1) / 2;
            if (h[i].score > h[parent].score) {
                swap(h[i], h[parent]);
                i = parent;
            } else break;
        }
    }

    void heapifyDown(int i) {
        int n = h.size();
        while (true) {
            int left = 2 * i + 1, right = 2 * i + 2, largest = i;
            if (left < n && h[left].score > h[largest].score) largest = left;
            if (right < n && h[right].score > h[largest].score) largest = right;
            if (largest == i) break;
            swap(h[i], h[largest]);
            i = largest;
        }
    }

public:
    bool empty() { return h.empty(); }
    int size() { return h.size(); }

    void push(Emergency e) {             // O(log N)
        h.push_back(e);
        heapifyUp(h.size() - 1);
    }

    Emergency pop() {                    // O(log N)
        Emergency top = h[0];
        h[0] = h.back();
        h.pop_back();
        if (!h.empty()) heapifyDown(0);
        return top;
    }

    // show all pending requests in priority order (works on a copy)
    void display() {
        if (h.empty()) { cout << "\nNo pending emergencies.\n"; return; }
        MaxHeap copy = *this;
        cout << "\n" << BOLD << "======== PENDING EMERGENCIES (highest priority first) ========" << RESET << "\n";
        int rank = 1;
        while (!copy.empty()) {
            Emergency e = copy.pop();
            cout << "\n #" << rank++ << "  " << severityTag(e.severity) << " " << BOLD << hazardName(e.type) << RESET
                 << " at " << placeName[e.village] << "  (ID " << e.id << ")\n";
            cout << "     Severity " << severityBar(e.severity) << "   People at risk: " << e.population
                 << "   SCORE = " << BOLD << e.score << RESET << "\n";
        }
    }
};

// ============================================================
// MODULE 3 : DIJKSTRA (shortest path ignoring blocked roads)
// ============================================================
class Router {
public:
    vector<int> dist, parent;

    void run(Graph &g, int src) {
        dist.assign(g.V, INF);
        parent.assign(g.V, -1);
        // min-heap of (distance, node)
        priority_queue<pair<int,int>, vector<pair<int,int>>, greater<pair<int,int>>> pq;
        dist[src] = 0;
        pq.push({0, src});

        while (!pq.empty()) {
            int d = pq.top().first;
            int u = pq.top().second;
            pq.pop();
            if (d > dist[u]) continue;

            for (auto &e : g.adj[u]) {
                if (e.blocked) continue;           // blocked road = infinity, skip it
                if (dist[u] + e.weight < dist[e.to]) {
                    dist[e.to] = dist[u] + e.weight;
                    parent[e.to] = u;
                    pq.push({dist[e.to], e.to});
                }
            }
        }
    }
};

// ============================================================
// MODULE 4 : DSU (Disjoint Set Union) - isolation detection
// ============================================================
class DSU {
    vector<int> parent, rnk;
public:
    DSU(int n) {
        parent.resize(n);
        rnk.assign(n, 0);
        for (int i = 0; i < n; i++) parent[i] = i;
    }

    int find(int x) {                       // with path compression
        if (parent[x] != x) parent[x] = find(parent[x]);
        return parent[x];
    }

    void unite(int a, int b) {              // union by rank
        a = find(a); b = find(b);
        if (a == b) return;
        if (rnk[a] < rnk[b]) swap(a, b);
        parent[b] = a;
        if (rnk[a] == rnk[b]) rnk[a]++;
    }
};

void isolationScan(Graph &g) {
    DSU dsu(g.V);
    for (int u = 0; u < g.V; u++)
        for (auto &e : g.adj[u])
            if (!e.blocked) dsu.unite(u, e.to);

    cout << "\n" << BOLD << "======== ISOLATION SCAN (connectivity with Central HQ) ========" << RESET << "\n\n";
    bool anyIsolated = false;
    for (int i = 1; i < g.V; i++) {
        bool connected = (dsu.find(i) == dsu.find(0));
        cout << "  " << (connected ? GREEN + "[ OK ]  " : RED + "[ CUT ] ") << RESET
             << placeName[i] << (connected ? "" : "  -> ISOLATED! Use air/boat rescue.") << "\n";
        if (!connected) anyIsolated = true;
    }
    if (anyIsolated) cout << "\n  " << RED << "WARNING: some sectors are cut off from HQ." << RESET << "\n";
    else cout << "\n  " << GREEN << "All sectors are connected to Central HQ." << RESET << "\n";
}

// ============================================================
// MODULE 5 : RULE-BASED ACTION PLAN (simple if/else rules)
// ============================================================
void printActionPlan(Emergency &e) {
    const int W = 62;
    cout << "\n" << boxTop(W) << "\n";
    cout << boxLine("ACTION PLAN : " + hazardName(e.type) + " at " + placeName[e.village], W) << "\n";
    cout << boxMid(W) << "\n";
    switch (e.type) {
        case 1:
            cout << boxLine("* Send boats and life jackets, evacuate low-lying areas.", W) << "\n";
            cout << boxLine("* Alert hospital for hypothermia / drowning cases.", W) << "\n"; break;
        case 2:
            cout << boxLine("* Send earth movers and search-and-rescue team.", W) << "\n";
            cout << boxLine("* Keep trauma and fracture care ready at hospital.", W) << "\n"; break;
        case 3:
            cout << boxLine("* Send HAZMAT team, evacuate people upwind.", W) << "\n";
            cout << boxLine("* Hospital must prepare decontamination unit.", W) << "\n"; break;
        default:
            cout << boxLine("* Send fire brigade and water tankers.", W) << "\n";
            cout << boxLine("* Keep burn-care unit ready at hospital.", W) << "\n"; break;
    }
    // rules based on severity and population
    if (e.severity >= 8)       cout << boxLine("* CRITICAL: 2 ambulances + advanced life support.", W) << "\n";
    else if (e.severity >= 5)  cout << boxLine("* SERIOUS: 1 ambulance + paramedic team.", W) << "\n";
    else                       cout << boxLine("* MINOR: basic first-aid unit.", W) << "\n";

    if (e.population > 100)    cout << boxLine("* Large crowd at risk: request extra buses for evacuation.", W) << "\n";
    cout << boxBottom(W) << "\n";
}

// ============================================================
// Dispatch the highest-priority emergency
// ============================================================
void processNext(Graph &g, MaxHeap &heap) {
    if (heap.empty()) { cout << "\nNo pending emergencies.\n"; return; }

    Emergency e = heap.pop();
    cout << "\n" << BOLD << "=== DISPATCHING EMERGENCY ID " << e.id << " (score " << e.score << ") ===" << RESET << "\n";
    cout << severityTag(e.severity) << " " << hazardName(e.type) << " at " << placeName[e.village] << "\n";

    Router r;
    r.run(g, e.village);   // roads are two-way, so distance from village = distance to village

    // find nearest reachable hospital
    int best = -1;
    for (int i = 0; i < g.V; i++)
        if (isHospital[i] && r.dist[i] != INF && (best == -1 || r.dist[i] < r.dist[best]))
            best = i;

    clearRoute();
    if (best == -1) {
        cout << RED << "\nNo hospital can reach this location! Sector is ISOLATED." << RESET << "\n";
        cout << RED << "Recommendation: air lift / boat rescue." << RESET << "\n";
        onRoute[e.village] = true;
    } else {
        cout << "\nNearest reachable hospital: " << CYAN << placeName[best] << RESET << "\n";
        cout << "Route: ";
        // parent[] points towards the village, so this prints hospital -> village
        for (int cur = best; cur != -1; cur = r.parent[cur]) {
            cout << YEL << placeName[cur] << RESET;
            onRoute[cur] = true;
            if (r.parent[cur] != -1) {
                cout << " -> ";
                routeEdge[cur][r.parent[cur]] = routeEdge[r.parent[cur]][cur] = true;
            }
        }
        cout << "\nTotal travel time: " << BOLD << r.dist[best] << " minutes" << RESET << "\n";
    }
    printActionPlan(e);
    drawMap(g);      // map shows the highlighted route
}

// ============================================================
// MAIN : menu driven program
// ============================================================
void showMenu() {
    const int W = 42;
    cout << "\n" << CYAN << boxTop(W) << RESET << "\n";
    cout << CYAN << boxLine("          L I F E L I N E - N E X U S", W) << RESET << "\n";
    cout << CYAN << boxMid(W) << RESET << "\n";
    cout << boxLine("1. Show district map", W) << "\n";
    cout << boxLine("2. Add emergency request", W) << "\n";
    cout << boxLine("3. View pending queue", W) << "\n";
    cout << boxLine("4. Dispatch highest-priority emergency", W) << "\n";
    cout << boxLine("5. Block / unblock a road", W) << "\n";
    cout << boxLine("6. Run isolation scan", W) << "\n";
    cout << boxLine("7. Show road list", W) << "\n";
    cout << boxLine("0. Exit", W) << "\n";
    cout << CYAN << boxBottom(W) << RESET << "\n";
    cout << "Enter choice: ";
}

int main() {
    enableColors();
    clearRoute();
    Graph g(10);
    MaxHeap heap;
    int nextId = 1;

    // build the district map  (u, v, minutes)
    g.addRoad(0, 3, 4);
    g.addRoad(0, 1, 6);
    g.addRoad(3, 1, 3);
    g.addRoad(3, 4, 5);
    g.addRoad(3, 5, 7);
    g.addRoad(4, 6, 6);
    g.addRoad(4, 2, 8);
    g.addRoad(5, 2, 4);
    g.addRoad(5, 7, 5);
    g.addRoad(2, 8, 9);
    g.addRoad(6, 9, 4);
    g.addRoad(7, 9, 6);

    int choice;
    do {
        showMenu();
        cin >> choice;

        if (choice == 1) {
            drawMap(g);
        }
        else if (choice == 2) {
            Emergency e;
            cout << "Villages: 6 Rampur, 7 Shivpur, 8 Hill, 9 Lake (any node 0-9 allowed)\n";
            cout << "Location node: "; cin >> e.village;
            cout << "Hazard (1 Flood, 2 Landslide, 3 Chemical, 4 Fire): "; cin >> e.type;
            cout << "Severity (1-10): "; cin >> e.severity;
            cout << "People at risk: "; cin >> e.population;
            if (e.village < 0 || e.village > 9 || e.type < 1 || e.type > 4 ||
                e.severity < 1 || e.severity > 10 || e.population < 0) {
                cout << RED << "Invalid input!" << RESET << "\n";
                continue;
            }
            e.id = nextId++;
            e.score = e.severity * 10 + e.population;
            heap.push(e);
            cout << GREEN << "Request added with ID " << e.id << ", priority score = " << e.score << RESET << "\n";
        }
        else if (choice == 3) {
            heap.display();
        }
        else if (choice == 4) {
            processNext(g, heap);
        }
        else if (choice == 5) {
            int u, v;
            cout << "Enter the two nodes of the road: "; cin >> u >> v;
            if (u >= 0 && u < 10 && v >= 0 && v < 10 && g.toggleRoad(u, v)) {
                clearRoute();       // old route may no longer be valid
                cout << "Road status toggled.\n";
                drawMap(g);
            } else
                cout << RED << "No such road!" << RESET << "\n";
        }
        else if (choice == 6) {
            isolationScan(g);
        }
        else if (choice == 7) {
            g.printRoads();
        }
    } while (choice != 0);

    cout << "System shut down.\n";
    return 0;
}
