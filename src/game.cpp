// A SIMPLE TWO BALL GAME , WHERE WE HAVE A LARGE SCRREN AND TWO PLAYER BALLS
// WHICH SHOOT BOUNCE BALLS AT EACH OTHER AND THEY BOUNCE ON THE WALLS BACK &
// FORTH
// - FEATURES: PLAYER SHOOTS BALLS , OPPOSITE BALLS CANCEL EACH OTHER , BALLS
// SHOT BOUNCE AROUND THE WALLS (FOR NOW A FOUR WALLS);

// DESIGN
// classes/structs :: Player(1,2) , balls(diff types)
// window size is the walls itself with a border
// Player { health,name,bullets array,sheild,speed }
// Bullet {type,damage,bounceLimit,speed}
#include <SFML/Config.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/PrimitiveType.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Transformable.hpp>
#include <SFML/Graphics/Vertex.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/WindowStyle.hpp>
#include <assert.h>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <set>
// UTILITY & INTIAL
struct Bullet {
  sf::CircleShape b;
  std::string owner;
  int type = 0;
  float damage;
  sf::Vector2f position;
  sf::Vector2f velocity;
  sf::Vector2f direction;
  float speed_constant = 10.f;
  float blastRadius = 2.5f;
  int bounceCount = 4;
  Bullet(sf::Vector2f pos, int type, sf::Vector2f dir = {1.f, 1.f},
         std::string name = "p2")
      : position(pos), direction(dir), owner(name) {
    damage = 5.f;
    b.setRadius(10.f);
    b.setOrigin(5.f, 5.f);
    b.setPosition(pos);

    float len =
        std::sqrt(direction.x * direction.x + direction.y * direction.y);
    if (len != 0)
      direction /= len;
    velocity = direction * speed_constant;
    // 1: fast bullet , 2:higher damage bullet , 3:higher blast radius
    switch (type) {
    case 1:
      std::cout << 1 << std::endl;
      velocity = velocity * 2.f;
      bounceCount += 2;
      break;
    case 2:
      std::cout << 2 << std::endl;
      damage *= 5;
      bounceCount += 1;
      break;
    case 3:
      std::cout << 3 << std::endl;
      // use the blast radius part in the collison part as it will be blast
      bounceCount += 0;
      break;
    default:
      break;
    }
  }
  void updatemovement(sf::Time dt, int ENDX, int ENDY) {

    sf::Vector2f nextPosition =
        position + velocity * dt.asSeconds() * speed_constant;

    if (nextPosition.x <= 64 || nextPosition.x >= ENDX) {
      int randomint = std::rand() % 3;
      type = randomint;
      velocity.x = -velocity.x;
      bounceCount--;
    }
    if (nextPosition.y <= 64 || nextPosition.y >= ENDY) {
      int randomint2 = std::rand() % 3;
      type = randomint2;
      velocity.y = -velocity.y;
      bounceCount--;
    }
    position += velocity * dt.asSeconds() * speed_constant;
    b.setPosition(position);
  }
  // rendering of bullets done under player itself
};

