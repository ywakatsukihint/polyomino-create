// polyomino_tree_unique.cpp
// Enumerate free polyominoes of size N that are trees, then deduplicate by tree-isomorphism (AHU).
// C++11 compatible.
//
// Compile:
//   g++ polyomino_tree_unique.cpp -std=gnu++11 -O2 -o polyomino_tree_unique

#include <bits/stdc++.h>
using namespace std;

using Cell  = pair<int,int>;
using Shape = vector<Cell>;

const int DX[4] = {1,-1,0,0};
const int DY[4] = {0,0,1,-1};

int N;

// ---------- polyomino canonical (rotation/reflection) ----------
// normalize (translate so min coords = 0, sort)
Shape normalize_shape(const Shape& s) {
    int minx = INT_MAX, miny = INT_MAX;
    for (size_t i = 0; i < s.size(); ++i) {
        minx = min(minx, s[i].first);
        miny = min(miny, s[i].second);
    }
    Shape r;
    r.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i)
        r.push_back(make_pair(s[i].first - minx, s[i].second - miny));
    sort(r.begin(), r.end());
    return r;
}

vector<Shape> all_transforms(const Shape& s) {
    vector<Shape> res(8);
    for (size_t i = 0; i < s.size(); ++i) {
        int x = s[i].first, y = s[i].second;
        res[0].push_back(make_pair( x,  y));
        res[1].push_back(make_pair( x, -y));
        res[2].push_back(make_pair(-x,  y));
        res[3].push_back(make_pair(-x, -y));
        res[4].push_back(make_pair( y,  x));
        res[5].push_back(make_pair( y, -x));
        res[6].push_back(make_pair(-y,  x));
        res[7].push_back(make_pair(-y, -x));
    }
    for (size_t k = 0; k < res.size(); ++k) res[k] = normalize_shape(res[k]);
    return res;
}

string canonical_shape_key(const Shape& s) {
    auto ts = all_transforms(s);
    string best;
    for (size_t i = 0; i < ts.size(); ++i){
        string cur;
        for (size_t j = 0; j < ts[i].size(); ++j) {
            cur += to_string(ts[i][j].first);
            cur += ",";
            cur += to_string(ts[i][j].second);
            cur += ";";
        }
        if (best.empty() || cur < best) best = cur;
    }
    return best;
}

// ---------- generation of polyomino trees (pruned DFS) ----------
set<string> seen_poly_keys;   // avoid polyomino duplicates (rot/ref)
vector<Shape> generated_shapes; // store normalized representative shapes

// count neighbors of a candidate cell within current set
int neighbor_count(const set<Cell>& cur, const Cell& c){
    int cnt = 0;
    for (int d = 0; d < 4; ++d){
        Cell q = make_pair(c.first + DX[d], c.second + DY[d]);
        if (cur.count(q)) ++cnt;
    }
    return cnt;
}

void dfs_gen(set<Cell>& cur){
    if ((int)cur.size() == N){
        Shape s(cur.begin(), cur.end());
        Shape norm = normalize_shape(s);
        string key = canonical_shape_key(norm);
        if (!seen_poly_keys.count(key)){
            seen_poly_keys.insert(key);
            generated_shapes.push_back(norm);
        }
        return;
    }

    // frontier: empty neighbors of cur
    set<Cell> frontier;
    for (auto it = cur.begin(); it != cur.end(); ++it){
        Cell p = *it;
        for (int d = 0; d < 4; ++d){
            Cell q = make_pair(p.first + DX[d], p.second + DY[d]);
            if (!cur.count(q)) frontier.insert(q);
        }
    }

    // For tree-only: allow only frontier cells that touch cur in exactly 1 cell
    for (auto it = frontier.begin(); it != frontier.end(); ++it){
        Cell q = *it;
        int nc = neighbor_count(cur, q);
        if (nc != 1) continue; // prune: would create cycle or be disconnected
        cur.insert(q);
        dfs_gen(cur);
        cur.erase(q);
    }
}

// ---------- convert shape -> adjacency list (graph of cells) ----------
vector<vector<int>> shape_to_adj(const Shape& s){
    int n = s.size();
    unordered_map<long long,int> mp; mp.reserve(n*2);
    for (int i = 0; i < n; ++i){
        long long h = ( ( (long long)s[i].first ) << 32 ) ^ (unsigned long long)(s[i].second & 0xffffffffULL);
        mp[h] = i;
    }
    vector<vector<int>> adj(n);
    for (int i = 0; i < n; ++i){
        for (int d = 0; d < 4; ++d){
            int nx = s[i].first + DX[d];
            int ny = s[i].second + DY[d];
            long long h = ( ( (long long)nx ) << 32 ) ^ (unsigned long long)(ny & 0xffffffffULL);
            auto it = mp.find(h);
            if (it != mp.end()){
                int j = it->second;
                adj[i].push_back(j);
            }
        }
    }
    return adj;
}

