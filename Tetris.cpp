#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <queue>
#include <fstream>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <string>

// Platform-specific includes
#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#else
#include <ncurses.h>
#include <unistd.h>
#endif
using namespace std;

// Constants
const int FIELD_WIDTH = 12, FIELD_HEIGHT = 22, TETROMINO_SIZE = 4;
const wstring TETROMINOS[7] = {
    L"..X...X...X...X.", L"..X..XX...X.....", L".....XX..XX.....",
    L"..X..XX..X......", L".X...XX...X.....", L".X...X...XX.....", L"..X...X..XX....."};

// Structure to hold score information
struct HighScore
{
    string playerName;
    int score;
};

void ClearScreen()
{
#ifdef _WIN32
    system("cls");
#else
    clear();
#endif
}
void SetCursorPosition(int x, int y)
{
#ifdef _WIN32
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD coord = {(SHORT)x, (SHORT)y};
    SetConsoleCursorPosition(hConsole, coord);
#else
    printf("\033[%d;%dH", y + 1, x + 1);
#endif
}

#ifdef _WIN32
void FlushInputBuffer()
{
    // Flush the console input buffer
    FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));

    // Also clear any remaining characters in the buffer
    while (_kbhit())
    {
        _getch();
    }
}
#endif

#ifdef _WIN32
class ConsoleBuffer
{
private:
    HANDLE hConsole;
    CHAR_INFO *buffer;
    COORD bufferSize;
    COORD bufferCoord;
    SMALL_RECT writeRegion;
    vector<vector<bool>> dirtyCells;

public:
    ConsoleBuffer(int width, int height) : bufferSize({(SHORT)width, (SHORT)height}),
                                           dirtyCells(height, vector<bool>(width, true))
    {
        hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        bufferCoord = {0, 0};
        writeRegion = {0, 0, (SHORT)(width - 1), (SHORT)(height - 1)};
        buffer = new CHAR_INFO[width * height];
        Clear();
    }

    ~ConsoleBuffer()
    {
        delete[] buffer;
    }

    void Clear(char fillChar = ' ', WORD attributes = 0)
    {
        for (int i = 0; i < bufferSize.X * bufferSize.Y; i++)
        {
            buffer[i].Char.UnicodeChar = fillChar;
            buffer[i].Attributes = attributes;
        }
        // Mark all cells as dirty
        for (auto &row : dirtyCells)
        {
            fill(row.begin(), row.end(), true);
        }
    }

    void Write(int x, int y, const char *text, WORD attributes)
    {
        for (int i = 0; text[i] != '\0'; i++)
        {
            if (x + i < bufferSize.X && y < bufferSize.Y)
            {
                int index = y * bufferSize.X + x + i;
                buffer[index].Char.UnicodeChar = text[i];
                buffer[index].Attributes = attributes;
                dirtyCells[y][x + i] = true;
            }
        }
    }

    void Fill(int x, int y, int width, int height, char fillChar, WORD attributes)
    {
        for (int i = y; i < y + height && i < bufferSize.Y; i++)
        {
            for (int j = x; j < x + width && j < bufferSize.X; j++)
            {
                int index = i * bufferSize.X + j;
                buffer[index].Char.UnicodeChar = fillChar;
                buffer[index].Attributes = attributes;
                dirtyCells[i][j] = true;
            }
        }
    }

    void Draw()
    {
        // Only write the dirty regions to console
        vector<SMALL_RECT> dirtyRegions;
        // Simple implementation - could be optimized further
        // by merging adjacent dirty regions

        // For now, we'll just write the entire buffer
        WriteConsoleOutput(hConsole, buffer, bufferSize, bufferCoord, &writeRegion);

        // Clear dirty flags
        for (auto &row : dirtyCells)
        {
            fill(row.begin(), row.end(), false);
        }
    }
};
#endif

