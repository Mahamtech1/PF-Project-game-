///// MEMBER 1 

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <optional>
#include <string>
#include <array>
#include <algorithm>
#include <map>
#include <sstream>

using namespace std;

const unsigned W = 1200;
const unsigned H = 760;

// Different screens of the game
enum Screen { Menu, Settings, HowToPlay, Game, RiddleScreen, Detective, Notebook, FinalGuess, Result };

struct Button {
    sf::FloatRect rect;
    string label;
};

struct RiddleData {
    string object, question, answer, hint1, image;
};

// Game variables
Screen screen = Menu;
Screen returnAfterNotebook = Game;
int currentRoom = 0;
int ghostRoom = 0;
int selectedChoice = -1;
int score = 0;
int playerGuess = -1;

array<bool, 4> clueCollected{ false, false, false, false };  // 4 rooms hein ab 
array<int, 4> hintsUsed{ 0, 0, 0, 0 }; // har room mai 1 hint allow
array<string, 4> collectedClues{};


string notebook = "";
string statusMessage = "Explore the room. Choose one object to investigate.";
string detectiveMessage = "Choose a puzzle first; then I can give you a hint.";

map<string, sf::Texture> textures;
sf::Font font;
sf::Music horrorMusic;

float musicVolume = 35.f;
bool soundEnabled = true;

const array<string, 4> roomNames{ "The Bedroom", "The Library", "The Living Room", "The Attic" };
const array<array <int, 4>, 4> roomCodes{ {

    {{2,4,1,3}}, // 0
    {{3,1,4,2}}, // 1
    {{4,2,3,1}}, // 2
    {{1,3,2,4}}  // 3

} };// answer of rooms randomly

// Functions used in game

//   by ref       main window   parameter 
void drawMenu(sf::RenderWindow& window);
void drawSettings(sf::RenderWindow& window);
void drawHowToPlay(sf::RenderWindow& window);
void drawGame(sf::RenderWindow& window);
void drawRiddle(sf::RenderWindow& window);
void drawDetective(sf::RenderWindow& window);
void drawNotebook(sf::RenderWindow& window);
void drawFinalGuess(sf::RenderWindow& window);
void drawResult(sf::RenderWindow& window);
void handleClick(sf::RenderWindow& win, float clickX, float clickY);

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

        //  dark brown,  higly visible
        ? sf::Color(94, 54, 34, 245)
        : sf::Color(7, 17, 32, 238));
    // blue-black

    shape.setOutlineColor(selected

        // golden yellow (border of box)   max 255 transparency here 
        ? sf::Color(255, 203, 103)
        : sf::Color(126, 153, 179));
    // dusty_blue grey colour

    shape.setOutlineThickness(selected ? 2.5f : 1.3f);

    win.draw(shape);

    drawText(win, b.label,

        //  12 add kia bez left size se 12 pixel ke sapce dene phir text show ho
        b.rect.position.x + 12,
        b.rect.position.y + 12, 18);
    // top sar 12 pixel space // 18 text ka font 
    // kiy keh koi colour ham nay dey rahay to deafult colour(cream) use hota with 255 transparency

}

// Checks whether the click coordinates are inside a button or area
bool hit(float px, float py, const sf::FloatRect& r)
{
    return r.contains({ px,py });
}

// Breaks long text into smaller lines
void drawWrappedApprox(sf::RenderWindow& win, const string& text,
    float x, float y, unsigned size, unsigned maxChars,
    sf::Color color = sf::Color(240, 235, 220))
    //                         cream colour 
{

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

        if (line != "")
        {
            line = line + " ";
        }
        line = line + word;
    }

    if (line != "") drawText(win, line, x, currentY, size, color);
}

