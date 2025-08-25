#ifndef SIMULATION_THREAD_H
#define SIMULATION_THREAD_H

#include <cmath>

namespace app
{

// coef knot to m/s
#define KNOT_COEF (0.5144)
// coef yard to m
#define YARD_COEF (0.9144)
// coef degree to radian
#define DEG_COEF (2 * M_PI / 360)

struct ShipInfo
{
    // 入力パラメータ
    float course_deg; // 針路
    float knot;       // 速力
    float depth;      // 深度
    // SI単位系変換後
    float course_rad;
    float speed;
    // シミュレーションごとの移動量
    float dx;
    float dy;
    // 絶対座標
    float x_abs;
    float y_abs;
    float z_abs;

    // 初期化メソッド
    void Init(float course_val, float knot_val, float depth_val, float dt_val);
    // 更新メソッド
    void Update(float dt);
};

struct TargetInfo
{
    // 入力パラメータ
    float course_deg; // 針路
    float knot;       // 速力
    float bear_deg;   // 方位
    float yard;       // 距離
    ShipInfo ship;
    // SI単位系変換後
    float course_rad;
    float speed;
    float bear_rad;
    float dist;
    // シミュレーションごとの移動量
    float dx;
    float dy;
    // 更新される状態変数
    float dist_2d;
    float dist_3d;
    float dist_3d_prev;
    float bear;
    float x_rel;
    float y_rel;
    float spd_rel;
    // 絶対座標
    float x_abs;
    float y_abs;
    float z_abs;

    // 初期化メソッド
    void Init(float course_val, float knot_val, float bear_val, float yard_val, float dt_val, ShipInfo &ship_inf);
    // 更新メソッド
    void Update(float dt);
};

void StatusThread(void);

} // namespace app

#endif // SIMULATION_THREAD_H
