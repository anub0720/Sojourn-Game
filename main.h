#ifndef MAIN_H
#define MAIN_H

#include <QtWidgets>
#include <QDebug>
#include <cmath>        // For std::sin and std::fmod
#include <QtMath>       // For qAtan2 and qRadiansToDegrees
#include <QRandomGenerator> // For Perlin noise seed
#include <QGraphicsTextItem> // For Game Over text

// --- Constants ---
const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 720;
const float PLAYER_SIZE = 40.0f;
const float GRAVITY = 0.5f;
const float JUMP_FORCE = -15.0f;
const float PLAYER_SPEED = 5.0f;
const int GAME_TIMER_MS = 1000 / 60; // 60 FPS
const float FLIP_SPEED = 7.0f; // NEW: Degrees per frame

// --- Forward Declarations ---
class Player;
class TerrainManager;
class BackgroundManager;
class ParallaxLayer;
class Obstacle;

// --- Game State ---
enum GameState {
    Playing,
    GameOver
};


// --- Obstacle Class ---
// ... (No changes) ...
class Obstacle : public QGraphicsPixmapItem
{
public:
    Obstacle(qreal x, qreal y);
};


// --- PerlinNoise Class ---
// ... (No changes) ...
class PerlinNoise {
public:
    PerlinNoise(quint32 seed) {
        p.resize(256);
        std::iota(p.begin(), p.end(), 0);
        std::default_random_engine engine(seed);
        std::shuffle(p.begin(), p.end(), engine);
        p.insert(p.end(), p.begin(), p.end());
    }
    double noise(double x) const {
        int X = (int)std::floor(x) & 255;
        x -= std::floor(x);
        double u = fade(x);
        int A = p[X], B = p[X + 1];
        return lerp(u, grad(p[A], x), grad(p[B], x - 1)) * 2.0;
    }
private:
    double fade(double t) const { return t * t * t * (t * (t * 6 - 15) + 10); }
    double lerp(double t, double a, double b) const { return a + t * (b - a); }
    double grad(int hash, double x) const { return (hash & 1) == 0 ? x : -x; }
    std::vector<int> p;
};


// --- ParallaxLayer Class ---
// ... (No changes) ...
class ParallaxLayer : public QGraphicsObject
{
    Q_OBJECT
public:
    ParallaxLayer(const QPixmap& pixmap, qreal speed);
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    void setCameraOffset(qreal cameraX);
private:
    QPixmap m_pixmap;
    qreal m_speed;
    qreal m_offset;
};


// --- BackgroundManager Class ---
// ... (No changes) ...
class BackgroundManager : public QObject
{
    Q_OBJECT
public:
    BackgroundManager(QGraphicsScene* scene);
    void update(qreal cameraX);
private:
    QGraphicsScene* m_scene;
    QList<ParallaxLayer*> m_layers;
};


// --- GameView Class ---
// UPDATED: Added recreateGameOverText()
class GameView : public QGraphicsView
{
    Q_OBJECT

public:
    GameState gameState;
    GameView(QGraphicsScene* scene) : QGraphicsView(scene), player(nullptr)
    {
        setFixedSize(SCREEN_WIDTH, SCREEN_HEIGHT);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scene->setSceneRect(0, 0, 1000000, SCREEN_HEIGHT);
        centerOn(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);

        // --- Create the text item (it's added to scene in recreateGameOverText) ---
        recreateGameOverText(); // Call it once on startup
        hideGameOver();

        gameState = Playing;
    }

    void setPlayer(Player* p) {
        player = p;
    }

    void showGameOver() {
        gameOverText->setPos(mapToScene(SCREEN_WIDTH/2, SCREEN_HEIGHT/2) - gameOverText->boundingRect().center());
        gameOverText->show();
    }
    void hideGameOver() {
        gameOverText->hide();
    }

    // --- NEW: This function fixes our restart bug ---
    void recreateGameOverText() {
        gameOverText = new QGraphicsTextItem("GAME OVER\nPress 'R' to Restart");
        gameOverText->setFont(QFont("Arial", 48, QFont::Bold));
        gameOverText->setDefaultTextColor(Qt::white);
        gameOverText->setZValue(100);
        // We MUST add it to the scene, since scene->clear() removes it
        scene()->addItem(gameOverText);
    }

signals:
    void restartGame();

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

private:
    Player* player;
    QGraphicsTextItem* gameOverText;
};