// Returns the riddle selected by the player based on the current room
RiddleData getCurrentRiddle() {
    RiddleData r;
    int ans = roomCodes[ghostRoom][currentRoom];
    r.answer = to_string(ans);

    // Room 1 Riddles
    if (currentRoom == 0) {
        if (selectedChoice == 0) {
            r.object = "The Hidden Number";
            r.image = "clock.png";
            r.question = "I am the only even number that is also prime. Which number am I?";
            r.hint1 = "Think about the smallest prime number.";
            r.answer = "2";
        }
        else if (selectedChoice == 1) {
            r.object = "The mystery";
            r.image = "bookshelf.png";
            r.question = "If a phantom loses one of its boots, how many feet does it still have left to walk?";
            r.hint1 = "Same as the number of feets that we have normally.";
            r.answer = "2";
        }
        else {
            r.object = "Calculation";
            r.image = "globe.png";
            r.question = "Solve the cursed sum: \n (7 x 3) - 19 = ?";
            r.hint1 = "Multiply 7 by 3 first.";
            r.answer = "2";
        }
    }
    // Room 2 Riddles (New 3 Riddles for Room 2)
    else if (currentRoom == 1) {
        if (selectedChoice == 0) {
            r.object = "The Grand Clock";
            r.image = "clock.png";
            r.question = "How many spooky corners are there in a haunted square room?";
            r.hint1 = "Same as a normal square.";
            r.answer = "4";
        }
        else if (selectedChoice == 1) {
            r.object = "The Dusty Mirror";
            r.image = "bookshelf.png";
            r.question = "How many wooden legs does the haunted table standing in the middle of the dark room have?";
            r.hint1 = "Same as a normal table.";
            r.answer = "4";
        }
        else {
            r.object = "The numeric nightmare";
            r.image = "globe.png";
            r.question = "Solve the cursed sum :\n 10 - (3 x 2) = ?";;
            r.hint1 = "Multiply 3 by 2 first.";
            r.answer = "4";
        }
    }
    // Room 3 Riddles (New 3 Riddles for Room 3)
    else if (currentRoom == 2) {
        if (selectedChoice == 0) {
            r.object = "The Cold Fire";
            r.image = "fireplace.png";
            r.question = "Five doors are locked, but the ghost unlocks just one. How many escape routes are open?";
            r.hint1 = "Only one door was unlocked.";
            r.answer = "1";
        }
        else if (selectedChoice == 1) {
            r.object = "The Broken Vase";
            r.image = "vase.png";
            r.question = "A dark room has only one flickering candle burning. How many flames are left in the dark?";
            r.hint1 = "Look at the single candle mentioned.";
            r.answer = "1";
        }
        else {
            r.object = "The Numeric Nightmare";
            r.image = "window.png";
            r.question = "Solve the cursed sum :\n (18 / 3) - 5 = ?";;
            r.hint1 = "Divide 18 by 3 first.";
            r.answer = "1";
        }
    }
    // Room 4 Riddles (New 3 Riddles for Room 4)
    else {
        if (selectedChoice == 0) {
            r.object = "The Cold Fire";
            r.image = "fireplace.png";
            r.question = "A cursed music box plays its haunting melody in three broken notes before falling dead silent. How many notes echo through the dark room?";
            r.hint1 = "Count the broken notes played.";
            r.answer = "3";
        }
        else if (selectedChoice == 1) {
            r.object = "The Dusty Window";
            r.image = "window.png";
            r.question = "Count the eyes of the beast carved in bone: one weeps blood, one sheds ash, and the third sees only your death. How many total gazes are fixed on you?";
            r.hint1 = "Count each eye described in the riddle.";
            r.answer = "3";
        }
        else {
            r.object = "The Numeric Nightmare";
            r.image = "vase.png";
            r.question = "Solve the cursed sum :\n 7 - ((14 x 2) / 7) = ?";
            r.hint1 = "Multiply 14 by 2 first, then divide by 7.";
            r.answer = "3";
        }
    }

    return r;
}

// Resets the game when a new game starts
void resetGame() {

    ghostRoom = 0;

    currentRoom = 0;
    selectedChoice = -1;
    score = 0;
    playerGuess = -1;


    clueCollected = { false, false, false, false };
    hintsUsed = { 0, 0, 0, 0 };
    collectedClues = {};
    notebook.clear();

    statusMessage = "Explore the room. Choose one object to investigate.";
    detectiveMessage = "Choose a puzzle first; then I can give you a hint.";
    screen = Game;
}


