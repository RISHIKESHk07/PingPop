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
#include <iostream>
// UTILITY & INTIAL
struct Bullet {
  sf::CircleShape b;
  int type;
  float damage;
  sf::Vector2f position;
  sf::Vector2f velocity;
  sf::Vector2f direction;
  float speed_constant = 2.f;
  float blastRadius = 2.5f;
  int bounceCount = 2;
  Bullet(sf::Vector2f pos, int type, sf::Vector2f dir = {1.f, 1.f})
      : position(pos), direction(dir) {
    damage = 5.f;
    b.setRadius(10.f);
    b.setOrigin(5.f, 5.f);
    b.setPosition(pos);

    float len =
        std::sqrt(direction.x * direction.x + direction.y * direction.y);
    if (len != 0)
      direction /= len; // velocity here now represents direction
    velocity = direction * speed_constant;
    // 1: fast bullet , 2:higher damage bullet , 3:higher blast radius
    switch (type) {
    case 1:
      break;
    case 2:
      break;
    case 3:
      break;
    }
  }
  void updatemovement(sf::Time dt, int ENDX, int ENDY) {

    sf::Vector2f nextPosition =
        position + velocity * dt.asSeconds() * speed_constant;

    if (nextPosition.x <= 64 || nextPosition.x >= ENDX) {
      velocity.x = -velocity.x;
      bounceCount--;
    }
    if (nextPosition.y <= 64 || nextPosition.y >= ENDY) {
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
  Player(sf::RenderWindow &win) : contextwindow(win) {
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
    const float coolDown = 0.3f; // seconds
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
      bullets.push_back(Bullet(position, 1, dir));
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
  void renderBullets() {
    for (auto bu : bullets) {
      contextwindow.draw(bu.b);
    }
  }
  void render() {
    sf::CircleShape p1(10);
    p1.setPosition(position.x, position.y);

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
  Player *p1 = new Player(window);
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

    p1->update(elapsed1);

    // updation of rendering occurs , remember we use double buffering so do
    // not use anyother strategy to draw stuff besides below clear , draw ,
    // display
    window.clear();
    // draw stuff over here
    window.draw(tilemap);
    p1->render();
    window.display();
  }

  return 0;
}

// CLEAN BUFFERS
