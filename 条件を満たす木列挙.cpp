// enumerate_matrices_from_trees.cpp
// Enumerate 0/1 matrices satisfying:
//  - r + c = n (rows + cols = n), total ones = n-1
//  - each row-sum and each column-sum in [1,4]
//  - the set of ones forms a connected structure under "share row or share column"
//    (equivalently the bipartite graph with r row-vertices and c col-vertices is a tree)
//  - rows and columns permuted independently are considered identical
//
// Approach:
//  - Enumerate unlabeled unrooted trees with up to degree 4 (AHU / rooted-DP generation)
//  - For each tree (size n): color bipartition (unique up to swap) giving sizes (a,b)
//  - For each orientation matching some (r,c) with r + c = n, build r x c matrix
//  - Canonicalize matrix under independent row/column permutations (simple refinement sort)
//  - Deduplicate by canonical string and print representatives.
//
// Note: practical for n up to ~11..12; exponential growth is unavoidable.
//
// Compile: g++ enumerate_matrices_from_trees.cpp -std=gnu++11 -O2 -o enumerate

#include <bits/stdc++.h>
using namespace std;

using StrVec = vector<string>;
using Cell = pair<int,int>;
using Shape = vector<Cell>;

const int DX[4] = {1, -1, 0, 0};
const int DY[4] = {0, 0, 1, -1};

// ---------- helper: integer partitions (used for rooted DP generation) ----------
void partitions_rec(int rem, int k, int minv, vector<int>& cur, vector<vector<int>>& out){
    if(k == 0){
        if(rem == 0) out.push_back(cur);
        return;
    }
    for(int v = minv; v <= rem; ++v){
        cur.push_back(v);
        partitions_rec(rem - v, k - 1, v, cur, out);
        cur.pop_back();
    }
}
vector<vector<int>> partitions(int sum, int k){
    vector<vector<int>> out;
    vector<int> cur;
    if(k <= 0) return out;
    partitions_rec(sum, k, 1, cur, out);
    return out;
}

// ---------- cartesian build for combining child-codes ----------
void cartesian_build(const vector<StrVec>& choices, int idx, vector<string>& cur, set<string>& out){
    int m = (int)choices.size();
    if(idx == m){
        vector<string> tmp = cur;
        sort(tmp.begin(), tmp.end());
        string s = "(";
        for(size_t i = 0; i < tmp.size(); ++i) s += tmp[i];
        s += ")";
        out.insert(s);
        return;
    }
    for(size_t j = 0; j < choices[idx].size(); ++j){
        cur.push_back(choices[idx][j]);
        cartesian_build(choices, idx + 1, cur, out);
        cur.pop_back();
    }
}

// ---------- build rooted canonical forms by DP (degree constraints) ----------
void build_rooted_forms(int N, vector<StrVec>& rooted3, vector<StrVec>& rooted4){
    rooted3.assign(N+1, StrVec());
    rooted4.assign(N+1, StrVec());
    rooted3[1].push_back(string("()"));
    rooted4[1].push_back(string("()"));

    for(int n = 2; n <= N; ++n){
        // rooted3[n] : root can have up to 3 children (used for non-root nodes)
        {
            set<string> res;
            for(int k = 1; k <= 3; ++k){
                if(k > n-1) break;
                auto parts = partitions(n-1, k);
                for(auto &parts_vec : parts){
                    vector<StrVec> choices;
                    bool ok = true;
                    for(int s : parts_vec){
                        if(s <= 0 || s > N || rooted3[s].empty()){ ok = false; break; }
                        choices.push_back(rooted3[s]);
                    }
                    if(!ok) continue;
                    vector<string> cur;
                    cartesian_build(choices, 0, cur, res);
                }
            }
            rooted3[n].assign(res.begin(), res.end());
        }
        // rooted4[n] : root can have up to 4 children (used for overall root)
        {
            set<string> res;
            for(int k = 1; k <= 4; ++k){
                if(k > n-1) break;
                auto parts = partitions(n-1, k);
                for(auto &parts_vec : parts){
                    vector<StrVec> choices;
                    bool ok = true;
                    for(int s : parts_vec){
                        if(s <= 0 || s > N || rooted3[s].empty()){ ok = false; break; }
                        // child-subtrees for root must be shapes that could appear as non-root subtrees
                        choices.push_back(rooted3[s]);
                    }
                    if(!ok) continue;
                    vector<string> cur;
                    cartesian_build(choices, 0, cur, res);
                }
            }
            rooted4[n].assign(res.begin(), res.end());
        }
    }
}

