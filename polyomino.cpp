#include <bits/stdc++.h>
using namespace std;

/* =====================
   型定義
   ===================== */

using Cell  = pair<int,int>;
using Shape = vector<Cell>;

int N;
set<string> canonical_set;
vector<Shape> all_shapes;

const int DX[4] = {1,-1,0,0};
const int DY[4] = {0,0,1,-1};

/* =====================
   正規化・同型判定
   ===================== */

Shape normalize(const Shape& s) {
    int minx = INT_MAX, miny = INT_MAX;
    for (auto p : s) {
        minx = min(minx, p.first);
        miny = min(miny, p.second);
    }
    Shape res;
    for (auto p : s)
        res.push_back({p.first - minx, p.second - miny});
    sort(res.begin(), res.end());
    return res;
}

vector<Shape> transforms(const Shape& s) {
    vector<Shape> res(8);
    for (auto p : s) {
        int x = p.first, y = p.second;
        res[0].push_back({ x,  y});
        res[1].push_back({ x, -y});
        res[2].push_back({-x,  y});
        res[3].push_back({-x, -y});
        res[4].push_back({ y,  x});
        res[5].push_back({ y, -x});
        res[6].push_back({-y,  x});
        res[7].push_back({-y, -x});
    }
    for (auto& v : res)
        v = normalize(v);
    return res;
}

string canonical(const Shape& s) {
    auto ts = transforms(s);
    string best;
    for (auto& t : ts) {
        string cur;
        for (auto p : t)
            cur += to_string(p.first) + "," + to_string(p.second) + ";";
        if (best.empty() || cur < best)
            best = cur;
    }
    return best;
}

/* =====================
   Polyomino 生成
   ===================== */

void dfs(set<Cell>& cur) {
    if ((int)cur.size() == N) {
        Shape s(cur.begin(), cur.end());
        string key = canonical(s);
        if (!canonical_set.count(key)) {
            canonical_set.insert(key);
            all_shapes.push_back(normalize(s));
        }
        return;
    }

    set<Cell> frontier;
    for (auto p : cur)
        for (int d = 0; d < 4; d++) {
            Cell q = {p.first + DX[d], p.second + DY[d]};
            if (!cur.count(q))
                frontier.insert(q);
        }

    for (auto c : frontier) {
        cur.insert(c);
        dfs(cur);
        cur.erase(c);
    }
}

/* =====================
   二部グラフ用 隣接行列
   ===================== */

vector<vector<int>> bipartite_adjacency_matrix(const Shape& s) {
    int n = s.size();

    // 市松模様で 2 彩色
    vector<int> color(n); // 0 or 1
    for (int i = 0; i < n; i++) {
        int x = s[i].first;
        int y = s[i].second;
        color[i] = (x + y) & 1;
    }

    // 並べ替え順
    vector<int> order;
    for (int c = 0; c <= 1; c++)
        for (int i = 0; i < n; i++)
            if (color[i] == c)
                order.push_back(i);

    // 隣接行列
    vector<vector<int>> A(n, vector<int>(n, 0));
    for (int ii = 0; ii < n; ii++) {
        for (int jj = ii + 1; jj < n; jj++) {
            int i = order[ii];
            int j = order[jj];
            int dx = abs(s[i].first  - s[j].first);
            int dy = abs(s[i].second - s[j].second);
            if (dx + dy == 1) {
                A[ii][jj] = A[jj][ii] = 1;
            }
        }
    }
    return A;
}

/* =====================
   出力
   ===================== */

void print_shape(const Shape& s) {
    int mx = 0, my = 0;
    set<Cell> st(s.begin(), s.end());
    for (auto p : s) {
        mx = max(mx, p.first);
        my = max(my, p.second);
    }

    for (int y = my; y >= 0; y--) {
        for (int x = 0; x <= mx; x++)
            cout << (st.count({x,y}) ? '#' : '.');
        cout << "\n";
    }
}

void print_matrix(const vector<vector<int>>& A) {
    for (auto& row : A) {
        for (int v : row)
            cout << v << " ";
        cout << "\n";
    }
}

/* =====================
   main
   ===================== */

int main() {
    cout << "N = ";
    cin >> N;

    set<Cell> start;
    start.insert({0,0});
    dfs(start);

    for (int i = 0; i < (int)all_shapes.size(); i++) {
        cout << "Polyomino #" << i+1 << "\n";
        print_shape(all_shapes[i]);

        cout << "Bipartite Adjacency Matrix:\n";
        auto A = bipartite_adjacency_matrix(all_shapes[i]);
        print_matrix(A);
        cout << "\n";
    }

    cout << "|P_n| = " << all_shapes.size() << "\n";
    return 0;
}