void ShowGameInstructions()
{
    ClearScreen();

#ifdef _WIN32
    // Windows version using console API
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    int consoleWidth = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    int consoleHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

    // Game Title
    SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    cout << string(consoleWidth / 2 - 10, ' ') << "     TETRIS GAME    \n\n";

    // Instruction box
    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    cout << string(consoleWidth / 2 - 20, ' ') << "                HOW TO PLAY                 \n";

    // Instruction lines
    vector<string> instructions = {
        "  - Arrange the falling blocks to complete lines",
        "  - Complete lines to earn points and level up",
        "  - The game speeds up as you progress levels",
        "  - Game ends when blocks reach the top",
        "",
        "  CONTROLS:",
        "  LEFT_ARROW, RIGHT_ARROW : Move block left/right",
        "  UP_ARROW : Rotate block",
        "  DOWN_ARROW : Soft drop (move down faster)",
        "  SPACE: Hard drop (instant drop)",
        "  S : Pause game",
        "  Ctrl + C : Quit game"};

    for (const auto &line : instructions)
    {
        cout << string(consoleWidth / 2 - 20, ' ') << line << "\n";
    }

    // Prompt to continue
    SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    cout << "\n"
         << string(consoleWidth / 2 - 15, ' ')
         << "Press any key to return to the game...";

    _getch();                                            // Wait for any key press
    SetConsoleTextAttribute(hConsole, csbi.wAttributes); // Restore original colors

#else
    // Linux version using ncurses
    initscr();
    start_color();
    init_pair(15, COLOR_CYAN, COLOR_BLACK);
    init_pair(16, COLOR_YELLOW, COLOR_BLACK);
    init_pair(17, COLOR_GREEN, COLOR_BLACK);

    // Game Title
    attron(COLOR_PAIR(15) | A_BOLD);
    mvprintw(2, COLS / 2 - 10, "     TETRIS GAME    ");
    attroff(COLOR_PAIR(15) | A_BOLD);

    // Instruction box
    attron(COLOR_PAIR(16));
    mvprintw(7, COLS / 2 - 20, "                HOW TO PLAY                 ");

    // Instruction lines
    vector<string> instructions = {
        "  - Arrange the falling blocks to complete lines",
        "  - Complete lines to earn points and level up",
        "  - The game speeds up as you progress levels",
        "  - Game ends when blocks reach the top",
        "",
        "  CONTROLS:",
        "  LEFT_KEY, RIGHT_KEY : Move block left/right",
        "  UP_KEY : Rotate block",
        "  DOWN_KEY : Soft drop (move down faster)",
        "  SPACE: Hard drop (instant drop)",
        "  S : Pause game",
        "  Ctrl+C : Quit game"};

    for (size_t i = 0; i < instructions.size(); i++)
    {
        mvprintw(9 + i, COLS / 2 - 20, "%-42s", instructions[i].c_str());
    }

    // Prompt to continue
    attron(COLOR_PAIR(17) | A_BOLD);
    mvprintw(LINES - 4, COLS / 2 - 15, "Press any key to return to the game...");
    attroff(COLOR_PAIR(17) | A_BOLD);

    refresh();
    getch();
    endwin();
#endif
}

// Function to read high scores from file
vector<HighScore> readHighScores()
{
    vector<HighScore> scores;
    ifstream file("Score.txt");

    if (file.is_open())
    {
        HighScore temp;
        while (file >> temp.playerName >> temp.score)
        {
            scores.push_back(temp);
        }
        file.close();
    }

    // Sort in descending order
    sort(scores.begin(), scores.end(), [](const HighScore &a, const HighScore &b)
         { return a.score > b.score; });

    return scores;
}

// Function to save high scores to file
void saveHighScores(const vector<HighScore> &scores)
{
    ofstream file("Score.txt");

    if (file.is_open())
    {
        for (const auto &score : scores)
        {
            file << score.playerName << " " << score.score << "\n";
        }
        file.close();
    }
}

// Function to update high scores
void updateHighScores(int newScore)
{
    vector<HighScore> scores = readHighScores();

    // If we have less than 5 scores or new score is better than the worst in top 5
    if (scores.size() < 5 || newScore > scores.back().score)
    {
        HighScore newEntry;

        // Get player name (you can modify this for your game's input method)

        cout << "\033[33m"
                "Congratulations! You made it to the top 5!\n"
             << "\033[0m";
        cout << "\033[31m"
                "Enter your name: "
             << "\033[0m";
        cin >> newEntry.playerName;
        newEntry.score = newScore;

        scores.push_back(newEntry);

        // Sort and keep only top 5
        sort(scores.begin(), scores.end(), [](const HighScore &a, const HighScore &b)
             { return a.score > b.score; });

        if (scores.size() > 5)
        {
            scores.resize(5);
        }

        saveHighScores(scores);
    }
}

// Function to display high scores
void displayHighScores()
{
    vector<HighScore> scores = readHighScores();

    cout << "\033[36m"
            "\n===== HIGH SCORES =====\n"
         << "\033[0m";
    if (scores.empty())
    {
        cout << "No high scores yet!\n";
    }
    else
    {
        for (size_t i = 0; i < scores.size(); ++i)
        {
            cout << "\033[33m" << i + 1 << ". " << scores[i].playerName << " - " << "\033[0m" << scores[i].score << "\n";
        }
    }
    cout << "\033[36m"
            "======================\n\n"
         << "\033[0m";
}