// ---------- parse rooted canonical string like "(()())" to adjacency list ----------
void parse_rooted_string_to_adj(const string& s, vector<vector<int>>& adj){
    adj.clear();
    vector<int> stack_nodes;
    int id_counter = 0;
    for(size_t i = 0; i < s.size(); ++i){
        char c = s[i];
        if(c == '('){
            adj.push_back(vector<int>());
            int cur = id_counter++;
            if(!stack_nodes.empty()){
                int parent = stack_nodes.back();
                adj[parent].push_back(cur);
                adj[cur].push_back(parent);
            }
            stack_nodes.push_back(cur);
        } else if(c == ')'){
            if(!stack_nodes.empty()) stack_nodes.pop_back();
        }
    }
}

// ---------- tree centers (leaf peeling) ----------
vector<int> tree_centers(const vector<vector<int> >& adj){
    int n = (int)adj.size();
    if(n == 0) return vector<int>();
    if(n == 1) return vector<int>(1,0);
    vector<int> deg(n);
    queue<int> q;
    for(int i = 0; i < n; ++i){
        deg[i] = (int)adj[i].size();
        if(deg[i] == 1) q.push(i);
    }
    int removed = 0;
    vector<int> last;
    while(!q.empty()){
        int sz = (int)q.size();
        last.clear();
        for(int i = 0; i < sz; ++i){
            int v = q.front(); q.pop();
            last.push_back(v);
            ++removed;
            for(size_t k = 0; k < adj[v].size(); ++k){
                int u = adj[v][k];
                if(--deg[u] == 1) q.push(u);
            }
        }
        if(removed == n) break;
    }
    return last; // 1 or 2 centers
}

// ---------- AHU rooted code ----------
string rooted_code_from_adj(const vector<vector<int>>& adj, int root, int parent){
    vector<string> child_codes;
    for(size_t i = 0; i < adj[root].size(); ++i){
        int nb = adj[root][i];
        if(nb == parent) continue;
        child_codes.push_back(rooted_code_from_adj(adj, nb, root));
    }
    sort(child_codes.begin(), child_codes.end());
    string s = "(";
    for(size_t i = 0; i < child_codes.size(); ++i) s += child_codes[i];
    s += ")";
    return s;
}
string unrooted_canonical_from_adj(const vector<vector<int>>& adj){
    vector<int> centers = tree_centers(adj);
    string best;
    for(size_t i = 0; i < centers.size(); ++i){
        int c = centers[i];
        string code = rooted_code_from_adj(adj, c, -1);
        if(best.empty() || code < best) best = code;
    }
    return best;
}

// ---------- convert parsed adjacency to ensure it's a valid tree (sanity) ----------
bool is_tree_and_degree_ok(const vector<vector<int>>& adj){
    int n = (int)adj.size();
    if(n == 0) return false;
    // check degrees in [1,4]
    for(int i = 0; i < n; ++i){
        int d = adj[i].size();
        if(d < 1 || d > 4) return false;
    }
    // edge count = n-1
    int edgecount = 0;
    for(int i = 0; i < n; ++i) edgecount += adj[i].size();
    edgecount /= 2;
    if(edgecount != n-1) return false;
    // connectivity BFS
    vector<char> seen(n, 0);
    queue<int> q; q.push(0); seen[0]=1; int seen_cnt=1;
    while(!q.empty()){
        int v = q.front(); q.pop();
        for(int nb : adj[v]) if(!seen[nb]){ seen[nb]=1; q.push(nb); ++seen_cnt; }
    }
    return seen_cnt == n;
}

// ---------- bipartition coloring (0/1) of a tree ----------
vector<int> bipartition_colors(const vector<vector<int>>& adj){
    int n = (int)adj.size();
    vector<int> color(n, -1);
    queue<int> q;
    color[0] = 0; q.push(0);
    while(!q.empty()){
        int v = q.front(); q.pop();
        for(int nb : adj[v]){
            if(color[nb] == -1){ color[nb] = color[v]^1; q.push(nb); }
            // else already colored and consistent because it's a tree
        }
    }
    return color;
}

