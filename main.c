#include<stdio.h>

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

int main(){

    printf("1,start 2,add");

    return 0;
}