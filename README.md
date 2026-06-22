# 暗記補助アプリアンキング

---

## 1. プロジェクト概要

英語や歴史等，暗記を必要とする教科の勉強を補助するアプリケーションです．

---

## 2. 選択テーマ

| 03 | 暗記フラッシュカード | テキストから単語を出題、正解率を記録 |

---

## 3. 班構成（役割分担）

| メンバー | 役割 | 担当ファイル |
|---|---|---|
| **小野 凛人** | メインロジック（計算・判定） | `main.c` |
| **斎藤 駿喜** | データ管理 | `.gitignore` `README.md` |
| **吉村 琉** | UI（メニュー・入力チェック） | `UI.c` |

全員がGitで互いのプルリクエストをレビュー。

---

## 4. ビルド方法

### 必要なもの

- C コンパイラ (`gcc` または `clang`)
- `make`
- Git

### プラットフォーム別セットアップ
> ⚠️ **NOTICE / 注意**
> Windows11 Home以外での動作を確認していません．そのため，macやlinuxでは動かない可能性があります．

**Windows (MSYS2 / MinGW)**
```bash
pacman -S mingw-w64-x86_64-gcc make
```

**macOS**
```bash
xcode-select --install
```

**Linux (Ubuntu)**
```bash
sudo apt install build-essential
```

### ビルドと実行

```bash
git clone https://github.com/NIT-Oita/student-utility-kit-innovation-3-12-39-19.git
cd <your-repo>
make          # コンパイル
./anking     # 実行（Windowsは anking.exe）
```

### Makefile ターゲット

| コマンド | 動作 |
|---|---|
| `make` | コンパイル＆リンク |
| `make run` | ビルドして実行 |
| `make clean` | 中間ファイルを削除 |

---

## 5. プロジェクト構成

```
anking/
├── Makefile
├── README.md
├── .gitignore
├── main.c
├── UI.c 
└── .github/
    └── .keep
```

---

## 6. Gitワークフロー

```bash
git pull origin main                    # 最新を取得
git checkout -b feat/<feature-name>     # 機能ごとにブランチ
# ... コード変更 ...
git commit -m "add: <意味のあるメッセージ>"
git push -u origin feat/<feature-name>  # PR作成 → レビュー → マージ
```

main ブランチには直接 push しない。

---

## 7. メンバー / Members

- **メンバー A** — 小野 凛人 (s2512) — `s2512@oita.kosen-ac.jp`
- **メンバー B** — 斎藤 駿喜 (s2519) — `s2519@oita.kosen-ac.jp`
- **メンバー C** — 吉村 琉 (s2540) — `s2540@oita.kosen-ac.jp`

班番号: **Group 3**

---

## 8. デモ / Demo

> ![alt text](image.png)

---

## ライセンス

学内提出用。商用利用なし。