// ---------- build matrix (r x c) for given bipartition orientation ----------------
// rows correspond to nodes with color = target_row_color
vector<vector<int>> build_matrix_from_adj_and_color(const vector<vector<int>>& adj, const vector<int>& color, int target_row_color){
    int n = adj.size();
    vector<int> row_ids, col_ids;
    for(int i = 0; i < n; ++i){
        if(color[i] == target_row_color) row_ids.push_back(i);
        else col_ids.push_back(i);
    }
    int r = row_ids.size(), c = col_ids.size();
    vector<int> col_index(n, -1);
    for(int j = 0; j < c; ++j) col_index[col_ids[j]] = j;
    vector<int> row_index(n, -1);
    for(int i = 0; i < r; ++i) row_index[row_ids[i]] = i;
    vector<vector<int>> M(r, vector<int>(c, 0));
    for(int i = 0; i < r; ++i){
        int vid = row_ids[i];
        for(int nb : adj[vid]){
            int j = col_index[nb];
            if(j >= 0) M[i][j] = 1;
        }
    }
    return M;
}

// ---------- canonicalize matrix under independent row/col permutations (iterative refinement) ----------
// returns a string key for deduplication
string canonicalize_matrix(vector<vector<int>> M){
    int r = M.size();
    int c = M.empty() ? 0 : M[0].size();
    // represent rows as strings, columns similarly; iteratively sort rows/cols to refine
    // This is deterministic and sufficient for our use (trees are already unique up to isom.)
    int max_iter = 6;
    vector<string> rowstr(r), colstr(c);
    for(int it = 0; it < max_iter; ++it){
        // build row strings
        for(int i = 0; i < r; ++i){
            string s;
            s.reserve(c);
            for(int j = 0; j < c; ++j) s.push_back(M[i][j] ? '1' : '0');
            rowstr[i] = s;
        }
        // sort row indices by rowstr then stabilize M (permute rows)
        vector<int> rid(r); iota(rid.begin(), rid.end(), 0);
        sort(rid.begin(), rid.end(), [&](int a, int b){
            if(rowstr[a] != rowstr[b]) return rowstr[a] < rowstr[b];
            return a < b;
        });
        vector<vector<int>> M2(r, vector<int>(c));
        for(int i = 0; i < r; ++i) M2[i] = M[rid[i]];
        M.swap(M2);

        // build col strings
        for(int j = 0; j < c; ++j){
            string s; s.reserve(r);
            for(int i = 0; i < r; ++i) s.push_back(M[i][j] ? '1' : '0');
            colstr[j] = s;
        }
        // sort columns by colstr
        vector<int> cid(c); iota(cid.begin(), cid.end(), 0);
        sort(cid.begin(), cid.end(), [&](int a, int b){
            if(colstr[a] != colstr[b]) return colstr[a] < colstr[b];
            return a < b;
        });
        vector<vector<int>> M3(r, vector<int>(c));
        for(int i = 0; i < r; ++i){
            for(int j = 0; j < c; ++j) M3[i][j] = M[i][cid[j]];
        }
        M.swap(M3);
    }
    // final string key
    string key;
    key.reserve(r * (c+1) + 16);
    key += to_string(r); key.push_back('x'); key += to_string(c); key.push_back('|');
    for(int i = 0; i < r; ++i){
        for(int j = 0; j < c; ++j) key.push_back(M[i][j] ? '1' : '0');
        key.push_back('/');
    }
    return key;
}

// ---------- print matrix ----------
void print_matrix(const vector<vector<int>>& M){
    int r = M.size();
    int c = M.empty() ? 0 : M[0].size();
    for(int i = 0; i < r; ++i){
        for(int j = 0; j < c; ++j){
            cout << M[i][j] << (j+1 < c ? ' ' : '\n');
        }
    }
}

