/*
 * ????????????????????????????????????????????
 * ?         FlashMaster - 暗記アプリ          ?
 * ?    忘却曲線(SM-2)による効率的な学習        ?
 * ????????????????????????????????????????????
 */

#include "anki.c"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

/* ── 定数定義 ── */
#define MAX_DECKS         50
#define MAX_CARDS         500
#define MAX_STR           256
#define DATA_FILE         "flashmaster_data.dat"
#define MAGIC_NUMBER      0x464D3031   /* "FM01" */

/* ── ANSI カラーコード ── */
#define C_RESET   "\033[0m"
#define C_BOLD    "\033[1m"
#define C_DIM     "\033[2m"
#define C_RED     "\033[31m"
#define C_GREEN   "\033[32m"
#define C_YELLOW  "\033[33m"
#define C_BLUE    "\033[34m"
#define C_MAGENTA "\033[35m"
#define C_CYAN    "\033[36m"
#define C_WHITE   "\033[37m"
#define C_BG_BLUE "\033[44m"
#define C_BG_DARK "\033[40m"

/* ── SM-2 忘却曲線パラメータ ── */
#define MIN_EASINESS  1.3
#define INIT_EASINESS 2.5

/* ── データ構造 ── */

typedef struct {
    int  id;
    char front[MAX_STR];   /* 表 */
    char back[MAX_STR];    /* 裏 */
    /* SM-2 パラメータ */
    double easiness;       /* 容易度係数 (E-Factor) */
    int    interval;       /* 次回復習までの日数 */
    int    repetitions;    /* 連続正解回数 */
    time_t next_review;    /* 次回復習日時 */
    /* 統計 */
    int    total_reviews;
    int    correct_total;
    time_t created_at;
} Card;

typedef struct {
    int  id;
    char name[MAX_STR];
    int  card_count;
    int  card_ids[MAX_CARDS];
    time_t created_at;
    /* 統計 */
    int  total_sessions;
    int  total_correct;
    int  total_reviewed;
} Deck;

typedef struct {
    int   magic;
    int   deck_count;
    int   card_count;
    int   total_sessions_global;
    Deck  decks[MAX_DECKS];
    Card  cards[MAX_CARDS];
} AppData;

/* ── グローバル ── */
static AppData g;

/* ??????????????????????????????????????????
    ユーティリティ関数
??????????????????????????????????????????? */

void clear_screen(void) {
    printf("\033[2J\033[H");
}

void print_separator(char c, int len) {
    for (int i = 0; i < len; i++) putchar(c);
    putchar('\n');
}

void press_enter(const char *msg) {
    if (msg) printf("%s", msg);
    else printf(C_DIM "  [Enter で続ける]" C_RESET);
    int ch;
    /* 入力バッファをクリアしてからEnterを待つ */
    while ((ch = getchar()) != '\n' && ch != EOF);
}

/* 安全な文字列入力 */
void read_line(char *buf, int maxlen) {
    if (!fgets(buf, maxlen, stdin)) {
        buf[0] = '\0';
        return;
    }
    /* 末尾の改行を除去 */
    size_t len = strlen(buf);
    if (len > 0 && buf[len-1] == '\n') buf[len-1] = '\0';
}

int read_int(void) {
    char buf[32];
    read_line(buf, sizeof(buf));
    return atoi(buf);
}

/* ── プログレスバー表示 ── */
void print_progress_bar(int done, int total, int width) {
    if (total == 0) return;
    int filled = (done * width) / total;
    double pct = (double)done / total * 100.0;
    printf("  [");
    for (int i = 0; i < width; i++) {
        if (i < filled)       printf(C_GREEN "?" C_RESET);
        else if (i == filled) printf(C_YELLOW "?" C_RESET);
        else                  printf(C_DIM "?" C_RESET);
    }
    printf("] %3.0f%% (%d/%d)\n", pct, done, total);
}

/* ── 日付文字列 ── */
void format_date(time_t t, char *buf, int buflen) {
    struct tm *tm = localtime(&t);
    strftime(buf, buflen, "%Y-%m-%d", tm);
}

