/*
 * FlashMaster - Flashcard Study App
 * Spaced repetition with the SM-2 algorithm
 */

#include "UI.c"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

/* Windows: set console to Shift-JIS (CP932) for Japanese I/O */
#ifdef _WIN32
#include <windows.h>
#endif

/* „Ÿ„Ÿ Constants „Ÿ„Ÿ */
#define MAX_DECKS     50
#define MAX_CARDS     500
#define MAX_STR       256
#define DATA_FILE     "flashmaster_data.dat"
#define MAGIC_NUMBER  0x464D3031

#define MIN_EASINESS  1.3
#define INIT_EASINESS 2.5

/* „Ÿ„Ÿ Data structures „Ÿ„Ÿ */

typedef struct {
    int    id;
    char   front[MAX_STR];
    char   back[MAX_STR];
    double easiness;
    int    interval;
    int    repetitions;
    time_t next_review;
    int    total_reviews;
    int    correct_total;
    time_t created_at;
} Card;

typedef struct {
    int    id;
    char   name[MAX_STR];
    int    card_count;
    int    card_ids[MAX_CARDS];
    time_t created_at;
    int    total_sessions;
    int    total_correct;
    int    total_reviewed;
} Deck;

typedef struct {
    int  magic;
    int  deck_count;
    int  card_count;
    int  total_sessions_global;
    Deck decks[MAX_DECKS];
    Card cards[MAX_CARDS];
} AppData;

static AppData g;

/* ??????????????????????????????????????????
Utilities
?????????????????????????????????????????? */

void clear_screen(void) {
    printf("\033[2J\033[H");
}

void sep(void) {
    puts("--------------------------------------------------");
}

void press_enter(void) {
    printf("  [Press Enter to continue]");
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF);
}

/* Read one line; strip trailing \n and \r (handles Windows CRLF) */
void read_line(char *buf, int maxlen) {
    if (!fgets(buf, maxlen, stdin)) { buf[0] = '\0'; return; }
    size_t len = strlen(buf);
    while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r'))
        buf[--len] = '\0';
}

int read_int(void) {
    char buf[32];
    read_line(buf, sizeof(buf));
    return atoi(buf);
}

/* Text progress bar: [====------] done/total */
void print_progress(int done, int total, int width) {
    if (total == 0) return;
    int filled = done * width / total;
    printf("  [");
    for (int i = 0; i < width; i++) putchar(i < filled ? '=' : '-');
    printf("] %d/%d\n", done, total);
}

/* ??????????????????????????????????????????
Save / Load
?????????????????????????????????????????? */

void save_data(void) {
    FILE *fp = fopen(DATA_FILE, "wb");
    if (!fp) { puts("  [Error] Failed to save data."); return; }
    fwrite(&g, sizeof(AppData), 1, fp);
    fclose(fp);
}

int load_data(void) {
    FILE *fp = fopen(DATA_FILE, "rb");
    if (!fp) return 0;
    int ok = (fread(&g, sizeof(AppData), 1, fp) == 1 && g.magic == MAGIC_NUMBER);
    fclose(fp);
    return ok;
}

void init_data(void) {
    memset(&g, 0, sizeof(AppData));
    g.magic = MAGIC_NUMBER;
}

/* ??????????????????????????????????????????
SM-2 Algorithm
quality: 0 = wrong, 1 = hard, 2 = correct
?????????????????????????????????????????? */

void sm2_update(Card *c, int quality) {
    double ef = c->easiness + (0.1 - (2 - quality) * (0.08 + (2 - quality) * 0.02));
    if (ef < MIN_EASINESS) ef = MIN_EASINESS;
    c->easiness = ef;

    if (quality < 1) {
        c->repetitions = 0;
        c->interval    = 1;
    } else {
        if      (c->repetitions == 0) c->interval = 1;
        else if (c->repetitions == 1) c->interval = 6;
        else c->interval = (int)round((double)c->interval * c->easiness);
        c->repetitions++;
    }
    c->next_review = time(NULL) + (time_t)c->interval * 86400;
}