void ShowCountdownAnimation()
{
#ifdef _WIN32
    // Windows version
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);

    int startX = csbi.srWindow.Right / 2;
    int startY = csbi.srWindow.Bottom / 2;

    // Hide cursor
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hConsole, &cursorInfo);
    cursorInfo.bVisible = false;
    SetConsoleCursorInfo(hConsole, &cursorInfo);

    for (int i = 3; i > 0; i--)
    {
        ClearScreen();
        SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        COORD coord = {(SHORT)startX, (SHORT)startY};
        SetConsoleCursorPosition(hConsole, coord);
        Beep(800, 300);
        cout << i;
        Sleep((1000));
    }

    ClearScreen();
    SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    COORD coord = {(SHORT)(startX - 5), (SHORT)startY};
    SetConsoleCursorPosition(hConsole, coord);
    cout << "Game Start!";
    Sleep((1000));
    ClearScreen();

    // Restore cursor
    SetConsoleCursorInfo(hConsole, &cursorInfo);
    SetConsoleTextAttribute(hConsole, csbi.wAttributes);

#else
    // Linux version (using ncurses)
    initscr();
    noecho();
    curs_set(0);
    start_color();
    init_pair(10, COLOR_YELLOW, COLOR_BLACK);
    init_pair(9, COLOR_GREEN, COLOR_BLACK);

    int startX = COLS / 2;
    int startY = LINES / 2;

    for (int i = 3; i > 0; i--)
    {
        clear();
        attron(COLOR_PAIR(10) | A_BOLD);
        mvprintw(startY, startX, "%d", i);
        attroff(COLOR_PAIR(10) | A_BOLD);
        refresh();
        this_thread::sleep_for(chrono::milliseconds(1000));
    }

    clear();
    attron(COLOR_PAIR(9) | A_BOLD);
    mvprintw(startY, startX - 5, "Game Start!");
    attroff(COLOR_PAIR(9) | A_BOLD);
    refresh();
    this_thread::sleep_for(chrono::milliseconds(1000));
    endwin();
#endif
}

void ShowGameOverAnimation()
{
#ifdef _WIN32
    // Windows version
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);

    int startX = (csbi.srWindow.Right / 2) - 5;
    int startY = csbi.srWindow.Bottom / 2;

    // Hide cursor
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hConsole, &cursorInfo);
    cursorInfo.bVisible = false;
    SetConsoleCursorInfo(hConsole, &cursorInfo);

    const char *gameOverText = "GAME OVER";

    for (int i = 0; i < 5; i++)
    {
        ClearScreen();
        SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);

        COORD coord = {(SHORT)startX, (SHORT)startY};
        SetConsoleCursorPosition(hConsole, coord);

        // Windows doesn't have direct blink, so we'll simulate it
        if (i % 2 == 0)
        {
            cout << gameOverText;
        }
        else
        {
            // Just move cursor to same position without printing
            SetConsoleCursorPosition(hConsole, coord);
        }

        Sleep(400);
    }

    // Final display
    SetConsoleCursorPosition(hConsole, {(SHORT)startX, (SHORT)startY});
    cout << gameOverText;
    Sleep((1000));

    // Restore cursor and colors
    SetConsoleCursorInfo(hConsole, &cursorInfo);
    SetConsoleTextAttribute(hConsole, csbi.wAttributes);

#else
    // Linux version (using ncurses)
    initscr();
    noecho();
    curs_set(0);
    start_color();
    init_pair(11, COLOR_RED, COLOR_BLACK);

    int startX = COLS / 2 - 5;
    int startY = LINES / 2;

    const char *gameOverText = "GAME OVER";
    system("ffplay -nodisp -autoexit game_over.mp3 2>/dev/null &");
    for (int i = 0; i < 5; i++)
    {
        clear();
        attron(COLOR_PAIR(11) | A_BOLD);
        if (i % 2 == 0)
            attron(A_BLINK);
        mvprintw(startY, startX, "%s", gameOverText);
        attroff(A_BLINK);
        refresh();
        this_thread::sleep_for(chrono::milliseconds(400));
    }

    this_thread::sleep_for(chrono::milliseconds(1000));
    endwin();
#endif
}

class Tetris
{
private:
    vector<vector<int>> field{FIELD_HEIGHT, vector<int>(FIELD_WIDTH, 0)};
    int currentPiece, nextPiece, currentRotation;
    int currentX, currentY;
    int score, level, speed, linesCleared;
    bool isGameOver, isPaused;
#ifdef _WIN32
    ConsoleBuffer screenBuffer;
#endif

#ifdef _WIN32
    HANDLE hConsole;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
#endif