/* ── 経過日数 ── */
int days_since(time_t t) {
    return (int)((time(NULL) - t) / 86400);
}

/* ??????????????????????????????????????????
    データ保存・読み込み
??????????????????????????????????????????? */

void save_data(void) {
    FILE *fp = fopen(DATA_FILE, "wb");
    if (!fp) {
        printf(C_RED "  [エラー] データ保存失敗\n" C_RESET);
        return;
    }
    fwrite(&g, sizeof(AppData), 1, fp);
    fclose(fp);
}

int load_data(void) {
    FILE *fp = fopen(DATA_FILE, "rb");
    if (!fp) return 0;
    int ok = (fread(&g, sizeof(AppData), 1, fp) == 1 &&
                g.magic == MAGIC_NUMBER);
    fclose(fp);
    return ok;
}

void init_data(void) {
    memset(&g, 0, sizeof(AppData));
    g.magic = MAGIC_NUMBER;
}

/* ??????????????????????????????????????????
    SM-2 忘却曲線アルゴリズム
??????????????????????????????????????????? */

/*
 * quality: 0=完全忘却, 1=難しい, 2=正解
 * SM-2 アルゴリズムに基づく次回復習間隔の計算
 */
void sm2_update(Card *c, int quality) {
    /* E-Factor 更新 */
    double ef = c->easiness;
    ef = ef + (0.1 - (2 - quality) * (0.08 + (2 - quality) * 0.02));
    if (ef < MIN_EASINESS) ef = MIN_EASINESS;
    c->easiness = ef;

    if (quality < 1) {
        /* 不正解: リセット */
        c->repetitions = 0;
        c->interval    = 1;
    } else {
        /* 正解 */
        if (c->repetitions == 0)      c->interval = 1;
        else if (c->repetitions == 1) c->interval = 6;
        else c->interval = (int)round((double)c->interval * c->easiness);
        c->repetitions++;
    }

    /* 次回復習日時を設定 */
    c->next_review = time(NULL) + (time_t)c->interval * 86400;
}

/* ??????????????????????????????????????????
    カード管理
??????????????????????????????????????????? */

Card *find_card(int id) {
    for (int i = 0; i < g.card_count; i++)
        if (g.cards[i].id == id) return &g.cards[i];
    return NULL;
}

Deck *find_deck(int id) {
    for (int i = 0; i < g.deck_count; i++)
        if (g.decks[i].id == id) return &g.decks[i];
    return NULL;
}

int new_card_id(void) {
    int max = 0;
    for (int i = 0; i < g.card_count; i++)
        if (g.cards[i].id > max) max = g.cards[i].id;
    return max + 1;
}

int new_deck_id(void) {
    int max = 0;
    for (int i = 0; i < g.deck_count; i++)
        if (g.decks[i].id > max) max = g.decks[i].id;
    return max + 1;
}

/* ??????????????????????????????????????????
    UI: ヘッダー・タイトル
??????????????????????????????????????????? */

void print_title(void) {
    printf("\n");
    printf(C_CYAN C_BOLD);
    printf("  ????????????????????????????????????????????\n");
    printf("  ?      FlashMaster  -  暗記の達人           ?\n");
    printf("  ?       忘却曲線で効率よく覚えよう           ?\n");
    printf("  ????????????????????????????????????????????\n");
    printf(C_RESET "\n");
}

void print_header(const char *title) {
    printf("\n");
    printf(C_BG_BLUE C_WHITE C_BOLD "  %-42s  " C_RESET "\n", title);
    printf("\n");
}

/* ??????????????????????????????????????????
    UI: デッキ一覧
??????????????????????????????????????????? */