void saveRoomClue()
{
    if (selectedChoice < 0 || clueCollected[currentRoom])
    {
        return;
    }

    clueCollected[currentRoom] = true;
    score += 25;

    collectedClues[currentRoom] = "Clue recorded. Note your puzzle digit.";

    statusMessage = "Clue saved in notebook.";

    screen = Game;
}

void giveSingleHint()
{
    if (selectedChoice < 0)
    {
        detectiveMessage = "Open a puzzle first so I can give you a hint.";
        return;
    }

    score = max(0, score - 5);
    RiddleData r = getCurrentRiddle();
    detectiveMessage = "Hint: " + r.hint1;
    hintsUsed[currentRoom] = 1;
}




///// MEMBER 2 - Game Screens

void drawMenu(sf::RenderWindow& window) {
    drawImage(window, "menu", 0, 0, static_cast<float>(W), static_cast<float>(H));
    //GETTING MOUSE POSITION TO CHECK WHICH MENU OPTION IS SELECTED
    sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
    float mx = mousePos.x;
    float my = mousePos.y;

    const array<sf::FloatRect, 4> menuZones{ {
        {{160,380},{410,78}}, {{160,475},{410,72}},
        {{160,565},{410,72}}, {{160,650},{410,72}}
    } };

    for (size_t i = 0; i < menuZones.size(); ++i) {
        if (hit(mx, my, menuZones[i])) {
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

    drawButton(window, { {{560, 350}, {100, 58}}, "- VOL" });
    drawButton(window, { {{680, 350}, {100, 58}}, "+ VOL" });
    drawText(window, "Volume: " + to_string(static_cast<int>(musicVolume)) + "%", 535, 435, 22, sf::Color(255, 211, 126));

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

    drawText(window, "READ BEFORE YOU EMBARRASS YOURSELF", 310, 130, 27,
        sf::Color(255, 203, 111));

    drawText(window,
        "1. Explore all 4 rooms and pick 1 of the 3 objects in each room.\n\n2. Each room gives a clue digit (1-4). Detective gives 1 hint per room (-5 pts).\n\n3. At the end, guess which room the phantom is hiding in.",
        285, 220, 20, sf::Color(240, 235, 220));

    drawButton(window, {
        {{440,650},{320,58}}, "BACK TO MENU"
        }, true);

}

// DISPLAY CURENT ROOM AND OBJECT
void drawGame(sf::RenderWindow& window) {
    if (currentRoom < 0) currentRoom = 0;
    if (currentRoom > 3) currentRoom = 3;

    drawImage(window, "room" + to_string(currentRoom + 1),
        0, 0, static_cast<float>(W), static_cast<float>(H));

    sf::RectangleShape shade({ static_cast<float>(W), static_cast<float>(H) });
    shade.setFillColor(sf::Color(0, 0, 0, 82));
    window.draw(shade);

    // ROOM INFO & SCORE
    drawPanel(window, 20, 16, 270, 58, sf::Color(4, 9, 19, 230));
    drawText(window, "ROOM " + to_string(currentRoom + 1) + " / 4",
        38, 31, 22, sf::Color(255, 205, 111));

    drawPanel(window, 875, 16, 300, 58, sf::Color(4, 9, 19, 230));
    drawText(window, "Score: " + to_string(score), 895, 31, 21, sf::Color(255, 218, 128));

    drawButton(window, { {{750, 88}, {190, 46}}, "NOTEBOOK" });
    drawButton(window, { {{950, 88}, {225, 46}}, "DETECTIVE (" + to_string(1 - hintsUsed[currentRoom]) + " hint)" });

    drawPanel(window, 32, 145, 515, 105, sf::Color(3, 8, 18, 222));
    drawText(window, roomNames[currentRoom], 52, 158, 26, sf::Color(255, 203, 111));
    drawText(window,
        clueCollected[currentRoom]
        ? "Evidence recorded. Click below to continue."
        : "Choose one of 3 objects to investigate.",
        52, 198, 17, sf::Color(200, 215, 235));

    const array<array<string, 3>, 4> roomObjects{ {
        {{"clock.png", "bookshelf.png", "globe.png"}},
        {{"clock.png", "bookshelf.png", "globe.png"}},
        {{"fireplace.png", "vase.png", "window.png"}},
        {{"fireplace.png", "window.png", "vase.png"}}
    } };

    const array<string, 3> objectLabels{
        "English Riddle 1", "English Riddle 2", "Maths Puzzle"
    };

    for (int i = 0; i < 3; ++i) {
        float x = 52.f + i * 370.f;

        drawPanel(window, x, 500, 340, 170,
            sf::Color(4, 8, 16, 238));

        drawImage(window, roomObjects[currentRoom][i], x + 74, 507, 192, 100);

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
            currentRoom < 3 ? "ENTER NEXT ROOM" : "MAKE FINAL GUESS"
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

    RiddleData r = getCurrentRiddle();

    drawText(window, "ROOM " + to_string(currentRoom + 1) + " / RIDDLE",
        285, 135, 22, sf::Color(255, 202, 111));

    drawImage(window, r.image, 500, 180, 200, 105);

    drawText(window, r.object, 520, 290, 26,
        sf::Color(205, 220, 246));

    drawWrappedApprox(window, r.question, 295, 350, 22, 49);

    drawButton(window, { {{700, 650}, {170, 48}}, "DETECTIVE" });
    drawButton(window, { {{880, 650}, {150, 48}}, "NOTEBOOK" });
    drawButton(window, { {{610,565},{320,54}}, "Clue mil gaya,MIRACLE...MIRAClE" }, true);
    drawButton(window, { {{1040,650},{120,48}}, "BACK" });
}

// Member 2 addition: Detective screen (1 hint only)
void drawDetective(sf::RenderWindow& window) {
    drawImage(window, "room" + to_string(currentRoom + 1), 0, 0, static_cast<float>(W), static_cast<float>(H));
    sf::RectangleShape shade({ static_cast<float>(W), static_cast<float>(H) });
    shade.setFillColor(sf::Color(0, 0, 0, 185));
    window.draw(shade);

    drawPanel(window, 180, 85, 840, 590, sf::Color(4, 9, 20, 245));
    drawImage(window, "detective", 205, 125, 290, 470);
    drawText(window, "THE DETECTIVE", 545, 130, 31, sf::Color(255, 203, 111));
    drawText(window, "Hints used: " + to_string(hintsUsed[currentRoom]) + " / 1", 545, 180, 19, sf::Color(198, 218, 247));

    drawPanel(window, 530, 215, 455, 185, sf::Color(12, 17, 28, 245));
    drawWrappedApprox(window, detectiveMessage, 550, 235, 21, 27);
    drawText(window, "Limit: 1 hint per room (-5 points).", 545, 425, 16, sf::Color(222, 199, 155));

    drawButton(window, { {{400,570},{260,60}}, hintsUsed[currentRoom] < 1 ? "GET 1 HINT (-5)" : "HINT USED" }, true);
    drawButton(window, { {{900,650},{220,50}}, "BACK" });
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

    drawText(window, "COLLECTED CLUES",
        180, 165, 20, sf::Color(74, 45, 27));

    float clueY = 210.f;
    for (int i = 0; i < 4; ++i) {
        string label = "Room " + to_string(i + 1) + ": ";
        string clue = clueCollected[i] ? collectedClues[i] : "Not discovered yet";
        drawText(window, label + clue, 175, clueY, 15, sf::Color(61, 42, 27));
        clueY += 52.f;
    }

    drawText(window, "Tip: Solve one puzzle in each room to gather clues.", 175, 422, 13, sf::Color(74, 45, 27));

    drawText(window, "CASE-FILE CODE KEY", 175, 442, 16, sf::Color(74, 45, 27));
    drawText(window, "2-4-1-3 = Room 1 Bedroom", 175, 468, 14, sf::Color(61, 42, 27));
    drawText(window, "3-1-4-2 = Room 2 Library", 175, 490, 14, sf::Color(61, 42, 27));
    drawText(window, "4-2-3-1 = Room 3 Living Room", 175, 512, 14, sf::Color(61, 42, 27));
    drawText(window, "1-3-2-4 = Room 4 Attic", 175, 534, 14, sf::Color(61, 42, 27));

    drawText(window, "MY NOTES", 650, 165, 20, sf::Color(74, 45, 27));
    drawText(window, "\n Space Bar: 1 word space \n\n Enter : move to next line", 670, 185, 12, sf::Color(74, 45, 27));

    float y = 257.f;
    string line = "";
    unsigned chars = 0;

    for (char c : notebook) {
        if (c == '\n' || chars >= 35) {
            drawText(window, line, 650, y, 16, sf::Color(42, 31, 24));
            line.clear();
            y += 27.f;
            chars = 0;
            if (c == '\n') continue;
        }

        line += c;
        ++chars;
        if (y > 555) break;
    }

    if (y <= 555)
        drawText(window, line, 650, y, 16, sf::Color(42, 31, 24));

    drawButton(window, { {{1000, 30}, {185, 48}}, "CLOSE NOTEBOOK" }, true);
}

// Member 2 addition: Final Guess & Result screens
void drawFinalGuess(sf::RenderWindow& window) {
    drawImage(window, "concept_final_guess", 0, 0, static_cast<float>(W), static_cast<float>(H));
    sf::RectangleShape shade({ static_cast<float>(W), static_cast<float>(H) });
    shade.setFillColor(sf::Color(0, 0, 0, 125));
    window.draw(shade);

    drawPanel(window, 260, 100, 680, 550, sf::Color(4, 8, 19, 235));
    drawText(window, "THE FINAL DEDUCTION", 380, 140, 34, sf::Color(255, 203, 111));
    drawText(window, "Where is the phantom hiding?", 375, 220, 22, sf::Color(200, 218, 242));

    for (int i = 0; i < 4; ++i) {
        drawButton(window, { {{355.f + (i % 2) * 255.f, 350.f + (i / 2) * 90.f}, {220, 64}}, "ROOM " + to_string(i + 1) }, true);
    }
    drawButton(window, { {{1010, 40}, {150, 48}}, "NOTEBOOK" });
}

void drawResult(sf::RenderWindow& window) {
    drawImage(window, playerGuess == ghostRoom ? "concept_correct_ending" : "concept_wrong_ending", 0, 0, static_cast<float>(W), static_cast<float>(H));
    sf::RectangleShape shade({ static_cast<float>(W), static_cast<float>(H) });
    shade.setFillColor(playerGuess == ghostRoom ? sf::Color(0, 8, 14, 125) : sf::Color(28, 0, 5, 135));
    window.draw(shade);

    drawImage(window, "phantom_reveal", 665, 65, 440, 650);
    drawPanel(window, 55, 145, 610, 450, sf::Color(2, 5, 12, 238));

    if (playerGuess == ghostRoom) {
        drawText(window, "YOU FOUND THE PHANTOM", 95, 195, 32, sf::Color(200, 225, 218));
        drawWrappedApprox(window, "You solved the mystery and escaped the curse.", 95, 260, 21, 43);
    }
    else {
        drawText(window, "WRONG ROOM GUESS", 95, 195, 30, sf::Color(255, 90, 100));
        drawWrappedApprox(window, "The phantom found you in the dark.", 95, 260, 21, 43);
        drawText(window, "Phantom was in Room " + to_string(ghostRoom + 1) + ".", 95, 365, 22, sf::Color(255, 175, 175));
    }

    drawText(window, "FINAL SCORE: " + to_string(score) + " / 100", 95, 440, 28, sf::Color(255, 211, 126));
    drawButton(window, { {{195, 555}, {320, 58}}, "RETURN TO MENU" }, true);
}

// Member 3

void handleClick(sf::RenderWindow& win, float clickX, float clickY) {

    if (screen == Menu) {
        if (hit(clickX, clickY, { {160, 380}, {410, 78} }))
            resetGame();

        else if (hit(clickX, clickY, { {160, 475}, {410, 72} }))
            screen = Settings;

        else if (hit(clickX, clickY, { {160, 565}, {410, 72} }))
            screen = HowToPlay;

        else if (hit(clickX, clickY, { {160, 650}, {410, 72} }))
            win.close();

        return;
    }

    if (screen == Settings) {
        if (hit(clickX, clickY, { {360, 350}, {170, 58} })) {
            soundEnabled = !soundEnabled;
            horrorMusic.setVolume(soundEnabled ? musicVolume : 0.f);
        }
        else if (hit(clickX, clickY, { {560, 350}, {100, 58} })) {
            musicVolume = max(0.f, musicVolume - 10.f);
            horrorMusic.setVolume(soundEnabled ? musicVolume : 0.f);
        }
        else if (hit(clickX, clickY, { {680, 350}, {100, 58} })) {
            musicVolume = min(100.f, musicVolume + 10.f);
            horrorMusic.setVolume(soundEnabled ? musicVolume : 0.f);
        }
        else if (hit(clickX, clickY, { {440, 650}, {320, 58} }))
            screen = Menu;

        return;
    }

    if (screen == HowToPlay) {
        if (hit(clickX, clickY, { {440, 650}, {320, 58} }))
            screen = Menu;

        return;
    }

    if (screen == Game) {

        if (hit(clickX, clickY, { {750, 88}, {190, 46} })) {
            returnAfterNotebook = Game;
            screen = Notebook;
            return;
        }

        if (hit(clickX, clickY, { {950, 88}, {225, 46} })) {
            screen = Detective;
            return;
        }

        if (!clueCollected[currentRoom]) {
            for (int i = 0; i < 3; ++i) {
                sf::FloatRect r({ 52.f + i * 370.f, 500.f }, { 340.f, 170.f });

                if (hit(clickX, clickY, r)) {
                    selectedChoice = i;
                    screen = RiddleScreen;
                    return;
                }
            }
        }
        else if (hit(clickX, clickY, { {430, 690}, {340, 48} })) {
            if (currentRoom < 3) {
                ++currentRoom;
                selectedChoice = -1;
            }
            else {
                screen = FinalGuess;
            }
        }

        return;
    }

    if (screen == RiddleScreen) {

        if (hit(clickX, clickY, { {700, 650}, {170, 48} })) {
            screen = Detective;
            return;
        }

        if (hit(clickX, clickY, { {880, 650}, {150, 48} })) {
            returnAfterNotebook = RiddleScreen;
            screen = Notebook;
            return;
        }

        if (hit(clickX, clickY, { {1040, 650}, {120, 48} })) {
            screen = Game;
            return;
        }

        if (hit(clickX, clickY, { {610, 565}, {320, 54} })) {
            saveRoomClue();
            return;
        }

        return;
    }

    if (screen == Detective) {
        if (hit(clickX, clickY, { {400, 570}, {260, 60} })) {
            giveSingleHint();
            return;
        }

        if (hit(clickX, clickY, { {900, 650}, {220, 50} })) {
            screen = selectedChoice < 0 ? Game : RiddleScreen;
            return;
        }

        return;
    }

    if (screen == Notebook) {
        if (hit(clickX, clickY, { {1000, 25}, {185, 48} }))
            screen = returnAfterNotebook;

        return;
    }

    if (screen == FinalGuess) {
        if (hit(clickX, clickY, { {1010, 40}, {150, 48} })) {
            returnAfterNotebook = FinalGuess;
            screen = Notebook;
            return;
        }

        for (int i = 0; i < 4; ++i) {
            sf::FloatRect r({ 355.f + (i % 2) * 255.f, 350.f + (i / 2) * 90.f }, { 220, 64.f });
            if (hit(clickX, clickY, r)) {
                playerGuess = i;
                if (playerGuess != ghostRoom) score = 0;
                screen = Result;
                return;
            }
        }

        return;
    }

    if (screen == Result) {
        if (hit(clickX, clickY, { {195, 555}, {320, 58} }))
            screen = Menu;

        return;
    }
}

// Draws whichever screen is currently open
void drawCurrentScreen(sf::RenderWindow& window) {
    window.clear(sf::Color(3, 8, 18));

    switch (screen) {
    case Menu:        drawMenu(window);       break;
    case Settings:    drawSettings(window);   break;
    case HowToPlay:   drawHowToPlay(window);  break;
    case Game:        drawGame(window);       break;
    case RiddleScreen:drawRiddle(window);     break;
    case Detective:   drawDetective(window);  break;
    case Notebook:    drawNotebook(window);   break;
    case FinalGuess:  drawFinalGuess(window); break;
    case Result:      drawResult(window);     break;
    }

    window.display();
}

// Checks mouse and keyboard events
void processEvents(sf::RenderWindow& window) {
    while (true) {
        optional<sf::Event> event = window.pollEvent();
        if (!event.has_value()) break;

        if (event->is<sf::Event::Closed>())
            window.close();

        if (const auto* mouse = event->getIf<sf::Event::MouseButtonPressed>()) {
            if (mouse->button == sf::Mouse::Button::Left) {
                sf::Vector2f clickPos = window.mapPixelToCoords(mouse->position);
                handleClick(window, clickPos.x, clickPos.y);
            }
        }

        // Handles typing inside the notebook
        if (screen == Notebook) {
            if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
                if (key->code == sf::Keyboard::Key::Backspace && !notebook.empty()) {
                    notebook.pop_back();
                }
            }

            if (const auto* textEvent = event->getIf<sf::Event::TextEntered>()) {
                if (textEvent->unicode >= 32 && textEvent->unicode < 127) {
                    if (notebook.size() < 400)
                        notebook += static_cast<char>(textEvent->unicode);
                }
                else if (textEvent->unicode == '\r' || textEvent->unicode == '\n') {
                    if (notebook.size() < 400)
                        notebook += '\n';
                }
            }
        }
    }
}

// Starts the game
int main() {
    sf::RenderWindow window(
        sf::VideoMode({ W, H }),
        "The Hunt Of the Cursed Phantom"
    );
    window.setFramerateLimit(60);

    // Loads the font
    if (!font.openFromFile("C:/Windows/Fonts/georgia.ttf")) {
        if (!font.openFromFile("C:/Windows/Fonts/arial.ttf"))
            return 1;
    }

    // Loads all game images
    loadTexture("menu", "assets/menu.jpg");
    loadTexture("concept_settings", "assets/concept_settings.jpg");
    loadTexture("concept_intro", "assets/concept_intro.jpg");
    loadTexture("concept_riddle", "assets/concept_riddle.jpg");
    loadTexture("concept_final_guess", "assets/concept_final_guess.jpg");
    loadTexture("concept_wrong_ending", "assets/concept_wrong_ending.jpg");
    loadTexture("concept_correct_ending", "assets/concept_correct_ending.jpg");
    loadTexture("notebook_art", "assets/notebook_blank.png");
    loadTexture("detective", "assets/detective_portrait.png");
    loadTexture("phantom_reveal", "assets/phantom_reveal.png");

    loadTexture("room1", "assets/room1_bg.jpg");
    loadTexture("room2", "assets/room2_bg.jpg");
    loadTexture("room3", "assets/room3_bg.jpg");
    loadTexture("room4", "assets/room4_bg.jpg");

    string objectFiles[6] = {
        "clock.png", "bookshelf.png", "globe.png",
        "fireplace.png", "vase.png", "window.png"
    };
    for (int i = 0; i < 6; ++i) {
        loadTexture(objectFiles[i], string("assets/") + objectFiles[i]);
    }

    // Starts the background music
    if (horrorMusic.openFromFile("assets/horror_ambience.wav")) {
        horrorMusic.setLooping(true);
        horrorMusic.setVolume(musicVolume);
        horrorMusic.play();
    }

    // Keeps the game running until the window is closed
    while (window.isOpen()) {
        processEvents(window);
        drawCurrentScreen(window);
    }

    return 0;
}