// ---------- AHU canonical for trees (rooted recursive code) ----------
string rooted_code(const vector<vector<int>>& adj, int v, int parent){
    vector<string> subs;
    for (size_t k = 0; k < adj[v].size(); ++k){
        int u = adj[v][k];
        if (u == parent) continue;
        subs.push_back(rooted_code(adj, u, v));
    }
    sort(subs.begin(), subs.end());
    string s = "(";
    for (size_t i = 0; i < subs.size(); ++i) s += subs[i];
    s += ")";
    return s;
}

// centers of a tree (1 or 2) by leaf peeling
vector<int> tree_centers(const vector<vector<int>>& adj){
    int n = adj.size();
    if (n == 0) return vector<int>();
    if (n == 1) return vector<int>(1,0);
    vector<int> deg(n);
    queue<int> q;
    for (int i = 0; i < n; ++i){
        deg[i] = adj[i].size();
        if (deg[i] <= 1) q.push(i);
    }
    int removed = 0;
    vector<int> last;
    while (!q.empty()){
        int sz = q.size();
        last.clear();
        for (int i = 0; i < sz; ++i){
            int v = q.front(); q.pop();
            last.push_back(v);
            ++removed;
            for (size_t k = 0; k < adj[v].size(); ++k){
                int u = adj[v][k];
                if (--deg[u] == 1) q.push(u);
            }
        }
        if (removed == n) break;
    }
    return last; // 1 or 2 vertices
}

// unrooted canonical code
string unrooted_tree_canonical(const vector<vector<int>>& adj){
    vector<int> centers = tree_centers(adj);
    string best;
    for (size_t i = 0; i < centers.size(); ++i){
        string c = rooted_code(adj, centers[i], -1);
        if (best.empty() || c < best) best = c;
    }
    return best;
}

// ---------- printing shape ---------- 
void print_shape(const Shape& s){
    if (s.empty()) return;
    int minx = INT_MAX, miny = INT_MAX, maxx = INT_MIN, maxy = INT_MIN;
    for (size_t i = 0; i < s.size(); ++i){
        minx = min(minx, s[i].first);
        miny = min(miny, s[i].second);
        maxx = max(maxx, s[i].first);
        maxy = max(maxy, s[i].second);
    }
    int W = maxx - minx + 1;
    int H = maxy - miny + 1;
    vector<string> grid(H, string(W, '.'));
    for (size_t i = 0; i < s.size(); ++i){
        int x = s[i].first - minx;
        int y = s[i].second - miny;
        int row = maxy - s[i].second; // invert y for printing
        int col = x;
        grid[row][col] = '#';
    }
    for (int r = 0; r < H; ++r) cout << grid[r] << "\n";
}

// ---------- MAIN ----------
int main(){
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    cout << "N = ";
    if (!(cin >> N)) return 0;
    if (N <= 0) return 0;

    // special-case N==1
    if (N == 1){
        cout << "Tree-Polyomino #1\n#\nUnique-trees = 1\n";
        return 0;
    }

    // 1) generate polyomino shapes that are trees (pruned)
    set<Cell> start;
    start.insert(make_pair(0,0));
    dfs_gen(start);

    // 2) for each generated shape compute tree canonical code and deduplicate by code
    unordered_map<string, Shape> canonical_tree_to_shape; // map tree-code -> representative shape
    canonical_tree_to_shape.reserve(generated_shapes.size()*2);
    for (size_t i = 0; i < generated_shapes.size(); ++i){
        Shape &s = generated_shapes[i];
        auto adj = shape_to_adj(s);
        // sanity: check adjacency forms a tree: connected and edges == n-1
        int n = adj.size();
        int edgecount = 0;
        for (int v = 0; v < n; ++v) edgecount += adj[v].size();
        edgecount /= 2;
        if (edgecount != n-1) continue; // not a tree (shouldn't happen)
        // connected: BFS
        vector<char> seen(n, 0);
        queue<int> q; q.push(0); seen[0]=1; int seencnt=1;
        while(!q.empty()){
            int v = q.front(); q.pop();
            for(size_t k=0;k<adj[v].size();++k){
                int u = adj[v][k];
                if(!seen[u]){ seen[u]=1; q.push(u); ++seencnt; }
            }
        }
        if (seencnt != n) continue; // not connected (shouldn't happen)

        string treecode = unrooted_tree_canonical(adj);
        if (!canonical_tree_to_shape.count(treecode)){
            canonical_tree_to_shape[treecode] = s; // store representative embedding
        }
    }

    // 3) output unique trees (one representative shape each)
    cout << "\nUnique tree structures (count = " << canonical_tree_to_shape.size() << ")\n\n";
    int idx = 0;
    for (auto &kv : canonical_tree_to_shape){
        ++idx;
        cout << "Tree #" << idx << " : canonical_code = " << kv.first << "\n";
        print_shape(kv.second);
        cout << "\n";
    }

    cout << "Finished. Unique tree count = " << canonical_tree_to_shape.size() << "\n";
    return 0;
}