void show_deck_list(void) {
    print_header("? デッキ一覧");
    if (g.deck_count == 0) {
        printf(C_DIM "  デッキがまだありません。\n" C_RESET);
        return;
    }
    printf("  %-4s  %-24s  %6s  %6s  %s\n",
            "No.", "デッキ名", "カード数", "習得済", "今日の復習");
    print_separator('-', 60);

    time_t now = time(NULL);
    for (int i = 0; i < g.deck_count; i++) {
        Deck *d = &g.decks[i];
        int due = 0, mastered = 0;
        for (int j = 0; j < d->card_count; j++) {
            Card *c = find_card(d->card_ids[j]);
            if (!c) continue;
            if (c->next_review <= now) due++;
            if (c->repetitions >= 3) mastered++;
        }
        const char *due_color = (due > 0) ? C_YELLOW : C_GREEN;
        printf("  " C_BOLD "[%d]" C_RESET "  %-24s  %6d  " C_GREEN "%6d" C_RESET "  %s%d枚" C_RESET "\n",
                i + 1, d->name, d->card_count, mastered, due_color, due);
    }
    printf("\n");
}

/* ??????????????????????????????????????????
    デッキ作成
??????????????????????????????????????????? */

void create_deck(void) {
    if (g.deck_count >= MAX_DECKS) {
        printf(C_RED "  デッキ上限に達しました。\n" C_RESET);
        return;
    }
    print_header(" デッキ作成");
    printf("  デッキ名を入力してください: ");
    char name[MAX_STR];
    read_line(name, sizeof(name));
    if (strlen(name) == 0) {
        printf(C_RED "  名前が空です。キャンセルしました。\n" C_RESET);
        return;
    }

    Deck *d = &g.decks[g.deck_count++];
    memset(d, 0, sizeof(Deck));
    d->id         = new_deck_id();
    d->created_at = time(NULL);
    strncpy(d->name, name, MAX_STR - 1);

    save_data();
    printf(C_GREEN "\n  ? デッキ「%s」を作成しました！\n" C_RESET, d->name);
    press_enter(NULL);
}

/* ??????????????????????????????????????????
    カード追加
??????????????????????????????????????????? */

void add_cards_to_deck(Deck *d) {
    print_header("  カード追加");
    printf(C_CYAN "  デッキ: %s\n" C_RESET, d->name);
    printf(C_DIM "  (表・裏を入力してください。空Enterで終了)\n\n" C_RESET);

    int added = 0;
    while (1) {
        if (g.card_count >= MAX_CARDS || d->card_count >= MAX_CARDS) {
            printf(C_RED "  カード上限に達しました。\n" C_RESET);
            break;
        }
        printf(C_YELLOW "  カード %d\n" C_RESET, added + 1);
        printf("    表（問題）: ");
        char front[MAX_STR], back[MAX_STR];
        read_line(front, sizeof(front));
        if (strlen(front) == 0) break;

        printf("    裏（答え）: ");
        read_line(back, sizeof(back));
        if (strlen(back) == 0) break;

        /* カード追加 */
        Card *c = &g.cards[g.card_count++];
        memset(c, 0, sizeof(Card));
        c->id         = new_card_id();
        c->easiness   = INIT_EASINESS;
        c->interval   = 1;
        c->created_at = time(NULL);
        c->next_review = time(NULL);
        strncpy(c->front, front, MAX_STR - 1);
        strncpy(c->back,  back,  MAX_STR - 1);

        d->card_ids[d->card_count++] = c->id;
        added++;
        printf(C_GREEN "    ? 追加完了！\n\n" C_RESET);
    }

    if (added > 0) {
        save_data();
        printf(C_GREEN "\n  合計 %d 枚のカードを追加しました！\n" C_RESET, added);
    } else {
        printf(C_DIM "  カードは追加されませんでした。\n" C_RESET);
    }
    press_enter(NULL);
}

/* ??????????????????????????????????????????
    カード編集・削除
??????????????????????????????????????????? */

