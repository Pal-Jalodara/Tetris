#include <iostream> 
#include <vector>
#include <conio.h>
#include <Windows.h>
#include <random>
#include <chrono>
#include <string>
#include <fstream>

using namespace std;

const int KEY_ESC = 27;
const int KEY_LEFT = 75;
const int KEY_RIGHT = 77;
const int KEY_UP = 72;
const int KEY_DOWN = 80;
const int KEY_SPACE = 32;

const int GRID_WIDTH = 10;
const int GRID_HEIGHT = 20;
const int INITIAL_SPEED = 500;
const int SPEED_INCREMENT = 50;
const int LINES_PER_LEVEL = 10;
const int SCORE_THRESHOLD = 500;

enum ConsoleColor {
    BLACK = 0,
    BLUE = 1,
    GREEN = 2,
    CYAN = 3,
    RED = 4,
    MAGENTA = 5,
    BROWN = 6,
    LIGHTGRAY = 7,
    DARKGRAY = 8,
    LIGHTBLUE = 9,
    LIGHTGREEN = 10,
    LIGHTCYAN = 11,
    LIGHTRED = 12,
    LIGHTMAGENTA = 13,
    YELLOW = 14,
    WHITE = 15
};

const ConsoleColor TETROMINO_COLORS[] = {
    BLUE,
    YELLOW,
    LIGHTMAGENTA,
    RED,
    GREEN,
    LIGHTRED,
    BROWN
};

const std::vector<std::vector<std::vector<int>>> TETROMINOES = {
    {
        {0, 0, 0, 0},
        {1, 1, 1, 1},
        {0, 0, 0, 0},
        {0, 0, 0, 0}
    },
    {
        {1, 1},
        {1, 1}
    },
    {
        {0, 1, 0},
        {1, 1, 1},
        {0, 0, 0}
    },
    {
        {0, 1, 1},
        {1, 1, 0},
        {0, 0, 0}   
    },
    {
        {1, 1, 0},
        {0, 1, 1},
        {0, 0, 0}
    },
    {
        {1, 0, 0},
        {1, 1, 1},
        {0, 0, 0}
    },
    {
        {0, 0, 1},
        {1, 1, 1},
        {0, 0, 0}
    }
};

void setCursorPosition(int x, int y) {
    COORD coord;
    coord.X = x;
    coord.Y = y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

void setConsoleColor(ConsoleColor textColor, ConsoleColor bgColor = BLACK) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), (textColor) | (bgColor << 4));
}

void resetConsoleColor() {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), WHITE | (BLACK << 4));
}

int loadHighScore() {
    std::ifstream file("tetris_highscore.txt");
    int highScore = 0;
    if (file.is_open()) {
        file >> highScore;
        file.close();
    }
    return highScore;
}

void saveHighScore(int score) {
    std::ofstream file("tetris_highscore.txt");
    if (file.is_open()) {
        file << score;
        file.close();
    }
}

class Tetromino {
private:
    int type;
    int rotation;
    int x, y;
    char symbol;
    ConsoleColor color;

public:
    Tetromino(int type) : type(type), rotation(0), x(GRID_WIDTH / 2 - 2), y(0) {
        symbol = '#';
        color = TETROMINO_COLORS[type];
    }

    vector<vector<int>> getShape() const {
        vector<vector<int>> shape = TETROMINOES[type];
        for (int r = 0; r < rotation; r++) {
            shape = rotateMatrix(shape);
        }
        return shape;
    }

    vector<vector<int>> rotateMatrix(const vector<vector<int>>& matrix) const {
        int n = matrix.size();
        vector<vector<int>> rotated(n, vector<int>(n, 0));
        
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                rotated[j][n - 1 - i] = matrix[i][j];
            }
        }
        return rotated;
    }

    void rotate() {
        rotation = (rotation + 1) % 4;
    }

    void moveLeft() {
        x--;
    }

    void moveRight() {
        x++;
    }

    void moveDown() {
        y++;
    }

    int getX() const { return x; }
    int getY() const { return y; }
    char getSymbol() const { return symbol; }
    ConsoleColor getColor() const { return color; }
    int getType() const { return type; }
    
    void setX(int newX) { x = newX; }
    void setY(int newY) { y = newY; }
    void setRotation(int newRotation) { rotation = newRotation; }
};

