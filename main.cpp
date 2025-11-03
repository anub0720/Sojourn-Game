#include "main.h"

// --- Procedural Rock Pixmap ---
QPixmap createRockPixmap() {
    int size = 40;
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    // Draw a simple, grey, lumpy rock
    QPolygonF rock;
    rock << QPointF(size * 0.5, size * 0.1)
         << QPointF(size * 0.8, size * 0.3)
         << QPointF(size * 0.9, size * 0.7)
         << QPointF(size * 0.6, size * 0.9)
         << QPointF(size * 0.2, size * 0.8)
         << QPointF(size * 0.1, size * 0.4);

    painter.setBrush(QColor(140, 140, 140));
    painter.setPen(QColor(100, 100, 100));
    painter.drawPolygon(rock);

    painter.end();
    return pixmap;
}

// --- Obstacle Constructor ---
Obstacle::Obstacle(qreal x, qreal y)
{
    static QPixmap rockPixmap = createRockPixmap();
    setPixmap(rockPixmap);

    // Set position, adjusting for pixmap size
    setPos(x, y - rockPixmap.height() / 2);
    setZValue(5); // Draw in front of terrain, behind player

    // Set offset for correct collision
    setOffset(-rockPixmap.width() / 2, -rockPixmap.height() / 2);
}


// --- Procedural Background Functions ---
QPixmap createDistantMountainPixmap() {
    QPixmap pixmap(800, 600);
    pixmap.fill(QColor(230, 245, 255)); // Sky color
    QPainter painter(&pixmap);

    // Draw some simple, dark, distant mountains
    QPolygonF mountains;
    mountains << QPointF(0, 500)
              << QPointF(150, 350)
              << QPointF(300, 450)
              << QPointF(450, 400)
              << QPointF(600, 480)
              << QPointF(800, 500)
              << QPointF(800, 600)
              << QPointF(0, 600);
    painter.setBrush(QColor(180, 200, 220)); // Distant blue-grey
    painter.setPen(Qt::NoPen);
    painter.drawPolygon(mountains);

    painter.end();
    return pixmap;
}
QPixmap createNearHillsPixmap() {
    QPixmap pixmap(800, 600);
    pixmap.fill(Qt::transparent); // Transparent, so we see mountains behind
    QPainter painter(&pixmap);

    // Draw some simple, slightly darker hills
    QPolygonF hills;
    hills   << QPointF(0, 550)
          << QPointF(200, 480)
          << QPointF(400, 520)
          << QPointF(600, 500)
          << QPointF(800, 550)
          << QPointF(800, 600)
          << QPointF(0, 600);
    painter.setBrush(QColor(210, 220, 210)); // Nearer green-grey
    painter.setPen(Qt::NoPen);
    painter.drawPolygon(hills);

    painter.end();
    return pixmap;
}

// --- ParallaxLayer Implementation ---
ParallaxLayer::ParallaxLayer(const QPixmap& pixmap, qreal speed)
    : m_pixmap(pixmap), m_speed(speed), m_offset(0)
{
    // We override boundingRect() instead.
}
QRectF ParallaxLayer::boundingRect() const
{
    return QRectF(-SCREEN_WIDTH, 0, SCREEN_WIDTH * 2, SCREEN_HEIGHT);
}
void ParallaxLayer::setCameraOffset(qreal cameraX)
{
    m_offset = std::fmod(cameraX * m_speed, m_pixmap.width());
    if (m_offset < 0) {
        m_offset += m_pixmap.width();
    }
    setPos(cameraX, 0);
}
void ParallaxLayer::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    qreal localLeft = -SCREEN_WIDTH / 2.0;
    painter->drawTiledPixmap(localLeft - m_pixmap.width(), 0,
                             SCREEN_WIDTH + m_pixmap.width() * 2, SCREEN_HEIGHT,
                             m_pixmap, m_offset, 0);
}

// --- BackgroundManager Implementation ---
BackgroundManager::BackgroundManager(QGraphicsScene* scene) : m_scene(scene)
{
    QPixmap mountains = createDistantMountainPixmap();
    ParallaxLayer* mountainLayer = new ParallaxLayer(mountains, 0.1); // Moves at 10% speed
    mountainLayer->setZValue(-30); // Draw waaaay in the back
    m_scene->addItem(mountainLayer);
    m_layers.append(mountainLayer);

    m_scene->setBackgroundBrush(mountains.copy(0,0,1,1));

    QPixmap hills = createNearHillsPixmap();
    ParallaxLayer* hillLayer = new ParallaxLayer(hills, 0.3); // Moves at 30% speed
    hillLayer->setZValue(-20); // Draw in front of mountains
    m_scene->addItem(hillLayer);
    m_layers.append(hillLayer);
}
void BackgroundManager::update(qreal cameraX)
{
    for(ParallaxLayer* layer : m_layers) {
        layer->setCameraOffset(cameraX);
    }
}

// --- createPlayerSprite() ---
QPixmap createPlayerSprite()
{
    QPixmap pixmap(PLAYER_SIZE, PLAYER_SIZE);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.setPen(QPen(QColor(80, 80, 80), 3));
    painter.drawLine(PLAYER_SIZE * 0.2, PLAYER_SIZE * 0.9,
                     PLAYER_SIZE * 0.8, PLAYER_SIZE * 0.9);

    painter.setPen(QPen(QColor(200, 50, 50), 4));
    painter.drawLine(PLAYER_SIZE * 0.5, PLAYER_SIZE * 0.4,
                     PLAYER_SIZE * 0.5, PLAYER_SIZE * 0.8);

    painter.setBrush(QColor(200, 50, 50));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(QPointF(PLAYER_SIZE * 0.5, PLAYER_SIZE * 0.3),
                        PLAYER_SIZE * 0.15, PLAYER_SIZE * 0.15);

    painter.end();
    return pixmap;
}