class Player {
public:
  Player(sf::RenderWindow &win, std::string name)
      : contextwindow(win), name(name) {
    sf::Vector2u windowSize = win.getSize();
    unsigned int width = windowSize.x;
    unsigned int height = windowSize.y;

    position = sf::Vector2f(width / 2.f, height / 2.f);
    health = 100.0f;
  }
  void setintialPosition(sf::Vector2f intialPosition) {
    position = intialPosition;
  }
  void update(sf::Time deltatime) {
    const float R = 70;
    const float speed = 200.f;   // pixels per second
    const float coolDown = 0.7f; // seconds
    float ENDX = contextwindow.getSize().x - R;
    float ENDY = contextwindow.getSize().y - R;

    sf::Vector2i mousePixel = sf::Mouse::getPosition(contextwindow);
    sf::Vector2f mouseWorld = contextwindow.mapPixelToCoords(mousePixel);
    dir = mouseWorld - position;

    float moveDistance = speed * deltatime.asSeconds();

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up) &&
        position.y - moveDistance >= R)
      position.y -= moveDistance;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down) &&
        position.y + moveDistance <= ENDY)
      position.y += moveDistance;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left) &&
        position.x - moveDistance >= R)
      position.x -= moveDistance;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right) &&
        position.x + moveDistance <= ENDX)
      position.x += moveDistance;

    // shoot logic with cooldown
    if ((sf::Keyboard::isKeyPressed(sf::Keyboard::A) ||
         sf::Mouse::isButtonPressed(sf::Mouse::Left)) &&
        shootclock.getElapsedTime().asSeconds() > coolDown) {
      bullets.push_back(Bullet(position, 1, dir, this->name));
      shootclock.restart();
    }

    updateBullets(deltatime);
  }
  void updateBullets(sf::Time dt) {
    float ENDX = contextwindow.getSize().x - 70;
    float ENDY = contextwindow.getSize().y - 70;
    for (auto &bi : bullets) {
      bi.updatemovement(dt, ENDX, ENDY);
    }
  }
  static void collisionBtwForeignEntites(std::vector<Player *> &players) {
    // remember i need to run the clear with bounce factor ....
    for (int i = 0; i < players.size(); i++) {
      for (int j = 0; j < players.size(); j++) {

        auto &bulletsI = players[i]->bullets;
        auto &bulletsJ = players[j]->bullets;

        if (i == j) {
          // Bullet-bullet collision within same player
          for (int x = bulletsI.size() - 1; x >= 0; --x) {
            for (int y = x - 1; y >= 0; --y) {
              auto posX = bulletsI[x].b.getPosition();
              auto posY = bulletsI[y].b.getPosition();

              float dx = posX.x - posY.x;
              float dy = posX.y - posY.y;
              float dist = std::sqrt(dx * dx + dy * dy);

              if (dist <=
                  bulletsI[x].b.getRadius() + bulletsI[y].b.getRadius()) {
                bulletsI[x].velocity = -bulletsI[x].velocity;
                bulletsI[y].velocity = -bulletsI[y].velocity;
                bulletsI[x].bounceCount--;
                bulletsI[y].bounceCount--;
                break;
              }
            }
          }
          continue;
        }

        // For i ≠ j: bullets of different players
        sf::Vector2f playerIposition = players[i]->position;

        for (int i1 = bulletsI.size() - 1; i1 >= 0; --i1) {
          for (int j1 = bulletsJ.size() - 1; j1 >= 0; --j1) {
            sf::Vector2f posI = bulletsI[i1].b.getPosition();
            sf::Vector2f posJ = bulletsJ[j1].b.getPosition();

            float dx = posI.x - posJ.x;
            float dy = posI.y - posJ.y;
            float distance = std::sqrt(dx * dx + dy * dy);

            float ex = playerIposition.x - posJ.x;
            float ey = playerIposition.y - posJ.y;
            float playerBulletDistance = std::sqrt(ex * ex + ey * ey);

            // Bullet-Bullet collision
            if (distance <=
                bulletsI[i1].b.getRadius() + bulletsJ[j1].b.getRadius()) {
              bulletsI.erase(bulletsI.begin() + i1);
              bulletsJ.erase(bulletsJ.begin() + j1);
              break;
            }

            // Enemy bullet hits player
            if (playerBulletDistance <= 10 + bulletsJ[j1].b.getRadius()) {

              if (bulletsJ[j1].type == 3) {
                float blastRadius = bulletsJ[j1].blastRadius;

                sf::Vector2f blastCenter = bulletsJ[j1].b.getPosition();

                // Apply Blat radius damage to players within the blast radius
                for (auto ps : players) {
                  float dx = ps->position.x - blastCenter.x;
                  float dy = ps->position.y - blastCenter.y;
                  float distSquared = dx * dx + dy * dy;

                  if (distSquared <= blastRadius * blastRadius) {
                    ps->health -= bulletsJ[j1].damage;
                  }
                }
              }

              players[i]->health -= bulletsJ[j1].damage;
              bulletsJ.erase(bulletsJ.begin() + j1);
              break;
            }
          }
        }
      }
    }
    for (auto allP : players) {
      for (int bulletsperP = allP->bullets.size() - 1; bulletsperP >= 0;
           bulletsperP--) {
        if (allP->bullets[bulletsperP].bounceCount == 0) {
          allP->bullets.erase(allP->bullets.begin() + bulletsperP);
        }
      }
    }
  }
  void renderBullets() {
    for (auto bu : bullets) {
      if (bu.type == 1)
        bu.b.setFillColor(sf::Color::Cyan);
      if (bu.type == 2)
        bu.b.setFillColor(sf::Color::Green);
      if (bu.type == 3)
        bu.b.setFillColor(sf::Color::Magenta);

      contextwindow.draw(bu.b);
    }
  }
  void render() {
    sf::CircleShape p1(10);
    p1.setPosition(position.x, position.y);
    if (this->name == "p1") {
      p1.setFillColor(sf::Color::Yellow);
    } else {
      p1.setFillColor(sf::Color::Blue);
    }
    sf::RectangleShape line(sf::Vector2f(40.f, 10.f));
    float lengthDir = std::sqrt(dir.x * dir.x + dir.y * dir.y);
    if (lengthDir != 0)
      dir /= lengthDir;

    line.setPosition(position);
    float angle = std::atan2(dir.y, dir.x) * 180.f / 3.14159f;

    p1.setOrigin(10, 10);
    line.setOrigin(20.f, 4.8f);

    p1.setRotation(angle);
    line.setRotation(angle);

    line.setFillColor(sf::Color::Red);

    contextwindow.draw(line);
    contextwindow.draw(p1);
    renderBullets();
  }

private:
  std::string name;
  sf::Vector2f position;
  float health;
  sf::Vector2f dir;
  sf::RenderWindow &contextwindow;
  std::vector<Bullet> bullets;
  sf::Clock shootclock;
};