void edit_cards(Deck *d) {
    while (1) {
        print_header("? カード一覧・編集");
        printf(C_CYAN "  デッキ: %s  (%d枚)\n\n" C_RESET, d->name, d->card_count);

        if (d->card_count == 0) {
            printf(C_DIM "  カードがありません。\n" C_RESET);
            press_enter(NULL);
            return;
        }

        for (int i = 0; i < d->card_count; i++) {
            Card *c = find_card(d->card_ids[i]);
            if (!c) continue;
            printf("  " C_BOLD "[%d]" C_RESET " 表: %-28s  裏: %s\n",
                        i + 1, c->front, c->back);
        }

        printf("\n  " C_DIM "番号=編集  0=戻る  d+番号=削除 (例: d3)" C_RESET "\n");
        printf("  > ");
        char input[32];
        read_line(input, sizeof(input));

        if (strcmp(input, "0") == 0) break;

        /* 削除 */
        if (input[0] == 'd' || input[0] == 'D') {
            int idx = atoi(input + 1) - 1;
            if (idx < 0 || idx >= d->card_count) {
                printf(C_RED "  無効な番号です。\n" C_RESET);
                press_enter(NULL);
                continue;
            }
            printf(C_RED "  本当に削除しますか？\n" C_RESET);
            int yn = choose((choosing){"Yes","No","","","","","","",2});
            if (yn == 0 ) 
            {
                /* カードIDをゼロクリア（簡易削除） */
                int rem_id = d->card_ids[idx];
                for (int i = idx; i < d->card_count - 1; i++)
                    d->card_ids[i] = d->card_ids[i + 1];
                d->card_count--;
                /* cards配列からも削除 */
                for (int i = 0; i < g.card_count; i++) {
                    if (g.cards[i].id == rem_id) {
                        for (int j = i; j < g.card_count - 1; j++)
                            g.cards[j] = g.cards[j + 1];
                        g.card_count--;
                        break;
                    }
                }
                save_data();
                printf(C_GREEN "  削除しました。\n" C_RESET);
                press_enter(NULL);
            }
            continue;
        }

        /* 編集 */
        int idx = atoi(input) - 1;
        if (idx < 0 || idx >= d->card_count) {
            printf(C_RED "  無効な番号です。\n" C_RESET);
            press_enter(NULL);
            continue;
        }
        Card *c = find_card(d->card_ids[idx]);
        if (!c) continue;

        printf("\n  現在の表: %s\n", c->front);
        printf("  新しい表 (空=変更なし): ");
        char buf[MAX_STR];
        read_line(buf, sizeof(buf));
        if (strlen(buf) > 0) strncpy(c->front, buf, MAX_STR - 1);

        printf("  現在の裏: %s\n", c->back);
        printf("  新しい裏 (空=変更なし): ");
        read_line(buf, sizeof(buf));
        if (strlen(buf) > 0) strncpy(c->back, buf, MAX_STR - 1);

        save_data();
        printf(C_GREEN "  ? 更新しました！\n" C_RESET);
        press_enter(NULL);
    }
}

/* ??????????????????????????????????????????
    学習セッション
??????????????????????????????????????????? */

/* シャッフル（Fisher-Yates） */
void shuffle(int *arr, int n) {
    for (int i = n - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int t = arr[i]; arr[i] = arr[j]; arr[j] = t;
    }
}