    int Rotate(int px, int py, int r)
    {
        switch (r % 4)
        {
        case 0:
            return py * TETROMINO_SIZE + px;
        case 1:
            return 12 + py - (px * TETROMINO_SIZE);
        case 2:
            return 15 - (py * TETROMINO_SIZE) - px;
        case 3:
            return 3 - py + (px * TETROMINO_SIZE);
        }
        return 0;
    }

    bool DoesPieceFit(int piece, int rotation, int posX, int posY)
    {
        for (int px = 0; px < TETROMINO_SIZE; px++)
        {
            for (int py = 0; py < TETROMINO_SIZE; py++)
            {
                int pi = Rotate(px, py, rotation);
                if (TETROMINOS[piece][pi] == L'.')
                    continue;

                int fx = posX + px;
                int fy = posY + py;

                if (fx < 0 || fx >= FIELD_WIDTH || fy < 0 || fy >= FIELD_HEIGHT)
                    return false;
                if (field[fy][fx] != 0)
                    return false;
            }
        }
        return true;
    }

    void ClearLines()
    {
        int linesClearedThisTurn = 0;
        for (int y = FIELD_HEIGHT - 2; y >= 1; y--)
        {
            bool isLineComplete = true;
            for (int x = 1; x < FIELD_WIDTH - 1; x++)
            {
                if (field[y][x] == 0)
                {
                    isLineComplete = false;
                    break;
                }
            }

            if (isLineComplete)
            {
                // Play sound (platform independent)
#ifdef _WIN32
                Beep(2000, 500);
#else
                if (linesCleared == 0)
                {
                    system("ffplay -nodisp -autoexit beep.wav 2>/dev/null &");
                }
#endif

                // Clear the line
                for (int x = 1; x < FIELD_WIDTH - 1; x++)
                    field[y][x] = 0;

                // Shift lines down
                for (int yy = y - 1; yy >= 1; yy--)
                {
                    for (int x = 1; x < FIELD_WIDTH - 1; x++)
                    {
                        field[yy + 1][x] = field[yy][x];
                        field[yy][x] = 0;
                    }
                }
                linesClearedThisTurn++;
                y++; // Re-check this line
            }
        }

        // Update score
        switch (linesClearedThisTurn)
        {
        case 1:
            score += 100 * level;
            break;
        case 2:
            score += 300 * level;
            break;
        case 3:
            score += 500 * level;
            break;
        case 4:
            score += 800 * level;
            break;
        }
        linesCleared += linesClearedThisTurn;

        // Level up
        if (linesCleared >= 5)
        {
            level++;
            linesCleared -= 5;
            speed = max(5, speed - 2);
#ifdef _WIN32
            Beep(880, 200);
#else
            system("ffplay -nodisp -autoexit beep.wav 2>/dev/null &");
#endif
        }
    }

public:
    Tetris() : currentPiece(rand() % 7), nextPiece(rand() % 7), currentRotation(0),
               currentX(FIELD_WIDTH / 2 - 2), currentY(1), score(0), level(1),
               speed(10), linesCleared(0), isGameOver(false), isPaused(false)
#ifdef _WIN32
               ,
               screenBuffer(FIELD_WIDTH * 2 + 30, FIELD_HEIGHT + 2)
#endif
    {

        // Initialize borders
        for (int y = 0; y < FIELD_HEIGHT; y++)
        {
            field[y][0] = 8;
            field[y][FIELD_WIDTH - 1] = 8;
        }
        for (int x = 0; x < FIELD_WIDTH; x++)
        {
            field[0][x] = 8;
            field[FIELD_HEIGHT - 1][x] = 8;
        }

#ifdef _WIN32
        hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        GetConsoleScreenBufferInfo(hConsole, &csbi);
#endif
    }