class TetrisGame {
private:
    vector<vector<int>> grid;
    vector<vector<int>> colorGrid;
    Tetromino* currentPiece;
    Tetromino* nextPiece;
    bool gameOver;
    bool paused;
    int score;
    int level;
    int linesCleared;
    int fallSpeed;
    std::chrono::time_point<std::chrono::system_clock> lastFall;
    std::mt19937 rng;

public:
    TetrisGame() : 
        grid(GRID_HEIGHT, vector<int>(GRID_WIDTH, 0)),
        colorGrid(GRID_HEIGHT, vector<int>(GRID_WIDTH, 0)),
        gameOver(false),
        paused(false),
        score(0),
        level(1),
        linesCleared(0),
        fallSpeed(INITIAL_SPEED) {
        
        random_device rd;
        rng = mt19937(rd());
        currentPiece = new Tetromino(generateRandomPieceType());
        nextPiece = new Tetromino(generateRandomPieceType());
        lastFall = chrono::system_clock::now();
    }

    ~TetrisGame() {
        delete currentPiece;
        delete nextPiece;
    }

    void reset() {
        grid.assign(GRID_HEIGHT, vector<int>(GRID_WIDTH, 0));
        colorGrid.assign(GRID_HEIGHT, vector<int>(GRID_WIDTH, 0));
        delete currentPiece;
        delete nextPiece;
        currentPiece = new Tetromino(generateRandomPieceType());
        nextPiece = new Tetromino(generateRandomPieceType());
        gameOver = false;
        paused = false;
        score = 0;
        level = 1;
        linesCleared = 0;
        fallSpeed = INITIAL_SPEED;
        lastFall = chrono::system_clock::now();
    }

    int generateRandomPieceType() {
        std::uniform_int_distribution<int> dist(0, TETROMINOES.size() - 1);
        return dist(rng);
    }

    bool isCollision(const Tetromino& piece) const {
        vector<vector<int>> shape = piece.getShape();
        int pieceHeight = shape.size();
        int pieceWidth = shape[0].size();
        
        for (int i = 0; i < pieceHeight; i++) {
            for (int j = 0; j < pieceWidth; j++) {
                if (shape[i][j] == 0) continue;
                
                int gridX = piece.getX() + j;
                int gridY = piece.getY() + i;
                
                if (gridX < 0 || gridX >= GRID_WIDTH || gridY >= GRID_HEIGHT) {
                    return true;
                }
                
                if (gridY >= 0 && grid[gridY][gridX] == 1) {
                    return true;
                }
            }
        }
        return false;
    }

    void placePiece() {
        vector<vector<int>> shape = currentPiece->getShape();
        int pieceHeight = shape.size();
        int pieceWidth = shape[0].size();
        int pieceType = currentPiece->getType();
        
        for (int i = 0; i < pieceHeight; i++) {
            for (int j = 0; j < pieceWidth; j++) {
                if (shape[i][j] == 0) continue;
                
                int gridX = currentPiece->getX() + j;
                int gridY = currentPiece->getY() + i;
                
                if (gridY < 0) {
                    gameOver = true;
                    return;
                }
                
                grid[gridY][gridX] = 1;
                colorGrid[gridY][gridX] = pieceType;
            }
        }
        
        clearLines();
        
        if (grid[0][0] == 1 || grid[0][1] == 1 || grid[0][2] == 1 || grid[0][3] == 1 || 
            grid[0][4] == 1 || grid[0][5] == 1 || grid[0][6] == 1 || grid[0][7] == 1 || 
            grid[0][8] == 1 || grid[0][9] == 1) {
            gameOver = true;
        }
        
        delete currentPiece;
        currentPiece = nextPiece;
        nextPiece = new Tetromino(generateRandomPieceType());
    }

