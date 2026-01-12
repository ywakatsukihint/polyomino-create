function isOmino(matrix, num) {
    const rows = matrix.length,
          cols = matrix[0].length,
          visited = Array.from({ length: rows }, () => Array(cols).fill(false));

    let onesCount = num,
        start = null;

    // 開始点を探す
    findStart1:
        for (let j = 0; j < cols; j++) {
            if (matrix[0][j] === 1) {
                start = [0, j];
                break findStart1;
            }
        }

    if (!start) return false;  // 1 がない場合

    // BFS で連結成分をたどる
    const queue = [start];
    visited[start[0]][start[1]] = true;
    let connectedCount = 1;

    const directions = [[0, 1], [1, 0], [0, -1], [-1, 0]];

    while (queue.length > 0) {
        const [x, y] = queue.shift();
        for (const [dx, dy] of directions) {
            const nx = x + dx,
                  ny = y + dy;
            if (nx >= 0 && ny >= 0 && nx < rows && ny < cols && matrix[nx][ny] === 1 && !visited[nx][ny]) {
                visited[nx][ny] = true;
                queue.push([nx, ny]);
                connectedCount++;
            }
        }
    }

    return connectedCount === onesCount;
}

function generateValidMatrices(n) {
    const total = n * n,
         result = [];

    // n個のインデックスを選ぶ再帰関数
    function combine(start, selected) {
        if (selected.length === n) {
            // 行列を構築
            const mat = Array.from({ length: n }, () => Array(n).fill(0));
            for (const idx of selected) {
                const row = Math.floor(idx / n),
                      col = idx % n;
                mat[row][col] = 1;
            }

            // 第1行と第1列に1が含まれているかチェック
            const row0HasOne = mat[0].some(v => v === 1),
                  col0HasOne = mat.some(row => row[0] === 1);

            if (row0HasOne && col0HasOne) {
                result.push(mat);
            }
            return;
        }

        for (let i = start; i < total; i++) {
            combine(i + 1, [...selected, i]);
        }
    }

    combine(0, []);
    return result;
}

function CREATE_Ominos(num) {
    let Omino = generateValidMatrices(num),
        NOminos = [];

    for (const item of Omino) {
        if (isOmino(item, num)) {
            NOminos.push(item);
        }
    }

    return NOminos;
}

// 回転・反転・左上寄せ（サイズ固定）で同型をまとめるユーティリティ

// --- 基本操作 ----------------------------------------------------------------
const rotate90 = (m) => {
  const R = m.length, C = m[0].length;
  // 回転後は C x R
  return Array.from({ length: C }, (_, i) =>
    Array.from({ length: R }, (_, j) => m[R - 1 - j][i])
  );
};

const flipH = (m) => m.map(row => [...row].reverse()); // 左右反転
const flipV = (m) => [...m].map(row => [...row]).reverse(); // 上下反転

// --- 左上に寄せて、指定サイズに戻す（サイズ固定） ----------------------------
function normalizeToSize(src, targetR, targetC) {
  const R = src.length, C = src[0].length;
  // 最小の "1"（非0）位置を探す
  let minR = Infinity, minC = Infinity;
  for (let i = 0; i < R; i++) {
    for (let j = 0; j < C; j++) {
      if (src[i][j] !== 0) {
        if (i < minR) minR = i;
        if (j < minC) minC = j;
      }
    }
  }

  // 全部0なら、ターゲットサイズのゼロ行列を返す
  const zeroMat = Array.from({ length: targetR }, () => Array(targetC).fill(0));
  if (minR === Infinity) return zeroMat;

  // 新しいターゲット行列に左上へ詰めてコピー（はみ出せば切る）
  const out = Array.from({ length: targetR }, () => Array(targetC).fill(0));
  for (let i = minR; i < R; i++) {
    for (let j = minC; j < C; j++) {
      const ni = i - minR;
      const nj = j - minC;
      if (ni >= 0 && ni < targetR && nj >= 0 && nj < targetC) {
        out[ni][nj] = src[i][j];
      }
    }
  }
  return out;
}

// --- バリアント生成（回転4通り × 反転バージョン） --------------------------
function generateVariants(mat, targetR, targetC) {
  const seen = new Set();
  let cur = mat;
  for (let k = 0; k < 4; k++) {
    // 回転 k 回の行列 cur
    const n0 = normalizeToSize(cur, targetR, targetC);
    seen.add(JSON.stringify(n0));

    const h = normalizeToSize(flipH(cur), targetR, targetC);
    seen.add(JSON.stringify(h));

    const v = normalizeToSize(flipV(cur), targetR, targetC);
    seen.add(JSON.stringify(v));

    // 反転両方（等価だが念のため）
    const hv = normalizeToSize(flipV(flipH(cur)), targetR, targetC);
    seen.add(JSON.stringify(hv));

    // 次の回転（90度）
    cur = rotate90(cur);
  }
  return Array.from(seen); // JSON 文字列配列
}

// --- メイン：同型（回転/反転/左上詰め）で重複排除 ---------------------------
function uniqueByOmino(matrices) {
  const kept = [];
  const seen = new Set();

  for (const mat of matrices) {
    const R = mat.length;
    const C = mat[0].length;
    // その行列を基準サイズとして、全バリアントを作る
    const variants = generateVariants(mat, R, C);

    // 代表文字列をバリアントのうち最小のもの（安定化）
    // （どれを採るかは任意だが安定させると結果が読みやすい）
    const representative = variants.slice().sort()[0];

    if (!seen.has(representative)) {
      seen.add(representative);
      // 保存は正規化された形にしておく（サイズは元と同じ）
      kept.push(JSON.parse(representative));
    }
    // もしすでにあればスキップ（＝配列から削除するイメージ）
  }
  return kept;
}

function transformMatrix(matrix) {
    const n = matrix.length;
    const m = matrix[0].length;
    const result = Array.from({ length: n }, () => Array(m).fill(0));

    const dirs = [
        [-1, 0], // 上
        [1, 0],  // 下
        [0, -1], // 左
        [0, 1]   // 右
    ];

    for (let i = 0; i < n; i++) {
        for (let j = 0; j < m; j++) {
            if (matrix[i][j] === 1) {
                let count = 0;
                for (let [dx, dy] of dirs) {
                    const x = i + dx, y = j + dy;
                    if (0 <= x && x < n && 0 <= y && y < m && matrix[x][y] === 1) {
                        count++;
                    }
                }
                result[i][j] = count;
            }
        }
    }
    return result;
}



function formatMatrix(matrix) {
    return matrix.map(row => row.join('\t')).join('\n');
}

function formatMatrixArray(matrices) {
    return matrices.map((matrix, index) => `Omino ${++index}:\n${formatMatrix(matrix)}`).join('\n\n');
}

let input = Number(prompt('input number of square:'));
let ominosIndex = 0;
let createdOminos = uniqueByOmino(CREATE_Ominos(input));
console.group(`${input}-Ominos : ${createdOminos.length}`);
for (const omino of createdOminos) {
    console.group(`Omino${++ominosIndex}`);
    console.table(transformMatrix(omino));
    console.groupEnd();
}
console.groupEnd();

if (Number.isInteger(input) && input >= 1) {
    if (input >= 7) {
        if (confirm('your input may be too big. run it?')) {
            alert(`the ominos with ${input} squares are output to the console.`);
        } else {
            alert('execution canceled.')
        }
    } else {
        alert(`the ominos with ${input} squares are output to the console.`);
    }
} else {
    alert('sorry, your input is not a positive integer.')
}