    void ProcessInput(int ch)
    {
        if (isPaused)
        {
            if (ch == 's' || ch == 'S')
                isPaused = false;
            return;
        }
#ifdef _WIN32
        FlushInputBuffer(); // Clear any buffered input when unpausing
#endif
        switch (ch)
        {
#ifdef _WIN32
        case 75: // Left arrow
#else
        case KEY_LEFT:
#endif
            if (DoesPieceFit(currentPiece, currentRotation, currentX - 1, currentY))
                currentX--;
            break;
#ifdef _WIN32
        case 77: // Right arrow
#else
        case KEY_RIGHT:
#endif
            if (DoesPieceFit(currentPiece, currentRotation, currentX + 1, currentY))
                currentX++;
            break;
#ifdef _WIN32
        case 72: // Up arrow
#else
        case KEY_UP:
#endif
            if (DoesPieceFit(currentPiece, currentRotation + 1, currentX, currentY))
                currentRotation++;
            break;
#ifdef _WIN32
        case 80: // Down arrow
#else
        case KEY_DOWN:
#endif
            if (DoesPieceFit(currentPiece, currentRotation, currentX, currentY + 1))
                currentY++;
            break;
        case ' ':
            while (DoesPieceFit(currentPiece, currentRotation, currentX, currentY + 1))
                currentY++;
            break;
        case 's':
        case 'S':
            isPaused = true;
            break;
        }
#ifdef _WIN32
        // Clear any additional buffered input after processing
        FlushInputBuffer();
#endif
    }

