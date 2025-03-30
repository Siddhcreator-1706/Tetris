#include <iostream>
#include <thread>
#include <vector>
#include <ncurses.h>
#include <chrono>
#include <queue>
#include <fstream>
#include <algorithm>
#include <cstdlib>
#include <ctime>

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

void ShowGameInstructions()
{
    clear();

    // Enable colors if not already done
    start_color();
    init_pair(15, COLOR_CYAN, COLOR_BLACK);   // Title color
    init_pair(16, COLOR_YELLOW, COLOR_BLACK); // Instruction color
    init_pair(17, COLOR_GREEN, COLOR_BLACK);  // Key color

    // Game Title
    attron(COLOR_PAIR(15) | A_BOLD);
    mvprintw(2, COLS / 2 - 10, "                                              ");
    mvprintw(3, COLS / 2 - 10, "     TETRIS GAME    ");
    mvprintw(4, COLS / 2 - 10, "                                              ");
    attroff(COLOR_PAIR(1) | A_BOLD);

    // Instruction box
    attron(COLOR_PAIR(16));
    mvprintw(6, COLS / 2 - 20, "                                                                                                      ");
    mvprintw(7, COLS / 2 - 20, "                HOW TO PLAY                 ");
    mvprintw(8, COLS / 2 - 20, "                                                                                                      ");

    // Instruction lines
    std::vector<std::string> instructions = {
        "  - Arrange the falling blocks to complete lines",
        "  - Complete lines to earn points and level up",
        "  - The game speeds up as you progress levels",
        "  - Game ends when blocks reach the top",
        "",
        "  CONTROLS:",
        "  LEFT_KEY , RIGHT_KEY : Move block left/right",
        "  UP_KEY  : Rotate block",
        "  DOWN_KEY   : Soft drop (move down faster)",
        "  SPACE: Hard drop (instant drop)",
        "  P    : Pause game",
        "  Ctrl + C    : Quit game"};

    for (size_t i = 0; i < instructions.size(); i++)
    {
        mvprintw(9 + i, COLS / 2 - 20, "%-42s", instructions[i].c_str());
    }

    mvprintw(9 + instructions.size(), COLS / 2 - 20, "                                                                                                      ");
    attroff(COLOR_PAIR(16));

    // Prompt to continue
    attron(COLOR_PAIR(17) | A_BOLD);
    mvprintw(LINES - 4, COLS / 2 - 15, "Press any key to return to the game...");
    attroff(COLOR_PAIR(17) | A_BOLD);

    refresh();
    getch(); // Wait for any key press
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
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    clear();
    attron(COLOR_PAIR(9) | A_BOLD);
    mvprintw(startY, startX - 5, "Game Start!");
    attroff(COLOR_PAIR(9) | A_BOLD);
    refresh();
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
}

