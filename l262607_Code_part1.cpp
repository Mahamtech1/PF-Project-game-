///// MEMBER 1 

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <optional>
#include <string>
#include <array>
#include <map>
#include <sstream>

using namespace std;

const unsigned W = 1200;
const unsigned H = 760;

// Different screens of the game
enum Screen { Menu, Settings, HowToPlay, Game, RiddleScreen, Notebook };

struct Button {
    sf::FloatRect rect;
    string label;
};

struct RiddleData {
    string object, question, answer, hint1, hint2, image;
};

// Game variables
Screen screen = Menu;
Screen returnAfterNotebook = Game;
int currentRoom = 0;
int ghostRoom = 0;
int selectedChoice = -1;
int score = 0;

array<bool, 2> clueCollected{ false, false };  // 2 rooms hein abhi 

string notebook = "";
string statusMessage = "Explore the room. Choose one object to investigate.";

map<string, sf::Texture> textures;
sf::Font font;
sf::Music horrorMusic;

float musicVolume = 35.f;
bool soundEnabled = true;

const array<string, 2> roomNames{ "The Bedroom", "The Library" };
const array<int, 2> roomCodes{ 2, 4 }; // answer of room 1 and room 2 

// Functions used in game
void drawMenu(sf::RenderWindow& window);
void drawSettings(sf::RenderWindow& window);
void drawHowToPlay(sf::RenderWindow& window);
void drawGame(sf::RenderWindow& window);
void drawRiddle(sf::RenderWindow& window);
void drawNotebook(sf::RenderWindow& window);
void handleClick(sf::RenderWindow& win, sf::Vector2f p);

// Loads an image and saves it for later use
bool loadTexture(const string& key, const string& path) {
    sf::Texture tex;
    if (!tex.loadFromFile(path)) return false;
    textures.insert_or_assign(key, move(tex));
    return true;
}

// Displays an image at the given position and size
void drawImage(sf::RenderWindow& win, const string& key,
    float x, float y, float w, float h) {

    auto it = textures.find(key);
    if (it == textures.end()) return;

    sf::Sprite sprite(it->second);
    const auto sz = it->second.getSize();

    sprite.setPosition({ x, y });
    sprite.setScale({
        w / static_cast<float>(sz.x),
        h / static_cast<float>(sz.y)
        });

    win.draw(sprite);
}

// Draws a panel on the screen
void drawPanel(sf::RenderWindow& win, float x, float y,
    float w, float h,
    // blue color     transparency
    sf::Color color = sf::Color(5, 10, 22, 225)) {

    sf::RectangleShape panel({ w, h });
    panel.setPosition({ x, y });
    panel.setFillColor(color);
    panel.setOutlineColor(sf::Color(190, 139, 62, 230));
    panel.setOutlineThickness(1.5f);

    win.draw(panel);
}

// Displays text on the screen
void drawText(sf::RenderWindow& win, const string& s,
    float x, float y, unsigned size = 20,
    sf::Color color = sf::Color(240, 235, 220)) {

    sf::Text t(font, s, size);
    t.setFillColor(color);
    t.setPosition({ x, y });

    win.draw(t);
}

// Draws a button with its label
void drawButton(sf::RenderWindow& win, const Button& b,
    bool selected = false) {

    sf::RectangleShape shape({ b.rect.size.x, b.rect.size.y });
    shape.setPosition(b.rect.position);

    // condition ? if true then do etc : if false then do etc
    shape.setFillColor(selected
        ? sf::Color(94, 54, 34, 245)
        : sf::Color(7, 17, 32, 238));

    shape.setOutlineColor(selected
        ? sf::Color(255, 203, 103)
        : sf::Color(126, 153, 179));

    shape.setOutlineThickness(selected ? 2.5f : 1.3f);

    win.draw(shape);

    drawText(win, b.label,
        b.rect.position.x + 12,
        b.rect.position.y + 12, 18);
}

// Checks whether the mouse is inside a button or area
bool hit(const sf::Vector2f& p, const sf::FloatRect& r) {
    return r.contains(p);
}

// Breaks long text into smaller lines
void drawWrappedApprox(sf::RenderWindow& win, const string& text,
    float x, float y, unsigned size, unsigned maxChars,
    sf::Color color = sf::Color(240, 235, 220)) {

    stringstream input(text);
    string word;
    string line = "";
    float currentY = y;

    while (input >> word) {
        if (line.length() + word.length() + 1 > maxChars && line != "") {
            drawText(win, line, x, currentY, size, color);
            currentY = currentY + size + 7;
            line = "";
        }

        if (line != "") line = line + " ";
        line = line + word;
    }

    if (line != "") drawText(win, line, x, currentY, size, color);
}