    void AppleGravity()
    {
        // Create a visited matrix to track which cells we've processed
        vector<vector<bool>> visited(FIELD_HEIGHT, vector<bool>(FIELD_WIDTH, false));

        // Process from bottom to top (skip borders)
        for (int y = FIELD_HEIGHT - 2; y >= 1; y--)
        {
            for (int x = 1; x < FIELD_WIDTH - 1; x++)
            {
                if (field[y][x] != 0 && !visited[y][x])
                {
                    int pieceID = field[y][x];
                    vector<pair<int, int>> cluster;
                    queue<pair<int, int>> toVisit;
                    toVisit.push(make_pair(y, x));

                    // Find all connected blocks of the same type
                    while (!toVisit.empty())
                    {
                        pair<int, int> current = toVisit.front();
                        toVisit.pop();
                        int cy = current.first;
                        int cx = current.second;

                        if (cy < 1 || cy >= FIELD_HEIGHT - 1 || cx < 1 || cx >= FIELD_WIDTH - 1)
                            continue;

                        if (visited[cy][cx] || field[cy][cx] != pieceID)
                            continue;

                        visited[cy][cx] = true;
                        cluster.push_back(make_pair(cy, cx));

                        // Check all 4-directional neighbors
                        if (cy > 1 && !visited[cy - 1][cx])
                            toVisit.push(make_pair(cy - 1, cx));
                        if (cy < FIELD_HEIGHT - 2 && !visited[cy + 1][cx])
                            toVisit.push(make_pair(cy + 1, cx));
                        if (cx > 1 && !visited[cy][cx - 1])
                            toVisit.push(make_pair(cy, cx - 1));
                        if (cx < FIELD_WIDTH - 2 && !visited[cy][cx + 1])
                            toVisit.push(make_pair(cy, cx + 1));
                    }

                    // Check if this cluster is floating
                    bool isFloating = true;
                    for (const auto &block : cluster)
                    {
                        int belowY = block.first + 1;
                        // If we hit bottom or a non-empty, non-matching block below
                        if (belowY >= FIELD_HEIGHT - 1 ||
                            (field[belowY][block.second] != 0 && field[belowY][block.second] != pieceID))
                        {
                            isFloating = false;
                            break;
                        }
                    }

                    // If floating, move the entire cluster down as far as possible
                    if (isFloating && !cluster.empty())
                    {
                        // Find how far we can drop the cluster
                        int maxDrop = FIELD_HEIGHT;
                        for (const auto &block : cluster)
                        {
                            int drop = 0;
                            while (block.first + drop + 1 < FIELD_HEIGHT - 1 &&
                                   field[block.first + drop + 1][block.second] == 0)
                            {
                                drop++;
                            }
                            maxDrop = min(maxDrop, drop);
                        }

                        // Sort cluster by bottom blocks first to prevent overwriting
                        sort(cluster.begin(), cluster.end(), [](const pair<int, int> &a, const pair<int, int> &b)
                             { return a.first > b.first; });

                        // Move each block down by maxDrop
                        for (auto &block : cluster)
                        {
                            field[block.first + maxDrop][block.second] = pieceID;
                            field[block.first][block.second] = 0;
                        }
                    }
                }
            }
        }
    }
    void Update()
    {
        if (isPaused || isGameOver)
            return;

        static auto lastUpdate = chrono::steady_clock::now();
        auto now = chrono::steady_clock::now();
        auto elapsed = chrono::duration_cast<chrono::milliseconds>(now - lastUpdate).count();

        // Calculate speed based on level (faster as level increases)
        int currentSpeed = max(50, 500 - (level * 30)); // Adjust these values as needed

        if (elapsed >= currentSpeed)
        {
            lastUpdate = now;

            if (DoesPieceFit(currentPiece, currentRotation, currentX, currentY + 1))
            {
                currentY++;
            }
            else
            {
#ifdef _WIN32
                Beep(420, 200);
#else
                system("ffplay -nodisp -autoexit beep-07a.wav 2>/dev/null &");
#endif
                score += 10 * level;

                // Lock piece
                for (int px = 0; px < TETROMINO_SIZE; px++)
                {
                    for (int py = 0; py < TETROMINO_SIZE; py++)
                    {
                        if (TETROMINOS[currentPiece][Rotate(px, py, currentRotation)] != L'.')
                        {
                            field[currentY + py][currentX + px] = currentPiece + 1;
                        }
                    }
                }

                // Game over check
                if (currentY <= 1)
                {
                    isGameOver = true;
                    return;
                }

                ClearLines();
                AppleGravity();

                // New piece
                currentX = FIELD_WIDTH / 2 - 2;
                currentY = 1;
                currentRotation = 0;
                currentPiece = nextPiece;
                nextPiece = rand() % 7;

                if (!DoesPieceFit(currentPiece, currentRotation, currentX, currentY))
                {
                    isGameOver = true;
                }
            }
        }
    }
    void Draw()
    {
#ifdef _WIN32
        // Clear the buffer
        screenBuffer.Clear();

        // Draw border and field
        for (int y = 0; y < FIELD_HEIGHT; y++)
        {
            for (int x = 0; x < FIELD_WIDTH; x++)
            {
                int cell = field[y][x];
                if (cell > 0 && cell < 8)
                {
                    screenBuffer.Write(x * 2, y, "[]", cell + 8);
                }
                else if (cell == 8)
                {
                    screenBuffer.Write(x * 2, y, "##", 15);
                }
            }
        }

        // Draw current piece
        for (int px = 0; px < TETROMINO_SIZE; px++)
        {
            for (int py = 0; py < TETROMINO_SIZE; py++)
            {
                if (TETROMINOS[currentPiece][Rotate(px, py, currentRotation)] != L'.')
                {
                    screenBuffer.Write((currentX + px) * 2, currentY + py, "[]", currentPiece + 9);
                }
            }
        }

        // Draw next piece preview border
        screenBuffer.Write(FIELD_WIDTH * 2 + 4, 0, "#############", 15);
        for (int y = 1; y <= 6; y++)
        {
            screenBuffer.Write(FIELD_WIDTH * 2 + 4, y, "#", 15);
            screenBuffer.Write(FIELD_WIDTH * 2 + 16, y, "#", 15);
        }
        screenBuffer.Write(FIELD_WIDTH * 2 + 4, 7, "#############", 15);

        // Draw next piece title
        screenBuffer.Write(FIELD_WIDTH * 2 + 7, 1, "NEXT", 15);

        // Clear preview area
        screenBuffer.Fill(FIELD_WIDTH * 2 + 5, 2, 10, 5, ' ', 0);

        // Draw next piece (centered)
        int baseX = FIELD_WIDTH * 2 + 8;
        int baseY = 4;
        for (int px = 0; px < TETROMINO_SIZE; px++)
        {
            for (int py = 0; py < TETROMINO_SIZE; py++)
            {
                if (TETROMINOS[nextPiece][Rotate(px, py, 0)] != L'.')
                {
                    int drawX = baseX + (px - 1) * 2;
                    int drawY = baseY + (py - 1);
                    screenBuffer.Write(drawX, drawY, "[]", nextPiece + 9);
                }
            }
        }

        // Draw game info
        screenBuffer.Write(FIELD_WIDTH * 2 + 5, 9, ("Score: " + to_string(score)).c_str(), 15);
        screenBuffer.Write(FIELD_WIDTH * 2 + 5, 10, ("Level: " + to_string(level)).c_str(), 15);
        screenBuffer.Write(FIELD_WIDTH * 2 + 5, 11, ("Lines: " + to_string(linesCleared)).c_str(), 15);

        // Draw controls
        screenBuffer.Write(FIELD_WIDTH * 2 + 5, 13, "Controls:", 15);
        screenBuffer.Write(FIELD_WIDTH * 2 + 5, 15, "LEFT/RIGHT: Move", 15);
        screenBuffer.Write(FIELD_WIDTH * 2 + 5, 16, "UP: Rotate", 15);
        screenBuffer.Write(FIELD_WIDTH * 2 + 5, 17, "DOWN: Soft Drop", 15);
        screenBuffer.Write(FIELD_WIDTH * 2 + 5, 18, "SPACE: Hard Drop", 15);
        screenBuffer.Write(FIELD_WIDTH * 2 + 5, 19, "S: Pause", 15);
        screenBuffer.Write(FIELD_WIDTH * 2 + 5, 20, "Ctrl + C: Quit", 15);

        if (isPaused)
        {
            screenBuffer.Write(FIELD_WIDTH - 4, FIELD_HEIGHT / 2, "PAUSED", 15);
        }

        // Draw everything at once
        screenBuffer.Draw();
#else
        // Linux/Unix version using ncurses
        erase();

        // Draw border
        attron(COLOR_PAIR(8));
        for (int y = 0; y < FIELD_HEIGHT; y++)
        {
            mvprintw(y + 1, 0, "  ");               // Left border
            mvprintw(y + 1, FIELD_WIDTH * 2, "  "); // Right border
        }
        for (int x = 0; x <= FIELD_WIDTH * 2; x += 2)
        {
            mvprintw(0, x, "  ");            // Top border
            mvprintw(FIELD_HEIGHT, x, "  "); // Bottom border
        }
        attroff(COLOR_PAIR(8));

        // Draw field contents
        for (int y = 0; y < FIELD_HEIGHT; y++)
        {
            for (int x = 0; x < FIELD_WIDTH; x++)
            {
                int cell = field[y][x];
                if (cell > 0 && cell < 8)
                {
                    attron(COLOR_PAIR(cell));
                    mvprintw(y + 1, x * 2 + 1, "  ");
                    attroff(COLOR_PAIR(cell));
                }
                else if (cell == 8)
                {
                    attron(COLOR_PAIR(8));
                    mvprintw(y + 1, x * 2 + 1, "  ");
                    attroff(COLOR_PAIR(8));
                }
            }
        }

        // Draw current piece
        for (int px = 0; px < TETROMINO_SIZE; px++)
        {
            for (int py = 0; py < TETROMINO_SIZE; py++)
            {
                if (TETROMINOS[currentPiece][Rotate(px, py, currentRotation)] != L'.')
                {
                    attron(COLOR_PAIR(currentPiece + 1));
                    mvprintw(currentY + py + 1, (currentX + px) * 2 + 1, "  ");
                    attroff(COLOR_PAIR(currentPiece + 1));
                }
            }
        }

        // Draw next piece preview with border
        attron(COLOR_PAIR(8));
        // Top border
        for (int x = FIELD_WIDTH * 2 + 5; x <= FIELD_WIDTH * 2 + 15; x++)
        {
            mvprintw(1, x, " ");
        }
        // Bottom border
        for (int x = FIELD_WIDTH * 2 + 5; x <= FIELD_WIDTH * 2 + 15; x++)
        {
            mvprintw(7, x, " ");
        }
        // Side borders
        for (int y = 2; y <= 6; y++)
        {
            mvprintw(y, FIELD_WIDTH * 2 + 5, " ");
            mvprintw(y, FIELD_WIDTH * 2 + 15, " ");
        }
        attroff(COLOR_PAIR(8));

        // Next piece title
        attron(A_BOLD | COLOR_PAIR(15));
        mvprintw(1, FIELD_WIDTH * 2 + 8, "NEXT");
        attroff(A_BOLD | COLOR_PAIR(15));

        // Clear preview area
        for (int y = 2; y <= 6; y++)
        {
            for (int x = FIELD_WIDTH * 2 + 6; x <= FIELD_WIDTH * 2 + 14; x++)
            {
                mvprintw(y, x, " ");
            }
        }

        // Draw next piece (centered)
        int baseX = FIELD_WIDTH * 2 + 9;
        int baseY = 4;
        for (int px = 0; px < TETROMINO_SIZE; px++)
        {
            for (int py = 0; py < TETROMINO_SIZE; py++)
            {
                if (TETROMINOS[nextPiece][Rotate(px, py, 0)] != L'.')
                {
                    int drawX = baseX + (px - 1) * 2;
                    int drawY = baseY + (py - 1);
                    attron(COLOR_PAIR(nextPiece + 1));
                    mvprintw(drawY, drawX, "  ");
                    attroff(COLOR_PAIR(nextPiece + 1));
                }
            }
        }

        // Draw game info
        attron(A_BOLD);
        mvprintw(9, FIELD_WIDTH * 2 + 5, "Score: %d", score);
        mvprintw(10, FIELD_WIDTH * 2 + 5, "Level: %d", level);
        mvprintw(11, FIELD_WIDTH * 2 + 5, "Lines: %d", linesCleared);
        attroff(A_BOLD);

        // Draw controls
        mvprintw(13, FIELD_WIDTH * 2 + 5, "Controls:");
        mvprintw(14, FIELD_WIDTH * 2 + 5, "LEFT/RIGHT: Move");
        mvprintw(15, FIELD_WIDTH * 2 + 5, "UP: Rotate");
        mvprintw(16, FIELD_WIDTH * 2 + 5, "DOWN: Soft Drop");
        mvprintw(17, FIELD_WIDTH * 2 + 5, "SPACE: Hard Drop");
        mvprintw(18, FIELD_WIDTH * 2 + 5, "S: Pause");
        mvprintw(19, FIELD_WIDTH * 2 + 5, "ESC: Quit");

        if (isPaused)
        {
            attron(A_BOLD | COLOR_PAIR(8));
            mvprintw(FIELD_HEIGHT / 2 + 1, FIELD_WIDTH - 4, "PAUSED");
            attroff(A_BOLD | COLOR_PAIR(8));
        }

        refresh();
#endif
    }
    bool IsGameOver() const { return isGameOver; }
    int GetScore() const { return score; }
};

