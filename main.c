#include <stdio.h>
#include <string.h>

#define MAX_CARDS 10
#define MAX_TEXT 100

// 暗記カード構造体
typedef struct {
    char question[MAX_TEXT];
    char answer[MAX_TEXT];
    int level; // 理解度
} FlashCard;

int main() {
    FlashCard cards[MAX_CARDS];
    int count = 0;
    int choice;

    while (1) {
        printf("\n=== 暗記カードアプリ ===\n");
        printf("1. カード追加\n");
        printf("2. カード一覧\n");
        printf("3. 暗記テスト\n");
        printf("4. 終了\n");
        printf("選択してください: ");

        scanf("%d", &choice);
        getchar(); // 改行除去

        // -----------------------
        // カード追加
        // -----------------------
        if (choice == 1) {

            if (count >= MAX_CARDS) {
                printf("これ以上追加できません！\n");
                continue;
            }

            printf("問題: ");
            fgets(cards[count].question, MAX_TEXT, stdin);
            cards[count].question[strcspn(cards[count].question, "\n")] = '\0';

            printf("答え: ");
            fgets(cards[count].answer, MAX_TEXT, stdin);
            cards[count].answer[strcspn(cards[count].answer, "\n")] = '\0';

            cards[count].level = 0;

            count++;

            printf("カードを追加しました！\n");
        }

        // -----------------------
        // カード一覧
        // -----------------------
        else if (choice == 2) {

            if (count == 0) {
                printf("カードがありません。\n");
                continue;
            }

            printf("\n--- カード一覧 ---\n");

            for (int i = 0; i < count; i++) {
                printf("%d. %s  [理解度:%d]\n",
                       i + 1,
                       cards[i].question,
                       cards[i].level);
            }
        }

        // -----------------------
        // 暗記テスト
        // -----------------------
        else if (choice == 3) {

            if (count == 0) {
                printf("カードがありません。\n");
                continue;
            }

            char judge;

            for (int i = 0; i < count; i++) {

                printf("\n====================\n");
                printf("問題%d\n", i + 1);
                printf("%s\n", cards[i].question);

                printf("\nエンターを押して答えを見る...");
                getchar();

                printf("\n答え: %s\n", cards[i].answer);

                printf("\n理解できた？\n");
                printf("a: 分かった\n");
                printf("z: 分かってない\n");
                printf("入力: ");

                scanf("%c", &judge);
                getchar(); // 改行除去

                if (judge == 'a') {
                    cards[i].level++;
                    printf("理解度アップ！\n");
                }
                else if (judge == 'z') {

                    if (cards[i].level > 0) {
                        cards[i].level--;
                    }

                    printf("復習しよう！\n");
                }
                else {
                    printf("無効な入力です。\n");
                }
            }
        }

        // -----------------------
        // 終了
        // -----------------------
        else if (choice == 4) {
            printf("終了します！\n");
            break;
        }

        else {
            printf("無効な入力です。\n");
        }
    }

    return 0;
}