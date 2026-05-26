#include<stdio.h>
#include <string.h>

#define MAX_CARDS 10
#define MAX_TEXT 100

// 暗記カード構造体
typedef struct {
    char question[MAX_TEXT];
    char answer[MAX_TEXT];
    int level; // ????x
} FlashCard;
typedef struct {
	char one[128];//選択肢1～
	char two[128];//2
	char three[128];//3
	char four[128];//4
	char five[128];//5
	char six[128];//6
	char seven[128];//7
	char eight[128];//8
	int element;//選択肢の数
}choosing;

int choose(choosing choices)
{
	fflush(stdout);
 
	int key;
	int choice = 0;  // 現在の選択位置
 
	// 文字列ポインタの配列を作成（配列長 = choices.element）
	char* items[8] = {
		choices.one, choices.two, choices.three, choices.four,
		choices.five, choices.six, choices.seven, choices.eight
	};
 
	int n = choices.element;  // 選択肢の数
 
	// 最初の表示
	printf("\r");
	for (int i = 0; i < n; i++) {
		if (i == 0) printf("\x1b[33m・%s\x1b[0m   ", items[i]);
		else        printf("%s   ", items[i]);
	}
	fflush(stdout);
 
	for (;;) {
		key = _getch();
 
		if (key == 13) {  // Enter
			printf("\x1b[0m\n");
			break;
		}
 
		// 特殊キー（矢印キー）
		if (key == 0 || key == 224) {
			key = _getch();
 
			if (key == 75 && choice > 0) choice--;       // ←
			else if (key == 77 && choice < n - 1) choice++; // →
 
			// 再描画
			printf("\r");
			for (int i = 0; i < n; i++) {
				if (i == choice) printf("\x1b[33m・%s\x1b[0m   ", items[i]);
				else             printf("  %s   ", items[i]);
			}
			fflush(stdout);
		}
	}
 
	return choice;
}

int main() {
    FlashCard cards[MAX_CARDS];
    int count = 0;
    int choice;

    while (1) {
        printf("\n=== 暗記カードアプリ ===\n");
        printf("選択してください: ");

        choice = choose((choosing){"カード追加", "カード一覧", "暗記テスト", "終了", "", "", "", "", 4});

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

            printf("カードが追加されました！\n");
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