// --- Player Constructor ---
Player::Player() : QGraphicsPixmapItem() {
    setPixmap(createPlayerSprite());
    setOffset(-PLAYER_SIZE / 2, -PLAYER_SIZE / 2);
    setPos(SCREEN_WIDTH / 4, SCREEN_HEIGHT / 2);
    setZValue(10);
    velocityY = 0.0f;
    onGround = false;
    m_terrain = nullptr;
    m_isDead = false;
    m_isFlipping = false;
}

// --- Player updatePhysics() ---
void Player::updatePhysics()
{
    if (m_isDead) return;

    // --- Check for obstacle collisions ---
    QList<QGraphicsItem*> collisions = collidingItems();
    for (QGraphicsItem* item : collisions) {
        if (dynamic_cast<Obstacle*>(item)) {
            die();
            return;
        }
    }

    if (!m_terrain) return; // Do nothing if no terrain

    // --- Apply gravity ---
    velocityY += GRAVITY;

    // --- Apply horizontal speed ---
    moveBy(PLAYER_SPEED, velocityY);

    // --- Get ground info ---
    qreal groundY = m_terrain->getGroundHeight(x());
    qreal groundSlope = m_terrain->getGroundSlope(x());

    bool wasInAir = !onGround; // Store state *before* collision check

    // --- Check for ground collision ---
    if (y() + PLAYER_SIZE / 2 >= groundY - 5.0) { // --- On Ground ---
        setY(groundY - PLAYER_SIZE / 2);
        velocityY = 0;
        onGround = true;

        // --- Add landing logic ---
        if (wasInAir) { // We just landed on this frame
            qreal landingAngle = rotation();
            // Normalize angle to -180 to 180
            while (landingAngle > 180) landingAngle -= 360;
            while (landingAngle < -180) landingAngle += 360;

            // Check for bad landing (e.g., more than 90 degrees off)
            if (qFabs(landingAngle) > 90) {
                die(); // Crash!
                return;
            }
        }

        // Smoothly rotate to match ground
        qreal angle = rotation();
        setRotation(angle * 0.9 + groundSlope * 0.1);

    } else { // --- In Air ---
        onGround = false;

        // --- Flip Logic ---
        if (m_isFlipping) {
            // If we are flipping, add constant rotation
            setRotation(rotation() + FLIP_SPEED);
        } else {
            // Otherwise, level out
            qreal angle = rotation();
            if (angle > 1.0 || angle < -1.0) {
                setRotation(angle * 0.95);
            }
        }
    }
}

// --- Player jump() ---
void Player::jump()
{
    // Only jump if on the ground
    if (onGround && !m_isDead) {
        velocityY = JUMP_FORCE;
        onGround = false;
    }
}


// --- Player flip functions ---
void Player::startFlip() {
    // We can only start a flip if we are in the air
    if (!onGround && !m_isDead) {
        m_isFlipping = true;
    }
}

void Player::endFlip() {
    m_isFlipping = false;
}


// --- Game Setup Function ---
// ... (No changes) ...
void setupGame(QGraphicsScene* scene, GameView* view,
               BackgroundManager*& bgManager,
               TerrainManager*& terrain,
               Player*& player)
{
    // 1. Create game objects
    bgManager = new BackgroundManager(scene);
    terrain = new TerrainManager(scene);
    player = new Player();
    scene->addItem(player); // Add player to the scene

    // 2. Connect objects
    view->setPlayer(player);
    player->setTerrainManager(terrain);
}


// --- Main Function ---
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QGraphicsScene scene;
    GameView view(&scene);

    // --- Pointers to our main objects ---
    BackgroundManager* backgroundManager;
    TerrainManager* terrain;
    Player* player;

    // --- Initial game setup ---
    setupGame(&scene, &view, backgroundManager, terrain, player);

    // --- Connect restart signal ---
    QObject::connect(&view, &GameView::restartGame, [&]() {
        // 1. Clear the scene (deletes all items)
        scene.clear();

        // 2. Delete the managers
        delete backgroundManager;
        delete terrain;

        // 3. Reset the view
        // --- NEW: Call our bug-fix function ---
        view.recreateGameOverText();
        view.hideGameOver();
        view.gameState = Playing;

        // 4. Re-create all objects
        setupGame(&scene, &view, backgroundManager, terrain, player);
    });

    // --- Game Loop ---
    QTimer gameLoop;
    QObject::connect(&gameLoop, &QTimer::timeout, [&]() {

        // --- ONLY run game logic if we are playing ---
        if (view.gameState == Playing) {

            // Check for death
            if (player->isDead()) {
                view.gameState = GameOver;
                view.showGameOver();
            } else {
                // --- Update ---
                terrain->update(player->x());
                scene.advance(); // This calls player->updatePhysics()

                // --- Camera Follow ---
                QPointF playerPos = player->pos();
                qreal cameraX = playerPos.x() + SCREEN_WIDTH / 4;
                view.centerOn(cameraX, SCREEN_HEIGHT / 2);

                // --- Parallax Update ---
                backgroundManager->update(cameraX);
            }
        }
    });

    gameLoop.start(GAME_TIMER_MS);

    view.show();
    return app.exec();
}