class GameTile : public sf::Drawable, public sf::Transformable {
public:
  // tiles: string of tile indices (as char) of size_width * size_height
  bool loadGameTile(const std::string &texturePath, const std::string &tiles,
                    sf::Vector2u tileSize, int size_width, int size_height,
                    int padding = 0) {

    if (!GTtexture.loadFromFile(texturePath))
      return false;

    GTarray.setPrimitiveType(sf::Quads);
    GTarray.resize(size_width * size_height * 4);

    outlineArray.setPrimitiveType(sf::Lines);
    outlineArray.clear();

    int textureCols = GTtexture.getSize().x / (tileSize.x + padding);

    for (int i = 0; i < size_width; ++i) {
      for (int j = 0; j < size_height; ++j) {
        int index = tiles[i + j * size_width];
        int xTile = index % textureCols;
        int yTile = index / textureCols;
        if (yTile == 0)
          yTile = 1;
        // Screen space corners
        sf::Vector2f topLeft(i * tileSize.x, j * tileSize.y);
        sf::Vector2f topRight((i + 1) * tileSize.x, j * tileSize.y);
        sf::Vector2f bottomRight((i + 1) * tileSize.x, (j + 1) * tileSize.y);
        sf::Vector2f bottomLeft(i * tileSize.x, (j + 1) * tileSize.y);

        // Get quad pointer
        sf::Vertex *quad = &GTarray[(i + j * size_width) * 4];

        quad[0].position = topLeft;
        quad[1].position = topRight;
        quad[2].position = bottomRight;
        quad[3].position = bottomLeft;

        // Texture coordinates

        sf::Vector2f texTopLeft(xTile * (tileSize.x + padding),
                                yTile * (tileSize.y + padding));
        sf::Vector2f texTopRight = texTopLeft + sf::Vector2f(tileSize.x, 0);
        sf::Vector2f texBottomRight =
            texTopLeft + sf::Vector2f(tileSize.x, tileSize.y);
        sf::Vector2f texBottomLeft = texTopLeft + sf::Vector2f(0, tileSize.y);

        if (i > 1 && i < size_width - 2 && j > 1 && j < size_height - 2) {

          quad[0].color = sf::Color::Black;
          quad[1].color = sf::Color::Black;
          quad[2].color = sf::Color::Black;
          quad[3].color = sf::Color::Black;

        } else {
          quad[0].texCoords = texTopLeft;
          quad[1].texCoords = texTopRight;
          quad[2].texCoords = texBottomRight;
          quad[3].texCoords = texBottomLeft;
        }

        // Outline lines (4 per quad)
        outlineArray.append(sf::Vertex(topLeft, sf::Color::White));
        outlineArray.append(sf::Vertex(topRight, sf::Color::White));

        outlineArray.append(sf::Vertex(topRight, sf::Color::White));
        outlineArray.append(sf::Vertex(bottomRight, sf::Color::White));

        outlineArray.append(sf::Vertex(bottomRight, sf::Color::White));
        outlineArray.append(sf::Vertex(bottomLeft, sf::Color::White));

        outlineArray.append(sf::Vertex(bottomLeft, sf::Color::White));
        outlineArray.append(sf::Vertex(topLeft, sf::Color::White));
      }
    }

    return true;
  }

private:
  sf::VertexArray GTarray;
  sf::VertexArray outlineArray;
  sf::Texture GTtexture;

  virtual void draw(sf::RenderTarget &target, sf::RenderStates states) const {
    states.transform *= getTransform();
    states.texture = &GTtexture;
    target.draw(GTarray, states);
    // target.draw(outlineArray);
  }
};

// GAME LOOP
int main() {
  sf::RenderWindow window(sf::VideoMode({600, 810}), "PingPOP",
                          sf::Style::Titlebar | sf::Style::Close);
  sf::Vector2u windowSize = window.getSize();
  unsigned int width = windowSize.x;
  unsigned int height = windowSize.y;
  window.setPosition(sf::Vector2i(500, 500));

  std::vector<Player *> players;
  Player *p1 = new Player(window, "p1");
  Player *p2 = new Player(window, "p2");
  p2->setintialPosition(sf::Vector2f{85.f, 85.f});
  players.push_back(p1);
  players.push_back(p2);
  GameTile tilemap;
  sf::Clock clock;

  // Generate dummy tile layout with characters (tile indices as char)
  std::string tileLayout;
  for (int i = 0; i < 19 * 25; ++i) {
    tileLayout +=
        static_cast<char>(i % 24); // assuming you have 24 tiles in the texture
  }
  if (!tilemap.loadGameTile("../fantasy-tileset.png", tileLayout, {32, 32}, 19,
                            25)) {
    return -1;
  }

  while (window.isOpen()) {

    // updation events
    sf::Event event;
    sf::Time elapsed1 = clock.restart();
    while (window.pollEvent(event)) {

      if (event.type == sf::Event::Closed)
        window.close();
    }

    players[0]->update(elapsed1);
    players[1]->update(elapsed1);
    Player::collisionBtwForeignEntites(players);
    // updation of rendering occurs , remember we use double buffering so do
    // not use anyother strategy to draw stuff besides below clear , draw ,
    // display
    window.clear();
    // draw stuff over here
    window.draw(tilemap);
    players[0]->render();
    players[1]->render();
    window.display();
  }

  return 0;
}

// CLEAN BUFFERS