/* ??????????????????????????????????????????
Lookup / ID helpers
?????????????????????????????????????????? */

Card *find_card(int id) {
    for (int i = 0; i < g.card_count; i++)
        if (g.cards[i].id == id) return &g.cards[i];
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
Fisher-Yates shuffle
?????????????????????????????????????????? */

void shuffle(int *arr, int n) {
    for (int i = n - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int t = arr[i]; arr[i] = arr[j]; arr[j] = t;
    }
}

/* ??????????????????????????????????????????
UI: title / header
?????????????????????????????????????????? */

void print_title(void) {
    puts("\n  FlashMaster - Spaced Repetition Study App");
    puts("  Learn efficiently with the SM-2 algorithm");
    sep();
}

void print_header(const char *title) {
    printf("\n  === %s ===\n\n", title);
}

/* ??????????????????????????????????????????
Deck list
?????????????????????????????????????????? */

void show_deck_list(void) {
    print_header("Deck List");
    if (g.deck_count == 0) {
        puts("  (No decks yet)");
        return;
    }

    time_t now = time(NULL);
    for (int i = 0; i < g.deck_count; i++) {
        Deck *d = &g.decks[i];
        int due = 0, mastered = 0;
        for (int j = 0; j < d->card_count; j++) {
            Card *c = find_card(d->card_ids[j]);
            if (!c) continue;
            if (c->next_review <= now) due++;
            if (c->repetitions >= 3)  mastered++;
        }
        printf("  [%d] %s  (%d cards / mastered:%d / due today:%d)\n",i + 1, d->name, d->card_count, mastered, due);
    }
    putchar('\n');
}

/* ??????????????????????????????????????????
Create deck
?????????????????????????????????????????? */

void create_deck(void) {
    if (g.deck_count >= MAX_DECKS) { puts("  Deck limit reached."); return; }

    print_header("Create Deck");
    printf("  Deck name: ");
    char name[MAX_STR];
    read_line(name, sizeof(name));
    if (strlen(name) == 0) { puts("  Cancelled."); return; }

    Deck *d       = &g.decks[g.deck_count++];
    memset(d, 0, sizeof(Deck));
    d->id         = new_deck_id();
    d->created_at = time(NULL);
    strncpy(d->name, name, MAX_STR - 1);

    save_data();
    printf("  Deck \"%s\" created.\n", d->name);
    press_enter();
}

/* ??????????????????????????????????????????
Add cards
?????????????????????????????????????????? */

void add_cards_to_deck(Deck *d) {
    print_header("Add Cards");
    printf("  Deck: %s\n", d->name);
    puts("  Enter front and back for each card. Leave front blank to finish.\n");

    int added = 0;
    while (g.card_count < MAX_CARDS && d->card_count < MAX_CARDS) {
        printf("  Card %d\n", added + 1);
        printf("    Front (question): ");
        char front[MAX_STR], back[MAX_STR];
        read_line(front, sizeof(front));
        if (strlen(front) == 0) break;

        /* Duplicate check: same front text already exists in this deck? */
        int duplicate = 0;
        for (int i = 0; i < d->card_count; i++) {
            Card *existing = find_card(d->card_ids[i]);
            if (existing && strcmp(existing->front, front) == 0) {
                duplicate = 1;
                break;
            }
        }
        if (duplicate) {
            puts("    Skipped: a card with that front already exists in this deck.");
            continue;
        }

        printf("    Back  (answer)  : ");
        read_line(back, sizeof(back));

        Card *c        = &g.cards[g.card_count++];
        memset(c, 0, sizeof(Card));
        c->id          = new_card_id();
        c->easiness    = INIT_EASINESS;
        c->interval    = 1;
        c->created_at  = time(NULL);
        c->next_review = time(NULL);
        strncpy(c->front, front, MAX_STR - 1);
        strncpy(c->back,  back,  MAX_STR - 1);

        d->card_ids[d->card_count++] = c->id;
        added++;
        puts("    Added.");
    }

    if (added > 0) {
        save_data();
        printf("\n  %d card(s) added.\n", added);
    } else {
        puts("  No cards added.");
    }
    press_enter();
}

/* ??????????????????????????????????????????
Edit / delete cards
?????????????????????????????????????????? */

void edit_cards(Deck *d) {
    while (1) {
        clear_screen();
        print_header("Edit Cards");
        printf("  Deck: %s  (%d cards)\n\n", d->name, d->card_count);

        if (d->card_count == 0) {
            puts("  No cards.");
            press_enter();
            return;
        }

        for (int i = 0; i < d->card_count; i++) {
            Card *c = find_card(d->card_ids[i]);
            if (!c) continue;
            printf("  [%d] Front: %-30s  Back: %s\n", i + 1, c->front, c->back);
        }

        puts("\n  number=edit  d+number=delete  0=back");
        printf("  > ");
        char input[32];
        read_line(input, sizeof(input));

        if (strcmp(input, "0") == 0) break;

        /* Delete */
        if (input[0] == 'd' || input[0] == 'D') {
            int idx = atoi(input + 1) - 1;
            if (idx < 0 || idx >= d->card_count) {
                puts("  Invalid number.");
                press_enter();
                continue;
            }
            printf("  Really delete? [y/N]: ");
            int yn = choose((choosing){"Yes","No","","","","","","",2});
            if (yn != 0) continue;

            int rem_id = d->card_ids[idx];
            for (int i = idx; i < d->card_count - 1; i++)
                d->card_ids[i] = d->card_ids[i + 1];
            d->card_count--;
            for (int i = 0; i < g.card_count; i++) {
                if (g.cards[i].id == rem_id) {
                    for (int j = i; j < g.card_count - 1; j++)
                        g.cards[j] = g.cards[j + 1];
                    g.card_count--;
                    break;
                }
            }
            save_data();
            puts("  Deleted.");
            press_enter();
            continue;
        }

        /* Edit */
        int idx = atoi(input) - 1;
        if (idx < 0 || idx >= d->card_count) {
            puts("  Invalid number.");
            press_enter();
            continue;
        }
        Card *c = find_card(d->card_ids[idx]);
        if (!c) continue;

        printf("  Current front: %s\n", c->front);
        printf("  New front (blank=keep): ");
        char buf[MAX_STR];
        read_line(buf, sizeof(buf));
        if (strlen(buf) > 0) strncpy(c->front, buf, MAX_STR - 1);

        printf("  Current back: %s\n", c->back);
        printf("  New back (blank=keep): ");
        read_line(buf, sizeof(buf));
        if (strlen(buf) > 0) strncpy(c->back, buf, MAX_STR - 1);

        save_data();
        puts("  Updated.");
        press_enter();
    }
}

/* ??????????????????????????????????????????
Study session
?????????????????????????????????????????? */

void study_session(Deck *d) {
    if (d->card_count == 0) {
        puts("  No cards in this deck.");
        press_enter();
        return;
    }

    clear_screen();
    print_title();
    printf("  Deck: %s  (%d cards)\n", d->name, d->card_count);
    puts("  Keep going until every card is marked correct.");
    press_enter();

    int mastered[MAX_CARDS] = {0};
    int queue[MAX_CARDS];
    int queue_size = d->card_count;
    for (int i = 0; i < queue_size; i++) queue[i] = i;
    shuffle(queue, queue_size);

    int session_correct = 0, session_wrong = 0, session_total = 0;
    int remaining = queue_size;
    time_t session_start = time(NULL);

    while (remaining > 0) {
        /* Rebuild queue from unmastered cards */
        int next[MAX_CARDS], next_size = 0;
        for (int i = 0; i < queue_size; i++)
            if (!mastered[queue[i]]) next[next_size++] = queue[i];
        shuffle(next, next_size);
        queue_size = next_size;
        for (int i = 0; i < queue_size; i++) queue[i] = next[i];

        /* One pass through the queue */
        for (int qi = 0; qi < queue_size; qi++) {
            if (mastered[queue[qi]]) continue;

            int deck_idx = queue[qi];
            Card *c = find_card(d->card_ids[deck_idx]);
            if (!c) continue;

            session_total++;

            /* Front side */
            clear_screen();
            printf("\n  Progress: %d/%d  (correct:%d  wrong:%d)\n",d->card_count - remaining, d->card_count,session_correct, session_wrong);
            print_progress(d->card_count - remaining, d->card_count, 30);
            sep();
            printf("\n  Question\n\n  %s\n\n", c->front);
            printf("  [streak:%d / interval:%d days]\n\n", c->repetitions, c->interval);
            sep();
            press_enter();

            /* Back side */
            clear_screen();
            printf("\n  Progress: %d/%d  (correct:%d  wrong:%d)\n",d->card_count - remaining, d->card_count,session_correct, session_wrong);
            print_progress(d->card_count - remaining, d->card_count, 30);
            sep();
            printf("\n  Question\n\n  %s\n\n", c->front);
            puts("  ----------");
            printf("\n  Answer\n\n  %s\n\n", c->back);
            sep();

            int ans = choose((choosing){"Got it","Try again","","","","","","",2});

            c->total_reviews++;
            if (ans == 0) {
                sm2_update(c, 2);
                c->correct_total++;
                session_correct++;
                mastered[deck_idx] = 1;
                remaining--;
                puts(remaining == 0 ? "\n  All cards done! Great job!" : "\n  Correct!");
            } else {
                sm2_update(c, 0);
                session_wrong++;
                puts("\n  Keep trying!");
            }
            press_enter();
        }

        /* Recount remaining */
        remaining = 0;
        for (int i = 0; i < d->card_count; i++)
            if (!mastered[i]) remaining++;
    }

    /* Session summary */
    time_t elapsed = time(NULL) - session_start;
    d->total_sessions++;
    d->total_correct  += session_correct;
    d->total_reviewed += session_total;
    g.total_sessions_global++;
    save_data();

    clear_screen();
    print_title();
    puts("  Session complete!\n");
    printf("  Deck        : %s\n",   d->name);
    printf("  Correct     : %d\n",   session_correct);
    printf("  Wrong       : %d\n",   session_wrong);
    printf("  Total tries : %d\n",   session_total);
    printf("  Time        : %d min %02d sec\n",
            (int)elapsed / 60, (int)elapsed % 60);
    if (session_total > 0)
        printf("  Accuracy    : %.1f%%\n",
               (double)session_correct / d->card_count * 100.0);

    double acc = session_total > 0
               ? (double)session_correct / session_total * 100.0 : 0.0;
    puts(acc >= 90 ? "\n  Perfect! Keep it up!"
        : acc >= 70 ? "\n  Good work! Consistency is key."
        : "\n  Keep reviewing - you will get there!");

    press_enter();
}

/* ??????????????????????????????????????????
Stats
?????????????????????????????????????????? */

void show_stats(void) {
    print_header("Study Stats");
    printf("  Total decks    : %d\n", g.deck_count);
    printf("  Total cards    : %d\n", g.card_count);
    printf("  Total sessions : %d\n\n", g.total_sessions_global);

    time_t now = time(NULL);
    for (int i = 0; i < g.deck_count; i++) {
        Deck *d = &g.decks[i];
        int mastered = 0, due = 0;
        for (int j = 0; j < d->card_count; j++) {
            Card *c = find_card(d->card_ids[j]);
            if (!c) continue;
            if (c->repetitions >= 3) mastered++;
            if (c->next_review <= now) due++;
        }

        sep();
        printf("  %s\n", d->name);
        printf("    Cards        : %d\n", d->card_count);
        printf("    Mastered     : %d (%.0f%%)\n",mastered,
               d->card_count > 0 ? (double)mastered / d->card_count * 100.0 : 0.0);
        printf("    Due today    : %d\n", due);
        printf("    Sessions     : %d\n", d->total_sessions);
        printf("    Total correct: %d\n", d->total_correct);
        if (d->card_count > 0) {
            printf("    Mastery      : ");
            print_progress(mastered, d->card_count, 20);
        }
        putchar('\n');
    }

    press_enter();
}

/* ??????????????????????????????????????????
Deck menu
?????????????????????????????????????????? */

void deck_menu(Deck *d) {
    while (1) {
        clear_screen();
        print_title();
        printf("  Deck: %s  (%d cards)\n", d->name, d->card_count);

        int due = 0;
        time_t now = time(NULL);
        for (int i = 0; i < d->card_count; i++) {
            Card *c = find_card(d->card_ids[i]);
            if (c && c->next_review <= now) due++;
        }
        if (due > 0) printf("  Due today: %d card(s)\n", due);
        putchar('\n');

        int ch = choose((choosing){"Study","Add cards","Edit / delete cards","Back","","","","",4});

        switch (ch) {
            case 0: study_session(d); break;
            case 1: clear_screen(); add_cards_to_deck(d); break;
            case 2: clear_screen(); edit_cards(d); break;
            case 3: return;
            default: puts("  Invalid choice."); press_enter();
        }
    }
}

/* ??????????????????????????????????????????
Main menu
?????????????????????????????????????????? */

void main_menu(void) {
    while (1) {
        clear_screen();
        print_title();

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
                printf("  Due today across all decks: %d card(s)\n\n", total_due);
        }

        show_deck_list();

        int ch = choose((choosing){"Study a deck","Create a new deck","Delete a deck","View stats","Quit","","","",5});
        if (ch == 4) {
            puts("\n  Goodbye! See you next time.");
            break;
        } else if (ch == 1) {
            clear_screen(); create_deck();
        } else if (ch == 2) {
            /* Delete a deck */
            if (g.deck_count == 0) {
                puts("  No decks to delete.");
                press_enter();
                continue;
            }
            printf("  Deck number to delete: ");
            int idx = read_int() - 1;
            if (idx < 0 || idx >= g.deck_count) {
                puts("  Invalid number.");
                press_enter();
                continue;
            }
            printf("  Delete deck \"%s\" and all its cards?  \n",g.decks[idx].name);
            int yn = choose((choosing){"Yes","No","","","","","","",2});
            if (yn != 0) continue;

            /* Remove all cards that belong to this deck */
            Deck *del = &g.decks[idx];
            for (int j = 0; j < del->card_count; j++) {
                int rem_id = del->card_ids[j];
                for (int k = 0; k < g.card_count; k++) {
                    if (g.cards[k].id == rem_id) {
                        for (int m = k; m < g.card_count - 1; m++)
                            g.cards[m] = g.cards[m + 1];
                        g.card_count--;
                        break;
                    }
                }
            }
            /* Remove the deck itself */
            for (int i = idx; i < g.deck_count - 1; i++)
                g.decks[i] = g.decks[i + 1];
            g.deck_count--;
            save_data();
            puts("  Deck deleted.");
            press_enter();
        } else if (ch == 3) {
            clear_screen(); show_stats();
        } else if (ch == 0) {
            if (g.deck_count == 0) {
                puts("  No decks yet. Create one first.");
                press_enter();
                continue;
            }
            printf("  Deck number: ");
            int idx = read_int() - 1;
            if (idx < 0 || idx >= g.deck_count) {
                puts("  Invalid number.");
                press_enter();
                continue;
            }
            deck_menu(&g.decks[idx]);
        } else {
            puts("  Invalid choice.");
            press_enter();
        }
    }
}

/* ??????????????????????????????????????????
Entry point
?????????????????????????????????????????? */

int main(void) {
#ifdef _WIN32
    /* Use Shift-JIS (CP932) for both input and output on Windows */
    SetConsoleCP(932);
    SetConsoleOutputCP(932);
#endif
    srand((unsigned int)time(NULL));
    if (!load_data()) init_data();
    main_menu();
    return 0;
}