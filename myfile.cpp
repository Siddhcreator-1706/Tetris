#include <iostream>
#include <thread>
#include <vector>
#include <ncurses.h>
#include <chrono>
#include <cstdlib>

using namespace std;

// Constants
const int FIELD_WIDTH = 12, FIELD_HEIGHT = 22, TETROMINO_SIZE = 4;
const wstring TETROMINOS[7] = {
    L"..X...X...X...X.", L"..X..XX...X.....", L".....XX..XX.....", 
    L"..X..XX..X......", L".X...XX...X.....", L".X...X...XX.....", L"..X...X..XX....."
};

// Game class
class Tetris {
    vector<vector<int>> field{FIELD_HEIGHT, vector<int>(FIELD_WIDTH, 0)}; // Game field grid
    int currentPiece{rand() % 7}, currentRotation{}, currentX{FIELD_WIDTH / 2 - 2}, currentY{}; // Current piece state
    int score{}, level{1}, speed{20}, linesCleared{}; // Game state variables
    bool isGameOver{}, isPaused{}; // Game status flags

    // Rotates a point in the tetromino based on rotation state
    int Rotate(int px, int py, int r) {
        switch (r % 4) {
            case 0: return py * TETROMINO_SIZE + px;
            case 1: return 12 + py - (px * TETROMINO_SIZE);
            case 2: return 15 - (py * TETROMINO_SIZE) - px;
            case 3: return 3 - py + (px * TETROMINO_SIZE);
        }
        return 0;
    }

    // Checks if the current piece fits at the given position
    bool DoesPieceFit(int piece, int rotation, int posX, int posY) {
        for (int px = 0; px < TETROMINO_SIZE; px++)
            for (int py = 0; py < TETROMINO_SIZE; py++) {
                int pi = Rotate(px, py, rotation);
                int fi = (posY + py) * FIELD_WIDTH + (posX + px);
                if ((posX + px >= 0 && posX + px < FIELD_WIDTH && posY + py >= 0 && posY + py < FIELD_HEIGHT) ?
                    (TETROMINOS[piece][pi] != L'.' && field[posY + py][posX + px] != 0) :
                    (TETROMINOS[piece][pi] != L'.'))
                    return false;
            }
        return true;
    }

    // Clears completed lines and shifts blocks above them down
    void ClearLines() {
        int linesClearedThisTurn = 0;
        for (int y = FIELD_HEIGHT - 2; y > 0; y--) {
            bool isLineComplete = true;
            for (int x = 1; x < FIELD_WIDTH - 1; x++)
                if (field[y][x] == 0) { isLineComplete = false; break; }

            if (isLineComplete) {
                for (int x = 1; x < FIELD_WIDTH - 1; x++) field[y][x] = 0;
                for (int yy = y - 1; yy > 0; yy--)
                    for (int x = 1; x < FIELD_WIDTH - 1; x++)
                        field[yy + 1][x] = field[yy][x], field[yy][x] = 0;
                linesClearedThisTurn++;
                y++; // Recheck the same line after shifting
            }
        }

        // Update score based on the number of lines cleared
        switch (linesClearedThisTurn) {
            case 1: score += 100 * level;        // Play sound for line clear
        playSound(); // Higher pitch for line clear 
break;
            case 2: score += 300 * level;        // Play sound for line clear
        playSound(); // Higher pitch for line clear 
break;
            case 3: score += 500 * level;        // Play sound for line clear
        playSound(); // Higher pitch for line clear 
break;
            case 4: score += 800 * level;        // Play sound for line clear
        playSound(); // Higher pitch for line clear
 break;
        }

        // Update total lines cleared and level
        linesCleared += linesClearedThisTurn;
        if (linesCleared >= 5) {
            level++;
            linesCleared -= 5;
            speed = max(5, speed - 2); // Increase speed (lower delay)
        }

    }

    // Play sound using Beep (Windows) or usleep (Unix-like systems)
    void playSound() {
        system("ffplay -nodisp -autoexit ~/sounds/beep.wav 2>/dev/null &");
    }

public:
    // Constructor: Initializes the game field with borders
    Tetris() {
        for (int x = 0; x < FIELD_WIDTH; x++)
            for (int y = 0; y < FIELD_HEIGHT; y++)
                field[y][x] = (x == 0 || x == FIELD_WIDTH - 1 || y == FIELD_HEIGHT - 1) ? 9 : 0;
    }