// Returns the riddle selected by the player
RiddleData getCurrentRiddle() {
    RiddleData r;
    r.answer = "2";

    if (selectedChoice == 0) {
        r.object = "The Hidden Number ";
        r.image = "clock.png";
        r.question = "I am the only even number that is also prime. Which number am I?";
        r.hint1 = "Think of the smallest prime number.";
    }
    else if (selectedChoice == 1) {
        r.object = "The Old Inscription ";
        r.image = "bookshelf.png";
        r.question = "Two footsteps enter the hall. What number represents a pair?";
        r.hint1 = "Count the footsteps.";
    }
    else {
        r.object = "The Cipher Calculation ";
        r.image = "globe.png";
        r.question = "Solve the cipher: (7 x 2) - 12 = ?";
        r.hint1 = "Multiply 7 by 2 first.";
    }

    return r;
}

// Resets the game when a new game starts
void resetGame() {
    currentRoom = 0;
    selectedChoice = -1;
    score = 0;
    clueCollected = { false, false };
    notebook.clear();

    statusMessage = "Explore the room. Choose one object to investigate.";
    screen = Game;
}



///// MEMBER 2 - Game Screens

void drawMenu(sf::RenderWindow& window) {
    drawImage(window, "menu", 0, 0, static_cast<float>(W), static_cast<float>(H));
//GETTING MOUSE POSITION TO CHECK WHICH MENU OPTION IS SELECTED
    const sf::Vector2f mp =
        window.mapPixelToCoords(sf::Mouse::getPosition(window));

    const array<sf::FloatRect, 4> menuZones{ {
        {{160,380},{410,78}}, {{160,475},{410,72}},
        {{160,565},{410,72}}, {{160,650},{410,72}}
    } };

    for (size_t i = 0; i < menuZones.size(); ++i) {
        if (menuZones[i].contains(mp)) {
// CREATING A GLOW EFFECT
            sf::RectangleShape glow(menuZones[i].size);
            glow.setPosition(menuZones[i].position);
            glow.setFillColor(sf::Color(255, 170, 65, 28));
            glow.setOutlineColor(sf::Color(255, 194, 93, 230));
            glow.setOutlineThickness(3.f);

            window.draw(glow);
        }
    }
}

// DISPLAY GAME SETTING
void drawSettings(sf::RenderWindow& window) {
    drawImage(window, "concept_settings", 0, 0,
        static_cast<float>(W), static_cast<float>(H));

    sf::RectangleShape shade({ static_cast<float>(W), static_cast<float>(H) });
    shade.setFillColor(sf::Color(0, 0, 0, 175));
    window.draw(shade);

    drawPanel(window, 260, 150, 680, 440, sf::Color(4, 9, 20, 245));

    drawText(window, "GAME SETTINGS", 440, 190, 36,
        sf::Color(255, 203, 111));

    drawButton(window, {
        {{360, 350}, {170, 58}},
        soundEnabled ? "SOUND: ON" : "SOUND: OFF"
        }, true);

    drawButton(window, {
        {{440,650},{320,58}}, "BACK TO MENU"
        }, true);
}

// DISPLAY THE INSTRUCTIONS
void drawHowToPlay(sf::RenderWindow& window) {
    drawImage(window, "concept_intro", 0, 0,
        static_cast<float>(W), static_cast<float>(H));

    sf::RectangleShape shade({ static_cast<float>(W), static_cast<float>(H) });
    shade.setFillColor(sf::Color(0, 0, 0, 170));
    window.draw(shade);

    drawPanel(window, 220, 100, 760, 560, sf::Color(4, 9, 20, 245));

    drawText(window, "READ BEFORE YOU EMBARRASS YOURSELF", 440, 130, 27,
        sf::Color(255, 203, 111));

    drawText(window,
        "1. Choose one of the 3 objects in each room and solve the puzzle.\n\n 2.You can write anything in notebook.",
        285, 220, 20, sf::Color(240, 235, 220));

    drawButton(window, {
        {{440,650},{320,58}}, "BACK TO MENU"
        }, true);
}