void study_session(Deck *d) {
    if (d->card_count == 0) {
        printf(C_RED "\n  カードがありません！\n" C_RESET);
        press_enter(NULL);
        return;
    }

    /* ── セッション開始 ── */
    clear_screen();
    print_title();
    printf(C_CYAN C_BOLD "  ? 勉強開始: %s\n" C_RESET, d->name);
    printf(C_DIM "  全 %d 枚のカードを1回ずつ正解するまで繰り返します。\n" C_RESET, d->card_count);
    printf(C_DIM "  ルール: すべてのカードを「理解した」と答えるまで続きます。\n\n" C_RESET);
    press_enter("  準備ができたら [Enter] を押してください...");

    /* 未正解カードのインデックス配列 */
    int queue[MAX_CARDS];
    int queue_size = d->card_count;
    for (int i = 0; i < queue_size; i++) queue[i] = i;
    shuffle(queue, queue_size);

    int session_correct = 0;
    int session_wrong   = 0;
    int session_total   = 0;
    time_t session_start = time(NULL);

    /* 残りカードの追跡（正解済みを除外するためのフラグ） */
    int mastered[MAX_CARDS] = {0};  /* 1=このセッションで正解済 */
    int remaining = queue_size;

    while (remaining > 0) {
        /* 未正解カードをキューに並べ直す */
        int next_queue[MAX_CARDS];
        int next_size = 0;
        for (int i = 0; i < queue_size; i++) {
            if (!mastered[queue[i]]) {
                next_queue[next_size++] = queue[i];
            }
        }
        /* シャッフルして次のラウンドへ */
        shuffle(next_queue, next_size);
        queue_size = next_size;
        for (int i = 0; i < queue_size; i++) queue[i] = next_queue[i];

        /* 1周する */
        for (int qi = 0; qi < queue_size; qi++) {
            if (mastered[queue[qi]]) continue;

            int card_deck_idx = queue[qi];
            Card *c = find_card(d->card_ids[card_deck_idx]);
            if (!c) continue;

            session_total++;

            /* ── カード表示（表） ── */
            clear_screen();

            /* 進捗表示 */
            int done_count = d->card_count - remaining;
            printf("\n  " C_BOLD "進捗" C_RESET "\n");
            print_progress_bar(done_count, d->card_count, 30);
            printf("  残り %d 枚 / 全 %d 枚  "
                    C_GREEN "? %d" C_RESET "  "
                    C_RED "? %d" C_RESET "\n\n",
                    remaining, d->card_count, session_correct, session_wrong);

            print_separator('-', 44);
            printf("\n");
            printf(C_BOLD C_YELLOW "  ★ 問題\n\n" C_RESET);
            printf(C_WHITE C_BOLD "  %s\n" C_RESET, c->front);
            printf("\n");
            print_separator('-', 44);

            /* 習得状況インジケータ */
            printf("  " C_DIM "[ 連続正解: %d回 | 容易度: %.1f | 復習間隔: %d日 ]" C_RESET "\n\n",
                    c->repetitions, c->easiness, c->interval);

            press_enter("  [Enter] で答えを見る...");

            /* ── 裏面表示 ── */
            clear_screen();

            done_count = d->card_count - remaining;
            printf("\n  " C_BOLD "進捗" C_RESET "\n");
            print_progress_bar(done_count, d->card_count, 30);
            printf("  残り %d 枚 / 全 %d 枚  "
                    C_GREEN "? %d" C_RESET "  "
                    C_RED "? %d" C_RESET "\n\n",
                    remaining, d->card_count, session_correct, session_wrong);

            print_separator('-', 44);
            printf("\n");
            printf(C_BOLD C_YELLOW "  ★ 問題\n\n" C_RESET);
            printf(C_WHITE C_BOLD "  %s\n\n" C_RESET, c->front);
            printf(C_DIM "  ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─\n" C_RESET);
            printf(C_BOLD C_CYAN "  ? 答え\n\n" C_RESET);
            printf(C_WHITE C_BOLD "  %s\n" C_RESET, c->back);
            printf("\n");
            print_separator('-', 44);
            printf("\n");

            /* ── 自己評価 ── */
            printf("  " C_BOLD "どうでしたか？\n" C_RESET);
            printf("    " C_GREEN "[1] 理解した ?" C_RESET "   →  次回は %d日後に復習\n",
                    (c->repetitions == 0 ? 1 : (c->repetitions == 1 ? 6 :
                    (int)round((double)c->interval * c->easiness))));
            printf("    " C_RED "[2] 難しかった ?" C_RESET " →  もう一度このセッションで\n");
            printf("\n  > ");

            char ans[8];
            read_line(ans, sizeof(ans));
            int choice = atoi(ans);

            c->total_reviews++;
            if (choice == 1) {
                /* 正解 */
                sm2_update(c, 2);
                c->correct_total++;
                session_correct++;
                mastered[card_deck_idx] = 1;
                remaining--;

                /* 正解アニメーション */
                printf("\n  " C_GREEN C_BOLD "  素晴らしい！? 正解！" C_RESET "\n");
                /* 小さな達成感 */
                if (remaining == 0)
                    printf("  " C_YELLOW C_BOLD "  ? 全カード正解！すごい！" C_RESET "\n");
                else if (done_count + 1 == d->card_count / 2)
                    printf("  " C_CYAN "  ? 折り返し地点！" C_RESET "\n");

            } else {
                /* 不正解 */
                sm2_update(c, 0);
                session_wrong++;
                printf("\n  " C_RED "  もう一度頑張ろう！?" C_RESET "\n");
            }

            /* 少し間を置く */
            press_enter(NULL);
        }

        /* 1周終わったら再計算 */
        remaining = 0;
        for (int i = 0; i < d->card_count; i++)
            if (!mastered[i]) remaining++;
    }

    /* ── セッション終了 ── */
    time_t elapsed = time(NULL) - session_start;
    d->total_sessions++;
    d->total_correct  += session_correct;
    d->total_reviewed += session_total;
    g.total_sessions_global++;
    save_data();

    clear_screen();
    print_title();

    printf(C_YELLOW C_BOLD);
    printf("  ????????????????????????????????????????\n");
    printf("  ?         ? セッション完了！           ?\n");
    printf("  ????????????????????????????????????????\n");
    printf(C_RESET "\n");

    printf("  " C_CYAN "デッキ:" C_RESET " %s\n\n", d->name);

    printf("  ┌─────────────────────────────────┐\n");
    printf("  │  %-14s  " C_GREEN "%8d 枚" C_RESET "    │\n", "正解",      session_correct);
    printf("  │  %-14s  " C_RED   "%8d 回" C_RESET "    │\n", "間違え",    session_wrong);
    printf("  │  %-14s  %8d 回    │\n", "合計挑戦",   session_total);
    printf("  │  %-14s  %6d 分 %02d 秒│\n", "学習時間",
            (int)elapsed / 60, (int)elapsed % 60);
    if (session_total > 0)
        printf("  │  %-14s       %7.1f %%  │\n", "正解率",
               (double)session_correct / d->card_count * 100.0);
    printf("  └─────────────────────────────────┘\n\n");

    /* モチベーション メッセージ */
    double acc = (session_total > 0) ?
                 (double)session_correct / session_total * 100.0 : 0.0;
    if (acc >= 90)
        printf("  " C_YELLOW "★ 完璧！この調子でどんどん続けよう！?" C_RESET "\n");
    else if (acc >= 70)
        printf("  " C_GREEN "? 良い調子！毎日の積み重ねが大切です?" C_RESET "\n");
    else
        printf("  " C_CYAN "? 諦めずに繰り返すことで必ず覚えられます！" C_RESET "\n");

    printf("\n");
    press_enter(NULL);
}