    void clearLines() {
        int linesCleared = 0;
        
        for (int y = GRID_HEIGHT - 1; y >= 0; y--) {
            bool lineFull = true;
            
            for (int x = 0; x < GRID_WIDTH; x++) {
                if (grid[y][x] == 0) {
                    lineFull = false;
                    break;
                }
            }
            
            if (lineFull) {
                linesCleared++;
                
                for (int y2 = y; y2 > 0; y2--) {
                    for (int x = 0; x < GRID_WIDTH; x++) {
                        grid[y2][x] = grid[y2 - 1][x];
                        colorGrid[y2][x] = colorGrid[y2 - 1][x];
                    }
                }
                
                for (int x = 0; x < GRID_WIDTH; x++) {
                    grid[0][x] = 0;
                    colorGrid[0][x] = 0;
                }
                
                y++;
            }
        }
        
        if (linesCleared > 0) {
            this->score += 100 * linesCleared * this->level;
            this->linesCleared += linesCleared;
            this->level = 1 + (this->linesCleared / LINES_PER_LEVEL);
            
            if (score / SCORE_THRESHOLD > 0) {
                fallSpeed = max(INITIAL_SPEED - (score / SCORE_THRESHOLD) * SPEED_INCREMENT, 100);
            }
        }
    }

    void hardDrop() {
        while (!isCollision(*currentPiece)) {
            currentPiece->moveDown();
        }
        
        currentPiece->setY(currentPiece->getY() - 1);
        placePiece();
    }

    void update() {
        if (paused) return;

        auto now = chrono::system_clock::now();
        auto elapsed = chrono::duration_cast<chrono::milliseconds>(now - lastFall).count();
        
        if (elapsed > fallSpeed) {
            currentPiece->moveDown();
            
            if (isCollision(*currentPiece)) {
                currentPiece->setY(currentPiece->getY() - 1);
                placePiece();
            }
            
            lastFall = now;
        }
    }

    void draw() {
        static bool firstDraw = true;
        if (firstDraw) {
            system("cls");
            firstDraw = false;
        }
        
        setCursorPosition(0, 0);
        resetConsoleColor();
        cout << "TETRIS" << endl;
        cout << "Score: " << score << " | Level: " << level << " | Lines: " << linesCleared << endl;
        cout << endl;
    
        if (paused) {
            setCursorPosition(GRID_WIDTH / 2 - 5, GRID_HEIGHT / 2);
            cout << "PAUSED";
            return;
        }
    
        setCursorPosition(0, 3);
        cout << "+" << string(GRID_WIDTH * 2, '-') << "+" << endl;
    
        vector<vector<int>> tempGrid = grid;
        vector<vector<int>> tempColorGrid = colorGrid;
    
        if (!gameOver) {
            vector<vector<int>> shape = currentPiece->getShape();
            int pieceHeight = shape.size();
            int pieceWidth = shape[0].size();
            int pieceType = currentPiece->getType();
    
            for (int i = 0; i < pieceHeight; i++) {
                for (int j = 0; j < pieceWidth; j++) {
                    if (shape[i][j] == 0) continue;
    
                    int gridX = currentPiece->getX() + j;
                    int gridY = currentPiece->getY() + i;
    
                    if (gridX >= 0 && gridX < GRID_WIDTH && gridY >= 0 && gridY < GRID_HEIGHT) {
                        tempGrid[gridY][gridX] = 1;
                        tempColorGrid[gridY][gridX] = pieceType;
                    }
                }
            }
        }
    
        for (int y = 0; y < GRID_HEIGHT; y++) {
            setCursorPosition(0, y + 4);
            cout << "|";
    
            for (int x = 0; x < GRID_WIDTH; x++) {
                if (tempGrid[y][x] == 1) {
                    ConsoleColor blockColor = TETROMINO_COLORS[tempColorGrid[y][x]];
                    setConsoleColor(blockColor);
                    cout << "##";
                    resetConsoleColor();
                } else {
                    cout << "  ";
                }
            }
            cout << "|";
        }
    
        setCursorPosition(0, GRID_HEIGHT + 4);
        cout << "+" << string(GRID_WIDTH * 2, '-') << "+" << endl;
    
        setCursorPosition(GRID_WIDTH * 2 + 5, 4);
        cout << "Next Piece:";
    
        for (int i = 0; i < 4; i++) {
            setCursorPosition(GRID_WIDTH * 2 + 5, i + 5);
            cout << "        ";
        }
    
        vector<vector<int>> nextShape = nextPiece->getShape();
        ConsoleColor nextColor = TETROMINO_COLORS[nextPiece->getType()];
    
        for (int i = 0; i < nextShape.size(); i++) {
            setCursorPosition(GRID_WIDTH * 2 + 5, i + 5);
            for (int j = 0; j < nextShape[i].size(); j++) {
                if (nextShape[i][j] == 1) {
                    setConsoleColor(nextColor);
                    cout << "##";
                    resetConsoleColor();
                } else {
                    cout << "  ";
                }
            }
        }
    
        setCursorPosition(GRID_WIDTH * 2 + 5, 11);
        cout << "Controls:";
        setCursorPosition(GRID_WIDTH * 2 + 5, 12);
        cout << "Left/Right: Move Left/Right";
        setCursorPosition(GRID_WIDTH * 2 + 5, 13);
        cout << "Up: Rotate";
        setCursorPosition(GRID_WIDTH * 2 + 5, 14);
        cout << "Down: Soft Drop";
        setCursorPosition(GRID_WIDTH * 2 + 5, 15);
        cout << "Space: Hard Drop";
        setCursorPosition(GRID_WIDTH * 2 + 5, 16);
        cout << "P: Pause/Resume";
        setCursorPosition(GRID_WIDTH * 2 + 5, 17);
        cout << "1: End Game";
        setCursorPosition(GRID_WIDTH * 2 + 5, 18);
        cout << "2: Restart Game";
        setCursorPosition(GRID_WIDTH * 2 + 5, 19);
        cout << "ESC: Quit";
    
        if (gameOver) {
            setCursorPosition(GRID_WIDTH - 4, GRID_HEIGHT / 2 + 4);
            cout << "GAME OVER";
        }
    }