// DISPLAY CURENT ROOM AND OBJECT
void drawGame(sf::RenderWindow& window){
//MAKING SURE THE ROOM NO STAYS WITHIN THE LIMIT
    if (currentRoom > 1) currentRoom = 0;
//LOADING BACKGROUND ACCORDING TO CURRENT ROOM
    drawImage(window, "room" + to_string(currentRoom + 1),
        0, 0, static_cast<float>(W), static_cast<float>(H));

    sf::RectangleShape shade({ static_cast<float>(W), static_cast<float>(H) });
    shade.setFillColor(sf::Color(0, 0, 0, 82));
    window.draw(shade);

    // ROOM INFO
    drawPanel(window, 20, 16, 270, 58, sf::Color(4, 9, 19, 230));

    drawText(window, "ROOM " + to_string(currentRoom + 1) + " / 2",
        38, 31, 22, sf::Color(255, 205, 111));

    drawButton(window, {
        {{750, 16}, {420, 46}}, "NOTEBOOK(ie:your last brain cell)"
        });

    drawPanel(window, 32, 145, 515, 105, sf::Color(3, 8, 18, 222));

    drawText(window, roomNames[currentRoom],
        52, 158, 26, sf::Color(255, 203, 111));

    drawText(window,
        clueCollected[currentRoom]
        ? "Evidence recorded. Click below to enter next room."
        : "Choose one of 3 objects to investigate.",
        52, 198, 17, sf::Color(200, 215, 235));

    //  IMAGES AND NAMES OF THE OBJECTS PLAYER CAN INVESTIGATE
    const array<string, 3> objectNames{
        "clock.png", "bookshelf.png", "globe.png"
    };

    const array<string, 3> objectLabels{
        "English Riddle 1", "English Riddle 2", "Maths Puzzle"
    };
//CREATING 3 OBJECT OPTION FOR PLAYER
    for (int i = 0; i < 3; ++i) {
        float x = 52.f + i * 370.f;

        drawPanel(window, x, 500, 340, 170,
            sf::Color(4, 8, 16, 238));

        drawImage(window, objectNames[i], x + 74, 507, 192, 100);

        drawButton(window, {
            {{x + 20, 614}, {300, 42}},
            clueCollected[currentRoom]
            ? "EVIDENCE FOUND"
            : (to_string(i + 1) + ". " + objectLabels[i])
            }, clueCollected[currentRoom]);
    }

    if (clueCollected[currentRoom]) {
        drawButton(window, {
            {{430, 690}, {340, 48}},
            currentRoom < 1 ? "ENTER NEXT ROOM" : "FINISH HALF CODE"
            }, true);
    }
    else {
        drawText(window, "Solve one puzzle to proceed.",
            450, 695, 17, sf::Color(255, 221, 163));
    }
}

// Displays the selected riddle
void drawRiddle(sf::RenderWindow& window) {
    drawImage(window, "concept_riddle", 0, 0,
        static_cast<float>(W), static_cast<float>(H));

    sf::RectangleShape shade({ static_cast<float>(W), static_cast<float>(H) });
    shade.setFillColor(sf::Color(0, 0, 0, 145));
    window.draw(shade);

    drawPanel(window, 235, 105, 730, 545,
        sf::Color(4, 9, 21, 245));
// LOADING RIDDLE ACCORDING TO THE SELECTED OBJECT
    RiddleData r = getCurrentRiddle();

    drawText(window, "ROOM " + to_string(currentRoom + 1) + " / RIDDLE",
        285, 135, 22, sf::Color(255, 202, 111));

    drawImage(window, r.image, 500, 180, 200, 105);

    drawText(window, r.object, 520, 290, 26,
        sf::Color(205, 220, 246));
//SHOWING ACTUAL PUZZLE FOR PLAYER TO SOLVE
    drawWrappedApprox(window, r.question, 295, 350, 22, 49);

    drawButton(window, {
        {{650,565},{480,54}}, "Clue mil gaya,MIRACLE...MIRACLE"
        }, true);

    drawButton(window, {
        {{1040,650},{120,48}}, "BACK"
        });
}

// Displays the detective's notebook
void drawNotebook(sf::RenderWindow& window) {
    drawImage(window, "room" + to_string(currentRoom + 1),
        0, 0, static_cast<float>(W), static_cast<float>(H));

    sf::RectangleShape shade({ static_cast<float>(W), static_cast<float>(H) });
    shade.setFillColor(sf::Color(0, 0, 0, 185));
    window.draw(shade);

    drawText(window, "DETECTIVE'S NOTEBOOK",
        390, 35, 31, sf::Color(255, 203, 111));

    drawImage(window, "notebook_art", 105, 90, 990, 575);

    // Shows the clues collected in each room
    drawText(window, "COLLECTED CLUES",
        180, 165, 20, sf::Color(74, 45, 27));

    float clueY = 210.f;
// GOING THROUGH BOTH ROOMS TO COLLECT CLUES
    for (int i = 0; i < 2; ++i) {
        string label = "Room " + to_string(i + 1) + ": ";

        string clue = clueCollected[i]
            ? "Puzzle Answer Recorded"
            : "Not discovered yet";

        drawText(window, label + clue,
            175, clueY, 15, sf::Color(61, 42, 27));

        clueY += 57.f;
    }

    // Displays the notes written by the player
    drawText(window,
        "MY NOTES \n Space Bar: 1 word space \n Enter : move to next line",
        650, 165, 20, sf::Color(74, 45, 27));

    float y = 230.f;
    string line = "";
    unsigned chars = 0;

    for (char c : notebook) {
        if (c == '\n' || chars >= 35) {
            drawText(window, line, 650, y, 16,
                sf::Color(42, 31, 24));

            line.clear();
            y += 27.f;
            chars = 0;

            if (c == '\n') continue;
        }

        line += c;
        ++chars;

        if (y > 555) break;
    }
//CHECKING IF THE TEXT GOES OUT OF THE BOUNDARY 
    if (y <= 555)
        drawText(window, line, 650, y, 16,
            sf::Color(42, 31, 24));

    drawButton(window, {
        {{1000, 25}, {155, 48}}, "CLOSE NOTEBOOK"
        }, true);
}