int main()
{
    // Initialize random seed
    srand(time(0));

#ifdef _WIN32
    // Windows initialization
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hConsole, &cursorInfo);
    cursorInfo.bVisible = false;
    SetConsoleCursorInfo(hConsole, &cursorInfo);

    // Set console title
    SetConsoleTitleA("Tetris Game");

    // Set console size
    SMALL_RECT windowSize = {0, 0, 79, 49}; // 80x50
    SetConsoleWindowInfo(hConsole, TRUE, &windowSize);
#else
    // Linux initialization (ncurses)
    initscr();
    ShowGameInstructions();
    noecho();
    curs_set(0);
    nodelay(stdscr, TRUE);
    keypad(stdscr, TRUE);
    start_color();

    // Initialize color pairs
    init_pair(1, COLOR_BLACK, COLOR_CYAN);    // I
    init_pair(2, COLOR_BLACK, COLOR_BLUE);    // J
    init_pair(3, COLOR_BLACK, 208);           // L (using yellow for orange)
    init_pair(4, COLOR_BLACK, COLOR_YELLOW);  // O
    init_pair(5, COLOR_BLACK, COLOR_GREEN);   // S
    init_pair(6, COLOR_BLACK, COLOR_MAGENTA); // T
    init_pair(7, COLOR_BLACK, COLOR_RED);     // Z
    init_pair(8, COLOR_RED, COLOR_WHITE);     // Border
