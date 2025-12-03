#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include "os_generic.h"
#include <GLES3/gl3.h>
#include <android_native_app_glue.h>
#include "CNFGAndroid.h"

#define CNFG_IMPLEMENTATION
#include "CNFG.h"

#define MAX_ENTRIES 1000
#define FILENAME "streak_data.txt"

typedef struct {
    int year;
    int month;
    int day;
    time_t timestamp;
} DateEntry;

DateEntry entries[MAX_ENTRIES];
int entryCount = 0;

// input & ui state
char inputBuffer[64] = {0};
int inputLen = 0;
int isTyping = 0;
int streakCount = 0;

// scrolling & touch state
int scrollY = 0;
int lastTouchY = 0;
int startTouchY = 0;
int isDragging = 0;
int isTap = 0; 

// helper to center text
void DrawTextCentered(const char* text, int scale, int y, int screenW) {
    int charWidth = 6 * scale;
    int textLen = strlen(text);
    int totalWidth = textLen * charWidth;
    
    CNFGPenX = (screenW - totalWidth) / 2;
    CNFGPenY = y;
    CNFGDrawText(text, scale);
}

// sorts dates newest first
int CompareDates(const void* a, const void* b) {
    return (int)(((DateEntry*)b)->timestamp - ((DateEntry*)a)->timestamp);
}

void CalculateStreak() {
    if (entryCount == 0) { streakCount = 0; return; }
    qsort(entries, entryCount, sizeof(DateEntry), CompareDates);

    time_t now = time(0);
    double secondsSinceLast = difftime(now, entries[0].timestamp);
    int daysSinceLast = (int)(secondsSinceLast / (60 * 60 * 24));

    if (daysSinceLast > 1) {
        streakCount = 0;
        return;
    }

    streakCount = 1;
    for (int i = 0; i < entryCount - 1; i++) {
        double diff = difftime(entries[i].timestamp, entries[i+1].timestamp);
        int daysDiff = (int)(diff / (60 * 60 * 24));

        if (daysDiff == 1) streakCount++;
        else if (daysDiff == 0) continue;
        else break;
    }
}

void SaveData() {
    const char* path = AndroidGetExternalFilesDir();
    char fullPath[256];
    snprintf(fullPath, sizeof(fullPath), "%s/%s", path, FILENAME);
    FILE* f = fopen(fullPath, "w");
    if (!f) return;
    for (int i = 0; i < entryCount; i++) {
        fprintf(f, "%d %d %d\n", entries[i].year, entries[i].month, entries[i].day);
    }
    fclose(f);
}

void LoadData() {
    const char* path = AndroidGetExternalFilesDir();
    char fullPath[256];
    snprintf(fullPath, sizeof(fullPath), "%s/%s", path, FILENAME);
    FILE* f = fopen(fullPath, "r");
    if (!f) return;
    entryCount = 0;
    struct tm tm = {0};
    while (fscanf(f, "%d %d %d", &tm.tm_year, &tm.tm_mon, &tm.tm_mday) == 3 && entryCount < MAX_ENTRIES) {
        entries[entryCount].year = tm.tm_year;
        entries[entryCount].month = tm.tm_mon;
        entries[entryCount].day = tm.tm_mday;
        tm.tm_year -= 1900; tm.tm_mon -= 1; tm.tm_hour = 12;    
        entries[entryCount].timestamp = mktime(&tm);
        entryCount++;
    }
    fclose(f);
    CalculateStreak();
}

void AddEntry(int y, int m, int d) {
    if (entryCount >= MAX_ENTRIES) return;
    struct tm tm = {0};
    tm.tm_year = y - 1900; tm.tm_mon = m - 1; tm.tm_mday = d; tm.tm_hour = 12;
    entries[entryCount].year = y; entries[entryCount].month = m; entries[entryCount].day = d;
    entries[entryCount].timestamp = mktime(&tm);
    entryCount++;
    SaveData();
    CalculateStreak();
}