// ---------- MAIN ----------
int main(){
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int n;
    cout << "Enter n (rows + cols = n): ";
    if(!(cin >> n)) return 0;
    if(n <= 1){
        cout << "n must be >= 2\n";
        return 0;
    }

    // 1) Build rooted canonical forms up to size n
    vector<StrVec> rooted3, rooted4;
    build_rooted_forms(n, rooted3, rooted4);
    if(rooted4[n].empty()){
        cout << "No rooted-forms produced for n = " << n << "\n";
        return 0;
    }

    // 2) Convert rooted forms -> adjacency -> unrooted canonical -> deduplicate to get unique unlabeled trees (degree<=4)
    set<string> unrooted_set;
    vector<vector<vector<int>>> unrooted_adjs;
    for(size_t i = 0; i < rooted4[n].size(); ++i){
        const string& rstr = rooted4[n][i];
        vector<vector<int>> adj;
        parse_rooted_string_to_adj(rstr, adj);
        if((int)adj.size() != n) continue;
        if(!is_tree_and_degree_ok(adj)) continue;
        string unc = unrooted_canonical_from_adj(adj);
        if(!unrooted_set.count(unc)){
            unrooted_set.insert(unc);
            unrooted_adjs.push_back(adj);
        }
    }

    cout << "Found " << unrooted_adjs.size() << " unlabeled trees (degree<=4) of size n=" << n << "\n";

    // 3) For each tree, get bipartition sizes and build matrices for orientations
    set<string> unique_matrix_keys;
    vector<pair<pair<int,int>, vector<vector<int>>>> results; // ((r,c), matrix)
    for(auto &adj : unrooted_adjs){
        // bipartition coloring
        vector<int> color = bipartition_colors(adj);
        int cnt0 = 0, cnt1 = 0;
        for(int v : color) if(v == 0) ++cnt0; else ++cnt1;

        // two orientations: rows=color0 / cols=color1  AND rows=color1 / cols=color0
        vector<pair<int,int>> orientations;
        orientations.push_back({cnt0, cnt1});
        if(cnt0 != cnt1) orientations.push_back({cnt1, cnt0});

        for(auto &oc : orientations){
            int r = oc.first, c = oc.second;
            // build matrix where rows = nodes with color == (r==cnt0 ? 0 : 1)
            int target_row_color = (r == cnt0 ? 0 : 1);
            auto M = build_matrix_from_adj_and_color(adj, color, target_row_color);
            // check row/col degree constraints (1..4)
            bool ok = true;
            for(int i = 0; i < (int)M.size(); ++i){
                int s = 0; for(int j = 0; j < (int)M[i].size(); ++j) s += M[i][j];
                if(s < 1 || s > 4){ ok = false; break; }
            }
            if(!ok) continue;
            for(int j = 0; j < (int)M[0].size(); ++j){
                int s = 0; for(int i = 0; i < (int)M.size(); ++i) s += M[i][j];
                if(s < 1 || s > 4){ ok = false; break; }
            }
            if(!ok) continue;

            // canonicalize and deduplicate under independent row/col permutations
            string key = canonicalize_matrix(M);
            if(!unique_matrix_keys.count(key)){
                unique_matrix_keys.insert(key);
                results.push_back({{r,c}, M});
            }
        }
    }

    // 4) Print results grouped by (r,c)
    // sort results by r then c then matrix key
    sort(results.begin(), results.end(), [](const pair<pair<int,int>, vector<vector<int>>>& A,
                                           const pair<pair<int,int>, vector<vector<int>>>& B){
        if(A.first.first != B.first.first) return A.first.first < B.first.first;
        if(A.first.second != B.first.second) return A.first.second < B.first.second;
        // fallback: compare flattened matrix
        const auto &MA = A.second, &MB = B.second;
        int ra = MA.size(), rb = MB.size();
        if(ra != rb) return ra < rb;
        int ca = (ra>0?MA[0].size():0), cb = (rb>0?MB[0].size():0);
        if(ca != cb) return ca < cb;
        for(int i=0;i<ra;i++){
            for(int j=0;j<ca;j++){
                if(MA[i][j] != MB[i][j]) return MA[i][j] < MB[i][j];
            }
        }
        return false;
    });

    cout << "\nEnumerated 0/1 matrices (unique under independent row/column permutations):\n";
    int idx = 0;
    for(auto &pr : results){
        ++idx;
        int r = pr.first.first, c = pr.first.second;
        cout << "\nMatrix #" << idx << " (" << r << " x " << c << "):\n";
        print_matrix(pr.second);
    }
    cout << "\nTotal unique matrices: " << results.size() << "\n";
    return 0;
}