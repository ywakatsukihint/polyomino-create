// trees_deg4.cpp
// Count & enumerate unlabeled unrooted trees with max degree <= 4.
// C++11 compatible, copy-paste & compile with: g++ trees_deg4.cpp -std=gnu++11 -O2 -o trees

#include <bits/stdc++.h>
using namespace std;

using StrVec = vector<string>;
using IntVec = vector<int>;
using Matrix = vector<vector<int>>;

// --- utilities: integer partitions of S into k parts (nondecreasing) ---
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

// --- cartesian product of choices, produce joined child-strings (sorted) ---
void cartesian_build(const vector<StrVec>& choices, int idx, vector<string>& cur, set<string>& out){
    int m = choices.size();
    if(idx == m){
        // sort children (unordered tree canonicalization)
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

// --- Build rooted-tree canonical forms by DP ---
// rooted3[n] : rooted trees of n nodes where root's max children <= 3 (used for non-root nodes)
// rooted4[n] : rooted trees of n nodes where root's max children <= 4 (used for root of full tree)
void build_rooted_forms(int N, vector<StrVec>& rooted3, vector<StrVec>& rooted4){
    rooted3.assign(N+1, StrVec());
    rooted4.assign(N+1, StrVec());
    rooted3[1].push_back(string("()"));
    rooted4[1].push_back(string("()"));

    for(int n = 2; n <= N; ++n){
        // build rooted3[n] (root max children = 3)
        {
            set<string> res;
            for(int k = 1; k <= 3; ++k){ // number of children
                if(k > n-1) break;
                auto parts = partitions(n-1, k); // split n-1 into k subtree sizes
                for(auto &parts_vec : parts){
                    // prepare choices: for each part size s, choices are rooted3[s]
                    vector<StrVec> choices;
                    bool ok = true;
                    for(int s : parts_vec){
                        if(s <= 0 || s > N || rooted3[s].empty()){ ok = false; break; }
                        choices.push_back(rooted3[s]);
                    }
                    if(!ok) continue;
                    // build all combinations, canonicalize by sorting children strings
                    vector<string> cur;
                    cartesian_build(choices, 0, cur, res);
                }
            }
            // move set->vector
            rooted3[n].assign(res.begin(), res.end());
        }
        // build rooted4[n] (root max children = 4)
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
                        choices.push_back(rooted3[s]); // child subtrees are non-root nodes => use rooted3
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

// --- parse rooted canonical string "(()())" into adjacency list ---
// returns adjacency list (0..n-1), node count
void parse_rooted_string_to_adj(const string& s, vector<vector<int>>& adj){
    adj.clear();
    vector<int> stack_nodes;
    int id_counter = 0;
    for(size_t i = 0; i < s.size(); ++i){
        char c = s[i];
        if(c == '('){
            // create node
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
        } else {
            // canonical strings here contain no extra chars because we used only concatenated () forms
        }
    }
    // adjacency built
}

// --- compute centers of an undirected tree (peeling leaves) ---
vector<int> tree_centers(const vector<vector<int>>& adj){
    int n = adj.size();
    if(n == 0) return vector<int>();
    if(n == 1) return vector<int>(1, 0);
    vector<int> deg(n);
    queue<int> q;
    for(int i = 0; i < n; ++i){
        deg[i] = adj[i].size();
        if(deg[i] == 1) q.push(i);
    }
    int removed = 0;
    vector<int> last;
    while(!q.empty()){
        int sz = q.size();
        last.clear();
        for(int t = 0; t < sz; ++t){
            int v = q.front(); q.pop();
            last.push_back(v);
            removed++;
            for(int nb : adj[v]){
                if(--deg[nb] == 1) q.push(nb);
            }
        }
        if(removed == n) break;
    }
    // last contains 1 or 2 centers
    return last;
}

// --- AHU rooted-canonical coding for an arbitrary root (recursive) ---
string rooted_code_from_adj(const vector<vector<int>>& adj, int root, int parent){
    vector<string> child_codes;
    for(int nb : adj[root]){
        if(nb == parent) continue;
        child_codes.push_back(rooted_code_from_adj(adj, nb, root));
    }
    sort(child_codes.begin(), child_codes.end());
    string s = "(";
    for(size_t i = 0; i < child_codes.size(); ++i) s += child_codes[i];
    s += ")";
    return s;
}

// --- build unrooted canonical form from adjacency (take minimal among center-rooted codes) ---
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

// --- adjacency matrix of tree (n x n) from adj list ---
Matrix adjacency_matrix_from_adj(const vector<vector<int>>& adj){
    int n = adj.size();
    Matrix A(n, vector<int>(n, 0));
    for(int i = 0; i < n; ++i){
        for(int j : adj[i]) A[i][j] = 1;
    }
    return A;
}

// --- pretty print matrix ---
void print_matrix(const Matrix& A){
    int n = A.size();
    for(int i = 0; i < n; ++i){
        for(int j = 0; j < n; ++j){
            cout << A[i][j];
            if(j+1 < n) cout << " ";
        }
        cout << "\n";
    }
}

int main(){
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int N;
    cout << "Enter number of vertices n (suggest n <= 16): ";
    if(!(cin >> N)) return 0;
    if(N <= 0){
        cout << "n must be positive\n";
        return 0;
    }

    // 1) build rooted canonical forms up to N
    vector<StrVec> rooted3, rooted4;
    build_rooted_forms(N, rooted3, rooted4);

    // quick check: if no rooted4[N] then impossible (should not happen)
    if(rooted4[N].empty()){
        cout << "No rooted forms produced for n = " << N << "\n";
        return 0;
    }

    // 2) convert each rooted4 form to adjacency, compute unrooted canonical, deduplicate
    set<string> unrooted_set;                 // store canonical codes
    vector<string> unrooted_codes_inorder;    // keep order for listing
    vector<vector<vector<int>>> unrooted_adjs; // corresponding adjacency lists

    for(size_t idx = 0; idx < rooted4[N].size(); ++idx){
        const string& rstr = rooted4[N][idx];
        vector<vector<int>> adj;
        parse_rooted_string_to_adj(rstr, adj);
        // double-check size
        if((int)adj.size() != N) continue; // safety

        // get canonical unrooted code
        string unc = unrooted_canonical_from_adj(adj);

        if(!unrooted_set.count(unc)){
            unrooted_set.insert(unc);
            unrooted_codes_inorder.push_back(unc);
            unrooted_adjs.push_back(adj);
        }
    }

    // 3) output
    int count = unrooted_set.size();
    cout << "\nNumber of unlabeled unrooted trees with n=" << N << " and max degree <= 4 : " << count << "\n\n";

    for(size_t i = 0; i < unrooted_codes_inorder.size(); ++i){
        cout << "Tree #" << (i+1) << " : canonical = " << unrooted_codes_inorder[i] << "\n";
        cout << "Adjacency list (0.." << (N-1) << "):\n";
        for(int v = 0; v < N; ++v){
            cout << v << ":";
            for(size_t t = 0; t < unrooted_adjs[i][v].size(); ++t){
                cout << (t==0 ? " " : ",") << unrooted_adjs[i][v][t];
            }
            cout << "\n";
        }
        cout << "Adjacency matrix:\n";
        Matrix A = adjacency_matrix_from_adj(unrooted_adjs[i]);
        print_matrix(A);
        cout << "\n";
    }

    return 0;
}