void LazyEntry() {
    if (entryCount >= MAX_ENTRIES) return;
    
    struct tm tm = {0};
    
    if (entryCount == 0) {
        time_t t = time(0);
        struct tm *now = localtime(&t);
        tm = *now;
    } else {
        // take newest entry
        tm.tm_year = entries[0].year - 1900;
        tm.tm_mon = entries[0].month - 1;
        tm.tm_mday = entries[0].day + 1;
        tm.tm_hour = 12;
        tm.tm_isdst = -1;
        
        // next month or next year fix
        mktime(&tm);
    }
    
    AddEntry(tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
}

void Undo() {
    if (entryCount <= 0) return;
    
    // remove the newest entry (index 0) and shift everything up
    for (int i = 0; i < entryCount - 1; i++) {
        entries[i] = entries[i+1];
    }
    
    entryCount--;
    SaveData();
    CalculateStreak();
}

volatile int suspended = 0;

void HandleKey( int keycode, int bDown ) {
    if (keycode == 4) { exit(0); } // back button
    if (!bDown || !isTyping) return;

    // enter key
    if (keycode == 66 || keycode == 10 || keycode == 13) {
        int y, m, d;
        if (sscanf(inputBuffer, "%d %d %d", &y, &m, &d) == 3) {
            AddEntry(y, m, d);
            inputBuffer[0] = 0; inputLen = 0; isTyping = 0;
            AndroidDisplayKeyboard(0); 
        }
        return;
    }
    // backspace
    if (keycode == 67 || keycode == 127 || keycode == 8) {
        if (inputLen > 0) inputBuffer[--inputLen] = 0;
        return;
    }
    // typing
    char c = 0;
    if (keycode >= '0' && keycode <= '9') c = keycode;
    else if (keycode >= 7 && keycode <= 16) c = '0' + (keycode - 7);
    else if (keycode == 62 || keycode == 32) c = ' ';

    if (c != 0 && inputLen < 63) {
        inputBuffer[inputLen++] = c; inputBuffer[inputLen] = 0;
    }
}

void HandleButton( int x, int y, int button, int bDown ) {
    short w, h;
    CNFGGetDimensions(&w, &h);

    if (bDown) {
        lastTouchY = y;
        startTouchY = y;
        isTap = 1; 
        isDragging = 1;
    } else {
        isDragging = 0;
        
        if (isTap) {
            char streakStr[32];
            sprintf(streakStr, "Streak: %d", streakCount);
            int charWidth = 6 * 10;
            int textWidth = strlen(streakStr) * charWidth;
            int textEndX = (w + textWidth) / 2;
            
            int btnY = 100;
            int btnH = 60;
            int btnX = textEndX + 20;
            int btnW = 60;
            
            // check lazy
            if (x > btnX && x < btnX + btnW && y > btnY && y < btnY + btnH) {
                LazyEntry();
                return;
            }
            
            // check undo
            int undoY = btnY + 70;
            if (x > btnX && x < btnX + btnW && y > undoY && y < undoY + btnH) {
                Undo();
                return;
            }

            // if keyboard is open, checking taps anywhere outside input box closes it
            if (isTyping) {
                isTyping = 0;
                AndroidDisplayKeyboard(0);
            }
            // if keyboard is closed, tapping add date opens it
            else if (y > h - 150) {
                isTyping = 1;
                AndroidDisplayKeyboard(1);
            }
        }
    }
}

void HandleMotion( int x, int y, int mask ) {
    if (isDragging) {
        int dy = y - lastTouchY;
        if (abs(y - startTouchY) > 10) isTap = 0;
        
        // only scroll if not typing
        if (!isTyping) {
            scrollY += dy;
            if (scrollY > 0) scrollY = 0; 
            int maxScroll = -(entryCount * 80); 
            if (maxScroll > 0) maxScroll = 0;
            if (scrollY < maxScroll - 500) scrollY = maxScroll - 500; 
        }
        lastTouchY = y;
    }
}

int HandleDestroy() { return 0; }
void HandleSuspend() { suspended = 1; }
void HandleResume() { suspended = 0; }

int main( int argc, char ** argv ) {
    CNFGSetupFullscreen( "Streak Tracker", 0 );
    LoadData(); 

    while(1) {
        CNFGHandleInput();
        if( suspended ) { usleep(50000); continue; }

        CNFGBGColor = 0x000000FF; 
        CNFGClearFrame();
        
        short w, h;
        CNFGGetDimensions( &w, &h );
        if (w == 0 || h == 0) { CNFGSwapBuffers(); continue; }
        
        // draw streak count
        CNFGColor(0xFFFFFFFF);
        char streakStr[32];
        sprintf(streakStr, "Streak: %d", streakCount);
        CNFGSetLineWidth(5); 
        DrawTextCentered(streakStr, 10, 100, w);

        // draw header buttons
        int charWidth = 6 * 10;
        int textWidth = strlen(streakStr) * charWidth;
        int textEndX = (w + textWidth) / 2;
        
        // lazy
        CNFGColor(0x00FF00FF);
        CNFGPenX = textEndX + 20;
        CNFGPenY = 100;
        CNFGDrawText("+", 10);
        
        // undo
        CNFGColor(0xFF0000FF);
        CNFGPenX = textEndX + 20;
        CNFGPenY = 170; // 100 + 70
        CNFGDrawText("<", 10);

        // draw scrollable list
        CNFGColor(0xFFFFFFFF);
        CNFGSetLineWidth(2);
        int listStartY = 300 + scrollY;
        char dateStr[64];
        
        for(int i = 0; i < entryCount; i++) {
            struct tm* t = localtime(&entries[i].timestamp);
            strftime(dateStr, sizeof(dateStr), "%A, %b %d %Y", t);
            
            int itemY = listStartY + (i * 80);
            if (itemY < -50 || itemY > h) continue;

            DrawTextCentered(dateStr, 5, itemY, w);
        }

        // ui layer
        if (isTyping) {
            CNFGColor(0x000000CC);
            CNFGTackRectangle(0, 0, w, h);

            // avoid keyboard obstruction
            int boxY = h / 5;
            int boxH = 250;
            
            CNFGColor(0x333333FF);
            CNFGTackRectangle(20, boxY, w-20, boxY + boxH);
            
            // instructions
            CNFGColor(0xAAAAAAFF);
            DrawTextCentered("Enter Date:", 4, boxY + 30, w);
            DrawTextCentered("YYYY MM DD", 3, boxY + 80, w);

            // actual input
            CNFGColor(0x00FF00FF);
            char prompt[100];
            sprintf(prompt, "%s_", inputBuffer);
            DrawTextCentered(prompt, 6, boxY + 150, w);
            
        } else {
            // draw black backing to hide scrolling text
            CNFGColor(0x000000FF); 
            CNFGTackRectangle(0, h-150, w, h);

            CNFGColor(0x333333FF);
            CNFGTackRectangle(0, h-150, w, h);
            CNFGColor(0xFFFFFFFF);
            DrawTextCentered("+ Add Date", 5, h-110, w);
        }

        CNFGSwapBuffers();
    }

    return 0;
}