/* ??????????????????????????????????????????
    統計表示
??????????????????????????????????????????? */

void show_stats(void) {
    print_header("? 学習統計");

    printf("  " C_BOLD "? 全体統計\n" C_RESET);
    printf("    デッキ数    : %d\n", g.deck_count);
    printf("    カード総数  : %d\n", g.card_count);
    printf("    総学習回数  : %d セッション\n", g.total_sessions_global);
    printf("\n");

    if (g.deck_count == 0) {
        press_enter(NULL);
        return;
    }

    printf("  " C_BOLD "? デッキ別統計\n" C_RESET "\n");

    for (int i = 0; i < g.deck_count; i++) {
        Deck *d = &g.decks[i];
        int mastered = 0, due = 0;
        time_t now = time(NULL);

        for (int j = 0; j < d->card_count; j++) {
            Card *c = find_card(d->card_ids[j]);
            if (!c) continue;
            if (c->repetitions >= 3) mastered++;
            if (c->next_review <= now) due++;
        }

        printf("  " C_BOLD C_CYAN "[%s]" C_RESET "\n", d->name);
        printf("    カード数      : %d\n",  d->card_count);
        printf("    習得済み      : " C_GREEN "%d 枚" C_RESET " (%.0f%%)\n",
                mastered,
                d->card_count > 0 ? (double)mastered / d->card_count * 100.0 : 0.0);
        printf("    今日の復習    : " C_YELLOW "%d 枚" C_RESET "\n", due);
        printf("    総セッション  : %d\n",  d->total_sessions);
        printf("    総正解数      : %d\n",  d->total_correct);

        /* 習得率バー */
        if (d->card_count > 0) {
            printf("    習得率        : ");
            print_progress_bar(mastered, d->card_count, 20);
        }
        printf("\n");
    }

    press_enter(NULL);
}