// --- TerrainManager Class ---
// ... (No changes) ...
class TerrainManager : public QObject
{
    Q_OBJECT
public:
    TerrainManager(QGraphicsScene* scene)
        : m_scene(scene),
        m_perlin(QRandomGenerator::global()->generate())
    {
        m_currentY = SCREEN_HEIGHT * 0.7;
        m_currentX = -SCREEN_WIDTH;
        m_segmentLength = 50;
        m_amplitude = 150;
        m_frequency = 0.005;

        for(int i = 0; i < (SCREEN_WIDTH * 2 / m_segmentLength); ++i) {
            generateSegment();
        }
    }

    void update(qreal playerX) {
        // Generate new terrain
        while (playerX > m_currentX - SCREEN_WIDTH) {
            generateSegment();
        }
        // Delete old terrain
        while (!m_segments.isEmpty() && m_segments.first()->boundingRect().right() < playerX - SCREEN_WIDTH * 1.5) {
            QGraphicsItem* oldSegment = m_segments.takeFirst();
            m_scene->removeItem(oldSegment);
            delete oldSegment;
        }
        // Delete old obstacles
        while (!m_obstacles.isEmpty() && m_obstacles.first()->boundingRect().right() < playerX - SCREEN_WIDTH * 1.5) {
            QGraphicsItem* oldObstacle = m_obstacles.takeFirst();
            m_scene->removeItem(oldObstacle);
            delete oldObstacle;
        }
    }

    qreal getGroundHeight(qreal x) {
        return (SCREEN_HEIGHT * 0.7) + m_perlin.noise(x * m_frequency) * m_amplitude;
    }

    qreal getGroundSlope(qreal x) {
        qreal y1 = getGroundHeight(x - 1.0);
        qreal y2 = getGroundHeight(x + 1.0);
        qreal angle = qRadiansToDegrees(qAtan2(y2 - y1, 2.0));
        return angle;
    }


private:
    void spawnObstacle(qreal x, qreal y) {
        Obstacle* rock = new Obstacle(x, y);
        m_scene->addItem(rock);
        m_obstacles.append(rock);
    }

    void generateSegment() {
        QPolygonF polygon;
        polygon << QPointF(m_currentX, SCREEN_HEIGHT);
        int segments = 10;
        for (int i = 0; i <= segments; ++i) {
            qreal newX = m_currentX + (i * m_segmentLength / segments);
            qreal newY = getGroundHeight(newX);
            polygon << QPointF(newX, newY);
        }

        qreal slope = getGroundSlope(m_currentX + m_segmentLength / 2);

        if (qFabs(slope) < 10 && QRandomGenerator::global()->bounded(100) < 50) {
            spawnObstacle(m_currentX + m_segmentLength / 2, getGroundHeight(m_currentX + m_segmentLength / 2));
        }

        m_currentX += m_segmentLength;
        polygon << QPointF(m_currentX, SCREEN_HEIGHT);
        QGraphicsPolygonItem* segment = m_scene->addPolygon(polygon, Qt::NoPen, QColor(240, 230, 220));
        segment->setZValue(-10);
        m_segments.append(segment);
    }

    QGraphicsScene* m_scene;
    QList<QGraphicsPolygonItem*> m_segments;
    QList<QGraphicsItem*> m_obstacles;
    qreal m_currentX;
    qreal m_currentY;
    qreal m_segmentLength;
    qreal m_amplitude;
    qreal m_frequency;
    PerlinNoise m_perlin;
};

// --- Player Class ---
// ... (No changes) ...
class Player : public QObject, public QGraphicsPixmapItem
{
    Q_OBJECT

public:
    Player();

    void setTerrainManager(TerrainManager* terrain) {
        m_terrain = terrain;
    }

    void die() {
        m_isDead = true;
    }
    bool isDead() const { return m_isDead; }

    void updatePhysics();
    void jump();

    void startFlip();
    void endFlip();


    void advance(int phase)
    {
        if (phase == 0) return;
        updatePhysics();
    }

private:
    float velocityY;
    bool onGround;
    TerrainManager* m_terrain;
    bool m_isDead;
    bool m_isFlipping;
};


// --- GameView Method Definitions ---
// ... (No changes) ...
inline void GameView::keyPressEvent(QKeyEvent* event)
{
    if (gameState == Playing) {
        if (event->key() == Qt::Key_Space && !event->isAutoRepeat()) {
            if (player) {
                player->jump();
                player->startFlip();
            }
        }
    } else if (gameState == GameOver) {
        if (event->key() == Qt::Key_R) {
            emit restartGame();
        }
    }
    QGraphicsView::keyPressEvent(event);
}

inline void GameView::keyReleaseEvent(QKeyEvent *event)
{
    if (gameState == Playing) {
        if (event->key() == Qt::Key_Space && !event->isAutoRepeat()) {
            if (player) {
                player->endFlip();
            }
        }
    }
    QGraphicsView::keyReleaseEvent(event);
}


#endif // MAIN_H

