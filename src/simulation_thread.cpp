#include <thread>

#include "tool_logger.h"
#include "simulation_thread.h"

namespace app
{

void ShipInfo::Init(float course_val, float knot_val, float depth_val, float dt)
{
    course_deg = course_val;
    knot = knot_val;
    depth = depth_val;

    // SI単位系に変換
    course_rad = course_deg * DEG_COEF;
    speed = knot * KNOT_COEF;
    // dtごとの移動量を算出
    dx = speed * dt * std::sin(course_rad);
    dy = speed * dt * std::cos(course_rad);
    // 絶対座標を初期化(開始時、自艦の平面上の位置を原点とする)
    x_abs = 0;
    y_abs = 0;
    z_abs = depth;
}

void ShipInfo::Update(float dt)
{
    // 絶対座標を更新(深さ方向の移動はしない)
    x_abs += dx;
    y_abs += dy;
}

void TargetInfo::Init(float course_val, float knot_val, float bear_val, float yard_val, float dt, ShipInfo &ship_inf)
{
    course_deg = course_val;
    knot = knot_val;
    bear_deg = bear_val;
    yard = yard_val;
    ship = ship_inf;

    // SI単位系変換
    course_rad = course_deg * DEG_COEF;
    speed = knot * KNOT_COEF;
    bear_rad = bear_deg * DEG_COEF;
    dist = yard * YARD_COEF;
    // dtごとの移動量を算出
    dx = speed * dt * std::sin(course_rad);
    dy = speed * dt * std::cos(course_rad);
    // 絶対座標を初期化(ターゲットは水上艦なのでzはゼロ)
    x_abs = dist * std::sin(bear_rad);
    y_abs = dist * std::cos(bear_rad);
    z_abs = 0;

    // シミュレーションごとに更新される状態変数を初期化
    dist_2d = dist;
    dist_3d = std::hypot(dist, ship.depth);
    dist_3d_prev = dist_3d;
    bear = bear_rad;
    x_rel = dist * std::sin(bear_rad);
    y_rel = dist * std::cos(bear_rad);
    spd_rel = 0.0f;
}

void TargetInfo::Update(float dt)
{
    // 相対距離・方位・速度を更新
    x_rel = dist_2d * std::sin(bear) + dx - ship.dx;
    y_rel = dist_2d * std::cos(bear) + dy - ship.dy;
    dist_2d = std::hypot(x_rel, y_rel);
    dist_3d = std::hypot(dist_2d, ship.depth);
    bear = std::atan2(x_rel, y_rel);
    if (bear < 0)
    {
        bear += 2 * M_PI;
    }
    spd_rel = (dist_3d - dist_3d_prev) / dt;
    dist_3d_prev = dist_3d;
    // 絶対座標を更新(深さ方向の移動はしない)
    x_abs += dx;
    y_abs += dy;
}

void StatusThread(void)
{
    LOG_INFO("Status thread start");

    // シミュレーション条件の初期化
    ShipInfo ship;
    TargetInfo target1, target2;

    float dt = 1.0f;
    float ship_course = 10.0f;
    float ship_knot = 15.0f;
    float ship_depth = 5.0f;

    float tgt1_course = 60.0f;
    float tgt1_knot = 25.0f;
    float tgt1_bear = 300.0f;
    float tgt1_yard = 50.0f;

    float tgt2_course = 120.0f;
    float tgt2_knot = 18.0f;
    float tgt2_bear = 270.0f;
    float tgt2_yard = 80.0f;

    // 自艦・ターゲット情報を初期化
    ship.Init(ship_course, ship_knot, ship_depth, dt);
    target1.Init(tgt1_course, tgt1_knot, tgt1_bear, tgt1_yard, dt, ship);
    target2.Init(tgt2_course, tgt2_knot, tgt2_bear, tgt2_yard, dt, ship);

    // デバッグ用、後で消す
    LOG_INFO("(target1) dist: %5.2f, deg: %6.2f", target1.dist_3d, target1.bear / DEG_COEF);
    LOG_INFO("(target2) dist: %5.2f, deg: %6.2f", target2.dist_3d, target2.bear / DEG_COEF);

    for (int i = 0; i < 10; ++i)
    {
        // 自艦・ターゲット情報を更新
        ship.Update(dt);
        target1.Update(dt);
        target2.Update(dt);

        // デバッグ用、後で消す(1秒毎
        LOG_INFO("(ship   ) x: %.2f, y: %.2f", ship.x_abs, ship.y_abs);
        LOG_INFO("(target1) x: %.2f, y: %.2f", target1.x_abs, target1.y_abs);
        LOG_INFO("(target2) x: %.2f, y: %.2f", target2.x_abs, target2.y_abs);
        LOG_INFO("(target1) dist: %5.2f, deg: %6.2f, spd: %5.2f", target1.dist_3d, target1.bear / DEG_COEF, target1.spd_rel);
        LOG_INFO("(target2) dist: %5.2f, deg: %6.2f, spd: %5.2f", target2.dist_3d, target2.bear / DEG_COEF, target2.spd_rel);
    }

    LOG_INFO("Status thread end");
}

} // namespace app