/* ??????????????????????????????????????????
    デッキメニュー
??????????????????????????????????????????? */

void deck_menu(Deck *d) {
    while (1) {
        clear_screen();
        print_title();
        printf(C_CYAN C_BOLD "  デッキ: %s" C_RESET "  (%d枚)\n\n", d->name, d->card_count);

        /* 今日の復習カード数 */
        int due = 0;
        time_t now = time(NULL);
        for (int i = 0; i < d->card_count; i++) {
            Card *c = find_card(d->card_ids[i]);
            if (c && c->next_review <= now) due++;
        }
        if (due > 0)
            printf("  " C_YELLOW "? 今日の復習: %d 枚" C_RESET "\n\n", due);

        int ch = choose((choosing){"勉強開始","カードを追加","カードを編集・削除","戻る","","","","",4});
        switch (ch) {
            case 0: study_session(d); break;
            case 1: clear_screen(); add_cards_to_deck(d); break;
            case 2: clear_screen(); edit_cards(d); break;
            case 3: return;
            default:
                printf(C_RED "  無効な選択です。\n" C_RESET);
                press_enter(NULL);
        }
    }
}

/* ??????????????????????????????????????????
    メインメニュー
??????????????????????????????????????????? */

void main_menu(void) {
    while (1) {
        clear_screen();
        print_title();

        /* サマリー表示 */
        if (g.deck_count > 0) {
            int total_due = 0;
            time_t now = time(NULL);
            for (int i = 0; i < g.deck_count; i++) {
                Deck *d = &g.decks[i];
                for (int j = 0; j < d->card_count; j++) {
                    Card *c = find_card(d->card_ids[j]);
                    if (c && c->next_review <= now) total_due++;
                }
            }
            if (total_due > 0)
                printf("  " C_YELLOW C_BOLD "? 今日の復習: 全デッキで %d 枚待っています！\n\n" C_RESET, total_due);
        }

        show_deck_list();

        int ch = choose((choosing){"デッキを選択して勉強","新しいデッキを作成","学習統計を見る","終了","","","","",4});

        if (ch == 3) {
            clear_screen();
            printf(C_CYAN "\n  お疲れ様でした！また明日も頑張りましょう！\n\n" C_RESET);
            break;
        } else if (ch == 1) {
            clear_screen();
            create_deck();
        } else if (ch == 2) {
            clear_screen();
            show_stats();
        } else if (ch == 0) {
            if (g.deck_count == 0) {
                printf(C_RED "  デッキがありません。先にデッキを作成してください。\n" C_RESET);
                press_enter(NULL);
                continue;
            }
            printf("  デッキ番号を選択: ");
            int idx = read_int() - 1;
            if (idx < 0 || idx >= g.deck_count) {
                printf(C_RED "  無効な番号です。\n" C_RESET);
                press_enter(NULL);
                continue;
            }
            deck_menu(&g.decks[idx]);
        } else {
            printf(C_RED "  無効な選択です。\n" C_RESET);
            press_enter(NULL);
        }
    }
}

/* ??????????????????????????????????????????
    エントリポイント
??????????????????????????????????????????? */

int main(void) {
    srand((unsigned int)time(NULL));

    /* データ読み込み */
    if (!load_data()) {
        init_data();
    }

    main_menu();

    return 0;
}