    void handleInput() {
        if (_kbhit()) {
            int ch = _getch();
            
            if (ch == 224) {
                ch = _getch();
                
                if (!paused) {
                    Tetromino tempPiece = *currentPiece;
                    
                    switch (ch) {
                        case KEY_LEFT:
                            currentPiece->moveLeft();
                            if (isCollision(*currentPiece)) {
                                *currentPiece = tempPiece;
                            }
                            break;
                            
                        case KEY_RIGHT:
                            currentPiece->moveRight();
                            if (isCollision(*currentPiece)) {
                                *currentPiece = tempPiece;
                            }
                            break;
                            
                        case KEY_UP:
                            currentPiece->rotate();
                            if (isCollision(*currentPiece)) {
                                *currentPiece = tempPiece;
                            }
                            break;
                            
                        case KEY_DOWN:
                            currentPiece->moveDown();
                            if (isCollision(*currentPiece)) {
                                currentPiece->setY(currentPiece->getY() - 1);
                                placePiece();
                            }
                            lastFall = chrono::system_clock::now();
                            break;
                    }
                }
            } else if (ch == KEY_SPACE) {
                hardDrop();
            } else if (ch == KEY_ESC) {
                gameOver = true;
            } else if (ch == 'p' || ch == 'P') {
                paused = !paused;
            } else if (ch == '1') {
                gameOver = true;
            } else if (ch == '2') {
                reset();
            }
        }
    }

    bool isGameOver() const {
        return gameOver;
    }

    int getScore() const {
        return score;
    }
};

int main() {
    system("cls");
    
    CONSOLE_CURSOR_INFO cursorInfo;
    cursorInfo.dwSize = 100;
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
    
    int highScore = loadHighScore();
    TetrisGame game;
    
    while (true) {
        game.handleInput();
        game.update();
        game.draw();
        
        if (game.isGameOver()) {
            break;
        }
        
        Sleep(50);
    }
    
    int finalScore = game.getScore();
    if (finalScore > highScore) {
        highScore = finalScore;
        saveHighScore(highScore);
    }
    
    setCursorPosition(0, GRID_HEIGHT + 6);
    cout << "Game Over! Final Score: " << finalScore << "\n";
    cout << "High Score: " << highScore << "\n";
    cout << "Press 1 to exit or 2 to replay...";
    
    while (true) {
        if (_kbhit()) {
            char ch = _getch();
            if (ch == '1') {
                break;
            } else if (ch == '2') {
                game.reset();
                main();
            }
        }
    }
    
    resetConsoleColor();
    return 0;
}