#endif

    Tetris game;

    // Play start sound
#ifdef _WIN32
    ShowGameInstructions();

    Beep(523, 200); // C note
    Beep(659, 200); // E note
    Beep(784, 200); // G note
#else
    system("ffplay -nodisp -autoexit game_start.mp3 2>/dev/null &");
#endif

    ShowCountdownAnimation();

    // Main game loop
    while (!game.IsGameOver())
    {
#ifdef _WIN32
        if (_kbhit())
        {
            int ch = _getch();
            // Handle arrow keys (Windows returns two codes for arrows)
            if (ch == 0 || ch == 224)
            {
                ch = _getch(); // Get the actual key code
            }
            game.ProcessInput(ch);
        }
#else
        int ch = getch();
        if (ch != ERR)
        {
            game.ProcessInput(ch);
            // Clear any additional buffered input
            flushinp();
        }
#endif

        game.Update();
        game.Draw();

        // Use proper sleep for Linux
#ifdef _WIN32
        Sleep(50);

        Beep(523, 200); // C note
        Beep(659, 200); // E note
        Beep(784, 200); // G note
#else
        usleep(50000); // 50ms in microseconds
#endif
    }
    ShowGameOverAnimation();

    // Display final score and high scores
#ifdef _WIN32
    system("cls");
    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
    cout << "Game Over! Final Score: " << game.GetScore() << endl;
    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#else
    endwin();
    cout << "\033[31m" << "Game Over! Final Score: " << game.GetScore() << endl
         << "\033[0m";
#endif

    vector<HighScore> currentScores = readHighScores();
    if (currentScores.size() < 5 || game.GetScore() > currentScores.back().score)
    {
        updateHighScores(game.GetScore());
    }
    displayHighScores();

#ifdef _WIN32
    // Restore console settings
    SetConsoleCursorInfo(hConsole, &cursorInfo);
    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#else
    // Ncurses already cleaned up by endwin()
#endif

    return 0;
}