    // Handles user input for moving, rotating, and pausing the game
    void ProcessInput(int ch) {
        if (isPaused) { if (ch == 'p' || ch == 'P') isPaused = false; return; }
        switch (ch) {
            case KEY_LEFT: if (DoesPieceFit(currentPiece, currentRotation, currentX - 1, currentY)) currentX--; break;
            case KEY_RIGHT: if (DoesPieceFit(currentPiece, currentRotation, currentX + 1, currentY)) currentX++; break;
            case KEY_UP: if (DoesPieceFit(currentPiece, currentRotation + 1, currentX, currentY)) currentRotation++; break;
            case KEY_DOWN: if (DoesPieceFit(currentPiece, currentRotation, currentX, currentY + 1)) currentY++; break;
            case ' ': while (DoesPieceFit(currentPiece, currentRotation, currentX, currentY + 1)) currentY++; break;
            case 27: isPaused = true; break;
        }
    }

    // Updates the game state: moves the piece, checks for collisions, and clears lines
    void Update() {
        if (isPaused || isGameOver) return;
        static int speedCount = 0;
        if (++speedCount == speed) {
            speedCount = 0;
            if (DoesPieceFit(currentPiece, currentRotation, currentX, currentY + 1)) {
                currentY++;
            } else {
                // Award points when a block lands
                score += 10 * level; // Points for landing a block
		system("ffplay -nodisp -autoexit ~/sounds/beep-07a.wav 2>/dev/null &");

                for (int px = 0; px < TETROMINO_SIZE; px++)
                    for (int py = 0; py < TETROMINO_SIZE; py++)
                        if (TETROMINOS[currentPiece][Rotate(px, py, currentRotation)] != L'.')
                            field[currentY + py][currentX + px] = currentPiece + 1;

                ClearLines();

                currentX = FIELD_WIDTH / 2 - 2, currentY = 0, currentRotation = 0, currentPiece = rand() % 7;
                if (!DoesPieceFit(currentPiece, currentRotation, currentX, currentY)) isGameOver = true;
            }
        }
    }

    // Draws the game field, current piece, score, level, and pause message
    void Draw() {
        erase();
        for (int y = 0; y < FIELD_HEIGHT; y++)
            for (int x = 0; x < FIELD_WIDTH; x++) {
                int cell = field[y][x];
                if (cell > 0 && cell < 9) {
                    attron(COLOR_PAIR(cell)); // Set color for the block
                    mvprintw(y + 2, x + 2, " ");
                    attroff(COLOR_PAIR(cell));
                } else {
                    mvprintw(y + 2, x + 2, "%c", " ABCDEFG=#"[cell]);
                }
            }

        for (int px = 0; px < TETROMINO_SIZE; px++)
            for (int py = 0; py < TETROMINO_SIZE; py++)
                if (TETROMINOS[currentPiece][Rotate(px, py, currentRotation)] != L'.') {
                    attron(COLOR_PAIR(currentPiece + 1)); // Set color for the current piece
                    mvprintw(currentY + py + 2, currentX + px + 2, " ");
                    attroff(COLOR_PAIR(currentPiece + 1));
                }

        mvprintw(1, FIELD_WIDTH + 5, "Score: %d", score);
        mvprintw(2, FIELD_WIDTH + 5, "Level: %d", level);
        if (isPaused) mvprintw(FIELD_HEIGHT / 2, FIELD_WIDTH / 2 - 5, "PAUSED");
        refresh();
    }

    // Returns whether the game is over
    bool IsGameOver() const { return isGameOver; }

    // Returns the current score
    int GetScore() const { return score; }
};

// Main function: Initializes the game and runs the main loop
int main() {
    initscr(); // Initialize ncurses
    noecho(); // Don't echo input
    curs_set(0); // Hide cursor
    nodelay(stdscr, TRUE); // Non-blocking input
    keypad(stdscr, TRUE); // Enable keypad
    start_color(); // Enable color

    // Define color pairs for each tetromino
    init_pair(1, COLOR_CYAN, COLOR_CYAN);    // I
    init_pair(2, COLOR_BLUE, COLOR_BLUE);    // J
    init_pair(3, COLOR_WHITE, COLOR_WHITE);  // L
    init_pair(4, COLOR_YELLOW, COLOR_YELLOW);// O
    init_pair(5, COLOR_GREEN, COLOR_GREEN);  // S
    init_pair(6, COLOR_MAGENTA, COLOR_MAGENTA); // T
    init_pair(7, COLOR_RED, COLOR_RED);      // Z

    Tetris game;

    while (!game.IsGameOver()) {
        int ch = getch();
        game.ProcessInput(ch);
        game.Update();
        game.Draw();
        this_thread::sleep_for(chrono::milliseconds(50));
    }

    endwin();
    cout << "Game Over! Final Score: " << game.GetScore() << endl;
    return 0;
}