void ShowGameOverAnimation()
{
    initscr();
    noecho();
    curs_set(0);
    start_color();
    init_pair(11, COLOR_RED, COLOR_BLACK);

    int startX = COLS / 2 - 5;
    int startY = LINES / 2;

    const char *gameOverText = "GAME OVER";

    for (int i = 0; i < 5; i++)
    {
        clear();
        attron(COLOR_PAIR(11) | A_BOLD);
        if (i % 2 == 0)
            attron(A_BLINK);
        mvprintw(startY, startX, "%s", gameOverText);
        attroff(A_BLINK);
        refresh();
        std::this_thread::sleep_for(std::chrono::milliseconds(400));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    endwin();
}

class Tetris
{
    vector<vector<int>> field{FIELD_HEIGHT, vector<int>(FIELD_WIDTH, 0)};
    int currentPiece{rand() % 7}, nextPiece{rand() % 7}, currentRotation{},
        currentX{FIELD_WIDTH / 2 - 2}, currentY{1}; // Start at y=1 below top border
    int score{}, level{1}, speed{10}, linesCleared{};
    bool isGameOver{}, isPaused{};

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

                // Check boundaries (including top border at y=0)
                if (fx < 0 || fx >= FIELD_WIDTH || fy < 0 || fy >= FIELD_HEIGHT)
                    return false;

                // Check collision with existing pieces
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
        { // Start from bottom, stop at top border
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
                if (linesCleared == 0)
                {
                    system("ffplay -nodisp -autoexit beep.wav 2>/dev/null &");
                }
                // Clear the line
                for (int x = 1; x < FIELD_WIDTH - 1; x++)
                    field[y][x] = 0;

                // Shift all lines above down
                for (int yy = y - 1; yy >= 1; yy--)
                {
                    for (int x = 1; x < FIELD_WIDTH - 1; x++)
                    {
                        field[yy + 1][x] = field[yy][x];
                        field[yy][x] = 0;
                    }
                }
                linesClearedThisTurn++;
                y++; // Re-check this line after shift
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

        // Level up every 5 lines
        if (linesCleared >= 5)
        {
            level++;
            linesCleared -= 5;
            speed = max(5, speed - 2);
            system("ffplay -nodisp -autoexit beep.wav 2>/dev/null &");
        }
    }

public:
    Tetris()
    {
        // Initialize borders (left, right, top, bottom)
        for (int y = 0; y < FIELD_HEIGHT; y++)
        {
            field[y][0] = 8;               // Left border
            field[y][FIELD_WIDTH - 1] = 8; // Right border
        }
        for (int x = 0; x < FIELD_WIDTH; x++)
        {
            field[0][x] = 8;                // Top border
            field[FIELD_HEIGHT - 1][x] = 8; // Bottom border
        }
    }

    void ProcessInput(int ch)
    {
        if (isPaused)
        {
            if (ch == 'p' || ch == 'P')
                isPaused = false;
            return;
        }

        switch (ch)
        {
        case KEY_LEFT:
            if (DoesPieceFit(currentPiece, currentRotation, currentX - 1, currentY))
                currentX--;
            break;
        case KEY_RIGHT:
            if (DoesPieceFit(currentPiece, currentRotation, currentX + 1, currentY))
                currentX++;
            break;
        case KEY_UP:
            if (DoesPieceFit(currentPiece, currentRotation + 1, currentX, currentY))
                currentRotation++;
            break;
        case KEY_DOWN:
            if (DoesPieceFit(currentPiece, currentRotation, currentX, currentY + 1))
                currentY++;
            break;
        case ' ':
            while (DoesPieceFit(currentPiece, currentRotation, currentX, currentY + 1))
                currentY++;
            break;
        case 'p':
        case 'P':
            isPaused = true;
            break;
        }
    }

    void AppleGravity()
    {
        bool visited[FIELD_HEIGHT][FIELD_WIDTH] = {false};

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
                    toVisit.push({y, x});

                    // Find all connected blocks
                    while (!toVisit.empty())
                    {
                        auto [cy, cx] = toVisit.front();
                        toVisit.pop();

                        if (visited[cy][cx] || field[cy][cx] != pieceID)
                            continue;
                        visited[cy][cx] = true;
                        cluster.push_back({cy, cx});

                        // Check neighbors
                        if (cy > 1 && field[cy - 1][cx] == pieceID)
                            toVisit.push({cy - 1, cx});
                        if (cy < FIELD_HEIGHT - 1 && field[cy + 1][cx] == pieceID)
                            toVisit.push({cy + 1, cx});
                        if (cx > 0 && field[cy][cx - 1] == pieceID)
                            toVisit.push({cy, cx - 1});
                        if (cx < FIELD_WIDTH - 1 && field[cy][cx + 1] == pieceID)
                            toVisit.push({cy, cx + 1});
                    }

                    // Check if cluster is floating
                    bool isFloating = true;
                    for (auto [cy, cx] : cluster)
                    {
                        if (cy + 1 < FIELD_HEIGHT &&
                            field[cy + 1][cx] != 0 &&
                            field[cy + 1][cx] != pieceID)
                        {
                            isFloating = false;
                            break;
                        }
                    }

                    // Move cluster down if floating
                    if (isFloating)
                    {
                        bool canMove = true;
                        while (canMove)
                        {
                            for (auto [cy, cx] : cluster)
                            {
                                if (cy + 1 >= FIELD_HEIGHT ||
                                    (field[cy + 1][cx] != 0 && field[cy + 1][cx] != pieceID))
                                {
                                    canMove = false;
                                    break;
                                }
                            }
                            if (canMove)
                            {
                                for (int i = cluster.size() - 1; i >= 0; i--)
                                {
                                    auto [cy, cx] = cluster[i];
                                    field[cy + 1][cx] = field[cy][cx];
                                    field[cy][cx] = 0;
                                }
                                for (auto &block : cluster)
                                    block.first++;
                            }
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

        static int speedCount = 0;
        if (++speedCount == speed)
        {
            speedCount = 0;

            if (DoesPieceFit(currentPiece, currentRotation, currentX, currentY + 1))
            {
                currentY++;
            }
            else
            {
                system("ffplay -nodisp -autoexit beep-07a.wav 2>/dev/null &");
                score += 10 * level;
                // Lock the piece in place
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

                // Check if piece is above the play area (game over)
                if (currentY <= 1)
                {
                    isGameOver = true;
                    return;
                }

                AppleGravity();
                ClearLines();

                // Spawn new piece
                currentX = FIELD_WIDTH / 2 - 2;
                currentY = 1; // Spawn below top border
                currentRotation = 0;
                currentPiece = nextPiece;
                nextPiece = rand() % 7;

                // Check if new piece fits
                if (!DoesPieceFit(currentPiece, currentRotation, currentX, currentY))
                {
                    isGameOver = true;
                }
            }
        }
    }

    void Draw()
    {
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

        // Draw next piece preview
        mvprintw(1, FIELD_WIDTH * 2 + 5, "Next Piece:");
        attron(COLOR_PAIR(8));
        for (int x = 0; x < 10; x++)
        {
            mvprintw(2, FIELD_WIDTH * 2 + 5 + x, "  ");
            mvprintw(7, FIELD_WIDTH * 2 + 5 + x, "  ");
        }
        for (int y = 0; y < 5; y++)
        {
            mvprintw(2 + y, FIELD_WIDTH * 2 + 5, "  ");
            mvprintw(2 + y, FIELD_WIDTH * 2 + 14, "  ");
        }
        attroff(COLOR_PAIR(8));

        for (int px = 0; px < TETROMINO_SIZE; px++)
        {
            for (int py = 0; py < TETROMINO_SIZE; py++)
            {
                if (TETROMINOS[nextPiece][Rotate(px, py, 0)] != L'.')
                {
                    attron(COLOR_PAIR(nextPiece + 1));
                    mvprintw(4 + py, FIELD_WIDTH * 2 + 8 + px * 2, "  ");
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
        mvprintw(14, FIELD_WIDTH * 2 + 5, "LEFT_KEY , RIGHT_KEY  : Move");
        mvprintw(15, FIELD_WIDTH * 2 + 5, "UP_KEY : Rotate");
        mvprintw(16, FIELD_WIDTH * 2 + 5, "DOWN_KEY : Soft Drop");
        mvprintw(17, FIELD_WIDTH * 2 + 5, "Space: Hard Drop");
        mvprintw(18, FIELD_WIDTH * 2 + 5, "P: Pause");
	mvprintw(19, FIELD_WIDTH * 2 + 5, "Ctrl + C: Quit");

        if (isPaused)
        {
            attron(A_BOLD | COLOR_PAIR(8));
            mvprintw(FIELD_HEIGHT / 2 + 1, FIELD_WIDTH - 4, "PAUSED");
            attroff(A_BOLD | COLOR_PAIR(8));
        }

        refresh();
    }

    bool IsGameOver() const { return isGameOver; }
    int GetScore() const { return score; }
};

int main()
{
    initscr();
    ShowGameInstructions();

    noecho();
    curs_set(0);
    nodelay(stdscr, TRUE);
    keypad(stdscr, TRUE);
    start_color();

    // Color pairs
    init_pair(1, COLOR_BLACK, COLOR_CYAN);    // I
    init_pair(2, COLOR_BLACK, COLOR_BLUE);    // J
    init_pair(3, COLOR_BLACK, 208);           // L (orange)
    init_pair(4, COLOR_BLACK, COLOR_YELLOW);  // O
    init_pair(5, COLOR_BLACK, COLOR_GREEN);   // S
    init_pair(6, COLOR_BLACK, COLOR_MAGENTA); // T
    init_pair(7, COLOR_BLACK, COLOR_RED);     // Z
    init_pair(8, COLOR_RED, COLOR_WHITE);     // Border

    Tetris game;

    system("ffplay -nodisp -autoexit game_start.mp3 2>/dev/null &");
    ShowCountdownAnimation();

    while (!game.IsGameOver())
    {
        int ch = getch();
        game.ProcessInput(ch);
        game.Update();
        game.Draw();
        this_thread::sleep_for(chrono::milliseconds(50));
    }

    system("ffplay -nodisp -autoexit game_over.mp3 2>/dev/null &");
    ShowGameOverAnimation();
    int startX = COLS / 2 - 5;
    int startY = LINES / 2;

    const char *gameOverText = "SCORE : ";

    for (int i = 0; i < 5; i++)
    {
        clear();
        attron(COLOR_PAIR(11) | A_BOLD);
        if (i % 2 == 0)
            attron(A_BLINK);
        mvprintw(startY, startX, "%s", gameOverText);
        mvprintw(startY, startX + 7, "%d", game.GetScore());
        attroff(A_BLINK);
        refresh();
        std::this_thread::sleep_for(std::chrono::milliseconds(400));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    endwin();

    cout << "\033[2J\033[1;1H";
    cout << "\033[31m" << "Game Over! Final Score: " << game.GetScore() << endl
         << "\033[0m";

    vector<HighScore> currentScores = readHighScores();

    // Check if score qualifies for high score list
    if (currentScores.size() < 5 || game.GetScore() > currentScores.back().score)
    {
        updateHighScores(game.GetScore());
    }

    // Display high scores
    displayHighScores();

    return 0;
}
